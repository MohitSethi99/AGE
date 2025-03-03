#include "arcpch.h"
#include "EntitySerializer.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include "Arc/Audio/AudioListener.h"
#include "Arc/Audio/AudioSource.h"
#include "Arc/Core/AssetManager.h"
#include "Arc/Project/Project.h"
#include "Arc/Scene/Entity.h"
#include "Arc/Scene/Components.h"
#include "Arc/Scene/Scene.h"
#include "Arc/Scripting/ScriptEngine.h"
#include "Arc/Utils/ColorUtils.h"
#include "Arc/Utils/StringUtils.h"

namespace YAML
{
	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			return convert<float>::decode(node[0], rhs.x)
				&& convert<float>::decode(node[1], rhs.y);
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			return convert<float>::decode(node[0], rhs.x)
				&& convert<float>::decode(node[1], rhs.y)
				&& convert<float>::decode(node[2], rhs.z);
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			return convert<float>::decode(node[0], rhs.x)
				&& convert<float>::decode(node[1], rhs.y)
				&& convert<float>::decode(node[2], rhs.z)
				&& convert<float>::decode(node[3], rhs.w);
		}
	};

	template<>
	struct convert<ArcEngine::UUID>
	{
		static Node encode(const ArcEngine::UUID& uuid)
		{
			Node node;
			node.push_back(static_cast<uint64_t>(uuid));
			return node;
		}

		static bool decode(const Node& node, ArcEngine::UUID& uuid)
		{
			uint64_t tmp = 0;
			bool success = convert<uint64_t>::decode(node, tmp);
			if (success)
				uuid = tmp;
			return success;
		}
	};
}

namespace ArcEngine
{
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const UUID& uuid)
	{
		out << YAML::Flow;
		out << static_cast<uint64_t>(uuid);
		return out;
	}

	template<typename T>
	inline T TrySet(T& value, const YAML::Node& node)
	{
		if (node)
			value = node.as<T>(value);
		return value;
	}

	template<typename T>
	inline T TrySetEnum(T& value, const YAML::Node& node)
	{
		if (node)
			value = static_cast<T>(node.as<int>(static_cast<int>(value)));
		return value;
	}

#define READ_FIELD_TYPE(Type, NativeType)										\
			case Type:															\
				out << fieldInstance.GetValue<NativeType>();					\
				break

#define WRITE_FIELD_TYPE(Type, NativeType)										\
			case Type:															\
				{																\
					auto value = field.GetDefaultValue<NativeType>();		\
					TrySet(value, fieldNode);									\
					fieldInstance.SetValue(value);								\
				}																\
				break

	void EntitySerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		ARC_CORE_ASSERT(entity.HasComponent<IDComponent>())

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;

			const auto& tc = entity.GetComponent<TagComponent>();
			out << YAML::Key << "Tag" << YAML::Value << tc.Tag;
			out << YAML::Key << "Layer" << YAML::Value << tc.Layer;
			out << YAML::Key << "Enabled" << YAML::Value << tc.Enabled;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap;

			const auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<RelationshipComponent>())
		{
			out << YAML::Key << "RelationshipComponent";
			out << YAML::BeginMap;

			const auto& tc = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << tc.Parent;

			out << YAML::Key << "ChildrenCount" << YAML::Value << tc.Children.size();
			out << YAML::Key << "Children";
			out << YAML::BeginMap;
			for (size_t i = 0; i < tc.Children.size(); i++)
				out << YAML::Key << i << YAML::Value << tc.Children[i];
			out << YAML::EndMap;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap;

			const auto& cameraComponent = entity.GetComponent<CameraComponent>();
			const auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "ProjectionType" << YAML::Value << static_cast<int>(camera.GetProjectionType());
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap;

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap;

			const auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			out << YAML::Key << "SortingOrder" << YAML::Value << spriteRendererComponent.SortingOrder;
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			std::string texturePath = spriteRendererComponent.Texture ? spriteRendererComponent.Texture->GetPath() : "";
			if (Project::IsPartOfProject(texturePath))
				texturePath = Project::GetAssetRelativeFileSystemPath(texturePath).string();
			std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
			out << YAML::Key << "TexturePath" << YAML::Value << texturePath;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap;

			const auto& skyLightComponent = entity.GetComponent<SkyLightComponent>();

			std::string texturePath = skyLightComponent.Texture ? skyLightComponent.Texture->GetPath() : "";
			if (Project::IsPartOfProject(texturePath))
				texturePath = Project::GetAssetRelativeFileSystemPath(texturePath).string();
			std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
			out << YAML::Key << "TexturePath" << YAML::Value << texturePath;

			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;
			out << YAML::Key << "Rotation" << YAML::Value << skyLightComponent.Rotation;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<LightComponent>())
		{
			out << YAML::Key << "LightComponent";
			out << YAML::BeginMap;

			const auto& lightComponent = entity.GetComponent<LightComponent>();
			out << YAML::Key << "Type" << YAML::Value << static_cast<int>(lightComponent.Type);
			out << YAML::Key << "UseColorTemperatureMode" << YAML::Value << lightComponent.UseColorTemperatureMode;
			out << YAML::Key << "Temperature" << YAML::Value << lightComponent.Temperature;
			out << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
			out << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
			out << YAML::Key << "Range" << YAML::Value << lightComponent.Range;
			out << YAML::Key << "CutOffAngle" << YAML::Value << lightComponent.CutOffAngle;
			out << YAML::Key << "OuterCutOffAngle" << YAML::Value << lightComponent.OuterCutOffAngle;
			out << YAML::Key << "ShadowQuality" << YAML::Value << static_cast<int>(lightComponent.ShadowQuality);

			out << YAML::EndMap;
		}

		if (entity.HasComponent<ParticleSystemComponent>())
		{
			out << YAML::Key << "ParticleSystemComponent";
			out << YAML::BeginMap;

			const auto& particleProps = entity.GetComponent<ParticleSystemComponent>().System->GetProperties();
			out << YAML::Key << "Duration" << YAML::Value << particleProps.Duration;
			out << YAML::Key << "Looping" << YAML::Value << particleProps.Looping;
			out << YAML::Key << "StartDelay" << YAML::Value << particleProps.StartDelay;
			out << YAML::Key << "StartLifetime" << YAML::Value << particleProps.StartLifetime;
			out << YAML::Key << "StartVelocity" << YAML::Value << particleProps.StartVelocity;
			out << YAML::Key << "StartColor" << YAML::Value << particleProps.StartColor;
			out << YAML::Key << "StartSize" << YAML::Value << particleProps.StartSize;
			out << YAML::Key << "StartRotation" << YAML::Value << particleProps.StartRotation;
			out << YAML::Key << "GravityModifier" << YAML::Value << particleProps.GravityModifier;
			out << YAML::Key << "SimulationSpeed" << YAML::Value << particleProps.SimulationSpeed;
			out << YAML::Key << "PlayOnAwake" << YAML::Value << particleProps.PlayOnAwake;
			out << YAML::Key << "MaxParticles" << YAML::Value << particleProps.MaxParticles;
			out << YAML::Key << "RateOverTime" << YAML::Value << particleProps.RateOverTime;
			out << YAML::Key << "RateOverDistance" << YAML::Value << particleProps.RateOverDistance;
			out << YAML::Key << "BurstCount" << YAML::Value << particleProps.BurstCount;
			out << YAML::Key << "BurstTime" << YAML::Value << particleProps.BurstTime;
			out << YAML::Key << "PositionStart" << YAML::Value << particleProps.PositionStart;
			out << YAML::Key << "PositionEnd" << YAML::Value << particleProps.PositionEnd;

			out << YAML::Key << "VelocityOverLifetime.Start" << YAML::Value << particleProps.VelocityOverLifetime.Start;
			out << YAML::Key << "VelocityOverLifetime.End" << YAML::Value << particleProps.VelocityOverLifetime.End;
			out << YAML::Key << "VelocityOverLifetime.Enabled" << YAML::Value << particleProps.VelocityOverLifetime.Enabled;

			out << YAML::Key << "ForceOverLifetime.Start" << YAML::Value << particleProps.ForceOverLifetime.Start;
			out << YAML::Key << "ForceOverLifetime.End" << YAML::Value << particleProps.ForceOverLifetime.End;
			out << YAML::Key << "ForceOverLifetime.Enabled" << YAML::Value << particleProps.ForceOverLifetime.Enabled;

			out << YAML::Key << "ColorOverLifetime.Start" << YAML::Value << particleProps.ColorOverLifetime.Start;
			out << YAML::Key << "ColorOverLifetime.End" << YAML::Value << particleProps.ColorOverLifetime.End;
			out << YAML::Key << "ColorOverLifetime.Enabled" << YAML::Value << particleProps.ColorOverLifetime.Enabled;

			out << YAML::Key << "ColorBySpeed.Start" << YAML::Value << particleProps.ColorBySpeed.Start;
			out << YAML::Key << "ColorBySpeed.End" << YAML::Value << particleProps.ColorBySpeed.End;
			out << YAML::Key << "ColorBySpeed.MinSpeed" << YAML::Value << particleProps.ColorBySpeed.MinSpeed;
			out << YAML::Key << "ColorBySpeed.MaxSpeed" << YAML::Value << particleProps.ColorBySpeed.MaxSpeed;
			out << YAML::Key << "ColorBySpeed.Enabled" << YAML::Value << particleProps.ColorBySpeed.Enabled;

			out << YAML::Key << "SizeOverLifetime.Start" << YAML::Value << particleProps.SizeOverLifetime.Start;
			out << YAML::Key << "SizeOverLifetime.End" << YAML::Value << particleProps.SizeOverLifetime.End;
			out << YAML::Key << "SizeOverLifetime.Enabled" << YAML::Value << particleProps.SizeOverLifetime.Enabled;

			out << YAML::Key << "SizeBySpeed.Start" << YAML::Value << particleProps.SizeBySpeed.Start;
			out << YAML::Key << "SizeBySpeed.End" << YAML::Value << particleProps.SizeBySpeed.End;
			out << YAML::Key << "SizeBySpeed.MinSpeed" << YAML::Value << particleProps.SizeBySpeed.MinSpeed;
			out << YAML::Key << "SizeBySpeed.MaxSpeed" << YAML::Value << particleProps.SizeBySpeed.MaxSpeed;
			out << YAML::Key << "SizeBySpeed.Enabled" << YAML::Value << particleProps.SizeBySpeed.Enabled;

			out << YAML::Key << "RotationOverLifetime.Start" << YAML::Value << particleProps.RotationOverLifetime.Start;
			out << YAML::Key << "RotationOverLifetime.End" << YAML::Value << particleProps.RotationOverLifetime.End;
			out << YAML::Key << "RotationOverLifetime.Enabled" << YAML::Value << particleProps.RotationOverLifetime.Enabled;

			out << YAML::Key << "RotationBySpeed.Start" << YAML::Value << particleProps.RotationBySpeed.Start;
			out << YAML::Key << "RotationBySpeed.End" << YAML::Value << particleProps.RotationBySpeed.End;
			out << YAML::Key << "RotationBySpeed.MinSpeed" << YAML::Value << particleProps.RotationBySpeed.MinSpeed;
			out << YAML::Key << "RotationBySpeed.MaxSpeed" << YAML::Value << particleProps.RotationBySpeed.MaxSpeed;
			out << YAML::Key << "RotationBySpeed.Enabled" << YAML::Value << particleProps.RotationBySpeed.Enabled;

			std::string texturePath = particleProps.Texture ? particleProps.Texture->GetPath() : "";
			if (Project::IsPartOfProject(texturePath))
				texturePath = Project::GetAssetRelativeFileSystemPath(texturePath).string();
			std::replace(texturePath.begin(), texturePath.end(), '\\', '/');
			out << YAML::Key << "TexturePath" << YAML::Value << texturePath;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap;

			const auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "Type" << YAML::Value << static_cast<int>(rb2d.Type);
			out << YAML::Key << "AutoMass" << YAML::Value << rb2d.AutoMass;
			out << YAML::Key << "Mass" << YAML::Value << rb2d.Mass;
			out << YAML::Key << "LinearDrag" << YAML::Value << rb2d.LinearDrag;
			out << YAML::Key << "AngularDrag" << YAML::Value << rb2d.AngularDrag;
			out << YAML::Key << "GravityScale" << YAML::Value << rb2d.GravityScale;
			out << YAML::Key << "AllowSleep" << YAML::Value << rb2d.AllowSleep;
			out << YAML::Key << "Awake" << YAML::Value << rb2d.Awake;
			out << YAML::Key << "Continuous" << YAML::Value << rb2d.Continuous;
			out << YAML::Key << "Interpolation" << YAML::Value << rb2d.Interpolation;
			out << YAML::Key << "FreezeRotation" << YAML::Value << rb2d.FreezeRotation;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap;

			const auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "IsSensor" << YAML::Value << bc2d.IsSensor;
			out << YAML::Key << "Size" << YAML::Value << bc2d.Size;
			out << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
			out << YAML::Key << "Density" << YAML::Value << bc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap;

			const auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "IsSensor" << YAML::Value << cc2d.IsSensor;
			out << YAML::Key << "Radius" << YAML::Value << cc2d.Radius;
			out << YAML::Key << "Offset" << YAML::Value << cc2d.Offset;
			out << YAML::Key << "Density" << YAML::Value << cc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << cc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << cc2d.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<PolygonCollider2DComponent>())
		{
			out << YAML::Key << "PolygonCollider2DComponent";
			out << YAML::BeginMap;

			const auto& pc2d = entity.GetComponent<PolygonCollider2DComponent>();
			out << YAML::Key << "IsSensor" << YAML::Value << pc2d.IsSensor;
			out << YAML::Key << "Offset" << YAML::Value << pc2d.Offset;
			out << YAML::Key << "Density" << YAML::Value << pc2d.Density;
			out << YAML::Key << "PointsSize" << YAML::Value << pc2d.Points.size();
			out << YAML::Key << "Points" << YAML::BeginMap;
			for (size_t i = 0; i < pc2d.Points.size(); ++i)
				out << YAML::Key << i << YAML::Key << pc2d.Points[i];
			out << YAML::EndMap;
			out << YAML::Key << "Friction" << YAML::Value << pc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << pc2d.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<DistanceJoint2DComponent>())
		{
			out << YAML::Key << "DistanceJoint2DComponent";
			out << YAML::BeginMap;

			const auto& dj2d = entity.GetComponent<DistanceJoint2DComponent>();
			out << YAML::Key << "EnableCollision" << YAML::Value << dj2d.EnableCollision;
			out << YAML::Key << "ConnectedRigidbody" << YAML::Value << dj2d.ConnectedRigidbody;
			out << YAML::Key << "Anchor" << YAML::Value << dj2d.Anchor;
			out << YAML::Key << "ConnectedAnchor" << YAML::Value << dj2d.ConnectedAnchor;
			out << YAML::Key << "AutoDistance" << YAML::Value << dj2d.AutoDistance;
			out << YAML::Key << "Distance" << YAML::Value << dj2d.Distance;
			out << YAML::Key << "MinDistance" << YAML::Value << dj2d.MinDistance;
			out << YAML::Key << "MaxDistanceBy" << YAML::Value << dj2d.MaxDistanceBy;
			out << YAML::Key << "BreakForce" << YAML::Value << dj2d.BreakForce;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SpringJoint2DComponent>())
		{
			out << YAML::Key << "SpringJoint2DComponent";
			out << YAML::BeginMap;

			const auto& sj2d = entity.GetComponent<SpringJoint2DComponent>();
			out << YAML::Key << "EnableCollision" << YAML::Value << sj2d.EnableCollision;
			out << YAML::Key << "ConnectedRigidbody" << YAML::Value << sj2d.ConnectedRigidbody;
			out << YAML::Key << "Anchor" << YAML::Value << sj2d.Anchor;
			out << YAML::Key << "ConnectedAnchor" << YAML::Value << sj2d.ConnectedAnchor;
			out << YAML::Key << "AutoDistance" << YAML::Value << sj2d.AutoDistance;
			out << YAML::Key << "Distance" << YAML::Value << sj2d.Distance;
			out << YAML::Key << "MinDistance" << YAML::Value << sj2d.MinDistance;
			out << YAML::Key << "MaxDistanceBy" << YAML::Value << sj2d.MaxDistanceBy;
			out << YAML::Key << "Frequency" << YAML::Value << sj2d.Frequency;
			out << YAML::Key << "DampingRatio" << YAML::Value << sj2d.DampingRatio;
			out << YAML::Key << "BreakForce" << YAML::Value << sj2d.BreakForce;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<HingeJoint2DComponent>())
		{
			out << YAML::Key << "HingeJoint2DComponent";
			out << YAML::BeginMap;

			const auto& hj2d = entity.GetComponent<HingeJoint2DComponent>();
			out << YAML::Key << "EnableCollision" << YAML::Value << hj2d.EnableCollision;
			out << YAML::Key << "ConnectedRigidbody" << YAML::Value << hj2d.ConnectedRigidbody;
			out << YAML::Key << "Anchor" << YAML::Value << hj2d.Anchor;
			out << YAML::Key << "UseLimits" << YAML::Value << hj2d.UseLimits;
			out << YAML::Key << "LowerAngle" << YAML::Value << hj2d.LowerAngle;
			out << YAML::Key << "UpperAngle" << YAML::Value << hj2d.UpperAngle;
			out << YAML::Key << "UseMotor" << YAML::Value << hj2d.UseMotor;
			out << YAML::Key << "MotorSpeed" << YAML::Value << hj2d.MotorSpeed;
			out << YAML::Key << "MaxMotorTorque" << YAML::Value << hj2d.MaxMotorTorque;
			out << YAML::Key << "BreakForce" << YAML::Value << hj2d.BreakForce;
			out << YAML::Key << "BreakTorque" << YAML::Value << hj2d.BreakTorque;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SliderJoint2DComponent>())
		{
			out << YAML::Key << "SliderJoint2DComponent";
			out << YAML::BeginMap;

			const auto& sj2d = entity.GetComponent<SliderJoint2DComponent>();
			out << YAML::Key << "EnableCollision" << YAML::Value << sj2d.EnableCollision;
			out << YAML::Key << "ConnectedRigidbody" << YAML::Value << sj2d.ConnectedRigidbody;
			out << YAML::Key << "Anchor" << YAML::Value << sj2d.Anchor;
			out << YAML::Key << "Angle" << YAML::Value << sj2d.Angle;
			out << YAML::Key << "UseLimits" << YAML::Value << sj2d.UseLimits;
			out << YAML::Key << "LowerTranslation" << YAML::Value << sj2d.LowerTranslation;
			out << YAML::Key << "UpperTranslation" << YAML::Value << sj2d.UpperTranslation;
			out << YAML::Key << "UseMotor" << YAML::Value << sj2d.UseMotor;
			out << YAML::Key << "MotorSpeed" << YAML::Value << sj2d.MotorSpeed;
			out << YAML::Key << "MaxMotorForce" << YAML::Value << sj2d.MaxMotorForce;
			out << YAML::Key << "BreakForce" << YAML::Value << sj2d.BreakForce;
			out << YAML::Key << "BreakTorque" << YAML::Value << sj2d.BreakTorque;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<WheelJoint2DComponent>())
		{
			out << YAML::Key << "WheelJoint2DComponent";
			out << YAML::BeginMap;

			const auto& wj2d = entity.GetComponent<WheelJoint2DComponent>();
			out << YAML::Key << "EnableCollision" << YAML::Value << wj2d.EnableCollision;
			out << YAML::Key << "ConnectedRigidbody" << YAML::Value << wj2d.ConnectedRigidbody;
			out << YAML::Key << "Anchor" << YAML::Value << wj2d.Anchor;
			out << YAML::Key << "Frequency" << YAML::Value << wj2d.Frequency;
			out << YAML::Key << "DampingRatio" << YAML::Value << wj2d.DampingRatio;
			out << YAML::Key << "UseLimits" << YAML::Value << wj2d.UseLimits;
			out << YAML::Key << "LowerTranslation" << YAML::Value << wj2d.LowerTranslation;
			out << YAML::Key << "UpperTranslation" << YAML::Value << wj2d.UpperTranslation;
			out << YAML::Key << "UseMotor" << YAML::Value << wj2d.UseMotor;
			out << YAML::Key << "MotorSpeed" << YAML::Value << wj2d.MotorSpeed;
			out << YAML::Key << "MaxMotorTorque" << YAML::Value << wj2d.MaxMotorTorque;
			out << YAML::Key << "BreakForce" << YAML::Value << wj2d.BreakForce;
			out << YAML::Key << "BreakTorque" << YAML::Value << wj2d.BreakTorque;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<BuoyancyEffector2DComponent>())
		{
			out << YAML::Key << "BuoyancyEffector2DComponent";
			out << YAML::BeginMap;

			const auto& be2d = entity.GetComponent<BuoyancyEffector2DComponent>();
			out << YAML::Key << "Density" << YAML::Value << be2d.Density;
			out << YAML::Key << "DragMultiplier" << YAML::Value << be2d.DragMultiplier;
			out << YAML::Key << "FlipGravity" << YAML::Value << be2d.FlipGravity;
			out << YAML::Key << "FlowMagnitude" << YAML::Value << be2d.FlowMagnitude;
			out << YAML::Key << "FlowAngle" << YAML::Value << be2d.FlowAngle;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<RigidbodyComponent>())
		{
			out << YAML::Key << "RigidbodyComponent";
			out << YAML::BeginMap;

			const auto& rb = entity.GetComponent<RigidbodyComponent>();
			out << YAML::Key << "Type" << YAML::Value << static_cast<int>(rb.Type);
			out << YAML::Key << "AutoMass" << YAML::Value << rb.AutoMass;
			out << YAML::Key << "Mass" << YAML::Value << rb.Mass;
			out << YAML::Key << "LinearDrag" << YAML::Value << rb.LinearDrag;
			out << YAML::Key << "AngularDrag" << YAML::Value << rb.AngularDrag;
			out << YAML::Key << "GravityScale" << YAML::Value << rb.GravityScale;
			out << YAML::Key << "AllowSleep" << YAML::Value << rb.AllowSleep;
			out << YAML::Key << "Awake" << YAML::Value << rb.Awake;
			out << YAML::Key << "Continuous" << YAML::Value << rb.Continuous;
			out << YAML::Key << "Interpolation" << YAML::Value << rb.Interpolation;
			out << YAML::Key << "IsSensor" << YAML::Value << rb.IsSensor;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::BeginMap;

			const auto& bc = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "Size" << YAML::Value << bc.Size;
			out << YAML::Key << "Offset" << YAML::Value << bc.Offset;
			out << YAML::Key << "Density" << YAML::Value << bc.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap;

			const auto& sc = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << sc.Radius;
			out << YAML::Key << "Offset" << YAML::Value << sc.Offset;
			out << YAML::Key << "Density" << YAML::Value << sc.Density;
			out << YAML::Key << "Friction" << YAML::Value << sc.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << sc.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			out << YAML::Key << "CapsuleColliderComponent";
			out << YAML::BeginMap;

			const auto& cc = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "Height" << YAML::Value << cc.Height;
			out << YAML::Key << "Radius" << YAML::Value << cc.Radius;
			out << YAML::Key << "Offset" << YAML::Value << cc.Offset;
			out << YAML::Key << "Density" << YAML::Value << cc.Density;
			out << YAML::Key << "Friction" << YAML::Value << cc.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << cc.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<TaperedCapsuleColliderComponent>())
		{
			out << YAML::Key << "TaperedCapsuleColliderComponent";
			out << YAML::BeginMap;

			const auto& tcc = entity.GetComponent<TaperedCapsuleColliderComponent>();
			out << YAML::Key << "Height" << YAML::Value << tcc.Height;
			out << YAML::Key << "TopRadius" << YAML::Value << tcc.TopRadius;
			out << YAML::Key << "BottomRadius" << YAML::Value << tcc.BottomRadius;
			out << YAML::Key << "Offset" << YAML::Value << tcc.Offset;
			out << YAML::Key << "Density" << YAML::Value << tcc.Density;
			out << YAML::Key << "Friction" << YAML::Value << tcc.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << tcc.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<CylinderColliderComponent>())
		{
			out << YAML::Key << "CylinderColliderComponent";
			out << YAML::BeginMap;

			const auto& cc = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "Height" << YAML::Value << cc.Height;
			out << YAML::Key << "Radius" << YAML::Value << cc.Radius;
			out << YAML::Key << "Offset" << YAML::Value << cc.Offset;
			out << YAML::Key << "Density" << YAML::Value << cc.Density;
			out << YAML::Key << "Friction" << YAML::Value << cc.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << cc.Restitution;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap;
			
			const auto& meshComponent = entity.GetComponent<MeshComponent>();
			std::string filepath = meshComponent.MeshGeometry ? meshComponent.MeshGeometry->GetFilepath() : "";
			if (Project::IsPartOfProject(filepath))
				filepath = Project::GetAssetRelativeFileSystemPath(filepath).string();
			std::replace(filepath.begin(), filepath.end(), '\\', '/');
			out << YAML::Key << "Filepath" << YAML::Value << filepath;
			out << YAML::Key << "SubmeshIndex" << YAML::Value << meshComponent.SubmeshIndex;
			out << YAML::Key << "CullMode" << YAML::Value << static_cast<int>(meshComponent.CullMode);
			
			out << YAML::EndMap;
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap;

			const auto& sc = entity.GetComponent<ScriptComponent>();

			out << YAML::Key << "ScriptCount" << YAML::Value << sc.Classes.size();

			out << YAML::Key << "Scripts" << YAML::BeginMap;
			int i = 0;
			for (const auto& className : sc.Classes)
			{
				out << YAML::Key << i << YAML::BeginMap;
				++i;

				out << YAML::Key << "Name" << YAML::Value << className;
				out << YAML::Key << "Fields" << YAML::BeginMap;

				const auto& fields = ScriptEngine::GetFieldMap(className.c_str());
				const auto& fieldInstances = ScriptEngine::GetFieldInstanceMap(entity, className.c_str());
				for (const auto& [fieldName, fieldInstance] : fieldInstances)
				{
					const auto& field = fields.at(fieldName);
					if (field.Type == FieldType::Unknown)
						continue;

					if (!field.Serializable)
						continue;

					out << YAML::Key << fieldName << YAML::Value;

					switch (field.Type)
					{
						READ_FIELD_TYPE(FieldType::Bool, bool);
						READ_FIELD_TYPE(FieldType::Float, float);
						READ_FIELD_TYPE(FieldType::Double, double);
						READ_FIELD_TYPE(FieldType::Byte, int8_t);
						READ_FIELD_TYPE(FieldType::UByte, uint8_t);
						READ_FIELD_TYPE(FieldType::Short, int16_t);
						READ_FIELD_TYPE(FieldType::UShort, uint16_t);
						READ_FIELD_TYPE(FieldType::Int, int32_t);
						READ_FIELD_TYPE(FieldType::UInt, uint32_t);
						READ_FIELD_TYPE(FieldType::Long, int64_t);
						READ_FIELD_TYPE(FieldType::ULong, uint64_t);
						READ_FIELD_TYPE(FieldType::Vector2, glm::vec2);
						READ_FIELD_TYPE(FieldType::Vector3, glm::vec3);
						READ_FIELD_TYPE(FieldType::Vector4, glm::vec4);
						READ_FIELD_TYPE(FieldType::Color, glm::vec4);
						default:
							ARC_CORE_ASSERT(false)
							break;
					}
				}
				out << YAML::EndMap;

				out << YAML::EndMap;
			}
			out << YAML::EndMap;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<AudioSourceComponent>())
		{
			out << YAML::Key << "AudioSourceComponent";
			out << YAML::BeginMap;

			const auto& audioSourceComponent = entity.GetComponent<AudioSourceComponent>();
			std::string filepath = audioSourceComponent.Source ? audioSourceComponent.Source->GetPath() : "";
			if (Project::IsPartOfProject(filepath))
				filepath = Project::GetAssetRelativeFileSystemPath(filepath).string();
			std::replace(filepath.begin(), filepath.end(), '\\', '/');
			out << YAML::Key << "Filepath" << YAML::Value << filepath;
			out << YAML::Key << "VolumeMultiplier" << YAML::Value << audioSourceComponent.Config.VolumeMultiplier;
			out << YAML::Key << "PitchMultiplier" << YAML::Value << audioSourceComponent.Config.PitchMultiplier;
			out << YAML::Key << "PlayOnAwake" << YAML::Value << audioSourceComponent.Config.PlayOnAwake;
			out << YAML::Key << "Looping" << YAML::Value << audioSourceComponent.Config.Looping;
			out << YAML::Key << "Spatialization" << YAML::Value << audioSourceComponent.Config.Spatialization;
			out << YAML::Key << "AttenuationModel" << YAML::Value << static_cast<int>(audioSourceComponent.Config.AttenuationModel);
			out << YAML::Key << "RollOff" << YAML::Value << audioSourceComponent.Config.RollOff;
			out << YAML::Key << "MinGain" << YAML::Value << audioSourceComponent.Config.MinGain;
			out << YAML::Key << "MaxGain" << YAML::Value << audioSourceComponent.Config.MaxGain;
			out << YAML::Key << "MinDistance" << YAML::Value << audioSourceComponent.Config.MinDistance;
			out << YAML::Key << "MaxDistance" << YAML::Value << audioSourceComponent.Config.MaxDistance;
			out << YAML::Key << "ConeInnerAngle" << YAML::Value << audioSourceComponent.Config.ConeInnerAngle;
			out << YAML::Key << "ConeOuterAngle" << YAML::Value << audioSourceComponent.Config.ConeOuterAngle;
			out << YAML::Key << "ConeOuterGain" << YAML::Value << audioSourceComponent.Config.ConeOuterGain;
			out << YAML::Key << "DopplerFactor" << YAML::Value << audioSourceComponent.Config.DopplerFactor;

			out << YAML::EndMap;
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			out << YAML::Key << "AudioListenerComponent";
			out << YAML::BeginMap;

			const auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
			out << YAML::Key << "Active" << YAML::Value << audioListenerComponent.Active;
			out << YAML::Key << "ConeInnerAngle" << YAML::Value << audioListenerComponent.Config.ConeInnerAngle;
			out << YAML::Key << "ConeOuterAngle" << YAML::Value << audioListenerComponent.Config.ConeOuterAngle;
			out << YAML::Key << "ConeOuterGain" << YAML::Value << audioListenerComponent.Config.ConeOuterGain;

			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}

	UUID EntitySerializer::DeserializeEntity(const YAML::Node& node, Scene& scene, bool preserveUUID)
	{
		YAML::Node entity = node;

		auto uuid = entity["Entity"].as<uint64_t>();

		std::string name;
		bool enabled = true;
		EntityLayer layer = 0;
		auto tagComponent = entity["TagComponent"];
		if (tagComponent)
		{
			name = tagComponent["Tag"].as<std::string>();
			TrySet(layer, tagComponent["Layer"]);
			TrySet(enabled, tagComponent["Enabled"]);
		}

		Entity deserializedEntity;
		if (preserveUUID)
			deserializedEntity = scene.CreateEntityWithUUID(uuid, name);
		else
			deserializedEntity = scene.CreateEntity(name);

		if (preserveUUID)
			ARC_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);
		else
			ARC_CORE_TRACE("Deserialized entity with oldID = {0}, newID = {1}, name = {2}", uuid, deserializedEntity.GetUUID(), name);

		if (tagComponent)
		{
			auto& tc = deserializedEntity.GetComponent<TagComponent>();
			tc.Layer = layer;
			tc.Enabled = enabled;
		}

		if (const auto& transformComponent = entity["TransformComponent"])
		{
			// Entities always have transforms
			auto& tc = deserializedEntity.GetComponent<TransformComponent>();
			TrySet(tc.Translation, transformComponent["Translation"]);
			TrySet(tc.Rotation, transformComponent["Rotation"]);
			TrySet(tc.Scale, transformComponent["Scale"]);
		}

		if (const auto& relationshipComponent = entity["RelationshipComponent"])
		{
			auto& tc = deserializedEntity.GetComponent<RelationshipComponent>();
			TrySet(tc.Parent, relationshipComponent["Parent"]);

			size_t childCount = 0;
			TrySet(childCount, relationshipComponent["ChildrenCount"]);
			tc.Children.clear();
			tc.Children.reserve(childCount);
			auto children = relationshipComponent["Children"];
			if (children && childCount > 0)
			{
				for (size_t i = 0; i < childCount; i++)
				{
					UUID child = 0;
					TrySet(child, children[i]);
					if (child)
						tc.Children.push_back(child);
				}
			}
		}

		if (const auto& cameraComponent = entity["CameraComponent"])
		{
			auto& cc = deserializedEntity.AddComponent<CameraComponent>();

			auto& cameraProps = cameraComponent["Camera"];
			cc.Camera.SetViewportSize(1, 1);
			SceneCamera::ProjectionType projectionType = cc.Camera.GetProjectionType();
			cc.Camera.SetProjectionType(TrySetEnum(projectionType, cameraProps["ProjectionType"]));

			float tmp = cc.Camera.GetPerspectiveVerticalFOV();
			cc.Camera.SetPerspectiveVerticalFOV(TrySet(tmp, cameraProps["PerspectiveFOV"]));

			tmp = cc.Camera.GetPerspectiveNearClip();
			cc.Camera.SetPerspectiveNearClip(TrySet(tmp, cameraProps["PerspectiveNear"]));

			tmp = cc.Camera.GetPerspectiveFarClip();
			cc.Camera.SetPerspectiveFarClip(TrySet(tmp, cameraProps["PerspectiveFar"]));

			tmp = cc.Camera.GetOrthographicSize();
			cc.Camera.SetOrthographicSize(TrySet(tmp, cameraProps["OrthographicSize"]));

			tmp = cc.Camera.GetOrthographicNearClip();
			cc.Camera.SetOrthographicNearClip(TrySet(tmp, cameraProps["OrthographicNear"]));

			tmp = cc.Camera.GetOrthographicFarClip();
			cc.Camera.SetOrthographicFarClip(TrySet(tmp, cameraProps["OrthographicFar"]));

			TrySet(cc.Primary, cameraComponent["Primary"]);
			TrySet(cc.FixedAspectRatio, cameraComponent["FixedAspectRatio"]);
		}

		if (const auto& spriteRenderer = entity["SpriteRendererComponent"])
		{
			auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();
			TrySet(src.Color, spriteRenderer["Color"]);
			TrySet(src.SortingOrder, spriteRenderer["SortingOrder"]);
			TrySet(src.TilingFactor, spriteRenderer["TilingFactor"]);

			std::string texturePath;
			TrySet(texturePath, spriteRenderer["TexturePath"]);

			if (!texturePath.empty())
			{
				std::filesystem::path path = Project::GetAssetFileSystemPath(texturePath);
				if (!std::filesystem::exists(path))
					path = texturePath;
				src.Texture = AssetManager::GetTexture2D(path.string());
			}
		}

		if (const auto& skyLight = entity["SkyLightComponent"])
		{
			auto& src = deserializedEntity.AddComponent<SkyLightComponent>();
			TrySet(src.Intensity, skyLight["Intensity"]);
			TrySet(src.Rotation, skyLight["Rotation"]);

			std::string texturePath;
			TrySet(texturePath, skyLight["TexturePath"]);
			if (!texturePath.empty())
			{
				std::filesystem::path path = Project::GetAssetFileSystemPath(texturePath);
				if (!std::filesystem::exists(path))
					path = texturePath;
				src.Texture = AssetManager::GetTextureCubemap(path.string());
			}
		}

		if (const auto& lightComponent = entity["LightComponent"])
		{
			auto& src = deserializedEntity.AddComponent<LightComponent>();
			TrySetEnum(src.Type, lightComponent["Type"]);
			TrySet(src.UseColorTemperatureMode, lightComponent["UseColorTemperatureMode"]);
			TrySet(src.Temperature, lightComponent["Temperature"]);
			TrySet(src.Color, lightComponent["Color"]);
			TrySet(src.Intensity, lightComponent["Intensity"]);
			TrySet(src.Range, lightComponent["Range"]);
			TrySet(src.CutOffAngle, lightComponent["CutOffAngle"]);
			TrySet(src.OuterCutOffAngle, lightComponent["OuterCutOffAngle"]);
			TrySetEnum(src.ShadowQuality, lightComponent["ShadowQuality"]);

			if (src.UseColorTemperatureMode)
				ColorUtils::TempratureToColor(src.Temperature, src.Color);
		}

		if (const auto& psComponent = entity["ParticleSystemComponent"])
		{
			auto& props = deserializedEntity.AddComponent<ParticleSystemComponent>().System->GetProperties();
			TrySet(props.Duration, psComponent["Duration"]);
			TrySet(props.Looping, psComponent["Looping"]);
			TrySet(props.StartDelay, psComponent["StartDelay"]);
			TrySet(props.StartLifetime, psComponent["StartLifetime"]);
			TrySet(props.StartVelocity, psComponent["StartVelocity"]);
			TrySet(props.StartColor, psComponent["StartColor"]);
			TrySet(props.StartSize, psComponent["StartSize"]);
			TrySet(props.StartRotation, psComponent["StartRotation"]);
			TrySet(props.GravityModifier, psComponent["GravityModifier"]);
			TrySet(props.SimulationSpeed, psComponent["SimulationSpeed"]);
			TrySet(props.PlayOnAwake, psComponent["PlayOnAwake"]);
			TrySet(props.MaxParticles, psComponent["MaxParticles"]);
			TrySet(props.RateOverTime, psComponent["RateOverTime"]);
			TrySet(props.RateOverDistance, psComponent["RateOverDistance"]);
			TrySet(props.BurstCount, psComponent["BurstCount"]);
			TrySet(props.BurstTime, psComponent["BurstTime"]);
			TrySet(props.PositionStart, psComponent["PositionStart"]);
			TrySet(props.PositionEnd, psComponent["PositionEnd"]);

			TrySet(props.VelocityOverLifetime.Start, psComponent["VelocityOverLifetime.Start"]);
			TrySet(props.VelocityOverLifetime.End, psComponent["VelocityOverLifetime.End"]);
			TrySet(props.VelocityOverLifetime.Enabled, psComponent["VelocityOverLifetime.Enabled"]);

			TrySet(props.ForceOverLifetime.Start, psComponent["ForceOverLifetime.Start"]);
			TrySet(props.ForceOverLifetime.End, psComponent["ForceOverLifetime.End"]);
			TrySet(props.ForceOverLifetime.Enabled, psComponent["ForceOverLifetime.Enabled"]);

			TrySet(props.ColorOverLifetime.Start, psComponent["ColorOverLifetime.Start"]);
			TrySet(props.ColorOverLifetime.End, psComponent["ColorOverLifetime.End"]);
			TrySet(props.ColorOverLifetime.Enabled, psComponent["ColorOverLifetime.Enabled"]);

			TrySet(props.ColorBySpeed.Start, psComponent["ColorBySpeed.Start"]);
			TrySet(props.ColorBySpeed.End, psComponent["ColorBySpeed.End"]);
			TrySet(props.ColorBySpeed.MinSpeed, psComponent["ColorBySpeed.MinSpeed"]);
			TrySet(props.ColorBySpeed.MaxSpeed, psComponent["ColorBySpeed.MaxSpeed"]);
			TrySet(props.ColorBySpeed.Enabled, psComponent["ColorBySpeed.Enabled"]);

			TrySet(props.SizeOverLifetime.Start, psComponent["SizeOverLifetime.Start"]);
			TrySet(props.SizeOverLifetime.End, psComponent["SizeOverLifetime.End"]);
			TrySet(props.SizeOverLifetime.Enabled, psComponent["SizeOverLifetime.Enabled"]);

			TrySet(props.SizeBySpeed.Start, psComponent["SizeBySpeed.Start"]);
			TrySet(props.SizeBySpeed.End, psComponent["SizeBySpeed.End"]);
			TrySet(props.SizeBySpeed.MinSpeed, psComponent["SizeBySpeed.MinSpeed"]);
			TrySet(props.SizeBySpeed.MaxSpeed, psComponent["SizeBySpeed.MaxSpeed"]);
			TrySet(props.SizeBySpeed.Enabled, psComponent["SizeBySpeed.Enabled"]);

			TrySet(props.RotationOverLifetime.Start, psComponent["RotationOverLifetime.Start"]);
			TrySet(props.RotationOverLifetime.End, psComponent["RotationOverLifetime.End"]);
			TrySet(props.RotationOverLifetime.Enabled, psComponent["RotationOverLifetime.Enabled"]);

			TrySet(props.RotationBySpeed.Start, psComponent["RotationBySpeed.Start"]);
			TrySet(props.RotationBySpeed.End, psComponent["RotationBySpeed.End"]);
			TrySet(props.RotationBySpeed.MinSpeed, psComponent["RotationBySpeed.MinSpeed"]);
			TrySet(props.RotationBySpeed.MaxSpeed, psComponent["RotationBySpeed.MaxSpeed"]);
			TrySet(props.RotationBySpeed.Enabled, psComponent["RotationBySpeed.Enabled"]);

			std::string texturePath;
			TrySet(texturePath, psComponent["TexturePath"]);
			if (!texturePath.empty())
			{
				std::filesystem::path path = Project::GetAssetFileSystemPath(texturePath);
				if (!std::filesystem::exists(path))
					path = texturePath;
				props.Texture = AssetManager::GetTexture2D(path.string());
			}
		}

		if (const auto& rb2dCpmponent = entity["Rigidbody2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<Rigidbody2DComponent>();
			TrySetEnum(src.Type, rb2dCpmponent["Type"]);
			TrySet(src.AutoMass, rb2dCpmponent["AutoMass"]);
			TrySet(src.Mass, rb2dCpmponent["Mass"]);
			TrySet(src.LinearDrag, rb2dCpmponent["LinearDrag"]);
			TrySet(src.AngularDrag, rb2dCpmponent["AngularDrag"]);
			TrySet(src.GravityScale, rb2dCpmponent["GravityScale"]);
			TrySet(src.AllowSleep, rb2dCpmponent["AllowSleep"]);
			TrySet(src.Awake, rb2dCpmponent["Awake"]);
			TrySet(src.Continuous, rb2dCpmponent["Continuous"]);
			TrySet(src.Interpolation, rb2dCpmponent["Interpolation"]);
			TrySet(src.FreezeRotation, rb2dCpmponent["FreezeRotation"]);
		}

		if (const auto& bc2dCpmponent = entity["BoxCollider2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<BoxCollider2DComponent>();
			TrySet(src.IsSensor, bc2dCpmponent["IsSensor"]);
			TrySet(src.Size, bc2dCpmponent["Size"]);
			TrySet(src.Offset, bc2dCpmponent["Offset"]);
			TrySet(src.Density, bc2dCpmponent["Density"]);
			TrySet(src.Friction, bc2dCpmponent["Friction"]);
			TrySet(src.Restitution, bc2dCpmponent["Restitution"]);
		}

		if (const auto& cc2dCpmponent = entity["CircleCollider2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<CircleCollider2DComponent>();
			TrySet(src.IsSensor, cc2dCpmponent["IsSensor"]);
			TrySet(src.Radius, cc2dCpmponent["Radius"]);
			TrySet(src.Offset, cc2dCpmponent["Offset"]);
			TrySet(src.Density, cc2dCpmponent["Density"]);
			TrySet(src.Friction, cc2dCpmponent["Friction"]);
			TrySet(src.Restitution, cc2dCpmponent["Restitution"]);
		}

		if (const auto& pc2dCpmponent = entity["PolygonCollider2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<PolygonCollider2DComponent>();
			TrySet(src.IsSensor, pc2dCpmponent["IsSensor"]);
			TrySet(src.Offset, pc2dCpmponent["Offset"]);
			TrySet(src.Density, pc2dCpmponent["Density"]);
			size_t pointsCount = 0;
			TrySet(pointsCount, pc2dCpmponent["PointsSize"]);
			if (pointsCount >= 3)
				src.Points.clear();
			auto& points = pc2dCpmponent["Points"];
			for (size_t i = 0; i < pointsCount; ++i)
			{
				auto point = glm::vec2(0.0f);
				TrySet(point, points[i]);
				src.Points.push_back(point);
			}
			TrySet(src.Friction, pc2dCpmponent["Friction"]);
			TrySet(src.Restitution, pc2dCpmponent["Restitution"]);
		}

		if (const auto& distJoint2dCpmponent = entity["DistanceJoint2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<DistanceJoint2DComponent>();
			TrySet(src.EnableCollision, distJoint2dCpmponent["EnableCollision"]);
			TrySet(src.ConnectedRigidbody, distJoint2dCpmponent["ConnectedRigidbody"]);
			TrySet(src.Anchor, distJoint2dCpmponent["Anchor"]);
			TrySet(src.ConnectedAnchor, distJoint2dCpmponent["ConnectedAnchor"]);
			TrySet(src.AutoDistance, distJoint2dCpmponent["AutoDistance"]);
			TrySet(src.Distance, distJoint2dCpmponent["Distance"]);
			TrySet(src.MinDistance, distJoint2dCpmponent["MinDistance"]);
			TrySet(src.MaxDistanceBy, distJoint2dCpmponent["MaxDistanceBy"]);
			TrySet(src.BreakForce, distJoint2dCpmponent["BreakForce"]);
		}

		if (const auto& springJoint2dCpmponent = entity["SpringJoint2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<SpringJoint2DComponent>();
			TrySet(src.EnableCollision, springJoint2dCpmponent["EnableCollision"]);
			TrySet(src.ConnectedRigidbody, springJoint2dCpmponent["ConnectedRigidbody"]);
			TrySet(src.Anchor, springJoint2dCpmponent["Anchor"]);
			TrySet(src.ConnectedAnchor, springJoint2dCpmponent["ConnectedAnchor"]);
			TrySet(src.AutoDistance, springJoint2dCpmponent["AutoDistance"]);
			TrySet(src.Distance, springJoint2dCpmponent["Distance"]);
			TrySet(src.MinDistance, springJoint2dCpmponent["MinDistance"]);
			TrySet(src.MaxDistanceBy, springJoint2dCpmponent["MaxDistanceBy"]);
			TrySet(src.Frequency, springJoint2dCpmponent["Frequency"]);
			TrySet(src.DampingRatio, springJoint2dCpmponent["DampingRatio"]);
			TrySet(src.BreakForce, springJoint2dCpmponent["BreakForce"]);
		}

		if (const auto& hingeJoint2dCpmponent = entity["HingeJoint2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<HingeJoint2DComponent>();
			TrySet(src.EnableCollision, hingeJoint2dCpmponent["EnableCollision"]);
			TrySet(src.ConnectedRigidbody, hingeJoint2dCpmponent["ConnectedRigidbody"]);
			TrySet(src.Anchor, hingeJoint2dCpmponent["Anchor"]);
			TrySet(src.UseLimits, hingeJoint2dCpmponent["UseLimits"]);
			TrySet(src.LowerAngle, hingeJoint2dCpmponent["LowerAngle"]);
			TrySet(src.UpperAngle, hingeJoint2dCpmponent["UpperAngle"]);
			TrySet(src.UseMotor, hingeJoint2dCpmponent["UseMotor"]);
			TrySet(src.MotorSpeed, hingeJoint2dCpmponent["MotorSpeed"]);
			TrySet(src.MaxMotorTorque, hingeJoint2dCpmponent["MaxMotorTorque"]);
			TrySet(src.BreakForce, hingeJoint2dCpmponent["BreakForce"]);
			TrySet(src.BreakTorque, hingeJoint2dCpmponent["BreakTorque"]);
		}

		if (const auto& sliderJoint2dCpmponent = entity["SliderJoint2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<SliderJoint2DComponent>();
			TrySet(src.EnableCollision, sliderJoint2dCpmponent["EnableCollision"]);
			TrySet(src.ConnectedRigidbody, sliderJoint2dCpmponent["ConnectedRigidbody"]);
			TrySet(src.Anchor, sliderJoint2dCpmponent["Anchor"]);
			TrySet(src.Angle, sliderJoint2dCpmponent["Angle"]);
			TrySet(src.UseLimits, sliderJoint2dCpmponent["UseLimits"]);
			TrySet(src.LowerTranslation, sliderJoint2dCpmponent["LowerTranslation"]);
			TrySet(src.UpperTranslation, sliderJoint2dCpmponent["UpperTranslation"]);
			TrySet(src.UseMotor, sliderJoint2dCpmponent["UseMotor"]);
			TrySet(src.MotorSpeed, sliderJoint2dCpmponent["MotorSpeed"]);
			TrySet(src.MaxMotorForce, sliderJoint2dCpmponent["MaxMotorForce"]);
			TrySet(src.BreakForce, sliderJoint2dCpmponent["BreakForce"]);
			TrySet(src.BreakTorque, sliderJoint2dCpmponent["BreakTorque"]);
		}

		if (const auto& wheelJoint2dCpmponent = entity["WheelJoint2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<WheelJoint2DComponent>();
			TrySet(src.EnableCollision, wheelJoint2dCpmponent["EnableCollision"]);
			TrySet(src.ConnectedRigidbody, wheelJoint2dCpmponent["ConnectedRigidbody"]);
			TrySet(src.Anchor, wheelJoint2dCpmponent["Anchor"]);
			TrySet(src.Frequency, wheelJoint2dCpmponent["Frequency"]);
			TrySet(src.DampingRatio, wheelJoint2dCpmponent["DampingRatio"]);
			TrySet(src.UseLimits, wheelJoint2dCpmponent["UseLimits"]);
			TrySet(src.LowerTranslation, wheelJoint2dCpmponent["LowerTranslation"]);
			TrySet(src.UpperTranslation, wheelJoint2dCpmponent["UpperTranslation"]);
			TrySet(src.UseMotor, wheelJoint2dCpmponent["UseMotor"]);
			TrySet(src.MotorSpeed, wheelJoint2dCpmponent["MotorSpeed"]);
			TrySet(src.MaxMotorTorque, wheelJoint2dCpmponent["MaxMotorTorque"]);
			TrySet(src.BreakForce, wheelJoint2dCpmponent["BreakForce"]);
			TrySet(src.BreakTorque, wheelJoint2dCpmponent["BreakTorque"]);
		}

		if (const auto& buoyancyEffector2DComponent = entity["BuoyancyEffector2DComponent"])
		{
			auto& src = deserializedEntity.AddComponent<BuoyancyEffector2DComponent>();
			TrySet(src.Density, buoyancyEffector2DComponent["Density"]);
			TrySet(src.DragMultiplier, buoyancyEffector2DComponent["DragMultiplier"]);
			TrySet(src.FlipGravity, buoyancyEffector2DComponent["FlipGravity"]);
			TrySet(src.FlowMagnitude, buoyancyEffector2DComponent["FlowMagnitude"]);
			TrySet(src.FlowAngle, buoyancyEffector2DComponent["FlowAngle"]);
		}

		if (const auto& rbComponent = entity["RigidbodyComponent"])
		{
			auto& src = deserializedEntity.AddComponent<RigidbodyComponent>();
			TrySetEnum(src.Type, rbComponent["Type"]);
			TrySet(src.AutoMass, rbComponent["AutoMass"]);
			TrySet(src.Mass, rbComponent["Mass"]);
			TrySet(src.LinearDrag, rbComponent["LinearDrag"]);
			TrySet(src.AngularDrag, rbComponent["AngularDrag"]);
			TrySet(src.GravityScale, rbComponent["GravityScale"]);
			TrySet(src.AllowSleep, rbComponent["AllowSleep"]);
			TrySet(src.Awake, rbComponent["Awake"]);
			TrySet(src.Continuous, rbComponent["Continuous"]);
			TrySet(src.Interpolation, rbComponent["Interpolation"]);
			TrySet(src.IsSensor, rbComponent["IsSensor"]);
		}

		if (const auto& bcComponent = entity["BoxColliderComponent"])
		{
			auto& src = deserializedEntity.AddComponent<BoxColliderComponent>();
			TrySet(src.Size, bcComponent["Size"]);
			TrySet(src.Offset, bcComponent["Offset"]);
			TrySet(src.Density, bcComponent["Density"]);
			TrySet(src.Friction, bcComponent["Friction"]);
			TrySet(src.Restitution, bcComponent["Restitution"]);
		}

		if (const auto& scComponent = entity["SphereColliderComponent"])
		{
			auto& src = deserializedEntity.AddComponent<SphereColliderComponent>();
			TrySet(src.Radius, scComponent["Radius"]);
			TrySet(src.Offset, scComponent["Offset"]);
			TrySet(src.Density, scComponent["Density"]);
			TrySet(src.Friction, scComponent["Friction"]);
			TrySet(src.Restitution, scComponent["Restitution"]);
		}

		if (const auto& ccComponent = entity["CapsuleColliderComponent"])
		{
			auto& src = deserializedEntity.AddComponent<CapsuleColliderComponent>();
			TrySet(src.Height, ccComponent["Height"]);
			TrySet(src.Radius, ccComponent["Radius"]);
			TrySet(src.Offset, ccComponent["Offset"]);
			TrySet(src.Density, ccComponent["Density"]);
			TrySet(src.Friction, ccComponent["Friction"]);
			TrySet(src.Restitution, ccComponent["Restitution"]);
		}

		if (const auto& tccComponent = entity["TaperedCapsuleColliderComponent"])
		{
			auto& src = deserializedEntity.AddComponent<TaperedCapsuleColliderComponent>();
			TrySet(src.Height, tccComponent["Height"]);
			TrySet(src.TopRadius, tccComponent["TopRadius"]);
			TrySet(src.BottomRadius, tccComponent["BottomRadius"]);
			TrySet(src.Offset, tccComponent["Offset"]);
			TrySet(src.Density, tccComponent["Density"]);
			TrySet(src.Friction, tccComponent["Friction"]);
			TrySet(src.Restitution, tccComponent["Restitution"]);
		}

		if (const auto& ccComponent = entity["CapsuleColliderComponent"])
		{
			auto& src = deserializedEntity.AddComponent<CapsuleColliderComponent>();
			TrySet(src.Height, ccComponent["Height"]);
			TrySet(src.Radius, ccComponent["Radius"]);
			TrySet(src.Offset, ccComponent["Offset"]);
			TrySet(src.Density, ccComponent["Density"]);
			TrySet(src.Friction, ccComponent["Friction"]);
			TrySet(src.Restitution, ccComponent["Restitution"]);
		}

		if (const auto& meshComponent = entity["MeshComponent"])
		{
			auto& src = deserializedEntity.AddComponent<MeshComponent>();
			std::string filepath;
			TrySet(filepath, meshComponent["Filepath"]);
			TrySet(src.SubmeshIndex, meshComponent["SubmeshIndex"]);
			TrySetEnum(src.CullMode, meshComponent["CullMode"]);

			if (!filepath.empty())
			{
				std::filesystem::path path = Project::GetAssetFileSystemPath(filepath);
				if (!std::filesystem::exists(path))
					path = filepath;
				src.MeshGeometry = AssetManager::GetMesh(path.string());
			}
		}

		if (const auto& scriptComponent = entity["ScriptComponent"])
		{
			auto& sc = deserializedEntity.AddComponent<ScriptComponent>();

			size_t scriptCount = 0;
			TrySet(scriptCount, scriptComponent["ScriptCount"]);

			sc.Classes.clear();
			auto scripts = scriptComponent["Scripts"];
			if (scripts && scriptCount > 0)
			{
				for (size_t i = 0; i < scriptCount; i++)
				{
					auto scriptNode = scripts[i];
					
					std::string scriptName;
					TrySet(scriptName, scriptNode["Name"]);
					if (!ScriptEngine::HasClass(scriptName))
					{
						ARC_CORE_ERROR("Class not found with name: {}", scriptName);
						continue;
					}

					sc.Classes.emplace_back(scriptName);

					const auto& fields = ScriptEngine::GetFieldMap(scriptName.c_str());
					auto& fieldInstances = ScriptEngine::GetFieldInstanceMap(deserializedEntity, scriptName.c_str());
					{
						for (const auto& [fieldName, field] : fields)
						{
							if (field.Type == FieldType::Unknown)
								continue;

							if (!field.Serializable)
								continue;

							auto fieldNode = scriptNode["Fields"][fieldName];
							if (fieldNode)
							{
								auto& fieldInstance = fieldInstances[fieldName];
								fieldInstance.Type = field.Type;

								switch (field.Type)
								{
									WRITE_FIELD_TYPE(FieldType::Bool, bool);
									WRITE_FIELD_TYPE(FieldType::Float, float);
									WRITE_FIELD_TYPE(FieldType::Double, double);
									WRITE_FIELD_TYPE(FieldType::Byte, int8_t);
									WRITE_FIELD_TYPE(FieldType::UByte, uint8_t);
									WRITE_FIELD_TYPE(FieldType::Short, int16_t);
									WRITE_FIELD_TYPE(FieldType::UShort, uint16_t);
									WRITE_FIELD_TYPE(FieldType::Int, int32_t);
									WRITE_FIELD_TYPE(FieldType::UInt, uint32_t);
									WRITE_FIELD_TYPE(FieldType::Long, int64_t);
									WRITE_FIELD_TYPE(FieldType::ULong, uint64_t);
									WRITE_FIELD_TYPE(FieldType::Vector2, glm::vec2);
									WRITE_FIELD_TYPE(FieldType::Vector3, glm::vec3);
									WRITE_FIELD_TYPE(FieldType::Vector4, glm::vec4);
									WRITE_FIELD_TYPE(FieldType::Color, glm::vec4);
									default:
										ARC_CORE_ASSERT(false)
										break;
								}
							}
						}
					}
				}
			}
		}

		if (const auto& audioSourceComponent = entity["AudioSourceComponent"])
		{
			auto& src = deserializedEntity.AddComponent<AudioSourceComponent>();
			std::string filepath;
			TrySet(filepath, audioSourceComponent["Filepath"]);
			TrySet(src.Config.VolumeMultiplier, audioSourceComponent["VolumeMultiplier"]);
			TrySet(src.Config.PitchMultiplier, audioSourceComponent["PitchMultiplier"]);
			TrySet(src.Config.PlayOnAwake, audioSourceComponent["PlayOnAwake"]);
			TrySet(src.Config.Looping, audioSourceComponent["Looping"]);
			TrySet(src.Config.Spatialization, audioSourceComponent["Spatialization"]);
			TrySetEnum(src.Config.AttenuationModel, audioSourceComponent["AttenuationModel"]);
			TrySet(src.Config.RollOff, audioSourceComponent["RollOff"]);
			TrySet(src.Config.MinGain, audioSourceComponent["MinGain"]);
			TrySet(src.Config.MaxGain, audioSourceComponent["MaxGain"]);
			TrySet(src.Config.MinDistance, audioSourceComponent["MinDistance"]);
			TrySet(src.Config.MaxDistance, audioSourceComponent["MaxDistance"]);
			TrySet(src.Config.ConeInnerAngle, audioSourceComponent["ConeInnerAngle"]);
			TrySet(src.Config.ConeOuterAngle, audioSourceComponent["ConeOuterAngle"]);
			TrySet(src.Config.ConeOuterGain, audioSourceComponent["ConeOuterGain"]);
			TrySet(src.Config.DopplerFactor, audioSourceComponent["DopplerFactor"]);

			if (!filepath.empty())
			{
				std::filesystem::path path = Project::GetAssetFileSystemPath(filepath);
				if (!std::filesystem::exists(path))
					path = filepath;
				src.Source = CreateRef<AudioSource>(path.string().c_str());
			}
		}

		if (const auto& audioListenerComponent = entity["AudioListenerComponent"])
		{
			auto& src = deserializedEntity.AddComponent<AudioListenerComponent>();
			TrySet(src.Active, audioListenerComponent["Active"]);
			TrySet(src.Config.ConeInnerAngle, audioListenerComponent["ConeInnerAngle"]);
			TrySet(src.Config.ConeOuterAngle, audioListenerComponent["ConeOuterAngle"]);
			TrySet(src.Config.ConeOuterGain, audioListenerComponent["ConeOuterGain"]);
		}

		return deserializedEntity.GetUUID();
	}

	static void GetAllChildren(Entity parent, std::vector<Entity>& outEntities)
	{
		std::vector<UUID> children = parent.GetComponent<RelationshipComponent>().Children;
		for (const auto& child : children)
		{
			Entity entity = parent.GetScene()->GetEntity(child);
			outEntities.push_back(entity);
			GetAllChildren(entity, outEntities);
		}
	}

	void EntitySerializer::SerializeEntityAsPrefab(const char* filepath, Entity entity)
	{
		if (entity.HasComponent<PrefabComponent>())
		{
			ARC_CORE_ERROR("Entity already has a prefab component!");
			return;
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::Value << entity.AddComponent<PrefabComponent>().ID;
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		std::vector<Entity> entities;
		entities.push_back(entity);
		GetAllChildren(entity, entities);

		for (const auto& child : entities)
		{
			if (child)
				EntitySerializer::SerializeEntity(out, child);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	Entity EntitySerializer::DeserializeEntityAsPrefab(const char* filepath, Scene& scene)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Prefab"])
			return {};

		UUID prefabID = 0;
		TrySet(prefabID, data["Prefab"]);

		if (!prefabID)
		{
			ARC_CORE_ERROR("Ivalid prefab id : {0} ({1})", StringUtils::GetName(filepath), prefabID);
			return {};
		}

		auto entities = data["Entities"];
		ARC_CORE_TRACE("Deserializing prefab : {0} ({1})", StringUtils::GetName(filepath), prefabID);

		Entity root = {};

		if (entities)
		{
			std::unordered_map<UUID, UUID> oldNewIdMap;
			for (const auto& entity : entities)
			{
				UUID oldUUID = 0;
				TrySet(oldUUID, entity["Entity"]);
				UUID newUUID = EntitySerializer::DeserializeEntity(entity, scene, false);
				oldNewIdMap.emplace(oldUUID, newUUID);
				
				if (!root)
					root = scene.GetEntity(newUUID);
			}

			root.AddComponent<PrefabComponent>().ID = prefabID;

			// Fix parent/children UUIDs
			for (const auto& [oldId, newId] : oldNewIdMap)
			{
				auto& relationshipComponent = scene.GetEntity(newId).GetRelationship();
				UUID parent = relationshipComponent.Parent;
				if (parent)
					relationshipComponent.Parent = oldNewIdMap.at(parent);

				auto& children = relationshipComponent.Children;
				for (auto& id : children)
					id = oldNewIdMap.at(id);
			}
		}
		else
		{
			ARC_CORE_ERROR("There are no entities in the prefab {0} ({1}) to deserialize!", StringUtils::GetName(filepath), prefabID);
		}

		return root;
	}
}
