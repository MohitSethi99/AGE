#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


#include "Arc/Renderer/Texture.h"
#include "Arc/Physics/Rigidbody2D.h"
#include "Arc/Physics/BoxCollider2D.h"
#include "Arc/Physics/CircleCollider2D.h"
#include "Arc/Renderer/Mesh.h"
#include "Arc/Scene/SceneCamera.h"
#include "Arc/Scene/ScriptableEntity.h"

namespace ArcEngine
{
	struct IDComponent
	{
		uint32_t ID = 0;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(const uint32_t id)
			: ID(id) {}
		operator uint32_t() { return ID; }
	};

	struct TagComponent
	{
		std::string Tag;
		bool renaming = false;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};
	
	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.0f), Translation) * glm::toMat4(glm::quat(Rotation)) * glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture = nullptr;
		std::string TextureFilepath;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {}
		void SetTexture(std::string& filepath)
		{
			Texture = Texture2D::Create(filepath);
			TextureFilepath = filepath;
		}
		void RemoveTexture()
		{
			Texture = nullptr;
			TextureFilepath = "";
		}
	};

	struct MeshComponent
	{
		Ref<Mesh> mesh;
		operator Ref<Mesh>& () { return mesh; }
		
		enum class Geometry
		{
			Cube = 0,
			Sphere,
			Plane,
			Quad,
			Cone,
			Cylinder
		};

		MeshComponent() = default;
		MeshComponent(int entityID, const std::string& meshPath)
		{
			mesh = CreateRef<Mesh>(entityID, meshPath);
		}
		MeshComponent(int entityID, Geometry geometry)
		{
			switch (geometry)
			{
				case Geometry::Cube:
					mesh = CreateRef<Mesh>(entityID, "assets/models/primitives/cube.fbx");
					break;
				case Geometry::Sphere:
					mesh = CreateRef<Mesh>(entityID, "assets/models/primitives/sphere.fbx");
					break;
				case Geometry::Plane:
					mesh = CreateRef<Mesh>(entityID, "assets/models/primitives/plane.fbx");
					break;
				case Geometry::Quad:
					mesh = CreateRef<Mesh>(entityID, "assets/models/primitives/quad.fbx");
					break;
				case Geometry::Cone:
					mesh = CreateRef<Mesh>(entityID, "assets/models/primitives/cone.fbx");
					break;
				case Geometry::Cylinder:
					mesh = CreateRef<Mesh>(entityID, "assets/models/primitives/cylinder.fbx");
					break;
			}
		}

		void Set(int entityID, const std::string& filepath)
		{
			mesh = CreateRef<Mesh>(entityID, filepath);
		}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);
		
		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	struct Rigidbody2DComponent
	{
		Rigidbody2D::Rigidbody2DSpecification Specification;
		Ref<Rigidbody2D> Body2D;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;

		void StartSimulation(const glm::vec2& translation, const float rotation)
		{
			Body2D = CreateRef<Rigidbody2D>(translation, rotation, Specification);
		}
		
		void ValidateSpecification()
		{
			if(!Body2D)
				return;

			const auto spec = Body2D->GetSpecification();


			if (spec->Type != Specification.Type)
				Body2D->SetType(Specification.Type);
			if (spec->LinearDamping != Specification.LinearDamping)
				Body2D->SetLinearDamping(Specification.LinearDamping);
			if (spec->AngularDamping != Specification.AngularDamping)
				Body2D->SetAngularDamping(Specification.AngularDamping);
			if (spec->GravityScale != Specification.GravityScale)
				Body2D->SetGravityScale(Specification.GravityScale);
			if (spec->CollisionDetection != Specification.CollisionDetection)
				Body2D->SetCollisionDetection(Specification.CollisionDetection);
			if (spec->SleepingMode != Specification.SleepingMode)
				Body2D->SetSleepingMode(Specification.SleepingMode);
			if (spec->FreezeRotationZ != Specification.FreezeRotationZ)
				Body2D->SetFreezeRotation(Specification.FreezeRotationZ);
		}
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Scale{ 1.0f, 1.0f };
		glm::vec2 Size{ 1.0f, 1.0f };
		glm::vec2 Offset{ 0.0f, 0.0f };
		bool IsTrigger = false;

		Ref<BoxCollider2D> Collider2D;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;

		void StartSimulation(Ref<Rigidbody2D>& rigidbody2D)
		{
			Collider2D = CreateRef<BoxCollider2D>(rigidbody2D, Size * Scale, Offset, IsTrigger);
		}

		void ValidateSpecification()
		{
			if (!Collider2D)
				return;
			
			if (Size * Scale != Collider2D->GetSize() || Offset != Collider2D->GetOffset() || IsTrigger != Collider2D->IsTrigger())
				Collider2D->SetSpecification(Size, Offset, IsTrigger);
		}
	};

	struct CircleCollider2DComponent
	{
		float Radius = 0.5f;
		glm::vec2 Offset{ 0.0f, 0.0f };
		bool IsTrigger = false;

		Ref<CircleCollider2D> Collider2D;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;

		void StartSimulation(Ref<Rigidbody2D>& rigidbody2D)
		{
			Collider2D = CreateRef<CircleCollider2D>(rigidbody2D, Radius, Offset, IsTrigger);
		}

		void ValidateSpecification()
		{
			if (!Collider2D)
				return;
			
			if(Radius != Collider2D->GetRadius() || Offset != Collider2D->GetOffset() || IsTrigger != Collider2D->IsTrigger())
				Collider2D->SetSpecification(Radius, Offset, IsTrigger);
		}
	};

	struct SkylightComponent
	{
		Ref<TextureCube> Texture = nullptr;
		std::string TextureFilepath;

		SkylightComponent() = default;
		SkylightComponent(const SkylightComponent&) = default;
		void SetTexture(std::string& filepath)
		{
			Texture = TextureCube::Create(filepath);
			TextureFilepath = filepath;
		}
		void RemoveTexture()
		{
			Texture = nullptr;
			TextureFilepath = "";
		}
	};

	struct LightComponent
	{
		enum class LightType
		{
			Directional = 0,
			Point,

			// Not Implemented in shader yet!
			Spot,
			Area
		};

		LightType Type = LightType::Directional;
		glm::vec3 Color = glm::vec3(1.0f);
		float Intensity = 1.0f;

		float Constant = 1.0f;
		float Linear = 0.09f;
		float Quadratic = 0.032f;

		LightComponent() = default;
		LightComponent(LightType type)
			: Type(type)
		{
		}
	};

	struct BehaviourComponent
	{
		bool Initialized = false;
		char BufferName[256];
		std::string ModuleName = "";
	};
}
