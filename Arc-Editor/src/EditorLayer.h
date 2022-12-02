#pragma once

#include <ArcEngine.h>

#include "Panels/ConsolePanel.h"
#include "Panels/EditorContext.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/SceneHierarchyPanel.h"

namespace ArcEngine
{
	class AssetPanel;
	class PropertiesPanel;
	class SceneViewport;
	class BasePanel;

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer() override = default;

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate([[maybe_unused]] Timestep ts) override;
		void OnImGuiRender() override;
		void OnEvent([[maybe_unused]] Event& e) override;

		void SetContext(EditorContextType type, const void* data, size_t size) { m_SelectedContext.Set(type, data, size); }
		void ResetContext() { m_SelectedContext.Reset(); }
		const EditorContext& GetContext() const { return m_SelectedContext; }
		
		void OpenScene(const char* filepath);

		static EditorLayer* GetInstance() { return s_Instance; }

	private:
		void BeginDockspace(const char* name) const;
		void EndDockspace() const;
		bool OnKeyPressed([[maybe_unused]] const KeyPressedEvent& e);
		bool OnMouseButtonPressed([[maybe_unused]] const MouseButtonPressedEvent& e) const;
		bool OnMouseButtonReleased([[maybe_unused]] const MouseButtonReleasedEvent& e) const;

		void OpenProject(const std::filesystem::path& path);
		void OpenProject();
		void SaveProject(const std::filesystem::path& path) const;

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		void OnScenePlay();
		void OnSceneStop();
		void OnScenePause();
		void OnSceneUnpause();

	private:
		Application* m_Application = nullptr;
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		Ref<Scene> m_RuntimeScene;
		enum class SceneState { Edit, Play, Pause, Step };
		SceneState m_SceneState = SceneState::Edit;
		std::filesystem::path m_ScenePath = std::filesystem::path();

		EditorContext m_SelectedContext = {};

		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		ConsolePanel m_ConsolePanel;
		ProjectSettingsPanel m_ProjectSettingsPanel;

		eastl::vector<Scope<BasePanel>> m_Panels;
		eastl::vector<Scope<SceneViewport>> m_Viewports;
		eastl::vector<Scope<PropertiesPanel>> m_Properties;
		eastl::vector<Scope<AssetPanel>> m_AssetPanels;

		bool m_ShowSceneHierarchyPanel = true;
		bool m_ShowDemoWindow = false;
		float m_TopMenuBarHeight = 6.0f;
		glm::vec2 m_LastMousePosition = glm::vec2(0.0f);

		static EditorLayer* s_Instance;
	};
}
