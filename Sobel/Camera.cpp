#include "pch.h"
#include "Camera.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace Bruce
{
	struct Camera::Impl
	{
		bool isViewDirty = false;
		
		Matrix mView;
		Matrix mProj;

		Vector3 vForward;
		Vector3 vRight;
		Vector3 vUp;
		Vector3 vPosition;
	};

	Camera::Camera()
		:pImpl(std::make_unique<Impl>())
	{

	}

	Camera::~Camera()
	{

	}

	void Camera::Walk(const float distance)
	{
		Vector3 d = -pImpl->vForward * distance;
		pImpl->vPosition += d;
		pImpl->isViewDirty = true;
	}

	void Camera::Strafe(const float distance)
	{
		Vector3 d = pImpl->vRight * distance;
		pImpl->vPosition += d;
		pImpl->isViewDirty = true;
	}

	void Camera::Fly(const float distance)
	{
		Vector3 d = pImpl->vUp * distance;
		pImpl->vPosition += d;
		pImpl->isViewDirty = true;
	}

	void Camera::Pitch(const float radian)
	{
		auto rot_m = Matrix::CreateFromAxisAngle(pImpl->vRight, radian);
 		pImpl->vUp		= Vector3::TransformNormal(pImpl->vUp, rot_m);
 		pImpl->vForward = Vector3::TransformNormal(pImpl->vForward, rot_m);
		pImpl->isViewDirty = true;
	}

	void Camera::RotateY(const float radian)
	{
		auto rot_m = Matrix::CreateRotationY(radian);
		pImpl->vUp		= Vector3::TransformNormal(pImpl->vUp, rot_m);
		pImpl->vForward	= Vector3::TransformNormal(pImpl->vForward, rot_m);
		pImpl->vRight	= Vector3::TransformNormal(pImpl->vRight, rot_m);
		pImpl->isViewDirty = true;
	}

	void Camera::UpdateViewMatrix()
	{
		if (pImpl->isViewDirty)
		{
			auto& up = pImpl->vUp;
			auto& right = pImpl->vRight;
			auto& forward = pImpl->vForward;
			auto& pos = pImpl->vPosition;

			forward.Normalize();
			up = forward.Cross(right);
			up.Normalize();
			right = up.Cross(forward);

			float x = -pos.Dot(right);
			float y = -pos.Dot(up);
			float z = -pos.Dot(forward);

			pImpl->mView = {
				right.x, up.x, forward.x, 0.0f,
				right.y, up.y, forward.y, 0.0f,
				right.z, up.z, forward.z, 0.0f,
				x, y, z, 1.0f
			};

			pImpl->isViewDirty = false;
		}
	}

	DirectX::SimpleMath::Matrix Camera::GetView() const
	{
		assert(!pImpl->isViewDirty);
		return pImpl->mView;
	}

	DirectX::SimpleMath::Matrix Camera::GetProj() const
	{
		return pImpl->mProj;
	}

	void Camera::CreateView(const Vector3& pos, const Vector3& target, const Vector3& worldUp)
	{
		(pos-target).Normalize(pImpl->vForward);
		worldUp.Cross(pImpl->vForward).Normalize(pImpl->vRight);
		pImpl->vUp = pImpl->vForward.Cross(pImpl->vRight);
		pImpl->vPosition = pos;
		pImpl->isViewDirty = true;
	}

	void Camera::CreateProj(float fovY, float aspectRatio, float zNear, float zFar)
	{
		pImpl->mProj = Matrix::CreatePerspectiveFieldOfView(fovY, aspectRatio, zNear, zFar);
	}

}