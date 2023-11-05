#pragma once

// #include "config.h"
// #include "Types.h"
// #include "util.h"
// #include <SDL3/SDL_rect.h>

// typedef struct SDL_Surface SDL_Surface;
// typedef struct SDL_Texture SDL_Texture;
// typedef struct SDL_Renderer SDL_Renderer;

// enum {
// 	MAP_MAX_TILE_X= 28,
// 	MAP_MAX_TILE_Y= 31,
// 	SCREEN_PIXEL_WIDTH= 224,
// 	SCREEN_PIXEL_HEIGHT= 288,
// };

// enum {
// 	_map_tile_none= 0,
// 	_map_tile_solid,
// 	_map_tile_monster,
// 	_map_tile_dot,
// 	_map_tile_big_dot,
// 	_map_tile_hidden_solid,

// 	_map_tile_wall_north= 16,
// 	_map_tile_wall_east,
// 	_map_tile_wall_south,
// 	_map_tile_wall_west,

// 	_map_tile_outer_corner_north_east= 32,
// 	_map_tile_outer_corner_south_east,
// 	_map_tile_outer_corner_south_west,
// 	_map_tile_outer_corner_north_west,

// 	_map_tile_inner_corner_north_east= 48,
// 	_map_tile_inner_corner_south_east,
// 	_map_tile_inner_corner_south_west,
// 	_map_tile_inner_corner_north_west,

// 	_map_tile_error= 255,
// };

// typedef struct s_map_node {
// 	float world_x, world_y;
// 	int tile_x, tile_y;
// } s_map_node;

// DECLARE_DATA_ARRAY(map_node, 256);

// typedef struct s_game_map_graph {
// 	DATA_ARRAY(map_node) nodes;
// 	h_map_node edges[DATA_ARRAY_CAPACITY(map_node)][4];
// } s_game_map_graph;

// typedef struct s_game_map {
// 	int16 data[MAP_MAX_TILE_X][MAP_MAX_TILE_Y];
// 	s_game_map_graph graph;
// } s_game_map;

// typedef struct s_sprite_sheet {
// 	SDL_Surface* surface;
// 	SDL_Texture* texture;
// 	int32 sprite_width;
// 	int32 sprite_height;
// 	int32 sprites_per_row;
// 	int32 sprites_per_column;
// 	int32 max_sprites;
// } s_sprite_sheet;

// typedef enum e_entity_id {
// 	_entity_id_empty,
// 	_entity_id_pacman,
// 	_entity_id_inky,
// 	_entity_id_blinky,
// 	_entity_id_pinky,
// 	_entity_id_clyde,
// 	_entity_id_fruit,
// 	k_entity_id_count,

// 	_entity_begin= _entity_id_empty + 1,
// 	_entity_end= k_entity_id_count,
// } e_entity_id;

// static const char* k_entity_names[k_entity_id_count]= {
// 	"NONE",
// 	"pacman",
// 	"inky",
// 	"blinky",
// 	"pinky",
// 	"clyde",
// 	"fruit",
// };

// typedef enum e_direction {
// 	_direction_none,
// 	_direction_east,
// 	_direction_south,
// 	_direction_west,
// 	_direction_north,
// 	k_direction_count,
// 	_direction_first= _direction_east,
// 	_direction_last= k_direction_count,
// } e_direction;

// static const e_direction directions_flipped[k_direction_count]=
// 	{_direction_none, _direction_west, _direction_north, _direction_east, _direction_south};

// static inline e_direction direction_flip(e_direction direction)
// {
// 	ASSERT(VALID_INDEX(direction, k_direction_count));
// 	return directions_flipped[direction];
// }

// typedef enum e_buttons {
// 	_buttons_left,
// 	_buttons_right,
// 	_buttons_up,
// 	_buttons_down,
// 	_buttons_action,
// 	_buttons_back,
// 	k_buttons_count,
// } e_buttons;

// typedef struct s_game_config {
// 	struct {
// 		int16 eyes_base_sprite_id;
// 	} general;
// 	struct {
// 		float move_input_early_forgiveness_seconds;
// 	} input;
// } s_game_config;

// typedef struct s_move_request {
// 	e_direction requested_direction;
// 	float request_early_forgiveness_timer;
// 	int32 last_horizontal_request_frame;
// 	int32 last_vertical_request_frame;
// } s_move_request;

// typedef struct s_entity_config {
// 	float move_speed;
// 	float size;
// 	int16 base_sprite_id;
// 	int16 animation_frames;
// 	float animation_speed;
// } s_entity_config;

// enum e_entity_flags {
// 	_entity_flags_dead= 1 << 0,
// 	_entity_flags_draw_eyes= 1 << 1,
// };

// typedef struct s_game_state {
// 	int32 game_frame;
// 	struct {
// 		// "component" arrays
// 		uint32 flags[k_entity_id_count];
// 		SDL_FPoint position[k_entity_id_count];
// 		SDL_FPoint velocity[k_entity_id_count];
// 		e_direction facing[k_entity_id_count];
// 		e_direction move_direction[k_entity_id_count];
// 		s_move_request move_request[k_entity_id_count];
// 		int16 sprite_id[k_entity_id_count];
// 		s_entity_config config[k_entity_id_count];
// 		float animation_time[k_entity_id_count];
// 		float animation_timescale[k_entity_id_count];
// 		h_map_node next_node[k_entity_id_count];
// 	} entities;
// 	s_game_map* map;
// 	struct {
// 		uint8 button_mask;
// 		uint8 last_button_mask;
// 	} input;
// } s_game_state;

// #define GAME_STATE_GET_COMPONENT(game_state, entity_id, component_name)                                                \
// 	(&(game_state)->entities.component_name[(entity_id)])

// #define GET_COMPONENT(entity_id, component_name)                                                                       \
// 	GAME_STATE_GET_COMPONENT(&get_game()->game_state, entity_id, component_name)

// typedef struct s_game {
// 	bool should_run;
// 	SDL_Texture* screen_texture;
// 	SDL_FRect screen_rect;
// 	SDL_FRect world_screen_rect;
// 	s_game_map game_map;
// 	s_sprite_sheet sprite_sheet;
// 	s_game_state game_state;
// 	h_config config_handle;
// 	s_game_config game_config;
// } s_game;

// s_game* get_game(void);
// void game_update(float delta_time);
// void game_render(SDL_Renderer* renderer);
// void game_on_apply_config_handler(h_config config_handle);

// bool try_save_game_map(const s_game_map* game_map, const char* map_name);
// bool try_load_game_map(const char* map_name, s_game_map* out_game_map);
// void game_map_process_walls(s_game_map* game_map);
// bool tile_id_is_solid(int16 tile_id);
// bool tile_id_is_walkable(int16 tile_id);
// int16 game_map_sample_clamped(const s_game_map* game_map, int x, int y);
// void game_map_set_clamped(s_game_map* game_map, int x, int y, int16 tile_id);
// bool game_map_position_is_solid(const s_game_map* game_map, float position_x, float position_y);
// bool game_map_tile_is_solid_clamped(const s_game_map* game_map, int x, int y);
// bool game_map_tile_is_intersection(
// 	const s_game_map* game_map,
// 	int tile_x,
// 	int tile_y,
// 	int* out_open_count,
// 	int* out_h_count,
// 	int* out_v_count);
// h_map_node find_node_handle_at_tile(int tile_x, int tile_y, s_map_node* nodes, int32 node_count);
// h_map_node scan_for_node_in_direction(float pos_x, float pos_y, e_direction direction,
// s_map_node* nodes, int32 node_count); void game_map_build_graph(const s_game_map* game_map,
// s_game_map_graph* out_graph); int32 find_index_of_node_at_tile(int tile_x, int tile_y,
// s_map_node* nodes, int32 node_count); int16* get_game_map_wall_adjacency_matrix();

// bool try_create_sprite_sheet_from_image_file(
// 	const char* file_name,
// 	SDL_Renderer* renderer,
// 	int32 sprite_width,
// 	int32 sprite_height,
// 	s_sprite_sheet* out_sprite_sheet);
// void sprite_sheet_destroy(s_sprite_sheet* sprite_sheet);
// bool sprite_sheet_get_sprite_rect(const s_sprite_sheet* sprite_sheet, int16 sprite_id, SDL_Rect*
// out_rect); bool sprite_sheet_get_sprite_rect_ex(const s_sprite_sheet* sprite_sheet, int16
// sprite_id, SDL_Rect* out_rect); void sprite_sheet_sample_sprite( 	const s_sprite_sheet*
// sprite_sheet, 	int16 sprite_id, 	int x, 	int y, 	uint8* red, 	uint8* green, 	uint8* blue, 	uint8*
// alpha);

// SDL_Point entity_get_tile(e_entity_id entity_id);
// bool entity_can_move_in_direction(e_entity_id entity_id, e_direction move_direction);
// void entity_move_with_collision(e_entity_id entity_id, float move_speed);

// void world_fpoint_to_screen_point(float x, float y, int* out_x, int* out_y);
// void world_point_to_screen_point(int x, int y, int* out_x, int* out_y);
// bool window_point_to_world_point(int window_x, int window_y, float* out_x, float* out_y);
// bool window_point_to_tile_coordinate(int window_x, int window_y, int* out_tile_x, int*
// out_tile_y);

// static inline bool test_button(uint8 button_mask, e_buttons button)
// {
// 	unsigned int mask= MASK(button);
// 	return (button_mask & mask) != 0;
// }

// #define TEST_BIT(mask, bit) (((mask)&MASK(bit)) != 0)

// static inline bool button_down(e_buttons button)
// {
// 	return TEST_BIT(get_game()->game_state.input.button_mask, button);
// }

// static inline bool button_pressed(e_buttons button)
// {
// 	return TEST_BIT(get_game()->game_state.input.button_mask, button)
// 		   && !TEST_BIT(get_game()->game_state.input.last_button_mask, button);
// }

// static inline bool button_released(e_buttons button)
// {
// 	return !TEST_BIT(get_game()->game_state.input.button_mask, button)
// 		   && TEST_BIT(get_game()->game_state.input.last_button_mask, button);
// }

// static inline float ping_pong(float t)
// {
// 	t*= 2;
// 	if (t >= 1.0f) t= 2.0f - t;
// 	return t;
// }
