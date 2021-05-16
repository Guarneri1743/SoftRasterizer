#ifndef _DEFINE_
#define _DEFINE_
#include <stdint.h>
#include <ostream>
#include <sstream>
#include "Marcos.h"
#include "Traits.hpp"

namespace Guarneri{
	// types
	typedef uint32_t property_name;
	const property_name albedo_prop = 0; //"texture_diffuse/albedo";
	const property_name normal_prop = 1; // "texture_normal";
	const property_name ao_prop = 2; // "texture_ao";
	const property_name emission_prop = 3; // "texture_emission"
	const property_name height_prop = 4; // "texture_emission"
	const property_name f0_prop = 5; // "texture_f0"

	// specular/glossiness workflow
	const property_name specular_prop = 11; // "texture_specular";
	const property_name glossiness_prop = 12; // "texture_glossiness";

	// metallic/roughness workflow
	const property_name metallic_prop = 13; // "texture_metallic"
	const property_name roughness_prop = 14; // "texture_roughness"
	const property_name base_color_prop = 15; //"texture_base_color";

	// other
	const property_name cubemap_prop = 20; // "skybox_cubemap";
	const property_name shadowmap_prop = 21; // "shadowmap";

	// statistics
	struct GraphicsStatistic
	{
		uint32_t triangle_count;
		uint32_t culled_triangle_count;
		uint32_t culled_backface_triangle_count;
		uint32_t earlyz_optimized;
	};

	// pixel block
	typedef struct
	{
		uint32_t start_x; uint32_t start_y; uint32_t size;
	} PixelBlock;

	// framebuffer color 
	typedef struct
	{
		uint8_t r; uint8_t g; uint8_t b;
	} color_rgb;

	typedef struct
	{
		uint8_t r; uint8_t g; uint8_t b; uint8_t a;
	} color_rgba;

	typedef struct
	{
		uint8_t b; uint8_t g; uint8_t r; uint8_t a;
	} color_bgra;

	typedef struct
	{
		uint8_t r; uint8_t g;
	}color_rg;

	typedef struct
	{
		uint8_t gray;
	}color_gray;

	typedef uint8_t image_ubyte;

	// pipeline defines
	enum class RasterizerStrategy {
		SCANBLOCK,
		SCANLINE
	};

	enum class ColorSpace
	{
		Gamma,
		Linear
	};

	enum class PBRWorkFlow
	{
		Metallic,
		Specular
	};

	enum class Projection {
		PERSPECTIVE,
		ORTHO
	};

	enum class CompareFunc {
		NEVER,
		ALWAYS,
		LESS,
		LEQUAL,
		EQUAL,
		GEQUAL,
		GREATER,
		NOT_EQUAL
	};

	enum class ZWrite {
		ON,
		OFF
	};

	enum class StencilOp {
		KEEP,
		ZERO,
		REPLACE,
		INCR,
		INCR_WRAP,
		DECR,
		DECR_WRAP,
		INVERT
	};

	enum class BlendFactor {
		ONE,
		SRC_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		ONE_MINUS_DST_COLOR
	};

	enum class BlendOp {
		ADD,
		SUB
	};

	enum class ColorMask {
		ZERO = 0,
		R = 1 << 0,
		G = 1 << 1,
		B = 1 << 2,
		A = 1 << 3
	};

	enum class CullingAndClippingFlag {
		DISABLE = 0,
		APP_FRUSTUM_CULLING = 1 << 0,
		NEAR_PLANE_CLIPPING = 1 << 1,
		SCREEN_CLIPPING = 1 << 2,
		BACK_FACE_CULLING = 1 << 3
	};

	enum class PerSampleOperation {
		DISABLE = 0,
		SCISSOR_TEST = 1 << 0,
		ALPHA_TEST = 1 << 1,
		STENCIL_TEST = 1 << 2,
		DEPTH_TEST = 1 << 3,
		BLENDING = 1 << 4
	};

	enum class BufferFlag {
		NONE = 0,
		COLOR = 1 << 0,
		DEPTH = 1 << 1,
		STENCIL = 1 << 2
	};

	enum class RenderFlag {
		DISABLE = 0,
		WIREFRAME = 1 << 0,
		SHADOWMAP = 1 << 1,
		DEPTH = 1 << 2,
		UV = 1 << 3,
		VERTEX_COLOR = 1 << 4,
		NORMAL = 1 << 5,
		MIPMAP = 1 << 6,
		STENCIL = 1 << 7,
		SCANLINE = 1 << 8,
		FRAME_TILE = 1 << 9,
		CULLED_BACK_FACE = 1 << 10,
		SPECULAR = 1 << 11,
		EARLY_Z_DEBUG = 1 << 12
	};

#ifdef OPAQUE
#undef OPAQUE
#endif
#ifdef TRANSPARENT
#undef TRANSPARENT
#endif

	enum class RenderPass {
		OBJECT,
		SHADOW,
		SKYBOX
	};

	template<>
	struct support_bitwise_enum<RenderFlag> : std::true_type {};

	template<>
	struct support_bitwise_enum<BufferFlag> : std::true_type {};

	template<>
	struct support_bitwise_enum<PerSampleOperation> : std::true_type {};

	template<>
	struct support_bitwise_enum<CullingAndClippingFlag> : std::true_type {};

	template<>
	struct support_bitwise_enum<ColorMask> : std::true_type {};


	static std::ostream& operator << (std::ostream& stream, const RenderFlag& flag) {
		int count = 0;
		if ((flag & RenderFlag::WIREFRAME) != RenderFlag::DISABLE) {
			stream << "WIREFRAME";
			count++;
		}
		if ((flag & RenderFlag::DEPTH) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | DEPTH" : "DEPTH");
			count++;
		}
		if ((flag & RenderFlag::UV) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | UV" : "UV");
			count++;
		}
		if ((flag & RenderFlag::VERTEX_COLOR) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | VERTEX_COLOR" : "VERTEX_COLOR");
			count++;
		}
		if ((flag & RenderFlag::NORMAL) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | NORMAL" : "NORMAL");
			count++;
		}
		if ((flag & RenderFlag::CULLED_BACK_FACE) != RenderFlag::DISABLE)
		{
			stream << (count > 0 ? " | CULLED_BACK_FACE" : "CULLED_BACK_FACE");
			count++;
		}
		if ((flag & RenderFlag::EARLY_Z_DEBUG) != RenderFlag::DISABLE)
		{
			stream << (count > 0 ? " | EARLY_Z_DEBUG" : "EARLY_Z_DEBUG");
			count++;
		}
		if ((flag & RenderFlag::MIPMAP) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | MIPMAP" : "MIPMAP");
			count++;
		}
		if ((flag & RenderFlag::STENCIL) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | STENCIL" : "STENCIL");
			count++;
		}
		if ((flag & RenderFlag::SCANLINE) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | SCANLINE" : "SCANLINE");
			count++;
		}
		if ((flag & RenderFlag::FRAME_TILE) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | FRAME_TILE" : "FRAME_TILE");
			count++;
		}
		if ((flag & RenderFlag::SHADOWMAP) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | SHADOWMAP" : "SHADOWMAP");
			count++;
		}
		if ((flag & RenderFlag::SPECULAR) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | SPECULAR" : "SPECULAR");
			count++;
		}
		return stream;
	}

	static std::stringstream& operator << (std::stringstream& stream, const RenderFlag& flag) {
		int count = 0;
		if ((flag & RenderFlag::WIREFRAME) != RenderFlag::DISABLE) {
			stream << "WIREFRAME";
			count++;
		}
		if ((flag & RenderFlag::DEPTH) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | DEPTH" : "DEPTH");
			count++;
		}
		if ((flag & RenderFlag::UV) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | UV" : "UV");
			count++;
		}
		if ((flag & RenderFlag::VERTEX_COLOR) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | VERTEX_COLOR" : "VERTEX_COLOR");
			count++;
		}
		if ((flag & RenderFlag::NORMAL) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | NORMAL" : "NORMAL");
			count++;
		}
		if ((flag & RenderFlag::CULLED_BACK_FACE) != RenderFlag::DISABLE)
		{
			stream << (count > 0 ? " | CULLED_BACK_FACE" : "CULLED_BACK_FACE");
			count++;
		}
		if ((flag & RenderFlag::EARLY_Z_DEBUG) != RenderFlag::DISABLE)
		{
			stream << (count > 0 ? " | EARLY_Z_DEBUG" : "EARLY_Z_DEBUG");
			count++;
		}
		if ((flag & RenderFlag::MIPMAP) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | MIPMAP" : "MIPMAP");
			count++;
		}
		if ((flag & RenderFlag::STENCIL) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | STENCIL" : "STENCIL");
			count++;
		}
		if ((flag & RenderFlag::SCANLINE) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | SCANLINE" : "SCANLINE");
			count++;
		}
		if ((flag & RenderFlag::FRAME_TILE) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | FRAME_TILE" : "FRAME_TILE");
			count++;
		}
		if ((flag & RenderFlag::SHADOWMAP) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | SHADOWMAP" : "SHADOWMAP");
			count++;
		}
		if ((flag & RenderFlag::SPECULAR) != RenderFlag::DISABLE) {
			stream << (count > 0 ? " | SPECULAR" : "SPECULAR");
			count++;
		}
		return stream;
	}

	static std::ostream& operator << (std::ostream& stream, const CullingAndClippingFlag& flag) {
		int count = 0;
		if ((flag & CullingAndClippingFlag::APP_FRUSTUM_CULLING) != CullingAndClippingFlag::DISABLE) {
			stream << "APP_FRUSTUM_CULLING";
			count++;
		}
		if ((flag & CullingAndClippingFlag::BACK_FACE_CULLING) != CullingAndClippingFlag::DISABLE) {
			stream << (count > 0 ? " | BACK_FACE_CULLING" : "BACK_FACE_CULLING");
			count++;
		}
		if ((flag & CullingAndClippingFlag::NEAR_PLANE_CLIPPING) != CullingAndClippingFlag::DISABLE) {
			stream << (count > 0 ? " | NEAR_PLANE_CLIPPING" : "NEAR_PLANE_CLIPPING");
			count++;
		}
		if ((flag & CullingAndClippingFlag::SCREEN_CLIPPING) != CullingAndClippingFlag::DISABLE) {
			stream << (count > 0 ? " | SCREEN_CLIPPING" : "SCREEN_CLIPPING");
			count++;
		}
		return stream;
	}

	static std::stringstream& operator << (std::stringstream& stream, const CullingAndClippingFlag& flag) {
		int count = 0;
		if ((flag & CullingAndClippingFlag::APP_FRUSTUM_CULLING) != CullingAndClippingFlag::DISABLE) {
			stream << "APP_FRUSTUM_CULLING";
			count++;
		}
		if ((flag & CullingAndClippingFlag::BACK_FACE_CULLING) != CullingAndClippingFlag::DISABLE) {
			stream << (count > 0 ? " | BACK_FACE_CULLING" : "BACK_FACE_CULLING");
			count++;
		}
		if ((flag & CullingAndClippingFlag::NEAR_PLANE_CLIPPING) != CullingAndClippingFlag::DISABLE) {
			stream << (count > 0 ? " | NEAR_PLANE_CLIPPING" : "NEAR_PLANE_CLIPPING");
			count++;
		}
		if ((flag & CullingAndClippingFlag::SCREEN_CLIPPING) != CullingAndClippingFlag::DISABLE) {
			stream << (count > 0 ? " | SCREEN_CLIPPING" : "SCREEN_CLIPPING");
			count++;
		}
		return stream;
	}
}

#endif