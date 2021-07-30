#include "SceneHierarchyPanel.h"

#include <iostream>

#include "Arc/Utils/PlatformUtils.h"

#include <glm/gtc/type_ptr.hpp>
#include "Arc/Scripting/ScriptEngine.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ArcEngine
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		m_Context->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, m_Context.get() };
			DrawEntityNode(entity);
		});

		if(ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectionContext = {};

		if(ImGui::BeginPopupContextWindow(0, 1, false))
		{
			m_SelectionContext = {};
			if(ImGui::MenuItem("Create Empty Entity"))
			{
				m_SelectionContext = m_Context->CreateEntity("Empty Entity");
			}

			if (ImGui::MenuItem("Camera"))
			{
				m_SelectionContext = m_Context->CreateEntity("Camera");
				m_SelectionContext.AddComponent<CameraComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Sprite"))
			{
				m_SelectionContext = m_Context->CreateEntity("Sprite");
				m_SelectionContext.AddComponent<SpriteRendererComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::BeginMenu("3D Object"))
			{
				if (ImGui::MenuItem("Mesh"))
				{
					m_SelectionContext = m_Context->CreateEntity("Mesh");
					m_SelectionContext.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}
				
				if (ImGui::MenuItem("Cube"))
				{
					m_SelectionContext = m_Context->CreateEntity("Cube");
					m_SelectionContext.AddComponent<MeshComponent>(m_SelectionContext.GetComponent<IDComponent>().ID, MeshComponent::Geometry::Cube);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sphere"))
				{
					m_SelectionContext = m_Context->CreateEntity("Sphere");
					m_SelectionContext.AddComponent<MeshComponent>(m_SelectionContext.GetComponent<IDComponent>().ID, MeshComponent::Geometry::Sphere);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Plane"))
				{
					m_SelectionContext = m_Context->CreateEntity("Plane");
					m_SelectionContext.AddComponent<MeshComponent>(m_SelectionContext.GetComponent<IDComponent>().ID, MeshComponent::Geometry::Plane);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Quad"))
				{
					m_SelectionContext = m_Context->CreateEntity("Quad");
					m_SelectionContext.AddComponent<MeshComponent>(m_SelectionContext.GetComponent<IDComponent>().ID, MeshComponent::Geometry::Quad);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Cone"))
				{
					m_SelectionContext = m_Context->CreateEntity("Cone");
					m_SelectionContext.AddComponent<MeshComponent>(m_SelectionContext.GetComponent<IDComponent>().ID, MeshComponent::Geometry::Cone);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Cylinder"))
				{
					m_SelectionContext = m_Context->CreateEntity("Cylinder");
					m_SelectionContext.AddComponent<MeshComponent>(m_SelectionContext.GetComponent<IDComponent>().ID, MeshComponent::Geometry::Cylinder);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Light"))
			{
				if (ImGui::MenuItem("Directional"))
				{
					m_SelectionContext = m_Context->CreateEntity("Directional Light");
					m_SelectionContext.AddComponent<LightComponent>(LightComponent::LightType::Directional);
					ImGui::CloseCurrentPopup();
				}
				else if (ImGui::MenuItem("Point"))
				{
					m_SelectionContext = m_Context->CreateEntity("Point Light");
					m_SelectionContext.AddComponent<LightComponent>(LightComponent::LightType::Point);
					ImGui::CloseCurrentPopup();
				}
				else if (ImGui::MenuItem("Spot"))
				{
					m_SelectionContext = m_Context->CreateEntity("Spot Light");
					m_SelectionContext.AddComponent<LightComponent>(LightComponent::LightType::Spot);
					ImGui::CloseCurrentPopup();
				}
				else if (ImGui::MenuItem("Area"))
				{
					m_SelectionContext = m_Context->CreateEntity("Area Light");
					m_SelectionContext.AddComponent<LightComponent>(LightComponent::LightType::Area);
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Skylight"))
			{
				m_SelectionContext = m_Context->CreateEntity("Skylight");
				m_SelectionContext.AddComponent<SkylightComponent>();
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}
		
		ImGui::End();
		

		
		ImGui::Begin("Properties");
		if(m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tagComponent = entity.GetComponent<TagComponent>();
		auto& tag = tagComponent.Tag;

		ImGuiTreeNodeFlags flags = (m_SelectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if(ImGui::IsItemClicked())
			m_SelectionContext = entity;

		bool entityDeleted = false;
		if(ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename")) 
				tagComponent.renaming = true;
			if(ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (tagComponent.renaming)
		{
			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
				tag = std::string(buffer);

			if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
				tagComponent.renaming = false;
		}
		
		if(opened)
		{
			ImGui::TreePop();
		}

		if(entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if(m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	static void SetLabel(const char* label)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImVec2 lineStart = ImGui::GetCursorScreenPos();
		const ImGuiStyle& style = ImGui::GetStyle();
		float fullWidth = ImGui::GetContentRegionAvail().x;
		float itemWidth = fullWidth * 0.6f;
		ImVec2 textSize = ImGui::CalcTextSize(label);
		ImRect textRect;
		textRect.Min = ImGui::GetCursorScreenPos();
		textRect.Max = textRect.Min;
		textRect.Max.x += fullWidth - itemWidth;
		textRect.Max.y += textSize.y;

		ImGui::SetCursorScreenPos(textRect.Min);

		ImGui::AlignTextToFramePadding();
		textRect.Min.y += window->DC.CurrLineTextBaseOffset;
		textRect.Max.y += window->DC.CurrLineTextBaseOffset;

		ImGui::ItemSize(textRect);
		if (ImGui::ItemAdd(textRect, window->GetID(label)))
		{
			ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textRect.Min, textRect.Max, textRect.Max.x,
				textRect.Max.x, label, nullptr, &textSize);

			if (textRect.GetWidth() < textSize.x && ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", label);
		}
		ImVec2 v(0, textSize.y + window->DC.CurrLineTextBaseOffset);
		ImGui::SetCursorScreenPos(ImVec2(textRect.Max.x - v.x, textRect.Max.y - v.y));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(itemWidth);
	}

	static void DrawCheckbox(const std::string& label, bool* flag)
	{
		ImGui::PushID(label.c_str());
		SetLabel(label.c_str());
		ImGui::Checkbox("##flag", flag);
		ImGui::PopID();
	}

	static void DrawFloatControl(const std::string& label, float* value, float min = 0, float max = 0, const char* format = "%.3f")
	{
		ImGui::PushID(label.c_str());
		SetLabel(label.c_str());
		ImGui::DragFloat("##value", value, 0.1f, min, max, format);
		ImGui::PopID();
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[1];
		
		ImGui::PushID(label.c_str());
		
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if(ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if(ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if(ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		
		ImGui::Columns(1);

		ImGui::PopID();
	}

	static void DrawVec2Control(const std::string& label, glm::vec2& values, float resetValue = 0.0f, const char* format = "%.2f", float columnWidth = 200.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[1];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0, 0, format);
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0, 0, format);
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, const bool removable = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		
		if(entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
			
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();

			bool removeComponent = false;
			if(removable)
			{
				ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
				if(ImGui::Button("V", ImVec2{ lineHeight, lineHeight }))
					ImGui::OpenPopup("ComponentSettings");

				if(ImGui::BeginPopup("ComponentSettings"))
				{
					if(ImGui::MenuItem("Remove Component"))
						removeComponent = true;
					
					ImGui::EndPopup();
				}
			}
			if(open)
			{
				uiFunction(entity, component);
				ImGui::TreePop();
			}

			if(removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	static void DrawPBRProperties(Ref<PbrMaterial>& materialInstance)
	{
		const char* filesFlag = "png\0*.png\0jpg\0*.jpg\0bmp\0*.bmp\0";
		int id = 0;
		//==============================================
		// Albedo
		ImGui::PushID(id);
		ImGui::Checkbox("", &materialInstance->UseAlbedoMap); ImGui::SameLine();
		id++;
		ImGui::PopID();
		if (ImGui::ImageButton((ImTextureID)materialInstance->AlbedoMap->GetRendererID(), ImVec2(16, 16), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
		{
			std::string path = FileDialogs::OpenFile(filesFlag);
			if (!path.empty())
				materialInstance->AlbedoMap = Texture2D::Create(path);
		}
		ImGui::SameLine(); ImGui::ColorEdit4("Albedo", (float*)&materialInstance->Color);
		//==============================================

		//==============================================
		// Normals
		ImGui::PushID(id);
		ImGui::Checkbox("", &materialInstance->UseNormalMap); ImGui::SameLine();
		id++;
		ImGui::PopID();
		if (ImGui::ImageButton((ImTextureID)materialInstance->NormalMap->GetRendererID(), ImVec2(16, 16), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
		{
			std::string path = FileDialogs::OpenFile(filesFlag);
			if (!path.empty())
				materialInstance->NormalMap = Texture2D::Create(path);
		}
		ImGui::SameLine();
		ImGui::Text("Normals");
		//==============================================

		//==============================================
		// Metallic
		ImGui::PushID(id);
		ImGui::Checkbox("", &materialInstance->UseMetallicMap); ImGui::SameLine();
		id++;
		ImGui::PopID();
		if (ImGui::ImageButton((ImTextureID)materialInstance->MetallicMap->GetRendererID(), ImVec2(16, 16), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
		{
			std::string path = FileDialogs::OpenFile(filesFlag);
			if (!path.empty())
				materialInstance->MetallicMap = Texture2D::Create(path);
		}
		ImGui::SameLine();
		ImGui::SliderFloat("Metallic", &materialInstance->Metallic, 0.0f, 1.0f);
		//==============================================

		//==============================================
		// Roughness
		ImGui::PushID(id);
		ImGui::Checkbox("", &materialInstance->UseRoughnessMap); ImGui::SameLine();
		id++;
		ImGui::PopID();
		if (ImGui::ImageButton((ImTextureID)materialInstance->RoughnessMap->GetRendererID(), ImVec2(16, 16), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
		{
			std::string path = FileDialogs::OpenFile(filesFlag);
			if (!path.empty())
				materialInstance->RoughnessMap = Texture2D::Create(path);
		}
		ImGui::SameLine();
		ImGui::SliderFloat("Roughness", &materialInstance->Roughness, 0.01f, 1.0f);
		//==============================================

		//==============================================
		// Occlusion
		ImGui::PushID(id);
		ImGui::Checkbox("", &materialInstance->UseOcclusionMap); ImGui::SameLine();
		id++;
		ImGui::PopID();
		if (ImGui::ImageButton((ImTextureID)materialInstance->AmbientOcclusionMap->GetRendererID(), ImVec2(16, 16), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
		{
			std::string path = FileDialogs::OpenFile(filesFlag);
			if (!path.empty())
				materialInstance->AmbientOcclusionMap = Texture2D::Create(path);
		}
		ImGui::SameLine();
		ImGui::SliderFloat("Occlusion", &materialInstance->AO, 0.0f, 1.0f);
		//==============================================


		//==============================================
		// Emmission
		ImGui::PushID(id);
		ImGui::Checkbox("", &materialInstance->UseEmissiveMap); ImGui::SameLine();
		id++;
		ImGui::PopID();
		if (ImGui::ImageButton((ImTextureID)materialInstance->EmissiveMap->GetRendererID(), ImVec2(16, 16), ImVec2(0,1), ImVec2(1,0), -1, ImVec4(0,0,0,0), ImVec4(0.9, 0.9f, 0.9f, 1.0f)))
		{
			std::string path = FileDialogs::OpenFile(filesFlag);
			if (!path.empty())
				materialInstance->EmissiveMap = Texture2D::Create(path);
		}
		ImGui::SameLine();
		ImGui::ColorEdit3("Color", (float*)&materialInstance->EmissiveColor);
		ImGui::SliderFloat("Intensity", &materialInstance->EmissiveIntensity, 0.0f, 10.0f);
		//==============================================
	}
	
	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if(entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, tag.c_str());
			if(ImGui::InputText("##Tag", buffer, sizeof(buffer)))
				tag = std::string(buffer);
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("Add Component");

		if (ImGui::BeginPopup("Add Component"))
		{
			if (ImGui::MenuItem("Camera"))
			{
				if (!entity.HasComponent<CameraComponent>())
					entity.AddComponent<CameraComponent>();
				else
					ARC_CORE_WARN("This entity already has the Camera Component!");
				ImGui::CloseCurrentPopup();
			}
			
			if (ImGui::MenuItem("Sprite Renderer"))
			{
				if (!entity.HasComponent<SpriteRendererComponent>())
					entity.AddComponent<SpriteRendererComponent>();
				else
					ARC_CORE_WARN("This entity already has the Sprite Renderer Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Rigidbody 2D"))
			{
				if (!entity.HasComponent<Rigidbody2DComponent>())
					entity.AddComponent<Rigidbody2DComponent>();
				else
					ARC_CORE_WARN("This entity already has the Rigidbody 2D Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Box Collider 2D"))
			{
				if (!entity.HasComponent<BoxCollider2DComponent>())
					entity.AddComponent<BoxCollider2DComponent>();
				else
					ARC_CORE_WARN("This entity already has the Box Collider 2D Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Circle Collider 2D"))
			{
				if (!entity.HasComponent<CircleCollider2DComponent>())
					entity.AddComponent<CircleCollider2DComponent>();
				else
					ARC_CORE_WARN("This entity already has the Circle Collider 2D Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Skylight"))
			{
				if (!entity.HasComponent<SkylightComponent>())
					entity.AddComponent<SkylightComponent>();
				else
					ARC_CORE_WARN("This entity already has the Skylight Component!");
				ImGui::CloseCurrentPopup();
			}
			
			if (ImGui::MenuItem("Light"))
			{
				if (!entity.HasComponent<LightComponent>())
					entity.AddComponent<LightComponent>();
				else
					ARC_CORE_WARN("This entity already has the Light Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Mesh"))
			{
				if (!entity.HasComponent<MeshComponent>())
					entity.AddComponent<MeshComponent>();
				else
					ARC_CORE_WARN("This entity already has the Mesh Component!");
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Behaviour"))
			{
				entity.AddComponent<BehaviourComponent>();
				ImGui::CloseCurrentPopup();
			}
			
			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();
		
		DrawComponent<TransformComponent>("Transform", entity, [](Entity& e, TransformComponent& component)
		{
			DrawVec3Control("Translation", component.Translation);
			
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);

			DrawVec3Control("Scale", component.Scale, 1.0f);
		}, false);

		DrawComponent<CameraComponent>("Camera", entity, [](Entity& e, CameraComponent& component)
		{
			auto& camera = component.Camera;

			DrawCheckbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
			if(ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if(ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if(isSelected)
						ImGui::SetItemDefaultFocus();
				}
				
				ImGui::EndCombo();
			}

			if(camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if(ImGui::DragFloat("Vertical FOV", &verticalFov, 0.1f))
					camera.SetPerspectiveVerticalFOV(glm::radians(verticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if(ImGui::DragFloat("Near Clip", &perspectiveNear, 0.1f))
					camera.SetPerspectiveNearClip(perspectiveNear);
				
				float perspectiveFar = camera.GetPerspectiveFarClip();
				if(ImGui::DragFloat("Far Clip", &perspectiveFar, 0.1f))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if(camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if(ImGui::DragFloat("Size", &orthoSize, 0.1f))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if(ImGui::DragFloat("Near Clip", &orthoNear, 0.1f))
					camera.SetOrthographicNearClip(orthoNear);
				
				float orthoFar = camera.GetOrthographicFarClip();
				if(ImGui::DragFloat("Far Clip", &orthoFar, 0.1f))
					camera.SetOrthographicFarClip(orthoFar);

				DrawCheckbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
		});
		
		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](Entity& e, SpriteRendererComponent& component)
		{
			SetLabel("Color");
			ImGui::ColorEdit4("##Color", glm::value_ptr(component.Color));

			const uint32_t id = component.Texture == nullptr ? 0 : component.Texture->GetRendererID();

			SetLabel("Texture");
			const ImVec2 buttonSize = { 80, 80 };
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
			if(ImGui::ImageButton((ImTextureID)id, buttonSize, { 0, 1 }, { 1, 0}, 0))
			{
				std::string filepath = FileDialogs::OpenFile("Texture (*.png)\0*.png\0");
				if (!filepath.empty())
					component.SetTexture(filepath);
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
			if(ImGui::Button("x", { buttonSize.x / 4, buttonSize.y } ))
				component.RemoveTexture();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();

			ImGui::Spacing();
			
			DrawFloatControl("Tiling Factor", &component.TilingFactor, 0, 0);
		});

		DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", entity, [](Entity& e, Rigidbody2DComponent& component)
		{
			{
				const char* items[] = { "Static", "Kinematic", "Dynamic" };
				const char* current_item = items[(int)component.Specification.Type];
				SetLabel("Body Type");
				if (ImGui::BeginCombo("##BodyType", current_item))
				{
					for (int n = 0; n < 3; n++)
					{
						bool is_selected = (current_item == items[n]);
						if (ImGui::Selectable(items[n], is_selected))
						{
							current_item = items[n];
							component.Specification.Type = (Rigidbody2dType)n;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			if (component.Specification.Type == Rigidbody2dType::Dynamic)
			{
				DrawFloatControl("Linear Damping", &component.Specification.LinearDamping, 0.0f, 1000000.0f);
				DrawFloatControl("Angular Damping", &component.Specification.AngularDamping, 0.0f, 1000000.0f);
				DrawFloatControl("Gravity Scale", &component.Specification.GravityScale, -1000000.0f, 1000000.0f);
			}
			if (component.Specification.Type == Rigidbody2dType::Dynamic || component.Specification.Type == Rigidbody2dType::Kinematic)
			{
				{
					const char* items[] = { "Discrete", "Continuous" };
					const char* current_item = items[(int)component.Specification.CollisionDetection];
					SetLabel("Collision Detection");
					if (ImGui::BeginCombo("##CollisionDetection", current_item))
					{
						for (int n = 0; n < 2; n++)
						{
							bool is_selected = (current_item == items[n]);
							if (ImGui::Selectable(items[n], is_selected))
							{
								current_item = items[n];
								component.Specification.CollisionDetection = (Rigidbody2D::CollisionDetectionType)n;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				{
					const char* items[] = { "NeverSleep", "StartAwake", "StartAsleep" };
					const char* current_item = items[(int)component.Specification.SleepingMode];
					SetLabel("Sleeping Mode");
					if (ImGui::BeginCombo("##SleepingMode", current_item))
					{
						for (int n = 0; n < 3; n++)
						{
							bool is_selected = (current_item == items[n]);
							if (ImGui::Selectable(items[n], is_selected))
							{
								current_item = items[n];
								component.Specification.SleepingMode = (Rigidbody2D::SleepType)n;
							}

							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				DrawCheckbox("FreezeRotationZ", &component.Specification.FreezeRotationZ);
				ImGui::SameLine();
				ImGui::Text("Z");
			}

			// Debug
			if (component.Body2D)
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth;
				if (ImGui::TreeNodeEx((void*)typeid(component.Body2D).hash_code(), treeNodeFlags, "Info"))
				{
					SetLabel("Mass");
					ImGui::Text("%.4f", component.Body2D->GetMass());
					SetLabel("Position");
					ImGui::Text("%f, %f", component.Body2D->GetPosition().x, component.Body2D->GetPosition().y);
					SetLabel("Rotation");
					ImGui::Text("%f", component.Body2D->GetRotation());
					SetLabel("Velocity");
					ImGui::Text("%f, %f", component.Body2D->GetVelocity().x, component.Body2D->GetVelocity().y);
					SetLabel("Angular Velocity");
					ImGui::Text("%f", component.Body2D->GetAngularVelocity());
					SetLabel("Intertia");
					ImGui::Text("%f", component.Body2D->GetInertia());
					SetLabel("Local Center of Mass");
					ImGui::Text("%f, %f", component.Body2D->GetLocalCenterOfMass().x, component.Body2D->GetLocalCenterOfMass().y);
					SetLabel("World Center of Mass");
					ImGui::Text("%f, %f", component.Body2D->GetWorldCenterOfMass().x, component.Body2D->GetWorldCenterOfMass().y);
					SetLabel("Sleep State");
					ImGui::Text("%s", component.Body2D->IsAwake() ? "Awake" : "Asleep");
					ImGui::TreePop();
				}
			}

			component.ValidateSpecification();
		});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](Entity& e, BoxCollider2DComponent& component)
		{
			DrawCheckbox("IsTrigger", &component.IsTrigger);

			SetLabel("Size");
			glm::vec2 size = component.Size;
			ImGui::DragFloat2("##Size", glm::value_ptr(size), 0.01f, 0, 0, "%.4f");
			if (size.x <= 0.1f)
				size.x = 0.1f;
			if (size.y <= 0.1f)
				size.y = 0.1f;
			component.Size = size;

			SetLabel("Offset");
			ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.01f, 0, 0, "%.4f");

			// Debug
			if (component.Collider2D)
			{
				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth;
				if (ImGui::TreeNodeEx((void*)typeid(component.Collider2D).hash_code(), treeNodeFlags, "Info"))
				{
					SetLabel("Density");
					ImGui::Text("%.4f", component.Collider2D->GetDensity());
					ImGui::TreePop();
				}
			}
			
			component.ValidateSpecification();
		});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](Entity& e, CircleCollider2DComponent& component)
		{
			DrawCheckbox("IsTrigger", &component.IsTrigger);

			SetLabel("Radius");
			ImGui::DragFloat("##Radius", &component.Radius, 0.01f, 0.1, 0, "%.4f");

			SetLabel("Offset");
			ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.01f, 0, 0, "%.4f");

			component.ValidateSpecification();
		});

		DrawComponent<SkylightComponent>("Skylight Component", entity, [](Entity& e, SkylightComponent& component)
		{
			const uint32_t id = component.Texture != nullptr ? component.Texture->GetHDRRendererID() : 0;
			
			SetLabel("Texture");
			const ImVec2 buttonSize = { 80, 80 };
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f });
			if (ImGui::ImageButton((ImTextureID)id, buttonSize, { 0, 1 }, { 1, 0 }, 0))
			{
				std::string filepath = FileDialogs::OpenFile("Texture (*.hdr)\0*.hdr\0");
				if (!filepath.empty())
					component.SetTexture(filepath);
			}
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f });
			if (ImGui::Button("x", { buttonSize.x / 4, buttonSize.y }))
				component.RemoveTexture();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();
		});

		DrawComponent<LightComponent>("Light Component", entity, [](Entity& e, LightComponent& component)
		{
			{
				const char* items[] = { "Directional", "Point", "Spot", "Area" };
				const char* current_item = items[(int)component.Type];
				SetLabel("Type");
				if (ImGui::BeginCombo("##Type", current_item))
				{
					for (int n = 0; n < 4; n++)
					{
						bool is_selected = (current_item == items[n]);
						if (ImGui::Selectable(items[n], is_selected))
						{
							current_item = items[n];
							component.Type = (LightComponent::LightType)n;
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			
			SetLabel("Light Color");
			ImGui::ColorEdit3("##LightColor", glm::value_ptr(component.Color));

			SetLabel("Intensity");
			ImGui::DragFloat("##Intensity", &component.Intensity);
		});
		
		DrawComponent<MeshComponent>("Mesh Component", entity, [](Entity& e, MeshComponent& component)
		{
			ImGui::Text("Mesh");

			if(component.mesh)
				ImGui::Text(component.mesh->GetFilePath().c_str());
			else
				ImGui::Text("Empty");

			ImGui::SameLine(0.0f, ImGui::GetContentRegionAvail().x - 75);
			if (ImGui::Button("...", { 30, 20 }))
			{
				std::string filepath = FileDialogs::OpenFile("3D Model (*.obj)\0*.obj\0 (*.fbx)\0*.fbx\0");
				if (!filepath.empty())
					component.Set(e.GetComponent<IDComponent>().ID, filepath);
			}

			if(component.mesh)
			{
				Ref<Mesh> mesh = component.mesh;
				
				ImGui::Text(mesh->GetName().c_str());

				ImGui::Separator();

				if (ImGui::TreeNode("Material List"))
				{
					for (int i = 0; i < mesh->GetMaterialsCount(); i++)
					{
						ImGui::PushID(i);
						auto& material = mesh->GetMaterialInstance(i);

						if (ImGui::TreeNode(material->Name.c_str()))
						{
							Ref<PbrMaterial> pbr = std::dynamic_pointer_cast<PbrMaterial>(material);
							DrawPBRProperties(pbr);

							ImGui::TreePop();
						}
						ImGui::Separator();

						ImGui::PopID();
					}
					ImGui::TreePop();
				}
			}
		});

		DrawComponent<BehaviourComponent>("Behaviour Component", entity, [](Entity& e, BehaviourComponent& component)
		{
			struct Test
			{
				static int Callback(ImGuiInputTextCallbackData* data)
				{
					ARC_CORE_TRACE("ScriptName: {0}", data->Buf);
					return 0;
				}
			};

			ImGui::Text("Script");
			
			if (component.Initialized)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(0, 255, 0)));
			else
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(255, 0, 0)));
			
			if (ImGui::InputText("Input", component.BufferName, 256, ImGuiInputTextFlags_CallbackCharFilter, Test::Callback))
			{
				component.ModuleName = component.BufferName;
				component.Initialized = ScriptEngine::HasClass(component);
			}

			ImGui::PopStyleColor();

			/*
			if (ImGui::TreeNodeEx((void*) ((uint32_t) e | typeid(BehaviourComponent).hash_code()), ImGuiTreeNodeFlags_DefaultOpen, "Script"))
			{
				// Public Fields
				auto& fieldMap = ScriptEngine::GetFieldMap();
				if (fieldMap.find(sc.ModuleName) != fieldMap.end())
				{
					auto& publicFields = fieldMap.at(sc.ModuleName);
					for (auto& field : publicFields)
					{
						switch (field.Type)
						{
							case FieldType::Int:
							{
								int value = field.GetValue<int>();
								if (Property(field.Name.c_str(), value))
								{
									field.SetValue(value);
								}
								break;
							}
							case FieldType::Float:
							{
								float value = field.GetValue<float>();
								if (Property(field.Name.c_str(), value, 0.2f))
								{
									field.SetValue(value);
								}
								break;
							}
							case FieldType::Vec2:
							{
								glm::vec2 value = field.GetValue<glm::vec2>();
								if (Property(field.Name.c_str(), value, 0.2f))
								{
									field.SetValue(value);
								}
								break;
							}
						}
					}
				}

				EndPropertyGrid();
				if (ImGui::Button("Run Script"))
				{
					ScriptEngine::OnCreateEntity(entity);
				}
				ImGui::TreePop();
				ImGui::Separator();
			}
			*/
		});
	}
}
