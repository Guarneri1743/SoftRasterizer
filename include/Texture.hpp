#ifndef _TEXTURE_
#define _TEXTURE_
#include <stdint.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Define.hpp"
#include "Object.hpp"
#include "Color.hpp"
#include "RawBuffer.hpp"

namespace Guarneri
{
	enum class WrapMode
	{
		REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class Filtering
	{
		POINT,
		BILINEAR,
		MAX,
		MIN
	};

	enum class TextureFormat
	{
		INVALID,
		rgb,
		rgba,
		rg,
		r32
	};

	class Texture : public Object
	{
	public:
		WrapMode wrap_mode;
		Filtering filtering;
		TextureFormat fmt;
		std::string path;
		uint32_t width;
		uint32_t height;
		uint32_t mip_count;
		Filtering mip_filtering;

	private:
		static std::unordered_map<uint32_t, std::shared_ptr<Texture>> texture_cache;
		std::shared_ptr<RawBuffer<color_rgb>> rgb_buffer;
		std::shared_ptr<RawBuffer<color_rgba>> rgba_buffer;
		std::shared_ptr<RawBuffer<color_gray>> gray_buffer;
		std::shared_ptr<RawBuffer<color_rg>> rg_buffer;
		std::vector< std::shared_ptr<RawBuffer<color_gray>>> gray_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<color_rgb>>> rgb_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<color_rgba>>> rgba_mipmaps;
		std::vector< std::shared_ptr<RawBuffer<color_rg>>> rg_mipmaps;

	public:
		Texture(const uint32_t& _width, const uint32_t& _height, const TextureFormat& _fmt);
		Texture(void* tex_buffer, const uint32_t& _width, const uint32_t& _height, const TextureFormat& _fmt);
		Texture(const char* path);
		Texture(const Texture& other);
		~Texture();

		static std::shared_ptr<Texture> create(const uint32_t& width, const uint32_t& height, const TextureFormat& fmt);
		static std::shared_ptr<Texture> create(void* tex_buffer, const uint32_t& width, const uint32_t& height, const TextureFormat& fmt);
		static std::shared_ptr<Texture> create(const Texture& other);
		static std::shared_ptr<Texture> create(const std::string& path);
		bool bilinear(const float& u, const float& v, Color& ret) const;
		bool point(const float& u, const float& v, Color& ret) const;
		void generate_mipmap(const int& mip_count, const Filtering& filtering);
		bool sample(const float& u, const float& v, Color& ret) const;
		bool read(const float& u, const float& v, Color& ret) const;
		bool read(const uint32_t& row, const uint32_t& col, Color& ret) const;
		bool write(const uint32_t& x, const uint32_t& y, const Color& data);
		void save2file();
		void resize();
		void release();
		std::string str() const;

	private:
		//todo:
		void wrap(float& u, float& v) const;
		void clear();
		Texture& operator =(const Texture& other);
		void copy(const Texture& other);
	};
}
#endif