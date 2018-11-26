#pragma once

#include "SimpleMath.h"

namespace Bruce
{
	class Camera
	{
	public:

		Camera();
		~Camera();

		void Walk(const float distance);
		void Strafe(const float distance);
		void Fly(const float distance);
		void Pitch(const float radian);
		void RotateY(const float radian);

		void UpdateViewMatrix();

		DirectX::SimpleMath::Matrix GetView() const;
		DirectX::SimpleMath::Matrix GetProj() const;

		void CreateView(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& target, const DirectX::SimpleMath::Vector3& worldUp);
		void CreateProj(float fovY, float aspectRatio, float zNear, float zFar);


	private:
		struct Impl;
		std::unique_ptr<Impl> pImpl;
	};
}