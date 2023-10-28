#pragma once

#include "Types.h"

typedef struct ini_t ini_t;

DECLARE_HANDLE(config);

typedef void (*t_on_apply_config)(h_config config_handle);

typedef enum e_config_property_type {
	_config_property_type_string,
	_config_property_type_float,
	_config_property_type_integer,
	_config_property_type_bool,
	k_config_property_type_count,
} e_config_property_type;

typedef struct s_config_property_type_entry {
	uint32 Key;
	e_config_property_type Value;
} s_config_property_type_entry;

void config_initialize(void);
void config_shutdown(void);
h_config config_load_file(const char* file_name);
void config_destroy(h_config config_handle);
void config_reload(h_config config_handle);
void config_save(h_config config_handle);
void config_apply(h_config config_handle);

const char* config_get_file_name(h_config config_handle);
ini_t* config_get_ini(h_config config_handle);
s_config_property_type_entry* config_get_property_type_map(h_config config_handle);
e_config_property_type
properties_map_get_type(s_config_property_type_entry* map, int section, int property);
void properties_map_set_type(
	s_config_property_type_entry* map,
	int section,
	int property,
	e_config_property_type type);
bool config_has_section(h_config config_handle, const char* section);
int32 config_get_or_default_int(
	h_config config_handle,
	const char* section,
	const char* property,
	int32 default_value);
float config_get_or_default_float(
	h_config config_handle,
	const char* section,
	const char* property,
	float default_value);
bool config_get_or_default_bool(
	h_config config_handle,
	const char* section,
	const char* property,
	bool default_value);
const char* config_get_or_default_string(
	h_config config_handle,
	const char* section,
	const char* property,
	const char* default_value);
void config_set_on_apply(h_config config_handle, t_on_apply_config on_apply_config);

bool try_parse_int(const char* string, int32* out);
bool try_parse_float(const char* string, float* out);
bool try_parse_bool(const char* string, bool* out);
const char* toggled_bool_string(const char* bool_string);
