#pragma once
#include "src/Scene/Systems/ScriptingSystem.h"
#include "src/Scene/Systems/PhysicsSystem.h"
#include "src/Scene/Systems/TransformSystem.h"
#include "src/Scene/Systems/QuakeMapBuilder.h"

#include "src/Rendering/SceneRenderer.h"
#include "Scene.h"
#include "Entities/Entity.h"

#include "src/Rendering/Renderer.h"
#include "src/Rendering/Textures/MaterialManager.h"
#include "src/Core/Physics/PhysicsManager.h"
#include "src/Core/Core.h"

#include <GL/glew.h>

#include "Engine.h"
#include "src/Core/Maths.h"
#include "src/Core/FileSystem.h"
#include "src/Scene/Components/Components.h"
#include "src/Scene/Components/BoxCollider.h"
#include "src/Scene/Components/WrenScriptComponent.h"
#include "src/Scene/Components/BSPBrushComponent.h"
#include "src/Scene/Components/InterfaceComponent.h"

#include <fstream>
#include <streambuf>
#include <chrono>
#include "src/Core/OS.h"

namespace Nuake {
	Ref<Scene> Scene::New()
	{
		return CreateRef<Scene>();
	}

	Scene::Scene()
	{
		m_Systems = std::vector<Ref<System>>();
		m_EditorCamera = CreateRef<EditorCamera>();
		m_Environement = CreateRef<Environment>();

		// Adding systems - Order is important
		m_Systems.push_back(CreateRef<ScriptingSystem>(this));
		m_Systems.push_back(CreateRef<TransformSystem>(this));
		m_Systems.push_back(CreateRef<PhysicsSystem>(this));

		mSceneRenderer = new SceneRenderer();
		mSceneRenderer->Init();
	}

	Scene::~Scene() 
	{
	}

	std::string Scene::GetName()
	{
		return this->Name;
	}

	bool Scene::SetName(std::string& newName)
	{
		if (newName == "")
			return false;

		this->Name = newName;
		return true;
	}

	Entity Scene::GetEntity(int handle)
	{
		return Entity((entt::entity)handle, this);
	}

	Entity Scene::GetEntityByID(int id)
	{
		auto idView = m_Registry.view<NameComponent>();
		for (auto e : idView) 
		{
			NameComponent& nameC = idView.get<NameComponent>(e);
			if (nameC.ID == id)
			{
				return Entity{ e, this };
			}
		}

		assert("Entity not found");
	}

	bool Scene::OnInit()
	{
		for (auto& system : m_Systems)
		{
			Logger::Log("Init system");
			if (!system->Init())
			{
				return false;
			}
		}

		return true;
	}

	void Scene::OnExit()
	{
		for (auto& system : m_Systems)
		{
			system->Exit();
		}
	}

	void Scene::Update(Timestep ts)
	{
		for (auto& system : m_Systems)
		{
			system->Update(ts);
		}
	}

	void Scene::FixedUpdate(Timestep ts)
	{
		for (auto& system : m_Systems)
		{
			system->FixedUpdate(ts);
		}
	}

	void Scene::EditorUpdate(Timestep ts)
	{
		
	}

	void Scene::Draw(FrameBuffer& framebuffer)
	{
		Ref<Camera> cam = nullptr;
		const auto& view = m_Registry.view<TransformComponent, CameraComponent, ParentComponent>();
		for (const auto& e : view)
		{
			auto [transform, camera, parent] = view.get<TransformComponent, CameraComponent, ParentComponent>(e);
			cam = camera.CameraInstance;

			cam->Translation = transform.GetGlobalPosition();
			break;
		}

		if (!cam)
		{
			return;
		}

		mSceneRenderer->BeginRenderScene(cam->GetPerspective(), cam->GetTransform(), cam->Translation);
		mSceneRenderer->RenderScene(*this, framebuffer);
	}

	void Scene::Draw(FrameBuffer& framebuffer, const Matrix4& projection, const Matrix4& view)
	{
		mSceneRenderer->BeginRenderScene(m_EditorCamera->GetPerspective(), m_EditorCamera->GetTransform(), m_EditorCamera->Translation);
		mSceneRenderer->RenderScene(*this, framebuffer);
	}
	
	std::vector<Entity> Scene::GetAllEntities() 
	{
		std::vector<Entity> allEntities;
		auto view = m_Registry.view<NameComponent>();
		for (auto& e : view) 
		{
			Entity newEntity(e, this);

			// Check if valid for deleted entities.
			if (newEntity.IsValid())
			{
				allEntities.push_back(newEntity);
			}
		}
		return allEntities;
	}

	Entity Scene::GetEntity(const std::string& name)
	{
		std::vector<Entity> allEntities;
		auto view = m_Registry.view<TransformComponent, NameComponent>();
		for (auto e : view) 
		{
			auto [transform, namec] = view.get<TransformComponent, NameComponent>(e);
			if (namec.Name == name)
				return Entity{ e, this };
		}

		return Entity();
	}

	Entity Scene::CreateEntity(const std::string& name) 
	{
		return CreateEntity(name, (int)OS::GetTime());
	}

	Entity Scene::CreateEntity(const std::string& name, int id)
	{
		if (name.empty())
		{
			Logger::Log("[Scene] Failed to create entity. Entity name cannot be empty.");
			return Entity();
		}

		std::string entityName;
		if (GetEntity(name) != Entity())
		{
			entityName = name;
		}
		else
		{
			// Try to generate a unique name
			for (uint32_t i = 1; i < 2048; i++)
			{
				const std::string& entityEnumName = name + std::to_string(i);
				const auto& entityId = GetEntity(entityEnumName).GetHandle();
				if (entityId != -1)
				{
					entityName = entityEnumName;
					break;
				}
			}

			if (entityName.empty()) // We ran out of names!!!
			{
				Logger::Log("[Scene] Failed to create entity. Limit reached with name: " + name, CRITICAL);
				return Entity();
			}
		}

		Entity entity = { m_Registry.create(), this };

		// Add all mandatory component. An entity cannot exist without these.
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<ParentComponent>();
		entity.AddComponent<VisibilityComponent>();

		NameComponent& nameComponent = entity.AddComponent<NameComponent>();
		nameComponent.Name = entityName;
		nameComponent.ID = id;

		Logger::Log("[Scene] Entity created with name: " + nameComponent.Name, LOG_TYPE::VERBOSE);
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		ParentComponent& parentC = entity.GetComponent<ParentComponent>();
		std::vector<Entity> copyChildrens = parentC.Children;

		if (parentC.HasParent) 
		{  
			// Remove self from parents children lists.
			ParentComponent& parent = parentC.Parent.GetComponent<ParentComponent>();
			parent.RemoveChildren(entity);
		}

		for (auto& c : copyChildrens) 
		{
			Logger::Log("Deleting entity " + std::to_string(c.GetHandle()));
			DestroyEntity(c);
		}

		Logger::Log("Deleted entity" + std::to_string(entity.GetHandle()) + " - " + entity.GetComponent<NameComponent>().Name);
		entity.Destroy();
		m_Registry.shrink_to_fit();
	}

	Ref<Camera> Scene::GetCurrentCamera()
	{
		if (Engine::IsPlayMode())
		{
			Ref<Camera> cam = nullptr;
			{
				auto view = m_Registry.view<TransformComponent, CameraComponent>();
				for (auto e : view) 
				{
					auto [transform, camera] = view.get<TransformComponent, CameraComponent>(e);
					cam = camera.CameraInstance;
					break;
				}
			}

			if (!cam)
				cam = m_EditorCamera;
			return cam;
		}

		return m_EditorCamera;
	}

	Ref<Environment> Scene::GetEnvironment() const
	{
		return m_Environement;
	}

	void Scene::SetEnvironment(Ref<Environment> env)
	{
		m_Environement = env;
	}

	bool Scene::Save()
	{
		if (Path == "")
			Path = FileSystem::AbsoluteToRelative(FileDialog::SaveFile("*.scene") + ".scene");

		return SaveAs(Path);
	}

	bool Scene::SaveAs(const std::string& path)
	{
		std::string fileContent = Serialize().dump(4);

		FileSystem::BeginWriteFile(FileSystem::Root + path);
		FileSystem::WriteLine(fileContent);
		FileSystem::EndWriteFile();

		Logger::Log("Scene saved successfully");
		return true;
	}

	template<typename Component>
	void Scene::CopyComponent(entt::registry& dst, entt::registry& src)
	{
		auto view = src.view<Component>();
		for (auto e : view)
		{
			int id = src.get<NameComponent>(e).ID;
			auto& component = src.get<Component>(e);

			auto idView = dst.view<NameComponent>();
			for (auto de : idView)
			{
				if (idView.get<NameComponent>(de).ID == src.get<NameComponent>(e).ID)
				{
					dst.emplace_or_replace<Component>(de, component);
				}
			}
		}
	}

	Ref<Scene> Scene::Copy()
	{
		Ref<Scene> sceneCopy = CreateRef<Scene>();
		sceneCopy->Path = Path;

		json serializedScene = Serialize();

		sceneCopy->Deserialize(serializedScene.dump());
		return sceneCopy;
	}

	json Scene::Serialize()
	{
		BEGIN_SERIALIZE();
		SERIALIZE_VAL(Name);
		SERIALIZE_OBJECT(m_Environement)
		SERIALIZE_VAL(Path)
		
		std::vector<json> entities = std::vector<json>();
		for (Entity e : GetAllEntities())
			entities.push_back(e.Serialize());
		SERIALIZE_VAL_LBL("Entities", entities);

		SERIALIZE_OBJECT(m_EditorCamera);

		END_SERIALIZE();
	}

	bool Scene::Deserialize(const std::string& str)
	{
		if (str == "")
			return false;

		BEGIN_DESERIALIZE();
		if (!j.contains("Name"))
			return false;

		m_Registry.clear();
		Name = j["Name"];

		if (j.contains(""))
		{
			Path = j["Path"];
		}

		m_Environement = CreateRef<Environment>();
		if (j.contains("m_Environement"))
		{
			m_Environement = CreateRef<Environment>();
			std::string env = j["m_Environement"].dump();
			m_Environement->Deserialize(env);
		}

		// Parse entities
		if (!j.contains("Entities"))
		{
			return 0;
		}
		
		for (json e : j["Entities"])
		{
			std::string name = e["NameComponent"]["Name"];
			Entity ent = { m_Registry.create(), this };
			ent.Deserialize(e.dump());
		}

		if (j.contains("m_EditorCamera"))
		{
			m_EditorCamera->Deserialize(j["m_EditorCamera"].dump());
		}

		auto view = m_Registry.view<ParentComponent>();
		for (auto e : view)
		{
			auto& parentComponent = view.get<ParentComponent>(e);
			if (!parentComponent.HasParent)
				continue;

			auto& entity = Entity{ e, this };
			auto parentEntity = GetEntityByID(parentComponent.ParentID);
			parentEntity.AddChild(entity);
		}

		return true;
	}
}
