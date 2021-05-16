#ifndef _FRAME_TILE_HPP_
#define _FRAME_TILE_HPP_
#include "SafeQueue.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"

namespace Guarneri
{
	typedef struct
	{
		uint32_t tile_size;
		uint32_t tile_task_size;
		uint32_t row_tile_count;
		uint32_t col_tile_count;
		uint32_t tile_length;
	} TileInfo;

	struct TileTask
	{
	public:
		Triangle triangle;
		Shader* shader;

	public:
		TileTask();
		TileTask(const Triangle& triangle, Shader* shader);
	};

	struct FrameTile
	{
	public:
		uint32_t tile_idx;
		int row_start;
		int row_end;
		int col_start;
		int col_end;
		SafeQueue<TileTask> tasks;

	public:
		FrameTile();
		~FrameTile();
		void push_task(const Triangle& tri, Shader* shader);
		bool pop_task(TileTask& task);
		bool is_task_empty();
		void clear();
		size_t task_size();
		static int coord2index(const int& row, const int& col, const int& col_tile_count);
		static void pixel2tile(const int& prow, const int& pcol, int& trow, int& tcol, const int& tile_size);
		static void build_tiles(
			FrameTile* tiles,
			const int& tile_size,
			const int& row_tile_count, const int& col_tile_count,
			const int& row_rest, const int& col_rest);
		static void dispatch_render_task(
			FrameTile* tiles,
			const Triangle& tri,
			Shader* shader,
			const int& w, const int& h,
			const int& tile_size,
			const int& col_tile_count);
	};
}
#endif