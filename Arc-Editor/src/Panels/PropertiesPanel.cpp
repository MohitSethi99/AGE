#include "PropertiesPanel.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Arc/Scene/EntitySerializer.h>

#include "../Utils/UI.h"
#include "../Utils/EditorTheme.h"
#include "../EditorLayer.h"

namespace ArcEngine
{
	void PropertiesPanel::OnImGuiRender()
	{
		ARC_PROFILE_SCOPE();

		if (OnBegin())
		{
			EditorContext context = EditorLayer::GetInstance()->GetContext();
			switch (context.Type)
			{
				case EditorContextType::None:
					break;
				case EditorContextType::Entity:
					if (m_Context.Data)
					{
						Entity selectedEntity = *((Entity*)m_Context.Data);
						if (selectedEntity && selectedEntity.GetScene())
							DrawComponents(selectedEntity);
					}
					break;
				case EditorContextType::File:
					if (m_Context.Data)
					{
						const char* selectedFile = (char*)m_Context.Data;
						DrawFileProperties(selectedFile);
					}
					break;
			}

			OnEnd();
		}
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const char* name, Entity entity, UIFunction uiFunction, const bool removable = true)
	{
		ARC_PROFILE_SCOPE();

		if(entity.HasComponent<T>())
		{
			static const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen
				| ImGuiTreeNodeFlags_SpanAvailWidth
				| ImGuiTreeNodeFlags_AllowItemOverlap
				| ImGuiTreeNodeFlags_Framed
				| ImGuiTreeNodeFlags_FramePadding;

			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
			
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + lineHeight * 0.25f);

			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeFlags, name);

			bool removeComponent = false;
			if(removable)
			{
				ImGui::SameLine(ImGui::GetContentRegionMax().x - lineHeight);
				if(ImGui::Button(ICON_MDI_SETTINGS, ImVec2{ lineHeight, lineHeight }))
					ImGui::OpenPopup("ComponentSettings");

				if(ImGui::BeginPopup("ComponentSettings"))
				{
					if(ImGui::MenuItem("Remove Component"))
						removeComponent = true;
					
					ImGui::EndPopup();
				}
			}
			ImGui::PopStyleVar();

			if(open)
			{
				ARC_PROFILE_SCOPE("UI Function");

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().IndentSpacing / 2);
				uiFunction(component);
				ImGui::TreePop();
			}

			if(removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	static void DrawFields(Entity entity, ScriptComponent& component)
	{
		static const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_FramePadding;

		eastl::string* toRemove = nullptr;
		
		for (auto it = component.Classes.begin(); it != component.Classes.end(); ++it)
		{
			eastl::string& className = *it;
			auto instance = ScriptEngine::GetInstance(entity, className);
			GCHandle handle = instance->GetHandle();

			ImGui::PushID(handle);

			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + lineHeight * 0.25f);

			bool open = ImGui::TreeNodeEx((void*)handle, treeFlags, className.c_str());

			{
				ImGui::SameLine(ImGui::GetContentRegionMax().x - lineHeight);
				if (ImGui::Button(ICON_MDI_SETTINGS, ImVec2{ lineHeight, lineHeight }))
					ImGui::OpenPopup("ComponentSettings");

				if (ImGui::BeginPopup("ComponentSettings"))
				{
					if (ImGui::MenuItem("Remove"))
						toRemove = it;

					ImGui::EndPopup();
				}
			}
			ImGui::PopStyleVar();

			if (open)
			{
				ARC_PROFILE_SCOPE("UI Function");

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().IndentSpacing / 2);

				// Public Fields
				UI::BeginProperties();
				auto& fields = ScriptEngine::GetFields(entity, className.c_str());
				auto& fieldMap = ScriptEngine::GetFieldMap(entity, className.c_str());
				for (auto& name : fields)
				{
					auto& field = fieldMap.at(name);
					if (field->Hidden)
						continue;

					const char* header = field->Header.empty() ? nullptr : field->Header.c_str();
					if (header)
					{
						UI::EndProperties();
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::TextUnformatted(header);
						ImGui::Spacing();
						UI::BeginProperties();
					}

					UI::DrawField(*field);
				}
				UI::EndProperties();

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		if (toRemove)
		{
			ScriptEngine::RemoveInstance(entity, *toRemove);
			component.Classes.erase(toRemove);
		}
	}

	static void DrawMaterialProperties(Ref<Material>& material)
	{
		UI::BeginProperties();

		auto& materialProperties = material->GetShader()->GetMaterialProperties();
		for (auto& [n, property] : materialProperties)
		{
			const char* name = n.c_str();
			const char* displayName = n.c_str() + 11;

			switch (property.Type)
			{
				case MaterialPropertyType::None: break;
				case MaterialPropertyType::Sampler2D:
				{
					uint32_t slot = material->GetData<uint32_t>(name);
					uint32_t tex = material->GetTexture(slot) ? material->GetTexture(slot)->GetRendererID() : 0;
					Ref<Texture2D> tmp = material->GetTexture(slot);
					if (UI::Property(displayName, tmp, tex))
						material->SetTexture(slot, tmp);
					break;
				}
				case MaterialPropertyType::Bool:
				{
					bool v = material->GetData<int32_t>(name) == 0 ? false : true;
					if (UI::Property(displayName, v))
						material->SetData(name, v ? 1 : 0);
					break;
				}
				case MaterialPropertyType::Int:
				{
					int32_t v = material->GetData<int32_t>(name);
					if (UI::Property(displayName, v))
						material->SetData(name, v);
					break;
				}
				case MaterialPropertyType::Float:
				{
					bool isSlider01 = n.find("01") != eastl::string::npos;
					float v = material->GetData<float>(name);
					if (isSlider01)
					{
						eastl::string displayName2 = displayName;
						displayName2[displayName2.size() - 2] = '\0';
						if (UI::Property(displayName2.c_str(), v, 0.0f, 1.0f))
							material->SetData(name, v);
					}
					else
					{
						if (UI::Property(displayName, v))
							material->SetData(name, v);
					}
					break;
				}
				case MaterialPropertyType::Float2:
				{
					glm::vec2 v = material->GetData<glm::vec2>(name);
					if (UI::Property(displayName, v))
						material->SetData(name, v);
					break;
				}
				case MaterialPropertyType::Float3:
				{
					bool isColor = n.find("color") != eastl::string::npos || n.find("Color") != eastl::string::npos;
					glm::vec3 v = material->GetData<glm::vec3>(name);
					if (isColor)
					{
						if (UI::PropertyColor3(displayName, v))
							material->SetData(name, v);
					}
					else
					{
						if (UI::Property(displayName, v))
							material->SetData(name, v);
					}
					break;
				}
				case MaterialPropertyType::Float4:
				{
					bool isColor = n.find("color") != eastl::string::npos || n.find("Color") != eastl::string::npos;
					glm::vec4 v = material->GetData<glm::vec4>(name);
					if (isColor)
					{
						if (UI::PropertyColor4(displayName, v))
							material->SetData(name, v);
					}
					else
					{
						if (UI::Property(displayName, v))
							material->SetData(name, v);
					}
					break;
				}
			}
		}

		UI::EndProperties();
	}

	static void WriteMesh(const eastl::string& filepath)
	{
		ARC_PROFILE_SCOPE();

		Assimp::Importer importer;
		importer.SetPropertyFloat("PP_GSN_MAX_SMOOTHING_ANGLE", 80.0f);
		const uint32_t meshImportFlags =
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_PreTransformVertices |
			aiProcess_SortByPType |
			aiProcess_GenNormals |
			aiProcess_GenUVCoords |
	        aiProcess_OptimizeMeshes |
			aiProcess_JoinIdenticalVertices |
			aiProcess_GlobalScale |
			aiProcess_ImproveCacheLocality |
	        aiProcess_ValidateDataStructure;

		const aiScene *scene = importer.ReadFile(filepath.c_str(), meshImportFlags);
		if (!scene)
		{
			ARC_CORE_ERROR("Could not import the file: {0}. Error: {1}", filepath, importer.GetErrorString());
			return;
		}

		Assimp::Exporter exporter;
		const aiExportFormatDesc* format = exporter.GetExportFormatDescription(0);
		auto lastDot = filepath.find_last_of(".");
		eastl::string path = filepath.substr(0, lastDot) + ".assbin";
		aiReturn ret = exporter.Export(scene, format->id, path.c_str(), meshImportFlags);
		if (ret != aiReturn_SUCCESS)
		{
			ARC_CORE_ERROR("Could not import the file: {0}. Error: {1}", filepath, exporter.GetErrorString());
			return;
		}
	}

	void PropertiesPanel::DrawComponents(Entity entity)
	{
		ARC_PROFILE_SCOPE();

		if(entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, tag.c_str());
			if(ImGui::InputText("##Tag", buffer, sizeof(buffer)))
				tag = eastl::string(buffer);
		}

		ImGui::SameLine();

		// Add Button
		{
			if (UI::IconButton("  " ICON_MDI_PLUS, "Add  "))
				ImGui::OpenPopup("AddComponentPopup");

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				DrawAddComponent<CameraComponent>(entity, ICON_MDI_CAMERA " Camera");
				DrawAddComponent<SpriteRendererComponent>(entity, ICON_MDI_IMAGE_SIZE_SELECT_ACTUAL " Sprite Renderer");
				DrawAddComponent<MeshComponent>(entity, ICON_MDI_VECTOR_SQUARE " Mesh");
				DrawAddComponent<SkyLightComponent>(entity, ICON_MDI_EARTH " Sky Light");
				DrawAddComponent<LightComponent>(entity, ICON_MDI_LIGHTBULB " Light");
				DrawAddComponent<Rigidbody2DComponent>(entity, ICON_MDI_SOCCER " Rigidbody 2D");
				DrawAddComponent<BoxCollider2DComponent>(entity, ICON_MDI_CHECKBOX_BLANK_OUTLINE " Box Collider 2D");
				DrawAddComponent<CircleCollider2DComponent>(entity, ICON_MDI_CIRCLE_OUTLINE " Circle Collider 2D");
				DrawAddComponent<DistanceJoint2DComponent>(entity, ICON_MDI_VECTOR_LINE " Distance Joint 2D");
				DrawAddComponent<SpringJoint2DComponent>(entity, ICON_MDI_VECTOR_LINE " Spring Joint 2D");
				DrawAddComponent<HingeJoint2DComponent>(entity, ICON_MDI_ANGLE_ACUTE " Hinge Joint 2D");
				DrawAddComponent<SliderJoint2DComponent>(entity, ICON_MDI_ANGLE_ACUTE " Slider Joint 2D");
				DrawAddComponent<WheelJoint2DComponent>(entity, ICON_MDI_CAR " Wheel Joint 2D");
				DrawAddComponent<BuoyancyEffector2DComponent>(entity, ICON_MDI_WATER " Buoyancy Effector 2D");
				DrawAddComponent<ScriptComponent>(entity, ICON_MDI_POUND_BOX " Script");
				DrawAddComponent<AudioSourceComponent>(entity, ICON_MDI_VOLUME_MEDIUM " Audio");
				DrawAddComponent<AudioListenerComponent>(entity, ICON_MDI_CIRCLE_SLICE_8 " Audio Listener");

				ImGui::EndPopup();
			}
		}

		ImGui::SameLine();

		// Lock Button
		{
			float frameHeight = ImGui::GetFrameHeight();
			ImVec2 region = ImGui::GetContentRegionMax();
			ImVec2 lockButtonSize = ImVec2(frameHeight * 1.5f, frameHeight);
			ImGui::SetCursorPosX(region.x - lockButtonSize.x);

			const char* icon = m_Locked ? ICON_MDI_LOCK : ICON_MDI_LOCK_OPEN_OUTLINE;
			if (UI::ToggleButton(icon, m_Locked, lockButtonSize))
				m_Locked = !m_Locked;
		}

		DrawComponent<TransformComponent>(ICON_MDI_VECTOR_LINE " Transform", entity, [](TransformComponent& component)
		{
			UI::BeginProperties();

			UI::DrawVec3Control("Translation", component.Translation);
			
			glm::vec3 rotation = glm::degrees(component.Rotation);
			UI::DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);

			UI::DrawVec3Control("Scale", component.Scale, nullptr, 1.0f);

			UI::EndProperties();
		}, false);

		DrawComponent<CameraComponent>(ICON_MDI_CAMERA " Camera", entity, [](CameraComponent& component)
		{
			auto& camera = component.Camera;

			UI::BeginProperties();

			UI::Property("Primary", component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			int projectionType = (int) camera.GetProjectionType();
			if (UI::Property("Projection", projectionType, projectionTypeStrings, 2))
				camera.SetProjectionType((SceneCamera::ProjectionType) projectionType);

			if(camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if(UI::Property("Vertical FOV", verticalFov))
					camera.SetPerspectiveVerticalFOV(glm::radians(verticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if(UI::Property("Near Clip", perspectiveNear))
					camera.SetPerspectiveNearClip(perspectiveNear);
				
				float perspectiveFar = camera.GetPerspectiveFarClip();
				if(UI::Property("Far Clip", perspectiveFar))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if(camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if(UI::Property("Size", orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if(UI::Property("Near Clip", orthoNear))
					camera.SetOrthographicNearClip(orthoNear);
				
				float orthoFar = camera.GetOrthographicFarClip();
				if(UI::Property("Far Clip", orthoFar))
					camera.SetOrthographicFarClip(orthoFar);

				UI::Property("Fixed Aspect Ratio", component.FixedAspectRatio);
			}

			UI::EndProperties();
		});
		
		DrawComponent<SpriteRendererComponent>(ICON_MDI_IMAGE_SIZE_SELECT_ACTUAL " Sprite Renderer", entity, [](SpriteRendererComponent& component)
		{
			UI::BeginProperties();
			UI::PropertyColor4("Color", component.Color);
			UI::Property("Tiling Factor", component.TilingFactor);
			UI::Property("Texture", component.Texture);
			UI::EndProperties();
		});

		DrawComponent<MeshComponent>(ICON_MDI_VECTOR_SQUARE " Mesh", entity, [&](MeshComponent& component)
		{
			if(ImGui::Button("Import"))
			{
				eastl::string filepath = FileDialogs::OpenFile("Mesh (*.obj)\0*.obj\0(*.fbx)\0*.fbx\0");
				if (!filepath.empty())
				{
					component.Filepath = filepath;
					WriteMesh(filepath.c_str());
					return;
				}
			}
			ImGui::SameLine();
			if(ImGui::Button(component.MeshGeometry ? component.MeshGeometry->GetName() : "null"))
			{
				eastl::string filepath = FileDialogs::OpenFile("Mesh (*.assbin)\0*.assbin\0(*.obj)\0*.obj\0(*.fbx)\0*.fbx\0");
				if (!filepath.empty())
				{
					component.Filepath = filepath;
					component.MeshGeometry = CreateRef<Mesh>(filepath.c_str());
					return;
				}
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const char* path = (const char*)payload->Data;
					component.Filepath = path;
					component.MeshGeometry = CreateRef<Mesh>(path);

					ImGui::EndDragDropTarget();
					return;
				}
				ImGui::EndDragDropTarget();
			}

			if (component.MeshGeometry)
			{
				UI::BeginProperties();

				UI::Property("Submesh Index", component.SubmeshIndex, 0, component.MeshGeometry->GetSubmeshCount() - 1);
				
				const char* cullModeTypeStrings[] = { "Front", "Back", "Double Sided" };
				int cullMode = (int) component.CullMode;
				if (UI::Property("Cull Mode", cullMode, cullModeTypeStrings, 3))
					component.CullMode = (MeshComponent::CullModeType) cullMode;
				UI::EndProperties();

				const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
					| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

				if (ImGui::TreeNodeEx("Material Properties", treeNodeFlags))
				{
					DrawMaterialProperties(component.MeshGeometry->GetSubmesh(component.SubmeshIndex).Mat);
					ImGui::TreePop();
				}
			}
		});

		DrawComponent<SkyLightComponent>(ICON_MDI_EARTH " Sky Light", entity, [](SkyLightComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Texture", component.Texture, component.Texture == nullptr ? 0 : component.Texture->GetHRDRendererID());
			UI::Property("Intensity", component.Intensity);
			UI::Property("Rotation", component.Rotation, 0.0f, 360.0f);
			UI::EndProperties();
		});

		DrawComponent<LightComponent>(ICON_MDI_LIGHTBULB " Light", entity, [](LightComponent& component)
		{
			UI::BeginProperties();
			const char* lightTypeStrings[] = { "Directional", "Point", "Spot" };
			int lightType = (int) component.Type;
			if (UI::Property("Light Type", lightType, lightTypeStrings, 3))
				component.Type = (LightComponent::LightType) lightType;

			if (UI::Property("Use color temperature mode", component.UseColorTemperatureMode))
			{
				if (component.UseColorTemperatureMode)
					ColorUtils::TempratureToColor(component.Temperature, component.Color);
			}

			if (component.UseColorTemperatureMode)
			{
				if (UI::Property("Temperature (K)", component.Temperature, 1000, 40000))
					ColorUtils::TempratureToColor(component.Temperature, component.Color);
			}
			else
			{
				UI::PropertyColor3("Color", component.Color);
			}

			if (UI::Property("Intensity", component.Intensity))
			{
				if (component.Intensity < 0.0f)
					component.Intensity = 0.0f;
			}

			ImGui::Spacing();

			if (component.Type == LightComponent::LightType::Point)
			{
				UI::Property("Range", component.Range);
			}
			else if (component.Type == LightComponent::LightType::Spot)
			{
				UI::Property("Range", component.Range);
				UI::Property("Outer Cut-Off Angle", component.OuterCutOffAngle, 1.0f, 90.0f);
				UI::Property("Cut-Off Angle", component.CutOffAngle, 1.0f, 90.0f);

				if (component.Range < 0.1f)
					component.Range = 0.1f;
				if (component.OuterCutOffAngle < component.CutOffAngle)
					component.CutOffAngle = component.OuterCutOffAngle;
				if (component.CutOffAngle > component.OuterCutOffAngle)
					component.OuterCutOffAngle = component.CutOffAngle;
			}
			else
			{
				const char* shadowQualityTypeStrings[] = { "Hard", "Soft", "Ultra Soft" };
				int shadowQualityType = (int) component.ShadowQuality;

				if (UI::Property("Shadow Quality Type", shadowQualityType, shadowQualityTypeStrings, 3))
					component.ShadowQuality = (LightComponent::ShadowQualityType) shadowQualityType;

				uint32_t textureID = component.ShadowMapFramebuffer->GetDepthAttachmentRendererID();
				ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ 256, 256 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			}

			UI::EndProperties();
		});

		DrawComponent<Rigidbody2DComponent>(ICON_MDI_SOCCER " Rigidbody 2D", entity, [](Rigidbody2DComponent& component)
		{
			UI::BeginProperties();

			const char* bodyTypeStrings[] = { "Static", "Kinematic", "Dynamic" };
			int bodyType = (int) component.Type;
			if (UI::Property("Body Type", bodyType, bodyTypeStrings, 3))
				component.Type = (Rigidbody2DComponent::BodyType)bodyType;

			if (component.Type == Rigidbody2DComponent::BodyType::Dynamic)
			{
				UI::Property("Auto Mass", component.AutoMass);
				if (!component.AutoMass)
					UI::Property("Mass", component.Mass, 0.1f, 0.01f, 100.0f);
				UI::Property("Linear Drag", component.LinearDrag);
				UI::Property("Angular Drag", component.AngularDrag);
				UI::Property("Gravity Scale", component.GravityScale);
				UI::Property("Allow Sleep", component.AllowSleep);
				UI::Property("Awake", component.Awake);
				UI::Property("Continuous", component.Continuous);
				UI::Property("Freeze Rotation", component.FreezeRotation);

				component.LinearDrag = glm::max(component.LinearDrag, 0.0f);
				component.AngularDrag = glm::max(component.AngularDrag, 0.0f);
			}

			UI::EndProperties();
		});

		DrawComponent<BoxCollider2DComponent>(ICON_MDI_CHECKBOX_BLANK_OUTLINE " Box Collider 2D", entity, [](BoxCollider2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Size", component.Size);
			UI::Property("Offset", component.Offset);
			UI::Property("Is Sensor", component.IsSensor);
			UI::EndProperties();
			
			if (!component.IsSensor)
			{
				ImGui::Spacing();
				UI::BeginProperties();
				UI::Property("Density", component.Density);
				UI::Property("Friction", component.Friction);
				UI::Property("Restitution", component.Restitution);
				UI::Property("Restitution Threshold", component.RestitutionThreshold);
				UI::EndProperties();
			}
		});

		DrawComponent<CircleCollider2DComponent>(ICON_MDI_CIRCLE_OUTLINE " Circle Collider 2D", entity, [](CircleCollider2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Radius", component.Radius);
			UI::Property("Offset", component.Offset);
			UI::Property("Is Sensor", component.IsSensor);
			UI::EndProperties();
			
			if (!component.IsSensor)
			{
				ImGui::Spacing();
				UI::BeginProperties();
				UI::Property("Density", component.Density);
				UI::Property("Friction", component.Friction);
				UI::Property("Restitution", component.Restitution);
				UI::Property("Restitution Threshold", component.RestitutionThreshold);
				UI::EndProperties();
			}
		});

		DrawComponent<DistanceJoint2DComponent>(ICON_MDI_VECTOR_LINE " Distance Joint 2D", entity, [&entity](DistanceJoint2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Enable Collision", component.EnableCollision);
			UI::PropertyComponent<Rigidbody2DComponent>("Connected Rigidbody 2D", "Rigidbody 2D", entity.GetScene(), component.ConnectedRigidbody, nullptr);
			UI::Property("Anchor", component.Anchor);
			UI::Property("Connected Anchor", component.ConnectedAnchor);
			UI::Property("Auto Distance", component.AutoDistance);
			if (!component.AutoDistance)
				UI::Property("Distance", component.Distance);
			UI::Property("Min Distance", component.MinDistance);
			UI::Property("Max Distance By", component.MaxDistanceBy);
			UI::Property("Break Force", component.BreakForce);
			UI::EndProperties();

			component.MinDistance = glm::max(component.MinDistance, 0.0f);
			component.MaxDistanceBy = glm::max(component.MaxDistanceBy, 0.0f);
		});

		DrawComponent<SpringJoint2DComponent>(ICON_MDI_VECTOR_LINE " Spring Joint 2D", entity, [&entity](SpringJoint2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Enable Collision", component.EnableCollision);
			UI::PropertyComponent<Rigidbody2DComponent>("Connected Rigidbody 2D", "Rigidbody 2D", entity.GetScene(), component.ConnectedRigidbody, nullptr);
			UI::Property("Anchor", component.Anchor);
			UI::Property("Connected Anchor", component.ConnectedAnchor);
			UI::Property("Auto Distance", component.AutoDistance);
			if (!component.AutoDistance)
				UI::Property("Distance", component.Distance);
			UI::Property("Min Distance", component.MinDistance);
			UI::Property("Max Distance By", component.MaxDistanceBy);
			UI::Property("Frequency", component.Frequency);
			UI::Property("Damping Ratio", component.DampingRatio, 0.0f, 1.0f);
			UI::Property("Break Force", component.BreakForce);
			UI::EndProperties();

			component.MinDistance = glm::max(component.MinDistance, 0.0f);
			component.MaxDistanceBy = glm::max(component.MaxDistanceBy, 0.0f);
			component.Frequency = glm::max(component.Frequency, 0.0f);
		});

		DrawComponent<HingeJoint2DComponent>(ICON_MDI_ANGLE_ACUTE " Hinge Joint 2D", entity, [&entity](HingeJoint2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Enable Collision", component.EnableCollision);
			UI::PropertyComponent<Rigidbody2DComponent>("Connected Rigidbody 2D", "Rigidbody 2D", entity.GetScene(), component.ConnectedRigidbody, nullptr);
			UI::Property("Anchor", component.Anchor);

			UI::Property("Use Limits", component.UseLimits);
			if (component.UseLimits)
			{
				ImGui::Indent();
				UI::Property("Lower Angle", component.LowerAngle, 0.0f, 359.0f);
				component.UpperAngle = glm::max(component.LowerAngle, component.UpperAngle);
				UI::Property("Upper Angle", component.UpperAngle, component.LowerAngle, 359.9f);
				ImGui::Unindent();
			}

			UI::Property("Use Motor", component.UseMotor);
			if (component.UseMotor)
			{
				ImGui::Indent();
				UI::Property("Motor Speed", component.MotorSpeed);
				UI::Property("Max Motor Torque", component.MaxMotorTorque);
				ImGui::Unindent();
			}

			UI::Property("Break Force", component.BreakForce);
			UI::Property("Break Torque", component.BreakTorque);
			UI::EndProperties();
		});

		DrawComponent<SliderJoint2DComponent>(ICON_MDI_ANGLE_ACUTE " Slider Joint 2D", entity, [&entity](SliderJoint2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Enable Collision", component.EnableCollision);
			UI::PropertyComponent<Rigidbody2DComponent>("Connected Rigidbody 2D", "Rigidbody 2D", entity.GetScene(), component.ConnectedRigidbody, nullptr);
			UI::Property("Anchor", component.Anchor);

			UI::Property("Angle", component.Angle);
			UI::Property("Use Limits", component.UseLimits);
			if (component.UseLimits)
			{
				ImGui::Indent();
				UI::Property("Lower Translation", component.LowerTranslation);
				component.UpperTranslation = glm::max(component.LowerTranslation, component.UpperTranslation);
				UI::Property("Upper Angle", component.UpperTranslation);
				ImGui::Unindent();
			}

			UI::Property("Use Motor", component.UseMotor);
			if (component.UseMotor)
			{
				ImGui::Indent();
				UI::Property("Motor Speed", component.MotorSpeed);
				UI::Property("Max Motor Force", component.MaxMotorForce);
				ImGui::Unindent();
			}

			UI::Property("Break Force", component.BreakForce);
			UI::Property("Break Torque", component.BreakTorque);
			UI::EndProperties();
		});

		DrawComponent<WheelJoint2DComponent>(ICON_MDI_CAR " Wheel Joint 2D", entity, [&entity](WheelJoint2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Enable Collision", component.EnableCollision);
			UI::PropertyComponent<Rigidbody2DComponent>("Connected Rigidbody 2D", "Rigidbody 2D", entity.GetScene(), component.ConnectedRigidbody, nullptr);
			UI::Property("Anchor", component.Anchor);

			UI::Property("Frequency", component.Frequency);
			UI::Property("Damping Ratio", component.DampingRatio);
			UI::Property("Use Limits", component.UseLimits);
			if (component.UseLimits)
			{
				ImGui::Indent();
				UI::Property("Lower Translation", component.LowerTranslation);
				component.UpperTranslation = glm::max(component.LowerTranslation, component.UpperTranslation);
				UI::Property("Upper Angle", component.UpperTranslation);
				ImGui::Unindent();
			}

			UI::Property("Use Motor", component.UseMotor);
			if (component.UseMotor)
			{
				ImGui::Indent();
				UI::Property("Motor Speed", component.MotorSpeed);
				UI::Property("Max Motor Torque", component.MaxMotorTorque);
				ImGui::Unindent();
			}

			UI::Property("Break Force", component.BreakForce);
			UI::Property("Break Torque", component.BreakTorque);
			UI::EndProperties();
		});

		DrawComponent<BuoyancyEffector2DComponent>(ICON_MDI_WATER " Buoyancy Effector 2D", entity, [&entity](BuoyancyEffector2DComponent& component)
		{
			UI::BeginProperties();
			UI::Property("Density", component.Density);
			UI::Property("Drag Multiplier", component.DragMultiplier);
			UI::Property("Flip Gravity", component.FlipGravity);
			UI::Property("Flow Magnitude", component.FlowMagnitude);
			UI::Property("Flow Angle", component.FlowAngle);
			UI::EndProperties();
		});

		DrawComponent<ScriptComponent>(ICON_MDI_POUND_BOX " Script", entity, [&](ScriptComponent& component)
		{
			float regionX = ImGui::GetContentRegionAvail().x;
			float frameHeight = ImGui::GetFrameHeight();
			if (ImGui::Button("Add", { regionX, frameHeight }))
			{
				ImGui::OpenPopup("ScriptAddPopup");
			}

			auto& classes = ScriptEngine::GetClasses();
			if (ImGui::BeginPopup("ScriptAddPopup"))
			{
				if (classes.size() > 0)
				{
					const float filterCursorPosX = ImGui::GetCursorPosX();
					m_Filter.Draw("###PropertiesFilter", ImGui::GetContentRegionAvail().x);

					if (!m_Filter.IsActive())
					{
						ImGui::SameLine();
						ImGui::SetCursorPosX(filterCursorPosX + ImGui::GetFontSize() * 0.5f);
						ImGui::TextUnformatted(ICON_MDI_MAGNIFY " Search...");
					}
				}

				for (auto [name, scriptClass] : classes)
				{
					bool notFound = (std::find(component.Classes.begin(), component.Classes.end(), name) == component.Classes.end());
					if (notFound)
					{
						if (!m_Filter.IsActive() || (m_Filter.IsActive() && m_Filter.PassFilter(name.c_str())))
						{
							if (ImGui::MenuItem(name.c_str()))
							{
								ScriptEngine::CreateInstance(entity, name);
								component.Classes.push_back(name);
							}
						}
					}
				}
				ImGui::EndPopup();
			}
			
			DrawFields(entity, component);
		});

		DrawComponent<AudioSourceComponent>(ICON_MDI_VOLUME_MEDIUM " Audio", entity, [&entity](AudioSourceComponent& component)
		{
			auto& config = component.Config;

			const char* filepath = component.Source ? component.Source->GetPath() : "Drop an audio file";
			const float x = ImGui::GetContentRegionAvail().x;
			const float y = ImGui::GetFrameHeight();
			ImGui::Button(filepath, { x, y });
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const char* path = (const char*)payload->Data;
					eastl::string ext = StringUtils::GetExtension(path);
					if (ext == "mp3" || ext == "wav")
						component.Source = CreateRef<AudioSource>(path);
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::Spacing();
			
			UI::BeginProperties();
			UI::Property("Volume Multiplier", config.VolumeMultiplier);
			UI::Property("Pitch Multiplier", config.PitchMultiplier);
			UI::Property("Play On Awake", config.PlayOnAwake);
			UI::Property("Looping", config.Looping);
			UI::EndProperties();

			ImGui::Spacing();
			ImVec2 region = ImGui::GetContentRegionAvail();
			region.y = ImGui::GetFrameHeight();
			ImGui::BeginHorizontal("AudioSourceComponentButtons", region);
			if (ImGui::Button(ICON_MDI_PLAY "Play "))
			{
				if (component.Source)
					component.Source->Play();
			}
			if (ImGui::Button(ICON_MDI_PAUSE "Pause "))
			{
				if (component.Source)
					component.Source->Pause();
			}
			if (ImGui::Button(ICON_MDI_STOP "Stop "))
			{
				if (component.Source)
					component.Source->Stop();
			}
			ImGui::EndHorizontal();
			ImGui::Spacing();

			UI::BeginProperties();
			UI::Property("Spatialization", config.Spatialization);

			if (config.Spatialization)
			{
				ImGui::Indent();
				const char* attenuationTypeStrings[] = { "None", "Inverse", "Linear", "Exponential" };
				int attenuationType = (int)config.AttenuationModel;
				if (UI::Property("Attenuation Model", attenuationType, attenuationTypeStrings, 4))
					config.AttenuationModel = (AttenuationModelType)attenuationType;
				UI::Property("Roll Off", config.RollOff);
				UI::Property("Min Gain", config.MinGain);
				UI::Property("Max Gain", config.MaxGain);
				UI::Property("Min Distance", config.MinDistance);
				UI::Property("Max Distance", config.MaxDistance);
				UI::Property("Cone Inner Angle", config.ConeInnerAngle);
				UI::Property("Cone Outer Angle", config.ConeOuterAngle);
				UI::Property("Cone Outer Gain", config.ConeOuterGain);
				UI::Property("Doppler Factor", config.DopplerFactor);
				ImGui::Unindent();
			}
			UI::EndProperties();

			if (component.Source)
			{
				const glm::mat4 inverted = glm::inverse(entity.GetWorldTransform());
				const glm::vec3 forward = normalize(glm::vec3(inverted[2]));
				component.Source->SetConfig(config);
				component.Source->SetPosition(entity.GetTransform().Translation);
				component.Source->SetDirection(-forward);
			}
		});

		DrawComponent<AudioListenerComponent>(ICON_MDI_CIRCLE_SLICE_8 " Audio Listener", entity, [](AudioListenerComponent& component)
		{
			auto& config = component.Config;
			UI::BeginProperties();
			UI::Property("Active", component.Active);
			UI::Property("Cone Inner Angle", config.ConeInnerAngle);
			UI::Property("Cone Outer Angle", config.ConeOuterAngle);
			UI::Property("Cone Outer Gain", config.ConeOuterGain);
			UI::EndProperties();
		});
	}

	void PropertiesPanel::DrawFileProperties(const char* filepath)
	{
		eastl::string name = StringUtils::GetNameWithExtension(filepath);
		eastl::string& ext = StringUtils::GetExtension(name.c_str());

		if (ext == "prefab")
		{
			static Entity prefab;
			if (m_Scene && prefab)
			{
				DrawComponents(prefab);
			}
			else
			{
				m_Scene = CreateRef<Scene>();
				prefab = EntitySerializer::DeserializeEntityAsPrefab(filepath, *m_Scene);
			}
		}
		else
		{
			if (m_Scene)
				m_Scene = nullptr;

			ImGui::Text(name.c_str());
		}
	}

	template<typename Component>
	void PropertiesPanel::DrawAddComponent(Entity entity, const char* name)
	{
		if (!entity.HasComponent<Component>())
		{
			if (ImGui::MenuItem(name))
			{
				entity.AddComponent<Component>();
				ImGui::CloseCurrentPopup();
			}
		}
	}
}
