#pragma once
#include "Camera.h"
#include <cmath>

namespace dae
{
	Camera::Camera()
		:origin{ 0.f, 0.f, 0.f },
		fovAngle{ 60.f },
		fov{ tanf((fovAngle * TO_RADIANS) / 2.f) },
		nearPlane{ 1.f },
		farPlane{ 1000.f },
		forward{ Vector3::UnitZ },
		up{ Vector3::UnitY },
		right{ Vector3::UnitX },
		totalPitch{},
		totalYaw{},
		invViewMatrix{},
		viewMatrix{},
		projectionMatrix{},
		CameraRotationSpeed{ PI * 5.f },
		initialCameraMoveSpeedMouse{ 500.f },
		initialCameraMoveSpeedKeys{ 50.f }
	{
	}

	Camera::Camera(const Vector3& _origin, float _fovAngle)
		: origin{ _origin },
		fovAngle{ _fovAngle },
		fov{ tanf((fovAngle * TO_RADIANS) / 2.f) },
		nearPlane{},
		farPlane{},
		forward{ Vector3::UnitZ },
		up{ Vector3::UnitY },
		right{ Vector3::UnitX },
		totalPitch{},
		totalYaw{},
		invViewMatrix{},
		viewMatrix{},
		projectionMatrix{},
		CameraRotationSpeed{ PI * 5.f },
		initialCameraMoveSpeedMouse{ 500.f },
		initialCameraMoveSpeedKeys{ 50.f }
	{
	}

	void Camera::Initialize(float _fovAngle, Vector3 _origin, float nearPlaneValue, float farPlaneValue)
	{
		fovAngle = _fovAngle;
		fov = tanf((fovAngle * TO_RADIANS) / 2.f);

		origin = _origin;

		nearPlane = nearPlaneValue;
		farPlane = farPlaneValue;

		forward = Vector3::UnitZ;
		up = Vector3::UnitY;
		right = Vector3::UnitX;

		CameraRotationSpeed = dae::PI * 5.f;
		initialCameraMoveSpeedMouse = 500.f;
		initialCameraMoveSpeedKeys = 50.f;
	}

	void Camera::CalculateViewMatrix()
	{
		viewMatrix = Matrix::CreateLookAtLH(origin, forward, Vector3::UnitY);

		//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
	}

	void Camera::CalculateProjectionMatrix(float aspectRatio)
	{
		projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);

		//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
	}

	void Camera::Update(const Timer* pTimer, float aspectRatio, const RasterizerMode currentRasterizerState)
	{
		const float deltaTime{ pTimer->GetElapsed() };

		if (currentRasterizerState == RasterizerMode::Software)
		{
#ifdef _DEBUG
			CameraRotationSpeed = dae::PI / 60.f; // low fps in DEBUG mode
			initialCameraMoveSpeedMouse = 1.f;
			initialCameraMoveSpeedKeys = 5.f;
#else
			CameraRotationSpeed = dae::PI / 5.f;
			initialCameraMoveSpeedMouse = 5.f;
			initialCameraMoveSpeedKeys = 30.f;
#endif
		}
		else
		{
			CameraRotationSpeed = dae::PI * 5.f;
			initialCameraMoveSpeedMouse = 500.f;
			initialCameraMoveSpeedKeys = 50.f;
		}

		float currentCameraMoveSpeedMouse{ initialCameraMoveSpeedMouse };
		float currentCameraMoveSpeedKeys{ initialCameraMoveSpeedKeys };

		//Keyboard Input
		const uint8_t* pKeyboardState{ SDL_GetKeyboardState(nullptr) };

		if (pKeyboardState[SDL_SCANCODE_LSHIFT])
		{
			currentCameraMoveSpeedMouse = initialCameraMoveSpeedMouse * 3;
			currentCameraMoveSpeedKeys = initialCameraMoveSpeedKeys * 3;
		}

		//Mouse Input
		int mouseX{}, mouseY{};
		const uint32_t mouseState{ SDL_GetRelativeMouseState(&mouseX, &mouseY) };
		const bool lmb{ (mouseState & SDL_BUTTON_LMASK) != 0 };
		const bool rmb{ (mouseState & SDL_BUTTON_RMASK) != 0 };

		if (lmb && rmb)
		{
			origin += static_cast<float>(-mouseY) * currentCameraMoveSpeedMouse * up * deltaTime;
		}
		else if (rmb)
		{
			totalYaw += static_cast<float>(mouseX) * CameraRotationSpeed * deltaTime;
			totalPitch += static_cast<float>(-mouseY) * CameraRotationSpeed * deltaTime;

			// Calculating the forward vector depending on yaw and pitch
			forward.x = cosf(totalPitch) * sinf(totalYaw);
			forward.y = sinf(totalPitch);
			forward.z = cosf(totalPitch) * cosf(totalYaw);
			forward.Normalize();

			right = Vector3::Cross(Vector3::UnitY, forward);
			right.Normalize();

			up = Vector3::Cross(forward, right);
			up.Normalize();
		}
		else if (lmb)
		{
			totalYaw += static_cast<float>(mouseX) * CameraRotationSpeed * deltaTime;

			forward.x = cosf(totalPitch) * sinf(totalYaw);
			forward.y = sinf(totalPitch);
			forward.z = cosf(totalPitch) * cosf(totalYaw);
			forward.Normalize();

			right = Vector3::Cross(Vector3::UnitY, forward);
			right.Normalize();

			origin += static_cast<float>(-mouseY) * currentCameraMoveSpeedMouse * forward * deltaTime;
		}
		// Change the origin depending on camera.forward
		if (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_UP])
		{
			origin += currentCameraMoveSpeedKeys * forward * deltaTime;
		}
		if (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN])
		{
			origin -= currentCameraMoveSpeedKeys * forward * deltaTime;
		}
		if (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT])
		{
			origin += currentCameraMoveSpeedKeys * right * deltaTime;
		}
		if (pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT])
		{
			origin -= currentCameraMoveSpeedKeys * right * deltaTime;
		}

		//Update Matrices
		CalculateViewMatrix();
		CalculateProjectionMatrix(aspectRatio); //Try to optimize this - should only be called once or when fov/aspectRatio changes
	}
}

