#include "game.h"

// #include "debug.h"
// #include "stb_ds.h"
// #include "stb_image.h"
// #include <SDL2/SDL.h>

// static s_game g_game;

// int16 game_map_sprite_id_adjacency_matrix[16]= {
// 	_map_tile_none,
// 	_map_tile_none,
// 	_map_tile_none,
// 	_map_tile_solid,

// 	_map_tile_none,
// 	_map_tile_outer_corner_south_east,
// 	_map_tile_outer_corner_south_west,
// 	_map_tile_wall_south,

// 	_map_tile_none,
// 	_map_tile_outer_corner_north_east,
// 	_map_tile_outer_corner_north_west,
// 	_map_tile_wall_north,

// 	_map_tile_solid,
// 	_map_tile_wall_east,
// 	_map_tile_wall_west,
// 	_map_tile_none,
// };

// s_game* get_game(void)
// {
// 	return &g_game;
// }

// bool try_save_game_map(const s_game_map* game_map, const char* map_name)
// {
// 	// Very basic csv saving for now
// 	bool success= false;
// 	fixed_buffer(map_file_name, 256)= {0};
// 	snprintf(map_file_name, map_file_name_size, "%s.csv", map_name);
// 	FILE* map_file= fopen(map_file_name, "w");
// 	if (map_file) {
// 		for (int y= 0; y < MAP_MAX_TILE_Y; ++y) {
// 			for (int x= 0; x < MAP_MAX_TILE_X; ++x) {
// 				fprintf(map_file, "%d", game_map->data[x][y]);
// 				if (x < MAP_MAX_TILE_X - 1) {
// 					fprintf(map_file, ",");
// 				}
// 			}
// 			fprintf(map_file, "\n");
// 		}
// 		success= true;
// 		fclose(map_file);
// 	}

// 	return success;
// }

// bool try_load_game_map(const char* map_name, s_game_map* out_game_map)
// {
// 	ASSERT(out_game_map != NULL);

// 	fixed_buffer(map_file_name, 256)= {0};
// 	snprintf(map_file_name, map_file_name_size, "%s.csv", map_name);

// 	bool success= false;
// 	char** lines= read_file_lines(map_file_name);
// 	int lines_count= (int)arrlen(lines);

// 	if (lines_count > 0) {
// 		int row= 0;
// 		const char* delimeter= ",";

// 		while (row < MAP_MAX_TILE_Y && row < lines_count) {
// 			int col= 0;
// 			char* element= strtok(lines[row], delimeter);
// 			while (element && col < MAP_MAX_TILE_X) {
// 				long element_value= strtol(element, NULL, 10);
// 				out_game_map->data[col][row]= (int16)element_value;
// 				element= strtok(NULL, delimeter);
// 				col++;
// 			}
// 			row++;
// 		}

// 		success= true;
// 	}

// 	free_file_lines(lines);

// 	return success;
// }

// //

// const int16 k_non_solid_tiles[]= {
// 	_map_tile_none,
// 	_map_tile_dot,
// 	_map_tile_big_dot,
// 	_map_tile_monster,
// };

// bool tile_id_is_solid(int16 tile_id)
// {
// 	bool is_solid= false;
// 	if (tile_id >= 0) {
// 		is_solid= true;

// 		for (int index= 0; index < ARRAY_COUNT(k_non_solid_tiles); ++index) {
// 			if (tile_id == k_non_solid_tiles[index]) {
// 				is_solid= false;
// 				break;
// 			}
// 		}
// 	}

// 	return is_solid;
// }

// bool tile_id_is_walkable(int16 tile_id)
// {
// 	return tile_id == _map_tile_none || tile_id == _map_tile_dot || tile_id == _map_tile_big_dot;
// }

// int16 game_map_sample_clamped(const s_game_map* game_map, int x, int y)
// {
// 	x= clamp(x, 0, MAP_MAX_TILE_X - 1);
// 	y= clamp(y, 0, MAP_MAX_TILE_Y - 1);
// 	int16 result= game_map->data[x][y];
// 	return result;
// }

// void game_map_set_clamped(s_game_map* game_map, int x, int y, int16 tile_id)
// {
// 	x= clamp(x, 0, MAP_MAX_TILE_X - 1);
// 	y= clamp(y, 0, MAP_MAX_TILE_Y - 1);
// 	game_map->data[x][y]= tile_id;
// }

// bool game_map_tile_is_solid_clamped(const s_game_map* game_map, int x, int y)
// {
// 	int16 id= game_map_sample_clamped(game_map, x, y);
// 	int is_solid= tile_id_is_solid(id);
// 	return is_solid;
// }

// bool game_map_position_is_solid(const s_game_map* game_map, float position_x, float position_y)
// {
// 	return game_map_tile_is_solid_clamped(game_map, (int)position_x, (int)position_y);
// }

// const int16 k_unprocessed_tiles[]= {
// 	_map_tile_monster,
// 	_map_tile_dot,
// 	_map_tile_big_dot,
// };

// bool exclude_from_processing(int16 id)
// {
// 	bool result= false;
// 	for (int i= 0; i < ARRAY_COUNT(k_unprocessed_tiles); ++i) {
// 		if (k_unprocessed_tiles[i] == id) {
// 			result= true;
// 			break;
// 		}
// 	}
// 	return result;
// }

// void game_map_process_walls(s_game_map* game_map)
// {
// 	s_game_map original= *game_map;

// 	for (int tile_x= 0; tile_x < MAP_MAX_TILE_X; tile_x++) {
// 		for (int tile_y= 0; tile_y < MAP_MAX_TILE_Y; tile_y++) {
// 			int16 sample_id= game_map_sample_clamped(game_map, tile_x, tile_y);
// 			if (sample_id >= 0 && !exclude_from_processing(sample_id)) {
// 				int samples[4]= {
// 					game_map_tile_is_solid_clamped(&original, tile_x - 1, tile_y),
// 					game_map_tile_is_solid_clamped(&original, tile_x + 1, tile_y),
// 					game_map_tile_is_solid_clamped(&original, tile_x, tile_y - 1),
// 					game_map_tile_is_solid_clamped(&original, tile_x, tile_y + 1),
// 				};
// 				uint8 mask= 0;
// 				for (int i= 0; i < 4; i++) {
// 					mask|= (samples[i] << i);
// 				}

// 				int16 sprite_id= game_map_sprite_id_adjacency_matrix[mask];

// 				if ((mask & 0xF) == 0xF) {
// 					static const int offsets[8]= {1, -1, 1, 1, -1, 1, -1, -1};
// 					static const int16 corner_sprites[4]= {
// 						_map_tile_inner_corner_north_east,
// 						_map_tile_inner_corner_south_east,
// 						_map_tile_inner_corner_south_west,
// 						_map_tile_inner_corner_north_west,
// 					};

// 					for (int i= 0; i < 4; ++i) {
// 						if (!game_map_tile_is_solid_clamped(
// 								&original, tile_x + offsets[i * 2], tile_y + offsets[i * 2 + 1]))
// 						{
// 							sprite_id= corner_sprites[i];
// 							break;
// 						}
// 					}
// 				}

// 				game_map->data[tile_x][tile_y]= sprite_id;
// 			}
// 		}
// 	}
// }

// int16* get_game_map_wall_adjacency_matrix()
// {
// 	return game_map_sprite_id_adjacency_matrix;
// }

// bool game_map_tile_is_intersection(
// 	const s_game_map* game_map,
// 	int tile_x,
// 	int tile_y,
// 	int* out_open_count,
// 	int* out_h_count,
// 	int* out_v_count)
// {
// 	bool is_intersection_tile= false;
// 	int16 tile_id= game_map_sample_clamped(game_map, tile_x, tile_y);
// 	int open_count= 0, h_count= 0, v_count= 0;
// 	if (tile_id_is_walkable(tile_id)) {
// 		if (tile_id_is_walkable(game_map_sample_clamped(game_map, tile_x - 1, tile_y))) {
// 			open_count++;
// 			h_count++;
// 		}
// 		if (tile_id_is_walkable(game_map_sample_clamped(game_map, tile_x + 1, tile_y))) {
// 			open_count++;
// 			h_count++;
// 		}
// 		if (tile_id_is_walkable(game_map_sample_clamped(game_map, tile_x, tile_y - 1))) {
// 			open_count++;
// 			v_count++;
// 		}
// 		if (tile_id_is_walkable(game_map_sample_clamped(game_map, tile_x, tile_y + 1))) {
// 			open_count++;
// 			v_count++;
// 		}

// 		is_intersection_tile= (open_count == 2 && h_count == v_count) || (open_count > 2);
// 		is_intersection_tile|= (tile_x < 0 || tile_y < 0 || tile_x >= MAP_MAX_TILE_X || tile_y >= MAP_MAX_TILE_Y);
// 	}

// 	if (out_open_count) *out_open_count= open_count;
// 	if (out_h_count) *out_h_count= h_count;
// 	if (out_v_count) *out_v_count= open_count;

// 	return is_intersection_tile;
// }

// static inline SDL_Point direction_xy(e_direction direction)
// {
// 	static const SDL_Point directions[]= {
// 		{0, 0},
// 		{1, 0},
// 		{0, 1},
// 		{-1, 0},
// 		{0, -1},
// 	};


// 	_Static_assert(ARRAY_COUNT(directions) == k_direction_count, "");
// 	// _STATIC_ASSERT(ARRAY_COUNT(directions) == k_direction_count);

// 	ASSERT(VALID_INDEX(direction, k_direction_count));
// 	return directions[direction];
// }

// int32 find_index_of_node_at_tile(int tile_x, int tile_y, s_map_node* nodes, int32 node_count)
// {
// 	int32 found= NONE;
// 	for (int32 node_index= 0; node_index < node_count; node_index++) {
// 		if (nodes[node_index].tile_x == tile_x && nodes[node_index].tile_y == tile_y) {
// 			found= node_index;
// 			break;
// 		}
// 	}
// 	return found;
// }

// h_map_node find_node_handle_at_tile(int tile_x, int tile_y, s_map_node* nodes, int32 node_count)
// {
// 	h_map_node handle= INVALID_HANDLE;
// 	int32 index= find_index_of_node_at_tile(tile_x, tile_y, nodes, node_count);
// 	if (index != NONE) {
// 		handle.value= index + 1;
// 	}
// 	return handle;
// }

// h_map_node scan_for_node_in_direction(
// 	float pos_x,
// 	float pos_y,
// 	e_direction direction,
// 	s_map_node* nodes,
// 	int32 node_count)
// {
// 	int32 tile_x= (int32)(pos_x - 0.0f);
// 	int32 tile_y= (int32)(pos_y - 0.0f);
// 	h_map_node handle= INVALID_HANDLE;

// 	SDL_Point delta= direction_xy(direction);

// 	do {
// 		handle= find_node_handle_at_tile(tile_x, tile_y, nodes, node_count);
// 		tile_x+= delta.x;
// 		tile_y+= delta.y;
// 	} while (!HANDLE_IS_VALID(handle) && tile_x >= 0 && tile_y >= 0 && tile_x < MAP_MAX_TILE_X
// 			 && tile_y < MAP_MAX_TILE_Y && (delta.x != 0 || delta.y != 0));

// 	return handle;
// }

// void game_map_build_graph(const s_game_map* game_map, s_game_map_graph* out_graph)
// {
// 	ZERO_STRUCT(out_graph);

// 	for (int tile_y= -1; tile_y <= MAP_MAX_TILE_Y; ++tile_y) {
// 		for (int tile_x= -1; tile_x <= MAP_MAX_TILE_X; ++tile_x) {
// 			int open_count, h_count, v_count;
// 			if (game_map_tile_is_intersection(game_map, tile_x, tile_y, &open_count, &h_count, &v_count)) {
// 				h_map_node node_handle= map_node_data_array_alloc(&out_graph->nodes);
// 				s_map_node* node= map_node_data_array_get(&out_graph->nodes, node_handle);

// 				node->tile_x= tile_x;
// 				node->tile_y= tile_y;
// 				node->world_x= (float)node->tile_x + 0.5f;
// 				node->world_y= (float)node->tile_y + 0.5f;
// 			}
// 		}
// 	}

// 	for (int32 node_index= 0; node_index < out_graph->nodes.count; node_index++) {
// 		h_map_node node_handle= HANDLE_CREATE_FROM_INDEX(map_node, node_index);
// 		s_map_node* node= map_node_data_array_get(&out_graph->nodes, node_handle);
// 		for (e_direction direction= _direction_first; direction != _direction_last; direction++) {
// 			if (!HANDLE_IS_VALID(out_graph->edges[node_index][direction - _direction_first])) {
// 				static const int deltas[]= {1, 0, 0, 1, -1, 0, 0, -1};
// 				int direction_index= (int)(direction - _direction_first);
// 				int delta_x= deltas[direction_index * 2 + 0];
// 				int delta_y= deltas[direction_index * 2 + 1];
// 				int tile_x= node->tile_x + delta_x;
// 				int tile_y= node->tile_y + delta_y;

// 				h_map_node found_node_handle= INVALID_HANDLE;

// 				while (!HANDLE_IS_VALID(found_node_handle) && !game_map_tile_is_solid_clamped(game_map, tile_x, tile_y)
// 					   && (tile_x >= -1 && tile_y >= -1 && tile_x <= MAP_MAX_TILE_X && tile_y <= MAP_MAX_TILE_Y))
// 				{
// 					found_node_handle=
// 						find_node_handle_at_tile(tile_x, tile_y, out_graph->nodes.data, out_graph->nodes.count);
// 					tile_x+= delta_x;
// 					tile_y+= delta_y;
// 				}

// 				if (HANDLE_IS_VALID(found_node_handle)) {
// 					out_graph->edges[node_index][direction - _direction_first]= found_node_handle;
// 					out_graph->edges[HANDLE_INDEX(found_node_handle)][direction_flip(direction) - _direction_first]=
// 						node_handle;
// 				}
// 			}
// 		}
// 	}
// }

// IMPLEMENT_DATA_ARRAY_INTERFACE(map_node);

// bool try_create_sprite_sheet_from_image_file(
// 	const char* file_name,
// 	SDL_Renderer* renderer,
// 	int32 sprite_width,
// 	int32 sprite_height,
// 	s_sprite_sheet* out_sprite_sheet)
// {
// 	ZERO_STRUCT(out_sprite_sheet);

// 	int image_width, image_height, image_bytes_per_pixel;
// 	stbi_uc* image_pixels= stbi_load(file_name, &image_width, &image_height, &image_bytes_per_pixel, 4);

// 	if (image_pixels) {
// 		// For now require sprite sheet is evenly divisible
// 		ASSERT(image_width % sprite_width == 0);
// 		ASSERT(image_height % sprite_height == 0);

// 		SDL_Surface* image_surface= SDL_CreateRGBSurfaceWithFormatFrom(
// 			image_pixels, image_width, image_height, 32, image_width * 4, SDL_PIXELFORMAT_ABGR8888);

// 		out_sprite_sheet->sprite_width= sprite_width;
// 		out_sprite_sheet->sprite_height= sprite_height;
// 		out_sprite_sheet->sprites_per_row= image_width / out_sprite_sheet->sprite_width;
// 		out_sprite_sheet->sprites_per_column= image_height / out_sprite_sheet->sprite_height;
// 		out_sprite_sheet->max_sprites= out_sprite_sheet->sprites_per_row * out_sprite_sheet->sprites_per_column;
// 		out_sprite_sheet->surface= image_surface;
// 		out_sprite_sheet->texture= SDL_CreateTextureFromSurface(renderer, out_sprite_sheet->surface);
// 	}

// 	return out_sprite_sheet->surface != NULL;
// }

// void sprite_sheet_destroy(s_sprite_sheet* sprite_sheet)
// {
// 	void* pixel_data= sprite_sheet->surface->pixels;
// 	SDL_FreeSurface(sprite_sheet->surface);
// 	stbi_image_free(pixel_data);
// 	SDL_DestroyTexture(sprite_sheet->texture);
// 	ZERO_STRUCT(sprite_sheet);
// }

// bool sprite_sheet_get_sprite_rect(const s_sprite_sheet* sprite_sheet, int16 sprite_id, SDL_Rect* out_rect)
// {
// 	ASSERT(sprite_sheet != NULL);
// 	ASSERT(out_rect != NULL);

// 	bool success= false;
// 	ZERO_STRUCT(out_rect);

// 	if (sprite_sheet->surface && sprite_id >= 0 && sprite_id < sprite_sheet->max_sprites) {
// 		int sprite_row= sprite_id / sprite_sheet->sprites_per_row;
// 		int sprite_column= sprite_id % sprite_sheet->sprites_per_row;

// 		out_rect->x= sprite_column * sprite_sheet->sprite_width;
// 		out_rect->y= sprite_row * sprite_sheet->sprite_height;

// 		out_rect->w= sprite_sheet->sprite_width;
// 		out_rect->h= sprite_sheet->sprite_height;

// 		success= true;
// 	}

// 	return success;
// }

// void sprite_sheet_sample_sprite(
// 	const s_sprite_sheet* sprite_sheet,
// 	int16 sprite_id,
// 	int x,
// 	int y,
// 	uint8* red,
// 	uint8* green,
// 	uint8* blue,
// 	uint8* alpha)
// {
// 	uint32* pixel= NULL;

// 	SDL_Rect sprite_rect;
// 	if (sprite_sheet_get_sprite_rect(sprite_sheet, sprite_id, &sprite_rect)) {
// 		if (x >= 0 && x < sprite_rect.w && y >= 0 && y < sprite_rect.h) {
// 			int pixel_x= sprite_rect.x + x;
// 			int pixel_y= sprite_rect.y + y;
// 			pixel= (uint32*)(((uint8*)sprite_sheet->surface->pixels) + (pixel_y * sprite_sheet->surface->pitch)
// 							 + (pixel_x * sprite_sheet->surface->format->BytesPerPixel));
// 		}
// 	}

// 	if (pixel != NULL) {
// 		uint8 r, g, b, a;
// 		SDL_GetRGBA(*pixel, sprite_sheet->surface->format, &r, &g, &b, &a);
// 		if (red) *red= r;
// 		if (green) *green= g;
// 		if (blue) *blue= b;
// 		if (alpha) *alpha= a;
// 	} else {
// 		if (red) *red= 0;
// 		if (blue) *blue= 0;
// 		if (green) *green= 0;
// 		if (alpha) *alpha= 0;
// 	}
// }

// SDL_Point entity_get_tile(e_entity_id entity_id)
// {
// 	SDL_FPoint* position= GET_COMPONENT(entity_id, position);
// 	SDL_Point tile= {
// 		(int)(position->x + 0.125f),
// 		(int)(position->y + 0.125f),
// 	};
// 	return tile;
// }

// static SDL_FPoint vector_from_direction(e_direction direction)
// {
// 	static const SDL_FPoint direction_vectors[5]= {
// 		{0.0f, 0.0f},
// 		{1.0f, 0.0f},
// 		{0.0f, 1.0f},
// 		{-1.0f, 0.0f},
// 		{0.0f, -1.0f},
// 	};

// 	return direction_vectors[direction];
// }

// void entity_move_with_collision(e_entity_id entity_id, float move_speed)
// {
// 	SDL_FPoint* position= GET_COMPONENT(entity_id, position);
// 	SDL_FPoint* velocity= GET_COMPONENT(entity_id, velocity);
// 	e_direction* facing= GET_COMPONENT(entity_id, facing);
// 	e_direction* move_direction= GET_COMPONENT(entity_id, move_direction);

// 	const s_game_map* map= get_game()->game_state.map;

// 	static const float k_scan_distance= 0.5125f;
// 	static const float k_collision_check_delta= 1.0f / 32.0f;

// 	h_map_node next_node_handle= scan_for_node_in_direction(
// 		position->x,
// 		position->y,
// 		*move_direction,
// 		g_game.game_state.map->graph.nodes.data,
// 		g_game.game_state.map->graph.nodes.count);

// 	int32 node_index= next_node_handle.value;
// 	s_map_node* next_node= &g_game.game_state.map->graph.nodes.data[node_index];

// 	float movement_remaining= move_speed;

// 	SDL_FPoint direction= vector_from_direction(*move_direction);
// 	*velocity= (SDL_FPoint){direction.x * movement_remaining, direction.y * movement_remaining};
// 	SDL_FPoint next_position= (SDL_FPoint){
// 		.x= position->x + velocity->x,
// 		.y= position->y + velocity->y,
// 	};

// 	s_move_request* move_request= GET_COMPONENT(entity_id, move_request);
// 	if (move_request->requested_direction != _direction_none && move_request->requested_direction != *move_direction) {
// 		bool switched_direction= false;
// 		float distance_to_node= 0.0f;

// 		if (*move_direction == _direction_none) {
// 			switched_direction= true;
// 		} else if (move_request->requested_direction == direction_flip(*move_direction)) {
// 			switched_direction= true;
// 		} else {
// 			h_map_node linked_node=
// 				g_game.game_state.map->graph.edges[node_index][move_request->requested_direction - 1];
// 			if (HANDLE_IS_VALID(linked_node)) {
// 				switch (*move_direction) {
// 					case _direction_east:
// 						if (position->x <= next_node->world_x && next_position.x >= next_node->world_x) {
// 							distance_to_node= next_node->world_x - position->x;
// 							position->x= next_node->world_x;
// 						}
// 						break;
// 					case _direction_west:
// 						if (position->x >= next_node->world_x && next_position.x <= next_node->world_x) {
// 							distance_to_node= position->x - next_node->world_x;
// 							position->x= next_node->world_x;
// 						}
// 						break;
// 					case _direction_south:
// 						if (position->y <= next_node->world_y && next_position.y >= next_node->world_y) {
// 							distance_to_node= next_node->world_y - position->y;
// 							position->y= next_node->world_y;
// 						}
// 						break;
// 					case _direction_north:
// 						if (position->y >= next_node->world_y && next_position.y <= next_node->world_y) {
// 							distance_to_node= position->y - next_node->world_y;
// 							position->y= next_node->world_y;
// 						}
// 						break;
// 					default: break;
// 				}
// 			}
// 		}

// 		if (switched_direction) {
// 			movement_remaining-= distance_to_node;
// 			*facing= move_request->requested_direction;
// 			*move_direction= move_request->requested_direction;
// 			ZERO_STRUCT(move_request);
// 		}
// 	}

// 	direction= vector_from_direction(*move_direction);
// 	*velocity= (SDL_FPoint){direction.x * movement_remaining, direction.y * movement_remaining};

// 	bool wall_hit= false;

// 	while (velocity->x > 0.0f
// 		   && (game_map_position_is_solid(
// 			   get_game()->game_state.map, position->x + velocity->x + k_scan_distance, position->y)))
// 	{
// 		velocity->x= max(velocity->x - k_collision_check_delta, 0.0f);
// 		wall_hit= true;
// 	}

// 	while (velocity->x < 0.0f
// 		   && (game_map_position_is_solid(
// 			   get_game()->game_state.map, position->x + velocity->x - k_scan_distance, position->y)))
// 	{
// 		velocity->x= min(velocity->x + k_collision_check_delta, 0.0f);
// 		wall_hit= true;
// 	}

// 	while (velocity->y > 0.0f
// 		   && (game_map_position_is_solid(
// 			   get_game()->game_state.map, position->x, position->y + velocity->y + k_scan_distance)))
// 	{
// 		velocity->y= max(velocity->y - k_collision_check_delta, 0.0f);
// 		wall_hit= true;
// 	}

// 	while (velocity->y < 0.0f
// 		   && (game_map_position_is_solid(
// 			   get_game()->game_state.map, position->x, position->y + velocity->y - k_scan_distance)))
// 	{
// 		velocity->y= min(velocity->y + k_collision_check_delta, 0.0f);
// 		wall_hit= true;
// 	}

// 	if (wall_hit) {
// 		*move_direction= _direction_none;
// 	}

// 	position->x+= velocity->x;
// 	position->y+= velocity->y;

// 	static const float map_width= (float)MAP_MAX_TILE_X + 2.0f;
// 	static const float map_height= (float)MAP_MAX_TILE_Y + 2.0f;
// 	static const float margin= 0.5f;

// 	if (position->x > map_width - margin) {
// 		position->x-= map_width;
// 	}
// 	if (position->x < -margin) {
// 		position->x+= map_width;
// 	}
// }

// bool entity_can_move_in_direction(e_entity_id entity_id, e_direction move_direction)
// {
// 	static const SDL_FPoint facing_directions[5]= {
// 		{0.0f, 0.0f},
// 		{1.0f, 0.0f},
// 		{0.0f, 1.0f},
// 		{-1.0f, 0.0f},
// 		{0.0f, -1.0f},
// 	};

// 	static const float k_scan_distance= 1.0f;
// 	static const float k_scan_half_width= 0.375f;
// 	SDL_FPoint direction= facing_directions[move_direction];
// 	SDL_FPoint perpendicular= (SDL_FPoint){direction.y, direction.x};

// 	SDL_FPoint origins[]= {
// 		{perpendicular.x * k_scan_half_width, perpendicular.y * k_scan_half_width},
// 		{perpendicular.x * -k_scan_half_width, perpendicular.y * -k_scan_half_width},
// 	};

// 	SDL_FPoint offsets[ARRAY_COUNT(origins)]= {0};

// 	if (fabsf(direction.x) > fabsf(direction.y)) {
// 		for (int32 index= 0; index < ARRAY_COUNT(offsets); index++) {
// 			offsets[index].x= direction.x;
// 			offsets[index].y= origins[index].y;
// 		}
// 	} else {
// 		for (int32 index= 0; index < ARRAY_COUNT(offsets); index++) {
// 			offsets[index].x= origins[index].x;
// 			offsets[index].y= direction.y;
// 		}
// 	}

// 	SDL_FPoint* position= GET_COMPONENT(entity_id, position);

// 	bool can_move= true;

// 	for (int32 index= 0; index < ARRAY_COUNT(offsets); index++) {
// 		SDL_FPoint offset= offsets[index];
// 		SDL_FPoint origin= origins[index];
// 		Color color= (Color){.g= 255, .a= 255};
// 		debug_line(
// 			position->x + origin.x, position->y + origin.y, position->x + offset.x, position->y + offset.y, color, 1);
// 		if (game_map_position_is_solid(get_game()->game_state.map, position->x + offset.x, position->y + offset.y)) {
// 			can_move= false;
// 			// break;
// 		}
// 	}

// 	return can_move;
// }

// void world_fpoint_to_screen_point(float x, float y, int* out_x, int* out_y)
// {
// 	if (out_x) *out_x= (int)(x * 8);
// 	if (out_y) *out_y= (int)(y * 8.0f + get_game()->world_screen_rect.y);
// }

// void world_point_to_screen_point(int x, int y, int* out_x, int* out_y)
// {
// 	world_fpoint_to_screen_point((float)x, (float)y, out_x, out_y);
// }

// bool window_point_to_world_point(int window_x, int window_y, float* out_x, float* out_y)
// {
// 	bool result= false;

// 	SDL_FPoint window_point= {(float)window_x, (float)window_y};

// 	if (SDL_PointInFRect(&window_point, &get_game()->screen_rect)) {
// 		SDL_FPoint window_screen_space= {
// 			window_point.x - get_game()->screen_rect.x,
// 			window_point.y - get_game()->screen_rect.y,
// 		};
// 		SDL_FRect window_screen_dimensions= {
// 			.w= get_game()->screen_rect.w,
// 			.h= get_game()->screen_rect.h,
// 		};
// 		if (SDL_PointInFRect(&window_screen_space, &window_screen_dimensions)) {
// 			SDL_FPoint screen_space= {
// 				window_screen_space.x * (SCREEN_PIXEL_WIDTH / get_game()->screen_rect.w),
// 				window_screen_space.y * (SCREEN_PIXEL_HEIGHT / get_game()->screen_rect.h),
// 			};
// 			if (SDL_PointInFRect(&screen_space, &get_game()->world_screen_rect)) {
// 				SDL_FPoint world_space= {
// 					(screen_space.x - get_game()->world_screen_rect.x) / 8.0f,
// 					(screen_space.y - get_game()->world_screen_rect.y) / 8.0f,
// 				};
// 				if (out_x) *out_x= world_space.x;
// 				if (out_y) *out_y= world_space.y;
// 				result= true;
// 			}
// 		}
// 	}

// 	return result;
// }

// bool window_point_to_tile_coordinate(int window_x, int window_y, int* out_tile_x, int* out_tile_y)
// {
// 	float world_x, world_y;
// 	bool result= false;
// 	if (window_point_to_world_point(window_x, window_y, &world_x, &world_y)) {
// 		if (out_tile_x) *out_tile_x= (int)world_x;
// 		if (out_tile_y) *out_tile_y= (int)world_y;
// 		result= true;
// 	}
// 	return result;
// }

// void game_update(float delta_time)
// {
// 	s_game* game= get_game();

// 	int input_move_x= button_down(_buttons_right) - button_down(_buttons_left);
// 	int input_move_y= button_down(_buttons_down) - button_down(_buttons_up);

// 	{
// 		e_direction* facing= GET_COMPONENT(_entity_id_pacman, facing);
// 		e_direction* move_direction= GET_COMPONENT(_entity_id_pacman, move_direction);
// 		s_move_request* move_request= GET_COMPONENT(_entity_id_pacman, move_request);
// 		e_direction input_direction= _direction_none;

// 		if (input_move_x != 0 && input_move_y != 0) {
// 			if (move_request->last_horizontal_request_frame > move_request->last_vertical_request_frame) {
// 				input_move_x= 0;
// 			} else {
// 				input_move_y= 0;
// 			}
// 		}

// 		if (move_request->request_early_forgiveness_timer > 0.0f) {
// 			move_request->request_early_forgiveness_timer-= delta_time;
// 			if (move_request->request_early_forgiveness_timer <= 0.0f) {
// 				move_request->request_early_forgiveness_timer= 0.0f;
// 				move_request->requested_direction= _direction_none;
// 			}
// 		}

// 		if (input_move_x != 0) {
// 			if (input_move_x > 0) {
// 				move_request->requested_direction= _direction_east;
// 			} else {
// 				move_request->requested_direction= _direction_west;
// 			}
// 			move_request->last_horizontal_request_frame= game->game_state.game_frame;
// 			move_request->request_early_forgiveness_timer= game->game_config.input.move_input_early_forgiveness_seconds;
// 		} else if (input_move_y != 0) {
// 			if (input_move_y > 0) {
// 				move_request->requested_direction= _direction_south;
// 			} else {
// 				move_request->requested_direction= _direction_north;
// 			}
// 			move_request->last_vertical_request_frame= game->game_state.game_frame;
// 			move_request->request_early_forgiveness_timer= game->game_config.input.move_input_early_forgiveness_seconds;
// 		}
// 	}

// 	for (e_entity_id entity= _entity_begin; entity != _entity_end; entity++) {
// 		s_entity_config* config= GET_COMPONENT(entity, config);
// 		SDL_FPoint* velocity= GET_COMPONENT(entity, velocity);
// 		float* animation_timescale= GET_COMPONENT(entity, animation_timescale);
// 		float* animation_time= GET_COMPONENT(entity, animation_time);

// 		entity_move_with_collision(entity, config->move_speed * delta_time);

// 		if (fabsf(velocity->x) + fabsf(velocity->y) != 0) {
// 			*animation_timescale= 1.0f;
// 		} else {
// 			*animation_timescale= 0.0f;
// 			*animation_time= 0.0f;
// 		}
// 		*animation_time+= *animation_timescale * config->animation_speed * delta_time;
// 		if (*animation_time >= 1.0f) *animation_time-= 1.0f;

// 		*GET_COMPONENT(entity, sprite_id)=
// 			(int16)(ping_pong(*animation_time) * config->animation_frames) * 2 + config->base_sprite_id;
// 	};

// 	// Eat dots
// 	{
// 		SDL_Point tile= entity_get_tile(_entity_id_pacman);
// 		int16 tile_id= game_map_sample_clamped(game->game_state.map, tile.x, tile.y);
// 		if (tile_id == _map_tile_big_dot || tile_id == _map_tile_dot) {
// 			game_map_set_clamped(game->game_state.map, tile.x, tile.y, _map_tile_none);
// 		}
// 	}
// }

// void game_render(SDL_Renderer* renderer)
// {
// 	s_game* game= get_game();

// 	// Draw map
// 	for (int tile_x= 0; tile_x < MAP_MAX_TILE_X; ++tile_x) {
// 		for (int tile_y= 0; tile_y < MAP_MAX_TILE_Y; ++tile_y) {
// 			int16 tile_id= game->game_map.data[tile_x][tile_y];
// 			if (tile_id < 0) tile_id= _map_tile_error;
// 			if (tile_id != _map_tile_monster && tile_id != _map_tile_none && tile_id != _map_tile_hidden_solid) {
// 				SDL_Rect source_rect;
// 				sprite_sheet_get_sprite_rect(&game->sprite_sheet, tile_id, &source_rect);
// 				SDL_Rect dest_rect= {
// 					.w= 8,
// 					.h= 8,
// 				};
// 				world_point_to_screen_point(tile_x, tile_y, &dest_rect.x, &dest_rect.y);
// 				SDL_RenderCopy(renderer, game->sprite_sheet.texture, &source_rect, &dest_rect);
// 			}
// 		}
// 	}

// 	// Draw entities
// 	for (e_entity_id entity= _entity_begin; entity != _entity_end; entity++) {
// 		int16 sprite_id= game->game_state.entities.sprite_id[entity];
// 		e_direction* facing= GET_COMPONENT(entity, facing);
// 		uint32 flags= *GET_COMPONENT(entity, flags);

// 		SDL_FPoint position= game->game_state.entities.position[entity];
// 		int screen_x, screen_y;
// 		world_fpoint_to_screen_point(position.x, position.y, &screen_x, &screen_y);
// 		SDL_Rect dest_rect= {
// 			.x= screen_x - 8,
// 			.y= screen_y - 8,
// 			.w= 16,
// 			.h= 16,
// 		};

// 		if (sprite_id > 0) {
// 			SDL_Rect source_rect;
// 			sprite_sheet_get_sprite_rect(&game->sprite_sheet, sprite_id, &source_rect);
// 			source_rect.w= 16;
// 			source_rect.h= 16;

// 			if (entity == _entity_id_pacman) {
// 				double facing_angle= 0.0;
// 				SDL_RendererFlip flip= SDL_FLIP_NONE;

// 				if (*facing == _direction_north || *facing == _direction_south) {
// 					facing_angle= 90.0;
// 				}

// 				if (*facing == _direction_west || *facing == _direction_north) {
// 					flip= SDL_FLIP_HORIZONTAL;
// 				}

// 				SDL_RenderCopyEx(
// 					renderer, game->sprite_sheet.texture, &source_rect, &dest_rect, facing_angle, NULL, flip);
// 			} else {
// 				SDL_RenderCopy(renderer, game->sprite_sheet.texture, &source_rect, &dest_rect);
// 			}
// 		}

// 		if ((flags & _entity_flags_draw_eyes) != 0 && game->game_config.general.eyes_base_sprite_id > 0) {
// 			int16 sprite_id= game->game_config.general.eyes_base_sprite_id + (*facing - _direction_east) * 2;
// 			SDL_Rect source_rect;
// 			sprite_sheet_get_sprite_rect(&game->sprite_sheet, sprite_id, &source_rect);
// 			source_rect.w= 16;
// 			source_rect.h= 16;
// 			SDL_RenderCopy(renderer, game->sprite_sheet.texture, &source_rect, &dest_rect);
// 		}
// 	}
// }

// void game_on_apply_config_handler(h_config config_handle)
// {
// 	s_game* game= get_game();

// 	game->game_config.general.eyes_base_sprite_id=
// 		config_get_or_default_int(game->config_handle, "general", "eyes_base_sprite_id", 0);
// 	game->game_config.input.move_input_early_forgiveness_seconds=
// 		config_get_or_default_float(game->config_handle, "input", "move_input_early_forgiveness_seconds", 0.0f);

// 	for (int32 entity= _entity_id_pacman; entity != _entity_end; entity++) {
// 		const char* entity_name= k_entity_names[entity];

// 		s_entity_config* config= GET_COMPONENT(entity, config);
// 		if (config_has_section(game->config_handle, entity_name)) {
// #define get_property(type, name, def) config_get_or_default_##type(game->config_handle, entity_name, name, def)
// 			config->move_speed= get_property(float, "move_speed", 1.0f);
// 			config->size= get_property(float, "size", 1.0f);
// 			config->base_sprite_id= (int16)get_property(int, "base_sprite_id", 0);
// 			config->animation_frames= (int16)get_property(int, "animation_frames", 0);
// 			config->animation_speed= (int16)get_property(float, "animation_speed", 1.0f);
// #undef get_property
// 		}
// 	}
// }
