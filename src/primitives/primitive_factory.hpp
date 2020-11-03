#pragma once 
#include <guarneri.hpp>

namespace guarneri {
	vertex cube_vertices[8] = {
		vertex(float4(-1, -1,  1, 1), float4(1.0f, 0.2f, 0.2f, 0.3f), float3::ONE, float2(0.0f, 0.0f), float3(), float3()),
		vertex(float4(1, -1,  1, 1), float4(0.2f, 0.2f, 1.0f, 0.3f),float3::ONE,  float2(0.0f, 1.0f), float3(), float3()),
		vertex(float4(1, 1,  1, 1), float4(0.2f, 0.2f, 1.0f, 0.3f), float3::ONE,  float2(1.0f, 1.0f), float3(), float3()),
		vertex(float4(-1,  1,  1, 1), float4(1.0f, 0.2f, 1.0f, 0.3f),  float3::ONE, float2(1.0f, 0.0f), float3(), float3()),
		vertex(float4(-1, -1, -1, 1), float4(1.0f, 1.0f, 0.2f, 0.3f), float3::ONE, float2(0.0f, 0.0f), float3(), float3()),
		vertex(float4(1, -1, -1, 1),float4(0.2f, 1.0f, 1.0f, 0.3f),  float3::ONE, float2(0.0f, 1.0f), float3(), float3()),
		vertex(float4(1,  1, -1, 1), float4(1.0f, 0.3f, 0.3f, 0.3f), float3::ONE,  float2(1.0f, 1.0f), float3(), float3()),
		vertex(float4(-1,  1, -1, 1),  float4(0.2f, 1.0f, 0.3f, 0.3f), float3::ONE, float2(1.0f, 0.0f), float3(), float3()),
	};

	uint32_t cube_indices[36] = {
		0, 1, 2, 2, 3, 1,
		7, 6, 5, 5, 4, 7,
		0, 4, 5, 5, 1, 0,
		1, 5, 6, 6, 2, 1,
		2, 6, 7, 7, 3, 2,
		3, 7, 4, 4, 0, 3
	};

	vertex plane_vertices[8] = {
		vertex(float4(1, 1,  1, 1), float4(0.2f, 0.2f, 1.0f, 0.3f), float3::ONE,  float2(0.0f, 0.0f), float3(), float3()),
		vertex(float4(1,  1, -1, 1), float4(1.0f, 0.3f, 0.3f, 0.3f), float3::ONE,  float2(0.0f, 1.0f), float3(), float3()),
		vertex(float4(-1,  1, -1, 1),  float4(0.2f, 1.0f, 0.3f, 0.3f), float3::ONE, float2(1.0f, 1.0f), float3(), float3()),
		vertex(float4(-1,  1,  1, 1), float4(1.0f, 0.2f, 1.0f, 0.3f),  float3::ONE, float2(1.0f, 0.0f), float3(), float3()),
	};

	uint32_t plane_indices[6] = {
		0, 1, 2, 2, 3, 0
	};

	class primitive_factory {
	public:
		static std::unique_ptr<model> plane(std::unique_ptr<material> material) {
			const std::vector<vertex> vert(plane_vertices, plane_vertices + 4);
			const std::vector<uint32_t> ind(plane_indices, plane_indices + 6);
			return model::create(vert, ind, std::move(material));
		}

		static std::unique_ptr<model> cube(std::unique_ptr<material> material) {
			const std::vector<vertex> vert(cube_vertices, cube_vertices + 8);
			const std::vector<uint32_t> ind(cube_indices, cube_indices + 36);
			return model::create(vert, ind, std::move(material));
		}
	};
}