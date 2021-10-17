#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Define.hpp"
#include "tinymath/color/Color.h"
#include "RawBuffer.hpp"

namespace CpuRasterizor
{
	class Texture 
	{
	public:
		WrapMode wrap_mode;
		Filtering filtering;
		TextureFormat format;
		std::string raw_path;
		std::string meta_path;
		size_t width;
		size_t height;
		size_t mip_count;
		Filtering mip_filtering;
		bool enable_mip;

	private:
		std::shared_ptr<RawBuffer<tinymath::color_rgb16f>> rgb16f_buffer;
		std::shared_ptr<RawBuffer<tinymath::color_rgba16f>> rgba16f_buffer;
		std::shared_ptr<RawBuffer<tinymath::color_rgb>> rgb_buffer;
		std::shared_ptr<RawBuffer<tinymath::color_rgba>> rgba_buffer;
		std::shared_ptr<RawBuffer<tinymath::color_gray>> gray_buffer;
		std::shared_ptr<RawBuffer<tinymath::color_rg>> rg_buffer;
		std::vector< std::shared_ptr<RawBuffer<tinymath::color_gray>>> gray_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<tinymath::color_rgb>>> rgb_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<tinymath::color_rgba>>> rgba_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<tinymath::color_rg>>> rg_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<tinymath::color_rgb16f>>> rgb16f_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<tinymath::color_rgba16f>>> rgba16f_mipmaps;

	public:
		Texture(size_t _width, size_t _height, TextureFormat _fmt);
		Texture(void* tex_buffer, size_t _width, size_t _height, TextureFormat _fmt);
		Texture(const Texture& other);
		~Texture();

		static std::shared_ptr<Texture> load_asset(const std::string& path);
		static std::shared_ptr<Texture> load_asset(const char* path);
		static std::shared_ptr<Texture> load_raw(const std::string& path);
		static std::shared_ptr<Texture> load_raw(const char* path);

		void reload(const char* texture_path);
		void generate_mipmap(int mip_count, Filtering filtering);
		bool sample(float u, float v, tinymath::Color& ret) const;
		bool sample(float u, float v, const tinymath::vec2f ddx, const tinymath::vec2f ddy, tinymath::Color& ret) const;
		bool sample(float u, float v, float mip, tinymath::Color& ret) const;
		bool read(float u, float v, size_t mip, tinymath::Color& ret) const;
		bool read(size_t row, size_t col, size_t mip, tinymath::Color& ret) const;
		bool read(float u, float v, tinymath::Color& ret) const;
		bool read(size_t row, size_t col, tinymath::Color& ret) const;
		bool write(size_t x, size_t y, const tinymath::Color& data);
		bool write(float u, float v, const tinymath::Color& data);
		void resize(size_t w, size_t h);
		void release();
		void copy(const Texture& other);
		static void export_image(const Texture& tex, const std::string& path);

	private:
		bool bilinear(float u, float v, size_t mip, tinymath::Color& ret) const;
		void wrap(float& u, float& v) const;
		void clear();
		Texture& operator =(const Texture& other);
		Texture();
	};
}