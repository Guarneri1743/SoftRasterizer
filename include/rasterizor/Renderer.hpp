#pragma once
#include <string>
#include <vector>
#include <memory>
#include <stdint.h>
#include "Object.hpp"
#include "tinymath.h"
#include "Triangle.hpp"
#include "Model.hpp"

namespace CpuRasterizor
{
	class Renderer : public Object
	{
	public:
		std::shared_ptr<Model> target;

	public:
		Renderer();
		Renderer(std::shared_ptr<Model> model);
		Renderer(const Renderer& other);
		~Renderer();
		static std::unique_ptr<Renderer> create(std::shared_ptr<Model> model);
		static std::unique_ptr<Renderer> create(const Renderer& other);
		virtual tinymath::mat4x4 view_matrix(const RenderPass& render_pass) const;
		virtual tinymath::mat4x4 projection_matrix(const RenderPass& render_pass) const;
		virtual tinymath::mat4x4 model_matrix() const;
		virtual void render_shadow() const;
		virtual void render() const;
		void render_internal(const RenderPass& render_pass) const;
		virtual void draw_gizmos() const;
		Renderer& operator =(const Renderer& other);
		void copy(const Renderer& other);

	protected:
		bool gizmos;
	};
}