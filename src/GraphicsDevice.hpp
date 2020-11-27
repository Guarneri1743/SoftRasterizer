#ifndef _GRAPHICS_DEVICE_
#define _GRAPHICS_DEVICE_
#include <Guarneri.hpp>

namespace Guarneri {
	struct PrimitiveCommand{
		PrimitiveCommand() {}
		PrimitiveCommand(const Triangle& triangle, Shader* shader) {
			this->triangle = triangle;
			this->shader = shader;
		}
		Triangle triangle;
		Shader* shader;
	};

	SafeQueue<PrimitiveCommand> cmd_queue;

	class GraphicsDevice {
	public:
		uint32_t width;
		uint32_t height;
		GraphicsStatistic statistics;

	private:
		// use 32 bits zbuffer here, for convenience 
		std::shared_ptr<RawBuffer<float>> zbuffer;
		// 32 bits bgra framebuffer, 8-bit per channel
		std::shared_ptr<RawBuffer<color_bgra>> framebuffer;
		// 8 bits stencil buffer
		std::shared_ptr<RawBuffer<uint8_t>> stencilbuffer;


	public:
		void initialize(void* bitmap_handle_t, uint32_t width_t, uint32_t height_t) {
			this->width = width_t;
			this->height = height_t;
			zbuffer = std::make_shared<RawBuffer<float>>(width_t, height_t);
			framebuffer = std::make_shared<RawBuffer<color_bgra>>(bitmap_handle_t, width_t, height_t, [](color_bgra* ptr) { unused(ptr); /*delete[] (void*)ptr;*/ });
			stencilbuffer = std::make_shared<RawBuffer<uint8_t>>(width_t, height_t);
			zbuffer->clear(FAR_Z);
			stencilbuffer->clear(DEFAULT_STENCIL);
			framebuffer->clear(DEFAULT_COLOR);
		}

		void draw(Shader* shader, const Vertex& v1, const Vertex& v2, const Vertex& v3, const Matrix4x4& m, const Matrix4x4& v, const Matrix4x4& p) {
			auto object_space_frustum = Frustum::create(p * v * m);
			if ((misc_param.culling_clipping_flag & CullingAndClippingFlag::APP_FRUSTUM_CULLING) != CullingAndClippingFlag::DISABLE)
			{
				if (Clipper::conservative_frustum_culling(object_space_frustum, v1, v2, v3)) {
					statistics.culled_triangle_count++;
					return;
				}
			}
			if ((misc_param.culling_clipping_flag & CullingAndClippingFlag::NEAR_PLANE_CLIPPING) != CullingAndClippingFlag::DISABLE) {
				auto triangles = Clipper::near_plane_clipping(object_space_frustum.near, v1, v2, v3);
				if (triangles.size() == 0) {
					statistics.culled_triangle_count++;
					return;
				}
				for (size_t idx = 0; idx < triangles.size(); idx++) {
					auto tri = triangles[idx];
					draw_triangle(shader, tri[0], tri[1], tri[2], m, v, p);
				}
			}
			else {
				draw_triangle(shader, v1, v2, v3, m, v, p);
			}
		}

		void rasterize_commands(const int& count) {
			for (int i = 0; i < count; i++) {
				PrimitiveCommand cmd;
				if (cmd_queue.try_pop(cmd)) {
					auto tri = cmd.triangle;
					auto shader = cmd.shader;

					//barycentric_rasterize(tri, shader);
					scanline_rasterize(tri, shader);

					// wireframe
					if ((misc_param.render_flag & RenderFlag::WIREFRAME) != RenderFlag::DISABLE) {
						draw_screen_segment(tri[0].position, tri[1].position, Color(1.0f, 1.0f, 1.0f, 1.0f));
						draw_screen_segment(tri[0].position, tri[2].position, Color(1.0f, 1.0f, 1.0f, 1.0f));
						draw_screen_segment(tri[2].position, tri[1].position, Color(1.0f, 1.0f, 1.0f, 1.0f));
					}
				}
				else {
					std::cerr << "consume error, cmd queue empty" << std::endl;
				}
			}
		}

		void dispatch() {
#ifdef MULTI_THREAD
			auto thread_size = std::thread::hardware_concurrency();
			ThreadPool tp(thread_size);
			const int task_size = 4096;
			int queue_size = cmd_queue.size();
			int task_count = queue_size / task_size;
			int rest = queue_size % task_size;
			for (int i = 0; i < task_count; i++) {
				tp.enqueue([=](const int& count) { rasterize_commands(count); }, task_size);
			}
			tp.enqueue([=](const int& count) { rasterize_commands(count); }, rest);
#else
			rasterize_commands(cmd_buffer.size());
#endif
		}

		void clear_buffer(const BufferFlag& flag) {
			if ((flag & BufferFlag::COLOR) != BufferFlag::NONE) {
				if ((misc_param.render_flag & RenderFlag::DEPTH) != RenderFlag::DISABLE)
				{
					framebuffer->clear(DEFAULT_DEPTH_COLOR);
				}
				else {
					framebuffer->clear(DEFAULT_COLOR);
				}
			}
			if ((flag & BufferFlag::DEPTH) != BufferFlag::NONE) {
				zbuffer->clear(FAR_Z);
			}
			if ((flag & BufferFlag::STENCIL) != BufferFlag::NONE) {
				stencilbuffer->clear(DEFAULT_STENCIL);
			}
			statistics.culled_triangle_count = 0;
			statistics.triangle_count = 0;
			statistics.earlyz_optimized = 0;
		}

		void draw_triangle(Shader* shader, const Vertex& v1, const Vertex& v2, const Vertex& v3, const Matrix4x4& m, const Matrix4x4& v, const Matrix4x4& p) {
			assert(shader != nullptr);

			REF(m);
			REF(v);
			REF(p);

			statistics.triangle_count++;

			v2f o1 = process_vertex(shader, v1);
			v2f o2 = process_vertex(shader, v2);
			v2f o3 = process_vertex(shader, v3);

			Vertex c1(o1.position,o1.world_pos, o1.shadow_coord, o1.color, o1.normal, o1.uv, o1.tangent, o1.bitangent);
			Vertex c2(o2.position,o2.world_pos, o2.shadow_coord, o2.color, o2.normal, o2.uv, o2.tangent, o2.bitangent);
			Vertex c3(o3.position,o3.world_pos, o3.shadow_coord, o3.color, o3.normal, o3.uv, o3.tangent, o3.bitangent);

			// clip in screen space
			//if (!material->skybox && Clipper::cvv_clipping(c1.position, c2.position, c3.position)) {
			//	return;
			//}

			// perspective division, position.
			Vertex n1 = clip2ndc(c1);
			Vertex n2 = clip2ndc(c2);
			Vertex n3 = clip2ndc(c3);

			bool double_face = shader->double_face;
			bool enable_backface_culling = (misc_param.culling_clipping_flag & CullingAndClippingFlag::BACK_FACE_CULLING) != CullingAndClippingFlag::DISABLE;
	
			if (!double_face && enable_backface_culling && !shader->skybox) {
				if (Clipper::backface_culling(n1.position, n2.position, n3.position)) {
					statistics.culled_triangle_count++;
					return;
				}
			}

			// perspective division, uv, color, normal, etc. 
			n1.perspective_division(c1.rhw);
			n2.perspective_division(c2.rhw);
			n3.perspective_division(c3.rhw);

			Vertex s1 = ndc2viewport(n1);
			Vertex s2 = ndc2viewport(n2);
			Vertex s3 = ndc2viewport(n3);

			s1.position.w = c1.position.w;
			s1.rhw = 1.0f / c1.position.w;

			s2.position.w = c2.position.w;
			s2.rhw = 1.0f / c2.position.w;

			s3.position.w = c3.position.w;
			s3.rhw = 1.0f / c3.position.w;

			Triangle tri(s1, s2, s3);

			// primitive assembly
			std::vector<Triangle> tris = tri.horizontally_split();

			// fill ring buffer
			for (auto iter = tris.begin(); iter != tris.end(); iter++) {
				PrimitiveCommand cmd(*iter, shader);
				cmd_queue.push(cmd);
			}
		}

		// per vertex processing
		v2f process_vertex(Shader* shader, const Vertex& vert) const {
			a2v input;
			input.position = vert.position;
			input.uv = vert.uv;
			input.color = vert.color;
			input.normal = vert.normal;
			input.tangent = vert.tangent;
			return shader->vertex_shader(input);
		}

		void barycentric_rasterize(const Triangle& tri, Shader* shader) {
			auto bounds = BoundingBox2D(tri[0].position, tri[1].position, tri[2].position);
			int row_start = (int)(bounds.min().y + 0.5f) - 1;
			int row_end = (int)(bounds.max().y + 0.5f) + 1;
			int col_start = (int)(bounds.min().x + 0.5f) - 1;
			int col_end = (int)(bounds.max().x + 0.5f) + 1;

			// build pixel block
			const int block_size = 2;
			int row_rest = (row_end - row_start) % block_size;
			if (row_rest != 0) {
				if (row_end < this->height) {
					row_end += block_size - row_rest;
				}
				else if (row_start > 0) {
					row_start -= block_size - row_rest;;
				}
			}
			int col_rest = (col_end - col_start) % block_size;
			if (col_rest != 0) {
				if (col_end < this->width) {
					col_end += block_size - col_rest;;
				}
				else if (col_start > 0) {
					col_start -= block_size - col_rest;;
				}
			}
			row_start = CLAMP_INT(row_start, 0, this->height);
			row_end = CLAMP_INT(row_end, 0, this->height);
			col_start = CLAMP_INT(col_start, 0, this->width);
			col_end = CLAMP_INT(col_end, 0, this->width);

			int row_block_count = (row_end - row_start) / block_size;
			int col_block_count = (col_end - col_start) / block_size;

			bool flip = tri.flip;
			int ccw_idx0 = 0;
			int ccw_idx1 = flip ? 2 : 1;
			int ccw_idx2 = flip ? 1 : 2;
				
			auto v0 = tri[ccw_idx0].position.xy();
			auto v1 = tri[ccw_idx1].position.xy();
			auto v2 = tri[ccw_idx2].position.xy();

			float area = Triangle::area_double(v0, v1, v2);

			std::vector<Vertex> prev_row;
			std::vector<Vertex> prev_col;
			for (int block_row_start = row_start; block_row_start < row_end; block_row_start += block_size) {
				std::vector<Vertex> cur_row;
				cur_row.reserve(row_end - row_start);
				for (int block_col_start = col_start; block_col_start < col_end; block_col_start += block_size) {
					std::vector<Vertex> cur_col;
					// process pixel block
					int row0 = block_row_start;
					int row1 = row0 + 1;
					int col0 = block_col_start;
					int col1 = col0 + 1;

					Vertex up00, up01, left00, left10;

					if (prev_row.size() > 0) {
						up00 = prev_row[col0 - col_start];
						up01 = prev_row[col1 - col_start];
					}

					if (prev_col.size() > 0) {
						left00 = prev_col[0];
						left10 = prev_col[1];
					}

					auto v00 = process_pixel_block(
						tri, v0, v1, v2, area,
						ccw_idx0, ccw_idx1, ccw_idx2,
						shader,
						row0, col0,
						up00, left00);

					auto v01 = process_pixel_block(
						tri, v0, v1, v2, area,
						ccw_idx0, ccw_idx1, ccw_idx2,
						shader,
						row0, col1,
						up01, v00);

					auto v10 = process_pixel_block(
						tri, v0, v1, v2, area,
						ccw_idx0, ccw_idx1, ccw_idx2,
						shader,
						row1, col0,
						v00, left10);

					auto v11 = process_pixel_block(
						tri, v0, v1, v2, area,
						ccw_idx0, ccw_idx1, ccw_idx2,
						shader,
						row1, col1,
						v01, v10);

					cur_row.emplace_back(v10);
					cur_row.emplace_back(v11);
					cur_col.emplace_back(v01);
					cur_col.emplace_back(v11);
					prev_col = cur_col;
				}
				prev_row = cur_row;
			}
		}

		Vertex process_pixel_block(
			const Triangle& tri, 
			const Vector2& v0, const Vector2& v1, const Vector2& v2, const float& area, 
			const int& ccw_idx0, const int& ccw_idx1, const int& ccw_idx2,
			Shader* shader, 
			const int& row, const int& col,
			const Vertex& up, const Vertex& left) {
			Vector2 pixel((float)col + 0.5f, (float)row + 0.5f);
			float w0 = Triangle::area_double(v1, v2, pixel);
			float w1 = Triangle::area_double(v2, v0, pixel);
			float w2 = Triangle::area_double(v0, v1, pixel);
			if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
				w0 /= area; w1 /= area; w2 /= area;
				Vertex vert = Vertex::barycentric_interpolate(tri[ccw_idx0], tri[ccw_idx1], tri[ccw_idx2], w0, w1, w2);
				auto ddx = Vertex::differential(vert, left);
				auto ddy = Vertex::differential(vert, up);
				process_fragment(vert, row, col, ddx, ddy, shader);
				return vert;
			}
			return Vertex();
		}

		void scanline_rasterize(const Triangle& tri, Shader* shader) {
			bool flip = tri.flip;
			int top_idx = flip ? 2 : 0;
			int bottom_idx = flip ? 0 : 2;
			int top = (int)(tri[top_idx].position.y + 0.5f);
			int bottom = (int)(tri[bottom_idx].position.y + 0.5f);
			top = CLAMP_INT(top, 0, this->height);
			bottom = CLAMP_INT(bottom, 0, this->height);
			assert(bottom >= top);

			for (int row = top; row < bottom; row ++) {
				Vertex lhs, rhs;
				tri.interpolate((float)row + 0.5f, lhs, rhs);

				// screen space clipping
				bool enable_screen_clipping = (misc_param.culling_clipping_flag & CullingAndClippingFlag::SCREEN_CLIPPING) != CullingAndClippingFlag::DISABLE;
				if (enable_screen_clipping) {
					Clipper::screen_clipping(lhs, rhs, this->width);
				}
				int left = (int)(lhs.position.x + 0.5f);
				left = CLAMP_INT(left, 0, this->width);
				int right = (int)(rhs.position.x + 0.5f);
				right = CLAMP_INT(right, 0, this->width);
				assert(right >= left);

				for (auto col = left; col < right; col++) {
					process_fragment(lhs, row, col, Vertex(), Vertex(), shader);
					auto dx = Vertex::differential(lhs, rhs);
					lhs = Vertex::intagral(lhs, dx);
				}
			}
		}

		// per fragment processing
		void process_fragment(const Vertex& v, const uint32_t& row, const uint32_t& col, const Vertex& ddx, const Vertex& ddy, Shader*  shader) {
			bool enable_scissor_test = (misc_param.persample_op_flag & PerSampleOperation::SCISSOR_TEST) != PerSampleOperation::DISABLE;
			bool enable_alpha_test = (misc_param.persample_op_flag & PerSampleOperation::ALPHA_TEST) != PerSampleOperation::DISABLE;
			bool enable_stencil_test = (misc_param.persample_op_flag & PerSampleOperation::STENCIL_TEST) != PerSampleOperation::DISABLE;
			bool enable_depth_test = (misc_param.persample_op_flag & PerSampleOperation::DEPTH_TEST) != PerSampleOperation::DISABLE;

			PerSampleOperation op_pass = PerSampleOperation::SCISSOR_TEST | PerSampleOperation::ALPHA_TEST | PerSampleOperation::STENCIL_TEST | PerSampleOperation::DEPTH_TEST;

			auto s = shader;
			float z = v.position.z;

			ColorMask color_mask = s->color_mask;
			CompareFunc stencil_func = s->stencil_func;
			StencilOp stencil_pass_op = s->stencil_pass_op;
			StencilOp stencil_fail_op = s->stencil_fail_op;
			StencilOp stencil_zfail_op = s->stencil_zfail_op;
			uint8_t stencil_read_mask = s->stencil_read_mask;
			uint8_t stencil_write_mask = s->stencil_write_mask;
			uint8_t stencil_ref_val = s->stencil_ref_val;
			CompareFunc ztest_func = s->ztest_func;
			ZWrite zwrite_mode = s->zwrite_mode;
			BlendFactor src_factor = s->src_factor;
			BlendFactor dst_factor = s->dst_factor;
			BlendOp blend_op = s->blend_op;

			REF(stencil_write_mask);

			bool enable_blending = (misc_param.persample_op_flag & PerSampleOperation::BLENDING) != PerSampleOperation::DISABLE && s->transparent;

			// early-z
			// todo: early-z conditions
			if (enable_depth_test && !(enable_blending && s != nullptr && s->transparent)) {
				if (!perform_depth_test(ztest_func, row, col, z)) {
					statistics.earlyz_optimized++;
					return;
				}
			}

			color_bgra pixel_color;

			// fragment shader
			Color fragment_result;
			if ((misc_param.render_flag & RenderFlag::SHADED) != RenderFlag::DISABLE && s != nullptr) {
				v2f v_out;
				float w = 1.0f / v.rhw;
				v_out.position = v.position;
				v_out.world_pos = v.world_pos * w;
				v_out.shadow_coord = v.shadow_coord * w;
				v_out.color = v.color * w;
				v_out.normal = v.normal * w;
				v_out.uv = v.uv * w;
				v_out.tangent = v.tangent * w;
				v_out.bitangent = v.bitangent * w;
				fragment_result = s->fragment_shader(v_out, ddx, ddy);
				pixel_color = Color::encode_bgra(fragment_result);
			}

			// todo: scissor test
			if (enable_scissor_test)
			{

			}

			// acctually alpha test is a deprecated feature in newer graphics API, cuz it can be emulated by discard() or clip()
			if (enable_alpha_test)
			{
				if (s->discarded) {
					return;
				}
			}

			// todo: stencil test
			if (enable_stencil_test)
			{
				if (!perform_stencil_test(stencil_ref_val, stencil_read_mask, stencil_func, row, col)) {
					op_pass &= ~PerSampleOperation::STENCIL_TEST;
				}
			}

			// depth test
			if (enable_depth_test) {
				if (!perform_depth_test(ztest_func, row, col, z)) {
					op_pass &= ~PerSampleOperation::DEPTH_TEST;
				}
			}

			// blending
			if (enable_blending && s != nullptr && s->transparent) {
				color_bgra dst;
				if (framebuffer->read(row, col, dst)) {
					Color dst_color = Color::decode(dst);
					Color src_color = fragment_result;
					Color blended_color = blend(src_color, dst_color, src_factor, dst_factor, blend_op);
					pixel_color = Color::encode_bgra(blended_color.r, blended_color.g, blended_color.b, blended_color.a);
				}
			}

			// write color
			if (validate_fragment(op_pass)) {
				if (color_mask == (ColorMask::R | ColorMask::G | ColorMask::B | ColorMask::A)) {
					framebuffer->write(row, col, pixel_color);
				}
				else {
					color_bgra cur;
					if (framebuffer->read(row, col, cur)) {
						if ((color_mask & ColorMask::R) == ColorMask::ZERO) {
							pixel_color.r = cur.r;
						}
						if ((color_mask & ColorMask::G) == ColorMask::ZERO) {
							pixel_color.g = cur.g;
						}
						if ((color_mask & ColorMask::B) == ColorMask::ZERO) {
							pixel_color.b = cur.b;
						}
						if ((color_mask & ColorMask::A) == ColorMask::ZERO) {
							pixel_color.a = cur.a;
						}
						framebuffer->write(row, col, pixel_color);
					}
				}
			}

			// update stencilbuffer
			if (enable_stencil_test) {
				update_stencil_buffer(row, col, op_pass, stencil_pass_op, stencil_fail_op, stencil_zfail_op, stencil_ref_val);
			}

			// write depth
			if ((op_pass & PerSampleOperation::DEPTH_TEST) != PerSampleOperation::DISABLE) {
				if (zwrite_mode == ZWrite::ON) {
					if (color_mask == ColorMask::ZERO || stencil_pass_op == StencilOp::REPLACE) {
						std::cerr << "error " << std::endl;
					}
					zbuffer->write(row, col, z);
				}
			}

			// stencil visualization
			if ((misc_param.render_flag & RenderFlag::STENCIL) != RenderFlag::DISABLE) {
				uint8_t stencil;
				if (stencilbuffer->read(row, col, stencil)) {
					color_bgra c = Color::encode_bgra(stencil, stencil, stencil, 255);
					framebuffer->write(row, col, c);
				}
			}

			// depth buffer visualization
			if ((misc_param.render_flag & RenderFlag::DEPTH) != RenderFlag::DISABLE) {
				float cur_depth;
				if (zbuffer->read(row, col, cur_depth)) {
					float linear_depth = linearize_depth(cur_depth, misc_param.cam_near, misc_param.cam_far);
					Color depth_color = Color::WHITE * linear_depth / 30.0f;
					if (EQUALS(cur_depth, FAR_Z)) {
						depth_color = Color::WHITE;
					}
					color_bgra c = Color::encode_bgra(depth_color);
					framebuffer->write(row, col, c);
				}
			}
		}

		bool validate_fragment(const PerSampleOperation& op_pass) const {
			if ((op_pass & PerSampleOperation::SCISSOR_TEST) == PerSampleOperation::DISABLE) return false;
			if ((op_pass & PerSampleOperation::ALPHA_TEST) == PerSampleOperation::DISABLE) return false;
			if ((op_pass & PerSampleOperation::STENCIL_TEST) == PerSampleOperation::DISABLE) return false;
			if ((op_pass & PerSampleOperation::DEPTH_TEST) == PerSampleOperation::DISABLE) return false;
			return true;
		}

		bool perform_stencil_test(const uint8_t& ref_val, const uint8_t& read_mask, const CompareFunc& func, const uint32_t& row, const uint32_t& col) const {
			bool pass = false;
			uint8_t stencil;
			if (stencilbuffer->read(row, col, stencil)) {
				switch (func) {
				case CompareFunc::NEVER:
					pass = false;
					break;
				case CompareFunc::ALWAYS:
					pass = true;
					break;
				case CompareFunc::EQUAL:
					pass = (ref_val & read_mask) == (stencil & read_mask);
					break;
				case CompareFunc::GREATER:
					pass = (ref_val & read_mask) > (stencil & read_mask);
					break;
				case CompareFunc::LEQUAL:
					pass = (ref_val & read_mask) <= (stencil & read_mask);
					break;
				case CompareFunc::NOT_EQUAL:
					pass = (ref_val & read_mask) != (stencil & read_mask);
					break;
				case CompareFunc::GEQUAL:
					pass = (ref_val & read_mask) > (stencil & read_mask);
					break;
				case CompareFunc::LESS:
					pass = (ref_val & read_mask) < (stencil & read_mask);
					break;
				}
			}
			return pass;
		}

		void update_stencil_buffer(const uint32_t& row, const uint32_t& col, const PerSampleOperation& op_pass, const StencilOp& stencil_pass_op, const StencilOp& stencil_fail_op, const StencilOp& stencil_zfail_op, const uint8_t& ref_val) const {
			bool stencil_pass = (op_pass & PerSampleOperation::STENCIL_TEST) != PerSampleOperation::DISABLE;
			bool z_pass = (op_pass & PerSampleOperation::DEPTH_TEST) != PerSampleOperation::DISABLE;
			uint8_t stencil;
			stencilbuffer->read(row, col, stencil);
			StencilOp stencil_op;
			if (stencil_pass) {
				stencil_op = z_pass ? stencil_pass_op : stencil_zfail_op;
			}
			else {
				stencil_op = stencil_fail_op;
			}
			switch (stencil_op) {
			case StencilOp::KEEP:
				break;
			case StencilOp::ZERO:
				this->stencilbuffer->write(row, col, 0);
				break;
			case StencilOp::REPLACE:
				this->stencilbuffer->write(row, col, ref_val);
				break;
			case StencilOp::INCR:
				this->stencilbuffer->write(row, col, CLAMP((int)stencil + 1, 0, 255));
				break;
			case StencilOp::DECR:
				this->stencilbuffer->write(row, col, CLAMP((int)stencil - 1, 0, 255));
				break;
			case StencilOp::INCR_WRAP:
				this->stencilbuffer->write(row, col, stencil + 1);
				break;
			case StencilOp::DECR_WRAP:
				this->stencilbuffer->write(row, col, stencil - 1);
				break;
			case StencilOp::INVERT:
				this->stencilbuffer->write(row, col, ~stencil);
				break;
			}
		}

		bool perform_depth_test(const CompareFunc& func, const uint32_t& row, const uint32_t& col, const float& z) const {
			float depth;
			bool pass = false;
			if (zbuffer->read(row, col, depth)) {
				pass = z <= depth;
				switch (func) {
				case CompareFunc::NEVER:
					pass = false;
					break;
				case CompareFunc::ALWAYS:
					pass = true;
					break;
				case CompareFunc::EQUAL:
					pass = EQUALS(z, depth); // percision concern
					break;
				case CompareFunc::GREATER:
					pass = z > depth;
					break;
				case CompareFunc::LEQUAL:
					pass = z <= depth;
					break;
				case CompareFunc::NOT_EQUAL:
					pass = z != depth;
					break;
				case CompareFunc::GEQUAL:
					pass = z >= depth;
					break;
				case CompareFunc::LESS:
					pass = z < depth;
					break;
				}
			}
			return pass;
		}

		// todo: alpha factor
		static Color blend(const Color& src_color, const Color& dst_color, const BlendFactor& src_factor, const BlendFactor& dst_factor, const BlendOp& op) {
			Color lhs, rhs;
			switch (src_factor) {
			case BlendFactor::ONE:
				lhs = src_color;
				break;
			case BlendFactor::SRC_ALPHA:
				lhs = src_color * src_color.a;
				break;
			case BlendFactor::SRC_COLOR:
				lhs = src_color * src_color;
				break;
			case BlendFactor::ONE_MINUS_SRC_ALPHA:
				lhs = src_color * (1.0f - src_color);
				break;
			case BlendFactor::ONE_MINUS_SRC_COLOR:
				lhs = src_color * (1.0f - dst_color);
				break;
			case BlendFactor::DST_ALPHA:
				lhs = src_color * dst_color.a;
				break;
			case BlendFactor::DST_COLOR:
				lhs = src_color * dst_color;
				break;
			case BlendFactor::ONE_MINUS_DST_ALPHA:
				lhs = src_color * (1.0f - dst_color.a);
				break;
			case BlendFactor::ONE_MINUS_DST_COLOR:
				lhs = src_color * (1.0f - dst_color);
				break;
			}

			switch (dst_factor) {
			case BlendFactor::ONE:
				rhs = src_color;
				break;
			case BlendFactor::SRC_ALPHA:
				rhs = dst_color * src_color.a;
				break;
			case BlendFactor::SRC_COLOR:
				rhs = dst_color * src_color;
				break;
			case BlendFactor::ONE_MINUS_SRC_ALPHA:
				rhs = dst_color * (1.0f - src_color);
				break;
			case BlendFactor::ONE_MINUS_SRC_COLOR:
				rhs = dst_color * (1.0f - dst_color);
				break;
			case BlendFactor::DST_ALPHA:
				rhs = dst_color * dst_color.a;
				break;
			case BlendFactor::DST_COLOR:
				rhs = dst_color * dst_color;
				break;
			case BlendFactor::ONE_MINUS_DST_ALPHA:
				rhs = dst_color * (1.0f - dst_color.a);
				break;
			case BlendFactor::ONE_MINUS_DST_COLOR:
				rhs = dst_color * (1.0f - dst_color);
				break;
			}

			switch (op) {
			case BlendOp::ADD:
				return lhs + rhs;
			case BlendOp::SUB:
				return lhs - rhs;
			}
			return lhs + rhs;
		}

		Vertex clip2ndc(const Vertex& v) const {
			Vertex ret = v;
			ret.position = clip2ndc(v.position);
			float w = ret.position.w;
			if (w == 0.0f) {
				w = EPSILON;
			}
			ret.rhw = 1.0f / w;
			return ret;
		}

		Vector4 clip2ndc(const Vector4& v) const {
			Vector4 ndc_pos = v;
			float w = v.w;
			if (w == 0.0f) {
				w += EPSILON;
			}
			float rhw = 1.0f / w;
			ndc_pos *= rhw;
			ndc_pos.w = 1.0f;
			return ndc_pos;
		}

		Vertex ndc2viewport(const Vertex& v) const {
			Vertex ret = v;
			ret.position = ndc2viewport(v.position);
			ret.rhw = 1.0f;
			return ret;
		}

		Vector4 ndc2viewport(const Vector4& v) const {
			Vector4 viewport_pos;
			viewport_pos.x = (v.x + 1.0f) * this->width * 0.5f;
			viewport_pos.y = (v.y + 1.0f) * this->height * 0.5f;
			viewport_pos.z = v.z;
			viewport_pos.w = 1.0f;
			return viewport_pos;
		}

		float linearize_depth(const float& depth, const float& near, const float& far) const
		{
			float ndc_z = depth * 2.0f - 1.0f;  // [0, 1] -> [-1, 1] (GL)
			return (2.0f * near * far) / (far + near - ndc_z * (far - near));
		}

	public:
		void draw_segment(const Vector3& start, const Vector3& end, const Color& col, const Matrix4x4& v, const Matrix4x4& p, const Vector2& screen_translation) {
			Vector4 clip_start = p * v * Vector4(start);
			Vector4 clip_end = p * v * Vector4(end);

			Vector4 n1 = clip2ndc(clip_start);
			Vector4 n2 = clip2ndc(clip_end);

			Vector4 s1 = ndc2viewport(n1);
			Vector4 s2 = ndc2viewport(n2);

			Matrix4x4 translation = Matrix4x4::translation(Vector3(screen_translation));

			s1 = translation * s1;
			s2 = translation * s2;

			SegmentDrawer::bresenham(framebuffer, (int)s1.x, (int)s1.y, (int)s2.x, (int)s2.y, Color::encode_bgra(col));
		}

		void draw_screen_segment(const Vector4& start, const Vector4& end, const Color& col) {
			SegmentDrawer::bresenham(framebuffer, (int)start.x, (int)start.y, (int)end.x, (int)end.y, Color::encode_bgra(col));
		}

		void draw_segment(const Vector3& start, const Vector3& end, const Color& col, const Matrix4x4& m, const Matrix4x4& v, const Matrix4x4& p) {
			Vector4 clip_start = p * v * m * Vector4(start);
			Vector4 clip_end = p * v * m * Vector4(end);
			draw_segment(clip_start, clip_end, col);
		}

		void draw_segment(const Vector3& start, const Vector3& end, const Color& col, const Matrix4x4& v, const Matrix4x4& p) {
			Vector4 clip_start = p * v * Vector4(start);
			Vector4 clip_end = p * v * Vector4(end);
			draw_segment(clip_start, clip_end, col);
		}

		void draw_segment(const Vector4& clip_start, const Vector4& clip_end, const Color& col) {
			Vector4 n1 = clip2ndc(clip_start);
			Vector4 n2 = clip2ndc(clip_end);

			Vector4 s1 = ndc2viewport(n1);
			Vector4 s2 = ndc2viewport(n2);

			SegmentDrawer::bresenham(framebuffer, (int)s1.x, (int)s1.y, (int)s2.x, (int)s2.y, Color::encode_bgra(col));
		}

		void draw_coordinates(const Vector3& pos, const Vector3& forward, const Vector3& up, const Vector3& right, const Matrix4x4& v, const Matrix4x4& p, const Vector2& offset) {
			Graphics().draw_segment(pos, pos + forward, Color::BLUE, v, p, offset);
			Graphics().draw_segment(pos, pos + right, Color::RED, v, p, offset);
			Graphics().draw_segment(pos, pos + up, Color::GREEN, v, p, offset);
		}

		void draw_coordinates(const Vector3& pos, const Vector3& forward, const Vector3& up, const Vector3& right, const Matrix4x4& v, const Matrix4x4& p) {
			Graphics().draw_segment(pos, pos + forward, Color::BLUE, v, p);
			Graphics().draw_segment(pos, pos + right, Color::RED, v, p);
			Graphics().draw_segment(pos, pos + up, Color::GREEN, v, p);
		}

		void draw_coordinates(const Vector3& pos, const Vector3& forward, const Vector3& up, const Vector3& right, const Matrix4x4& m, const Matrix4x4& v, const Matrix4x4& p) {
			Graphics().draw_segment(pos, pos + forward, Color::BLUE, m, v, p);
			Graphics().draw_segment(pos, pos + right, Color::RED, m, v, p);
			Graphics().draw_segment(pos, pos + up, Color::GREEN, m, v, p);
		}
	};
}
#endif