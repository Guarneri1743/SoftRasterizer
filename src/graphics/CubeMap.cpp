#include "CubeMap.hpp"
#include <assert.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <execution>
#include "Define.hpp"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/filereadstream.h"
#include "Utility.hpp"
#include "Logger.hpp"
#include "Cache.hpp"
#include "stb_image.h"
#include "Sampling.hpp"

namespace Guarneri
{
	CubeMap::CubeMap()
	{
		this->wrap_mode = WrapMode::REPEAT;
		this->filtering = Filtering::POINT;
	}

	CubeMap::CubeMap(const char* path)
	{
		std::shared_ptr<CubeMap> map;
		spawn(path, map);
		this->copy_from(*map);
	}

	void CubeMap::reload(const std::string& path)
	{
		texture_path = path;
		texture = Texture::load_raw(path);
		texture->wrap_mode = this->wrap_mode;
		texture->filtering = this->filtering;
		precompute_ibl_textures();
	}

	std::shared_ptr<CubeMap> CubeMap::load_asset(std::string path)
	{
		std::shared_ptr<CubeMap> map;
		spawn(path, map);
		return map;
	}	

	void CubeMap::copy_from(const CubeMap& cubemap)
	{
		this->name = cubemap.name;
		this->meta_path = cubemap.meta_path;
		this->texture_path = cubemap.texture_path;
		this->texture = cubemap.texture;
		this->filtering = cubemap.filtering;
		this->wrap_mode = cubemap.wrap_mode;
		this->irradiance_map = cubemap.irradiance_map;
		this->prefilter_map = cubemap.prefilter_map;
		this->brdf_lut = cubemap.brdf_lut;
	}

	std::shared_ptr<CubeMap> CubeMap::load_asset(const char* path)
	{
		std::shared_ptr<CubeMap> map;
		spawn(path, map);
		return map;
	}
	 
	bool CubeMap::sample(const Vector3& dir, Color& ret)
	{
		auto uv = spherical_coord_to_uv(dir);
		return texture->sample(uv.x, uv.y, ret);
	}

	bool CubeMap::sample_irradiance_map(const Vector3& dir, Color& ret)
	{
		auto uv = spherical_coord_to_uv(dir);
		return irradiance_map->sample(uv.x, uv.y, ret);
	}

	bool CubeMap::sample_prefilter_map(const Vector3& dir, Color& ret)
	{
		return false;
	}

	bool CubeMap::sample_brdf(const Vector3& dir, Color& ret)
	{
		return false;
	}

	void CubeMap::precompute_ibl_textures()
	{
		if (texture != nullptr)
		{
			precompute_irradiance_map();
			// todo: mipmap
			const float kMaxMip = 5.0f;
			float mip = 3.2f;
			float roughness = mip / (float)(kMaxMip - 1.0f);
			precompute_prefilter_map(roughness);
			precompute_brdf_lut();
		}
		else
		{
			LOG("no original texture");
		}
	}

	constexpr float kSampleStep = 0.5f;

	void CubeMap::precompute_irradiance_map()
	{
		size_t begin = texture_path.find(".hdr");
		std::string irradiance_path = texture_path;
		irradiance_path = irradiance_path.replace(begin, begin + 4, "_irradiance.hdr");

		std::filesystem::path abs_path(RES_PATH + irradiance_path);
		if (std::filesystem::exists(abs_path))
		{
			irradiance_map = Texture::load_raw(irradiance_path);
			return;
		}

		irradiance_map = std::make_shared<Texture>(texture->width, texture->height, TextureFormat::rgb16f);
		irradiance_map->filtering = Filtering::POINT;

		std::vector<Vector2> indexers;
		indexers.reserve((size_t)texture->width * (size_t)texture->height);
		for (uint32_t row = 0; row < texture->height; row++)
		{
			for (uint32_t col = 0; col < texture->width; col++)
			{
				indexers.push_back(Vector2((float)row, (float)col));
			}
		}

		std::for_each(
			std::execution::par_unseq,
			indexers.begin(),
			indexers.end(),
			[this](auto&& coord)
		{
			uint32_t row = (uint32_t)coord.x;
			uint32_t col = (uint32_t)coord.y;
			float u = (float)col / (float)texture->width;
			float v = (float)row / (float)texture->height;
			Vector2 uv = { u, v };
			Vector3 normal = uv_to_spherical_coord(uv);

			Color irradiance = Color(0.0f);
			float step = kSampleStep;
			float samples = 0.0f;
			for (float phi = 0.0f; phi < TWO_PI; phi += step)
			{
				for (float theta = 0.0f; theta < PI; theta += step)
				{
					Vector3 light_dir = radian_to_spherical_coord(theta, phi);
					float ndl = std::max(0.0f, Vector3::dot(normal, light_dir));
					if (ndl > 0.0f)
					{
						Color color;
						sample(light_dir, color);
						irradiance += color * ndl;
					}
					samples++;
				}
			}

			irradiance = irradiance * PI / samples;
			irradiance_map->write(u, 1.0f - v, irradiance);
		});

		Texture::export_image(*irradiance_map, irradiance_path);
	}

	constexpr uint32_t kSampleCount = 32u;

	void CubeMap::precompute_prefilter_map(const float& roughness)
	{
		size_t begin = texture_path.find(".hdr");
		std::string prefilter_map_path = texture_path;
		prefilter_map_path = prefilter_map_path.replace(begin, begin + 4, "_prefilter_" + std::to_string((int)roughness) +".hdr");

		if (std::filesystem::exists(RES_PATH + prefilter_map_path))
		{
			prefilter_map = Texture::load_raw(prefilter_map_path);
			return;
		}

		prefilter_map = std::make_shared<Texture>(texture->width, texture->height, TextureFormat::rgb16f);
		prefilter_map->filtering = Filtering::POINT;

		std::vector<Vector2> indexers;
		indexers.reserve((size_t)texture->width * (size_t)texture->height);
		for (uint32_t row = 0; row < texture->height; row++)
		{
			for (uint32_t col = 0; col < texture->width; col++)
			{
				indexers.push_back(Vector2((float)row, (float)col));
			}
		}

		float r = roughness;
		std::for_each(
			std::execution::par_unseq,
			indexers.begin(),
			indexers.end(),
			[this, r](auto&& coord)
		{
			uint32_t row = (uint32_t)coord.x;
			uint32_t col = (uint32_t)coord.y;
			float u = (float)col / (float)texture->width;
			float v = (float)row / (float)texture->height;
			Vector2 uv = { u, v };
			Vector3 normal = uv_to_spherical_coord(uv);

			// Epic Approximation
			Vector3 reflect_dir = normal;
			Vector3 view_dir = reflect_dir;

			float total_weight = 0.0f;
			Color prefilter_color = Color(0.0f);

			for (uint32_t sample_index = 0u; sample_index < kSampleCount; sample_index++)
			{
				Vector2 random_dir = Vector2((float)(rand() % 10000) / 10000.0f, (float)(rand() % 10000) / 10000.0f);//hammersley(sample_index, kSampleCount);
				Vector3 half_way = importance_sample_ggx(random_dir, normal, r);
				float vdh = std::max(Vector3::dot(view_dir, half_way), 0.0f);
				Vector3 light_dir = Vector3::normalize(2.0f * vdh * half_way - view_dir);
				float ndl = std::max(Vector3::dot(normal, light_dir), 0.0f);
				if (ndl > 0.0f)
				{
					Color color;
					sample(light_dir, color);
					prefilter_color += color * ndl;
					total_weight += ndl;
				}
			}

			prefilter_color /= total_weight;
			prefilter_map->write(u, 1.0f - v, prefilter_color);
		});

		Texture::export_image(*prefilter_map, prefilter_map_path);
	}

	void CubeMap::precompute_brdf_lut()
	{
		brdf_lut = std::make_shared<Texture>(texture->width, texture->height, TextureFormat::rgb16f);
	
	}

	//Vector2 CubeMap::sample(const Vector3& dir, int& index)
	//{
	//	Vector3 abs_dir = Vector3::abs(dir);
	//	float max_axis = std::max(std::max(abs_dir.x, abs_dir.y), abs_dir.z);
	//	if (abs_dir.x > abs_dir.y && abs_dir.x > abs_dir.z)
	//	{
	//		// x is the max axis 
	//		if (dir.x > 0)
	//		{
	//			index = 0;
	//			return Vector2(1.0f - (dir.z + 1.0f) / 2.0f, (dir.y + 1.0f) / 2.0f);
	//		}
	//		else 
	//		{
	//			index = 1;
	//			return Vector2((dir.z + 1.0f) / 2.0f, (dir.y + 1.0f) / 2.0f);
	//		}
	//	}
	//	else if (abs_dir.y > abs_dir.x && abs_dir.y > abs_dir.z)
	//	{
	//		// y is the max axis
	//		if (dir.y > 0)
	//		{
	//			index = 2;
	//			return Vector2((dir.x + 1.0f) / 2.0f, 1.0f - (dir.z + 1.0f) / 2.0f);
	//		}
	//		else 
	//		{
	//			index = 3;
	//			return Vector2((dir.x + 1.0f) / 2.0f, (dir.z + 1.0f) / 2.0f);
	//		}
	//	}
	//	else 
	//	{
	//		// z is the max axis
	//		if (dir.z > 0)
	//		{
	//			index = 4;
	//			return Vector2((dir.x + 1.0f) / 2.0f, (dir.y + 1.0f) / 2.0f);
	//		}
	//		else 
	//		{
	//			index = 5;
	//			return Vector2(1.0f - (dir.x + 1.0f) / 2.0f, (dir.y + 1.0f) / 2.0f);
	//		}
	//	}
	//}

	void CubeMap::serialize(const CubeMap& cube_map, std::string path)
	{
		rapidjson::Document doc;
		doc.SetObject();

		rapidjson::Value name;
		name.SetString(cube_map.name.c_str(), doc.GetAllocator());
		doc.AddMember("name", name, doc.GetAllocator());

		rapidjson::Value cube_map_path;
		cube_map_path.SetString(path.c_str(), doc.GetAllocator());
		doc.AddMember("meta_path", cube_map_path, doc.GetAllocator());

		rapidjson::Value right;
		right.SetString(cube_map.texture_path.c_str(), doc.GetAllocator());
		doc.AddMember("hdr_texture", right, doc.GetAllocator());

		doc.AddMember("wrap_mode", (int32_t)cube_map.wrap_mode, doc.GetAllocator());

		doc.AddMember("filtering", (int32_t)cube_map.filtering, doc.GetAllocator());

		std::filesystem::path abs_path(ASSETS_PATH + path);
		if (!std::filesystem::exists(abs_path.parent_path()))
		{
			std::filesystem::create_directories(abs_path.parent_path());
		}
		std::FILE* fd = fopen(abs_path.string().c_str(), "w+");
		if (fd != nullptr)
		{
			char write_buffer[256];
			rapidjson::FileWriteStream fs(fd, write_buffer, sizeof(write_buffer));
			rapidjson::PrettyWriter<rapidjson::FileWriteStream> material_writer(fs);
			doc.Accept(material_writer);
			fclose(fd);
			LOG("save cubemap: {}", path.c_str());
		}
		else
		{
			ERROR("path does not exist: {}", (ASSETS_PATH + path).c_str());
		}
	}

	void CubeMap::spawn(std::string path, std::shared_ptr<CubeMap>& cubemap)
	{
		cubemap = std::shared_ptr<CubeMap>(new CubeMap());
		CubeMap::deserialize(path, *cubemap);
	}


	void CubeMap::deserialize(std::string path, CubeMap& cubemap)
	{
		std::FILE* fd = fopen((ASSETS_PATH + path).c_str(), "r");
		if (fd != nullptr)
		{
			LOG("deserialize: {}", (ASSETS_PATH + path).c_str());
			char read_buffer[256];
			rapidjson::FileReadStream fs(fd, read_buffer, sizeof(read_buffer));
			rapidjson::Document doc;
			doc.ParseStream(fs);
			const char* texture_path = doc["texture_path"].GetString();
			fclose(fd);
			cubemap.name = doc["name"].GetString();
			cubemap.meta_path = doc["meta_path"].GetString();
			cubemap.wrap_mode = (WrapMode)doc["wrap_mode"].GetInt();
			cubemap.filtering = (Filtering)doc["filtering"].GetInt();
			cubemap.reload(texture_path);
		}
		else
		{
			ERROR("path does not exist: {}", ASSETS_PATH + path);
		}
	}
}