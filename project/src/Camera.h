#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Timer.h"
#include "Math.h"

namespace dae
{
	struct Camera final
	{
		Camera();
		Camera(const Vector3& _origin, float _fovAngle);

		Vector3 origin;
		float fovAngle;
		float fov;

		float nearPlane;
		float farPlane;

		Vector3 forward;
		Vector3 up;
		Vector3 right;

		float totalPitch;
		float totalYaw;

		Matrix invViewMatrix;
		Matrix viewMatrix;
		Matrix projectionMatrix;

		float CameraRotationSpeed;
		float initialCameraMoveSpeedMouse;
		float initialCameraMoveSpeedKeys;

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f }, float nearPlaneValue = 1.f, float farPlaneValue = 1000.f);

		void CalculateViewMatrix();

		void CalculateProjectionMatrix(float aspectRatio);

		void Update(const Timer* pTimer, float aspectRatio, const RasterizerMode currentRasterizerState);
	};
}
