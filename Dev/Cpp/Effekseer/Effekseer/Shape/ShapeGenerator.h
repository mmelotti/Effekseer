#ifndef __EFFEKSEER_SHAPE_GENERATOR_H__
#define __EFFEKSEER_SHAPE_GENERATOR_H__

#include "../Effekseer.Base.h"
#include "../SIMD/Effekseer.Vec2f.h"
#include "../SIMD/Effekseer.Vec3f.h"
#include "../Utils/Effekseer.CustomAllocator.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Effekseer
{

struct RotatorSphere
{
	float Radius;

	Vec2f GetPosition(float value) const
	{
		float axisPos = -Radius + Radius * 2.0f * value;
		return sqrt(Radius * Radius - axisPos * axisPos);
	}
};

struct RotatorCone
{
	float Radius;
	float Length;

	Vec2f GetPosition(float value) const
	{
		float axisPos = Length * value;
		return Radius / Length * axisPos;
	}
};

struct RotatorCylinder
{
	float RadiusStart;
	float RadiusEnd;
	float Length;

	Vec2f GetPosition(float value) const
	{
		float axisPos = Length * value;
		return (RadiusEnd - RadiusStart) / Length * axisPos + RadiusStart;
	}
};

struct ShapeVertex
{
	Vec3f Position;
	Vec3f Normal;
	Vec3f Tangent;
	Vec2f UV;
};

struct Shape
{
	CustomAlignedVector<ShapeVertex> Vertexes;
	CustomVector<int32_t> Indexes;
};

template <typename T> struct RotatorShapeGenerator
{
	float AxisValueMin;
	float AxisValueMax;
	float AngleMin;
	float AngleMax;
	T Rotator;

	Vec3f GetPosition(float axisValue, float angleValue) const
	{
		auto value = (AxisValueMax - AxisValueMin) * axisValue + AxisValueMin;
		auto angle = (AngleMax - AngleMin) * angleValue + AngleMin;

		Vec2f pos2d = Rotator.GetPosition(value);

		float s;
		float c;
		SinCos(angle, s, c);

		auto rx = pos2d.GetX() * s;
		auto rz = pos2d.GetX() * c;
		auto y = pos2d.GetY();

		return Vec3f(rx, y, rz);
	}

	Shape Generate(int32_t axisDivision, int32_t angleDivision)
	{
		assert(axisDivision > 1);
		assert(angleDivision > 1);

		Shape ret;

		ret.Vertexes.resize(axisDivision * angleDivision);
		ret.Indexes.resize((axisDivision - 1) * (angleDivision - 1) * 6);

		for (int32_t v = 0; v < axisDivision; v++)
		{
			for (int32_t u = 0; u < angleDivision; u++)
			{
				ret.Vertexes[u + v * angleDivision].Position = GetPosition(u / float(angleDivision - 1), v / float(axisDivision - 1));
				ret.Vertexes[u + v * angleDivision].UV = Vec2f(u / float(angleDivision - 1), v / float(axisDivision - 1));
			}
		}

		return ret;
	}
};

class ShapeGenerator
{
public:
	// Sugoi tube

	// wire

	// Rectangle

	ShapeVertex Rectangle(float width, float height, float x, float y) {}

	// Box

	// Others

	// Filter
	// Twist
	// Jitter
	// Require normal
	// Calculate tangent
};

} // namespace Effekseer

#endif