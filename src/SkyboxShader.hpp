#ifndef _SKYBOX_SHADER_
#define _SKYBOX_SHADER_
#include <Guarneri.hpp>

namespace Guarneri {
	class SkyboxShader : public Shader {
	public:
		SkyboxShader() {}
		~SkyboxShader(){}

	private:
		std::shared_ptr<CubeMap> cube_map;

	public:
		void sync(const std::shared_ptr<CubeMap>& cube_map) {
			this->cube_map = cube_map;
		}

		v2f vertex_shader(const a2v& input) {
			v2f o;
			auto clip_pos = p * v * input.position;
			o.position = Vector4(clip_pos.xy(), clip_pos.ww());
			o.shadow_coord = input.position;
			return o;
		}

		Color fragment_shader(const v2f& input)  {
			Color sky_color;
			return Color::BLUE;
			if (cube_map != nullptr && cube_map->sample(input.shadow_coord.xyz(), sky_color)) {
				return sky_color;
			}
			return Color::BLACK;
		}

		std::string str() const {
			std::stringstream ss;
			ss << "SkyboxShader[" << this->id << "]" << std::endl;
			return ss.str();
		}
	};
}
#endif