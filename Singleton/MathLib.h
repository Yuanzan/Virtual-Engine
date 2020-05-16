#pragma once
#include "../Common/d3dUtil.h"
#include "../Singleton/MathLib.h"

const float Deg2Rad = 0.0174532924;
const float Rad2Deg = 57.29578;
struct Cone
{
	DirectX::XMFLOAT3 vertex;
	float height;
	DirectX::XMFLOAT3 direction;
	float radius;
	Cone(const DirectX::XMFLOAT3& position, float distance, const DirectX::XMFLOAT3& direction, float angle) :
		vertex(position),
		height(distance),
		direction(direction)
	{
		radius = tan(angle * 0.5 * Deg2Rad) * height;
	}
};
class MathLib final
{
public:
	MathLib() = delete;
	~MathLib() = delete;
	static Math::Vector4 GetPlane(
		const Math::Vector3& normal,
		const Math::Vector3& inPoint);
	static Math::Vector4 GetPlane(
		const Math::Vector3& a,
		const Math::Vector3& b,
		const Math::Vector3& c);
	/*static bool BoxIntersect(
		const Math::Matrix4& localToWorldMatrix,
		Math::Vector4* planes,
		const Math::Vector3& localPosition,
		const Math::Vector3& localExtent);*/
	static bool BoxIntersect(
		const Math::Matrix4& localToWorldMatrix,
		Math::Vector4* planes,
		const Math::Vector3& localPosition,
		const Math::Vector3& localExtent,
		const Math::Vector3& frustumMinPoint,
		const Math::Vector3& frustumMaxPoint);
	/*static void GetCameraNearPlanePoints(
		const Math::Matrix4& localToWorldMatrix,
		double fov,
		double aspect,
		double distance,
		Math::Vector3* corners
	);*/

	static void GetCameraNearPlanePoints(
		const Math::Vector3& right,
		const Math::Vector3& up,
		const Math::Vector3& forward,
		const Math::Vector3& position,
		double fov,
		double aspect,
		double distance,
		Math::Vector3* corners
	);
	/*
	static void GetPerspFrustumPlanes(
		const Math::Matrix4& localToWorldMatrix,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		DirectX::XMFLOAT4* frustumPlanes);*/
	static void GetPerspFrustumPlanes(
		const Math::Vector3& right,
		const Math::Vector3& up,
		const Math::Vector3& forward,
		const Math::Vector3& position,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		Math::Vector4* frustumPlanes);
	static void GetPerspFrustumPlanes(
		const Math::Vector3& right,
		const Math::Vector3& up,
		const Math::Vector3& forward,
		const Math::Vector3& position,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		float4* frustumPlanes);
	/*static void GetPerspFrustumPlanes(
		const Math::Matrix4& localToWorldMatrix,
		double fov,
		double aspect,
		double nearPlane,
		double farPlane,
		Math::Vector4* frustumPlanes
	);
	
	static void GetFrustumBoundingBox(
		const Math::Matrix4& localToWorldMatrix,
		double nearWindowHeight,
		double farWindowHeight,
		double aspect,
		double nearZ,
		double farZ,
		Math::Vector3* minValue,
		Math::Vector3* maxValue
	);*/
	static void GetFrustumBoundingBox(
		const Math::Vector3& right,
		const Math::Vector3& up,
		const Math::Vector3& forward,
		const Math::Vector3& position,
		double nearWindowHeight,
		double farWindowHeight,
		double aspect,
		double nearZ,
		double farZ,
		Math::Vector3* minValue,
		Math::Vector3* maxValue
	);

	static float GetPointDistanceToPlane(
		const Math::Vector4& plane,
		const Math::Vector3& point)
	{
		float dotValue = dot((Math::Vector3)plane, (Math::Vector3)point);
		return dotValue + plane.GetW();
	}
	static bool ConeIntersect(const Cone& cone, const Math::Vector4& plane);
	static void GetOrthoCamFrustumPlanes(
		const Math::Vector3& right,
		const Math::Vector3& up,
		const Math::Vector3& forward,
		const Math::Vector3& position,
		float xSize,
		float ySize,
		float nearPlane,
		float farPlane,
		Math::Vector4* results);
	static void GetOrthoCamFrustumPoints(
		const Math::Vector3& right,
		const Math::Vector3& up,
		const Math::Vector3& forward,
		const Math::Vector3& position,
		float xSize,
		float ySize,
		float nearPlane,
		float farPlane,
		Math::Vector3* results);

	static double DistanceToCube(const Math::Vector3& size, const Math::Vector3& quadToTarget);
	static double DistanceToQuad(double size, float2 quadToTarget);
	static bool BoxIntersect(const Math::Vector3& position, const Math::Vector3& extent, Math::Vector4* planes, int len);
	static bool BoxContactWithBox(double3 min0, double3 max0, double3 min1, double3 max1);
	static Math::Vector4 CameraSpacePlane(const Math::Matrix4& worldToCameraMatrix, const Math::Vector3& pos, const Math::Vector3& normal, float clipPlaneOffset);
	static Math::Matrix4 CalculateObliqueMatrix(const Math::Vector4& clipPlane, const Math::Matrix4& projection);
	static void GetCubemapRenderData(
		const Math::Vector3& position,
		float nearZ,
		float farZ,
		Math::Matrix4* viewProj,
		Math::Vector4** frustumPlanes,
		std::pair<Math::Vector3, Math::Vector3>* minMaxBound);
};