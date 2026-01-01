#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"
#include "ColorRGB.h"
#include "MathHelpers.h"

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
	};

	struct VertexOut
	{
		Vector4 position{};
		Vector2 UVCoordinate{};
		Vector3 normal{};
		Vector3 tangent{};
	};
}