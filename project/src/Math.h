#pragma once
#include "ColorRGB.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"
#include "MathHelpers.h"

#include "DataStructs.h"

enum class RasterizerMode
{
	Hardware = 0,
	Software = 1
};

enum class CullMode
{
	Back = 0,
	None = 1,
	Front = 2
};

using namespace dae;

inline bool IsPixelIn_Triangle(const std::array<VertexOut, 3>& screenTriangle, VertexIn& pixel, std::array<float, 3>& areaRatios)
{
	std::array<Vector2, 3> transformedTriangle;

	const Vector2 transformedPixel{ pixel.position.x, pixel.position.y };

	for (int i{}; i < 3; ++i)
	{
		transformedTriangle[i].x = screenTriangle[i].position.x;
		transformedTriangle[i].y = screenTriangle[i].position.y;
	}

	int correctVector = 0;

	for (size_t i{}; i < 3; ++i)
	{
		const Vector2 e{ transformedTriangle[(i + 1) % 3] - transformedTriangle[i] }; // edge
		const Vector2 p{ transformedPixel - transformedTriangle[i] }; // to pixel

		const float triPointToPixelCross{ Vector2::Cross(p, e) };

		if (triPointToPixelCross > 0.f)
		{
			return false; // early out
		}

		const Vector2 secondEdge{ transformedTriangle[(i + 2) % 3] - transformedTriangle[i] };
		const float triPointToNextPointCross{ Vector2::Cross(secondEdge, e) };

		const float areaRatio{ triPointToPixelCross / triPointToNextPointCross };

		if (triPointToPixelCross <= 0.f)
		{
			correctVector++;

			if (correctVector == 1)
			{
				areaRatios[0] = areaRatio;
			}
			else if (correctVector == 2)
			{
				areaRatios[1] = areaRatio;
			}
			else if (correctVector == 3)
			{
				areaRatios[2] = areaRatio;

				// --- Using the NEWLY INTERPOLATED Z value for the DEPTH, NON-LINEAR ---
				const float ZValue{ areaRatios[0] / screenTriangle[2].position.z + areaRatios[1] / screenTriangle[0].position.z + areaRatios[2] / screenTriangle[1].position.z };
				const float interpolatedZ{ 1 / ZValue };

				pixel.position.z = interpolatedZ; // Value we use for depth buffer
			}
		}
	}

	return true;
}

inline void InterpolateVertex(const std::array<float, 3>& areaRatios, const std::array<VertexOut, 3>& screenTriangle, VertexIn& pixel)
{
	// --- Using the ORIGINAL view space Z for every other pixel vairable !!! ---
	const float WValue{ areaRatios[0] * screenTriangle[2].position.w + areaRatios[1] * screenTriangle[0].position.w + areaRatios[2] * screenTriangle[1].position.w };
	const float interpolatedW{ 1 / WValue };

	const Vector2 uvValue{ areaRatios[0] * screenTriangle[2].UVCoordinate +
		areaRatios[1] * screenTriangle[0].UVCoordinate +
		areaRatios[2] * screenTriangle[1].UVCoordinate };

	pixel.UVCoordinate = uvValue * interpolatedW;

	// --- Interpolate Normal and Tangent by W ---
	const Vector3 normalValue{ areaRatios[0] * screenTriangle[2].normal +
		 areaRatios[1] * screenTriangle[0].normal +
		 areaRatios[2] * screenTriangle[1].normal };
	pixel.normal = normalValue;

	const Vector3 tangentValue{ areaRatios[0] * screenTriangle[2].tangent +
						  areaRatios[1] * screenTriangle[0].tangent +
						  areaRatios[2] * screenTriangle[1].tangent };
	pixel.tangent = tangentValue;

	// Normalize after !!!
	pixel.normal.Normalize();
	pixel.tangent.Normalize();

	// --- Pixel View Direction Interpolation --- (For Sepcular Lighting)
	const Vector3 viewDirectionValue{ areaRatios[0] * screenTriangle[2].viewDirection +
		 areaRatios[1] * screenTriangle[0].viewDirection +
		 areaRatios[2] * screenTriangle[1].viewDirection };
	pixel.viewDirection = viewDirectionValue;

	pixel.viewDirection; // Normalizing later when needed (Phong)
}

inline int GetPixelNumber(int px, int py, int screenWidth)
{
	return px + py * screenWidth;
}

inline auto RemapValue(float value, float threshold = 0.95f) -> ColorRGB
{
	float clamped{ value };
	if (value < threshold)
	{
		clamped = threshold;
	}
	else if (value > 1.f)
	{
		clamped = 1.f;
	}

	const float denom{ 1.f - threshold };
	const float t{ (denom > 0.f) ? (clamped - threshold) / denom : 0.f };

	return ColorRGB{ t, t, t };
}

inline ColorRGB GetLambertColor(const ColorRGB& sampledDiffuse, float cosTheta)
{
	constexpr float diffuseReflectance{ 7.f };

	const ColorRGB LambertColor{ (sampledDiffuse * diffuseReflectance / PI) * cosTheta };

	return LambertColor;
}

inline ColorRGB Phong(const ColorRGB& specularColor, float reflectance, float phongExponent, const Vector3& lightVector, const Vector3& viewVector, const Vector3& normal)
{
	const float lightHitNormalDot{ Vector3::Dot(normal, lightVector) };

	if (lightHitNormalDot <= 0.f)
		return {};

	const Vector3 reflectRay{ 2 * lightHitNormalDot * normal - lightVector };

	const float cosA{ std::max(0.f, Vector3::Dot(reflectRay, viewVector)) };

	const float expCosA{ powf(cosA, phongExponent) };

	return reflectance * specularColor * expCosA;
}

inline bool IsTriangleOffScreen(const std::array<VertexOut, 3>& tri, int screenWidth, int screenHeight)
{
	for (const auto& v : tri)
	{
		// If any vertex is outside the screen
		if (v.position.x < 0.f || v.position.x >= screenWidth ||
			v.position.y < 0.f || v.position.y >= screenHeight)
		{
			return true;
		}
	}
	return false;
}