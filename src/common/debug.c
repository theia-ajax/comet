#include "debug.h"

#include "game.h"
#include "util.h"
#include <SDL2/SDL.h>

static struct {
	struct line {
		SDL_Point points[16];
		int32 point_count;
		int32 frames_remaining;
		SDL_Color color;
	};
	struct line lines_ring_buffer[1024 * 4];
	int32 lines_ring_index;
} g_debug;

void debug_next_frame()
{
	for (int32 index= 0; index < ARRAY_COUNT(g_debug.lines_ring_buffer); index++) {
		if (g_debug.lines_ring_buffer[index].frames_remaining > 0) {
			g_debug.lines_ring_buffer[index].frames_remaining--;
		}
	}
}

void debug_draw(SDL_Renderer* renderer)
{
	for (int32 raw_index= 0; raw_index < ARRAY_COUNT(g_debug.lines_ring_buffer); raw_index++) {
		int32 index= (g_debug.lines_ring_index + ARRAY_COUNT(g_debug.lines_ring_buffer))
					 % ARRAY_COUNT(g_debug.lines_ring_buffer);
		index= raw_index;
		if (g_debug.lines_ring_buffer[index].frames_remaining != 0) {
			SDL_Color color= g_debug.lines_ring_buffer[index].color;
			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
			SDL_RenderDrawLines(
				renderer, g_debug.lines_ring_buffer[index].points, g_debug.lines_ring_buffer[index].point_count);
		}
	}
}

static int32 next_index()
{
	int32 index= g_debug.lines_ring_index;
	g_debug.lines_ring_index++;
	if (g_debug.lines_ring_index >= ARRAY_COUNT(g_debug.lines_ring_buffer)) g_debug.lines_ring_index= 0;
	return index;
}

void debug_line(float x0, float y0, float x1, float y1, Color color, int frames)
{
#if 0
#define line(index) g_debug.lines_ring_buffer[index]
#define line_point(index, point) line(index).points[point]
	int32 index= next_index();
	line(index).frames_remaining= frames;
	line(index).point_count= 2;
	line(index).color= *((SDL_Color*)&color);
	world_fpoint_to_screen_point(x0, y0, &line_point(index, 0).x, &line_point(index, 0).y);
	world_fpoint_to_screen_point(x1, y1, &line_point(index, 1).x, &line_point(index, 1).y);
#undef line_point
#undef line
#endif
}

void debug_box(float x0, float y0, float x1, float y1, Color color, int frames)
{
#if 0
#define line(index) g_debug.lines_ring_buffer[index]
#define line_point(index, point) line(index).points[point]
	int32 index= next_index();
	line(index).frames_remaining= frames;
	line(index).point_count= 5;
	line(index).color= *((SDL_Color*)&color);

	world_fpoint_to_screen_point(x0, y0, &line_point(index, 0).x, &line_point(index, 0).y);
	world_fpoint_to_screen_point(x1, y0, &line_point(index, 1).x, &line_point(index, 1).y);
	world_fpoint_to_screen_point(x1, y1, &line_point(index, 2).x, &line_point(index, 2).y);
	world_fpoint_to_screen_point(x0, y1, &line_point(index, 3).x, &line_point(index, 3).y);
	world_fpoint_to_screen_point(x0, y0, &line_point(index, 4).x, &line_point(index, 4).y);
#undef line_point
#undef line
#endif
}
