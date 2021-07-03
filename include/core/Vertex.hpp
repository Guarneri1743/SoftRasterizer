#ifndef _VERTEX_
#define _VERTEX_
#include <string>
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include "rapidjson/document.h"

namespace Guarneri
{
	struct Vertex
	{
	public:
		// serializable
		Vector4 position;
		Vector4 color;
		Vector2 uv;
		Vector3 tangent;
		Vector3 normal;

		// runtime
		Vector3 world_pos;
		Vector4 shadow_coord;
		Vector3 bitangent;
		float rhw;

	public:
		Vertex();
		Vertex(const Vector4& _position, const Vector3& _world_pos, Vector4& _shadow_coord, const Vector4& _color, const Vector3& _normal, const Vector2& _uv, const Vector3& _tangent, const Vector3& _bitangent);
		Vertex(const Vector4& _position, const Vector3& _normal, const Vector2& _uv);
		static Vertex barycentric_interpolate(const Vertex& v0, const Vertex& v1, const Vertex& v2, const float& w0, const float& w1, const float& w2);
		static Vertex interpolate_screen_space(const Vertex& left, const Vertex& right, const float& t);
		static Vertex interpolate_attributes(const Vertex& left, const Vertex& right, const float& t);
		static Vertex interpolate_clip_space(const Vertex& left, const Vertex& right, const float& t);
		static Vertex differential(const Vertex& lhs, const Vertex& rhs);
		static Vertex intagral(const Vertex& left, const Vertex& differential);
		static Vertex clip2ndc(const Vertex& v);
		static Vector4 clip2ndc(const Vector4& v);
		static Vertex ndc2screen(const int& width, const int& height, const Vertex& v);
		static Vector4 ndc2screen(const int& width, const int& height, const Vector4& v);
		static rapidjson::Value serialize(rapidjson::Document& doc, const Vertex& v);
		static Vertex deserialize(const rapidjson::Value& v);
		std::string str() const;
	};
}
#endif