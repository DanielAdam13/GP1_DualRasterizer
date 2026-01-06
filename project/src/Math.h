#pragma once
#include "ColorRGB.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"
#include "MathHelpers.h"

#include "DataStructs.h"
using namespace dae;

inline bool IsPixelIn_Triangle(const std::array<VertexOut, 3>& screenTriangle, VertexIn& pixel, std::array<float, 3>& areaRatios)
{
	return false;
}