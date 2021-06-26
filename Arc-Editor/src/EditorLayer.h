#pragma once

#include <ArcEngine.h>

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/StatsPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include "Arc/Renderer/EditorCamera.h"

namespace ArcEngine
{
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;
		
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveSceneAs();

		void OnScenePlay();
		void OnSceneStop();

		void DrawRectAroundWindow(const glm::vec4& color);
	private:
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		Ref<Scene> m_RuntimeScene;

		enum class SceneState
		{
			Edit = 0, Play = 1, Pause = 2
		};
		SceneState m_SceneState = SceneState::Edit;

		Entity m_HoveredEntity;

		EditorCamera m_EditorCamera;
		bool m_HasViewportEvent = false;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		float frameTime = 0.0f;

		int m_GizmoType = -1;
		
		// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		SettingsPanel m_SettingsPanel;
		StatsPanel m_StatsPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
	};
}
