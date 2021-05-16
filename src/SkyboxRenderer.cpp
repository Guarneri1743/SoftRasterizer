#include "SkyboxRenderer.hpp"
#include "PrimitiveFactory.hpp"
#include "SkyboxShader.hpp"
#include "Utility.hpp"
#include "Material.hpp"
#include "Singleton.hpp"

namespace Guarneri
{
	SkyboxRenderer::SkyboxRenderer()
	{
		auto shader = std::make_unique<SkyboxShader>();
		auto mat = std::make_unique<Material>(shader);
		mat->ztest_func = CompareFunc::LEQUAL;
		target = PrimitiveFactory::skybox(mat);
	}

	SkyboxRenderer::~SkyboxRenderer()
	{}

	Matrix4x4 SkyboxRenderer::view_matrix(const RenderPass& render_pass) const
	{
		UNUSED(render_pass);
		auto view = INST(MiscParameter).view_matrix;
		auto view3x3 = mat4tomat3(view);
		auto view4x4 = Matrix4x4(
			Vector4(view3x3.row(0), 0),
			Vector4(view3x3.row(1), 0),
			Vector4(view3x3.row(2), 0),
			Vector4(0, 0, 0, 1));
		return view4x4;
	}

	Matrix4x4 SkyboxRenderer::projection_matrix(const RenderPass& render_pass) const
	{
		UNUSED(render_pass);
		return INST(MiscParameter).proj_matrix;
	}

	Matrix4x4 SkyboxRenderer::model_matrix() const
	{
		return target->transform.local2world;
	}

	void SkyboxRenderer::draw_gizmos() const
	{}
}