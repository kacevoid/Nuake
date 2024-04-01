#pragma once
#include "../Core/Timestep.h"
#include "../Scene/Entities/Entity.h"
#include "DynamicWorld.h"
#include "Rigibody.h"
#include "CollisionData.h"

#include "RaycastResult.h"

namespace Nuake
{
	class PhysicsManager
	{
	private:
		Ref<Physics::DynamicWorld> m_World;
		bool m_IsRunning = false;

		bool m_DrawDebug = false;
		static PhysicsManager* m_Instance;

	public:
		static PhysicsManager& Get()
		{
			static PhysicsManager instance;
			return instance;
		}

		Ref<Physics::DynamicWorld> GetWorld() { return m_World; }

		PhysicsManager() { if (!m_Instance) m_Instance = this; }

		void SetDrawDebug(bool value) 
		{
			m_DrawDebug = value;
		}

		bool GetDrawDebug() {
			return m_DrawDebug;
		}

		void Init();

		void Start() { m_IsRunning = true; }
		void Stop() { m_IsRunning = false; }
		void DrawDebug();
		bool IsRunning() { return m_IsRunning; }
		void Step(Timestep ts);

		void Reset();

		std::vector<RaycastResult> Raycast(const Vector3& from, const Vector3& to);

		const std::vector<Physics::CollisionData>& GetCollisions();

		void RegisterBody(Ref<Physics::RigidBody> rb);
		void RegisterGhostBody(Ref<GhostObject> rb);
		void RegisterCharacterController(Ref<Physics::CharacterController> c);
	};
}
