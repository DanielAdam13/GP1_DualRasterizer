#pragma once
#include "MathHelpers.h"

namespace dae
{
	struct ColorRGB final
	{
		float r{};
		float g{};
		float b{};

		void MaxToOne();

		static ColorRGB Lerp(const ColorRGB& c1, const ColorRGB& c2, float factor);

		
		const ColorRGB& operator+=(const ColorRGB& c);
		ColorRGB operator+(const ColorRGB& c) const;
		const ColorRGB& operator-=(const ColorRGB& c);
		ColorRGB operator-(const ColorRGB& c) const;
		const ColorRGB& operator*=(const ColorRGB& c);
		ColorRGB operator*(const ColorRGB& c) const;
		const ColorRGB& operator/=(const ColorRGB& c);
		const ColorRGB operator/(const ColorRGB& c) const;
		const ColorRGB& operator*=(float s);
		ColorRGB operator*(float s) const;
		const ColorRGB& operator/=(float s);
		const ColorRGB operator/(float s) const;
		#pragma endregion
	};

	//ColorRGB (Global) Operators
	inline ColorRGB operator*(float s, const ColorRGB& c)
	{
		return c * s;
	}

	namespace colors
	{
		static ColorRGB Red{ 1,0,0 };
		static ColorRGB Blue{ 0,0,1 };
		static ColorRGB Green{ 0,1,0 };
		static ColorRGB Yellow{ 1,1,0 };
		static ColorRGB Cyan{ 0,1,1 };
		static ColorRGB Magenta{ 1,0,1 };
		static ColorRGB White{ 1,1,1 };
		static ColorRGB Black{ 0,0,0 };
		static ColorRGB Gray{ 0.5f,0.5f,0.5f };
	}
}