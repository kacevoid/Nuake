#include <Engine.h>

#include <dependencies/GLEW/include/GL/glew.h>
#include <src/Vendors/imgui/imgui.h>

#include <string>

void main(int argc, char* argv[])
{
    using namespace Nuake;

    Engine::Init();

    auto window = Nuake::Engine::GetCurrentWindow();

    const std::string projectPath = "./game.project";
    FileSystem::SetRootDirectory(FileSystem::GetParentPath(projectPath));

    Ref<Nuake::Project> project = Nuake::Project::New();
    project->FullPath = projectPath;
    project->Deserialize(json::parse(FileSystem::ReadFile(projectPath, true)));

    window->SetTitle(project->Name);

    Nuake::Engine::LoadProject(project);
    Nuake::Engine::EnterPlayMode();

    while (!window->ShouldClose())
    {
        Nuake::Engine::Tick();
        Engine::Draw();

        const auto& WindowSize = window->GetSize();
        glViewport(0, 0, WindowSize.x, WindowSize.y);
        Nuake::Renderer2D::BeginDraw(WindowSize);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        const auto& windowSize = Vector2(viewport->Size.x, viewport->Size.y);
        Ref<FrameBuffer> framebuffer = Engine::GetCurrentWindow()->GetFrameBuffer();
        if (framebuffer->GetSize() != windowSize)
        {
            framebuffer->QueueResize(windowSize);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::Begin("Game", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
        {
            ImGui::Image((void*)Window::Get()->GetFrameBuffer()->GetTexture()->GetID(), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();

        ImGui::PopStyleVar();

        Engine::EndDraw();
    }
}
