#pragma once
#include "Math.h"

#include <iostream>

#include <vector>
#include <array>

namespace dae
{
	struct VertexIn
	{
		Vector3 position{};
		Vector2 UVCoordinate{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{}; // Only for Software
	};

	struct VertexOut
	{
		Vector4 position{};
		Vector2 UVCoordinate{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{}; // Only for Software
	};
}