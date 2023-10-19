#include "config.h"

#include <cimgui.h>

#include "ini.h"
#include "stb_ds.h"
#include "util.h"

const uint32 INVALID_HASH= (uint32)-1;

typedef struct s_config {
	char file_name[256];
	ini_t* ini;
	s_config_property_type_entry* property_type_map;
} s_config;

enum { k_maximum_configs= 64 };

DECLARE_DATA_ARRAY_NO_HANDLE(config, k_maximum_configs);

typedef struct s_handler {
	t_on_apply_config on_apply_config;
} s_handler;

DECLARE_DATA_ARRAY(handler, k_maximum_configs);

struct {
	DATA_ARRAY(config) entries;
	DATA_ARRAY(handler) handlers;
} g_config;

static h_config alloc_config(void);
static s_config* get_config(h_config config_handle);
static bool try_load_config(const char* file_name, s_config* out_config);
static const char* get_property_value(h_config config_handle, const char* section_name, const char* property_name);
static bool contains(const char** strings, int count, const char* search);
static uint32 hash_section_and_property(int section, int property);
static void build_property_map(s_config* config);

IMPLEMENT_DATA_ARRAY(config);
IMPLEMENT_DATA_ARRAY(handler);

void config_initialize(void)
{
	ZERO_STRUCT(&g_config);
}

void config_shutdown(void)
{
	for (int32 index= 0; index < DATA_ARRAY_CAPACITY(config); ++index) {
		h_config handle= HANDLE_CREATE_FROM_INDEX(config, index);
		config_destroy(handle);
	}
}

h_config config_load_file(const char* file_name)
{
	h_config result_handle= INVALID_HANDLE;
	s_config new_config;
	if (try_load_config(file_name, &new_config)) {
		result_handle= alloc_config();
		s_config* config= get_config(result_handle);
		*config= new_config;
		build_property_map(config);
	}
	return result_handle;
}

void config_destroy(h_config config_handle)
{
	s_config* config= config_data_array_get(&g_config.entries, config_handle);
	ini_destroy(config->ini);
	hmfree(config->property_type_map);
	ZERO_STRUCT(config);
}

void config_reload(h_config config_handle)
{
	s_config* config= get_config(config_handle);
	ini_destroy(config->ini);
	if (try_load_config(config->file_name, config)) {
		build_property_map(config);
	}
}

void config_save(h_config config_handle)
{
	s_config* config= get_config(config_handle);
	fixed_buffer(out_buffer, 4096);
	int end= ini_save(config->ini, out_buffer, (int)out_buffer_size);
	if (end != 0) {
		FILE* file= fopen(config->file_name, "w");
		if (file != NULL) {
			fprintf(file, "%s", out_buffer);
			fclose(file);
		}
	}
}

void config_apply(h_config config_handle)
{
	s_handler* handler= handler_data_array_get(&g_config.handlers, HANDLE_CONVERT_TO(handler, config_handle));
	if (handler->on_apply_config != NULL) {
		handler->on_apply_config(config_handle);
	}
}

void config_set_on_apply(h_config config_handle, t_on_apply_config on_apply_config)
{
	s_handler* handler= handler_data_array_get(&g_config.handlers, HANDLE_CONVERT_TO(handler, config_handle));
	handler->on_apply_config= on_apply_config;
}

static bool try_load_config(const char* file_name, s_config* out_config)
{
	size_t length= 0;
	char* config_file_buffer= read_file("config.ini", &length);

	bool result= false;

	if (config_file_buffer) {
		ini_t* ini= ini_load(config_file_buffer, NULL);

		if (ini) {
			ZERO_STRUCT(out_config);
			strncpy(out_config->file_name, file_name, ARRAY_COUNT(out_config->file_name));
			out_config->ini= ini;
			result= true;
		}

		free(config_file_buffer);
	}

	return result;
}

const char* config_get_file_name(h_config config_handle)
{
	return get_config(config_handle)->file_name;
}

ini_t* config_get_ini(h_config config_handle)
{
	return get_config(config_handle)->ini;
}

s_config_property_type_entry* config_get_property_type_map(h_config config_handle)
{
	return get_config(config_handle)->property_type_map;
}

e_config_property_type properties_map_get_type(s_config_property_type_entry* map, int section, int property)
{
	e_config_property_type result= _config_property_type_string;
	uint32 key= hash_section_and_property(section, property);
	if (key != INVALID_HASH) {
		result= hmget(map, key);
	}
	return result;
}

void properties_map_set_type(s_config_property_type_entry* map, int section, int property, e_config_property_type type)
{
	uint32 key= hash_section_and_property(section, property);
	if (key != INVALID_HASH) {
		hmput(map, key, type);
	}
}

bool config_has_section(h_config config_handle, const char* section)
{
	s_config* config= get_config(config_handle);
	return ini_find_section(config->ini, section, -1) != INI_NOT_FOUND;
}

int32 config_get_or_default_int(h_config config_handle, const char* section, const char* property, int32 default_value)
{
	int32 result= default_value;
	const char* property_value= get_property_value(config_handle, section, property);

	(property_value != NULL) && try_parse_int(property_value, &result);

	return result;
}

float config_get_or_default_float(
	h_config config_handle,
	const char* section,
	const char* property,
	float default_value)
{
	float result= default_value;
	const char* property_value= get_property_value(config_handle, section, property);

	(property_value != NULL) && try_parse_float(property_value, &result);

	return result;
}

bool config_get_or_default_bool(h_config config_handle, const char* section, const char* property, bool default_value)
{
	bool result= default_value;
	const char* property_value= get_property_value(config_handle, section, property);

	(property_value != NULL) && try_parse_bool(property_value, &result);

	return result;
}

const char* config_get_or_default_string(
	h_config config_handle,
	const char* section,
	const char* property,
	const char* default_value)
{
	const char* result= default_value;
	const char* property_value= get_property_value(config_handle, section, property);

	if (property_value != NULL) {
		result= property_value;
	}

	return result;
}

static h_config alloc_config(void)
{
	h_config config_handle= config_data_array_alloc(&g_config.entries);
	handler_data_array_alloc(&g_config.handlers);
	return config_handle;
}

static s_config* get_config(h_config config_handle)
{
	return config_data_array_get(&g_config.entries, config_handle);
}

static const char* get_property_value(h_config config_handle, const char* section_name, const char* property_name)
{
	s_config* config= get_config(config_handle);
	const char* result= NULL;
	int section= ini_find_section(config->ini, section_name, -1);
	if (section != INI_NOT_FOUND) {
		int property= ini_find_property(config->ini, section, property_name, -1);
		if (property != INI_NOT_FOUND) {
			result= ini_property_value(config->ini, section, property);
		}
	}
	return result;
}

bool try_parse_int(const char* string, int32* out)
{
	ASSERT(out != NULL);
	bool result= false;
	char* end;
	long long_value= strtol(string, &end, 10);
	if (end != string) {
		if (out) *out= (int32)long_value;
		result= true;
	}
	return result;
}

// TODO: locale independence
bool try_parse_float(const char* string, float* out)
{
	ASSERT(out != NULL);
	bool result= false;
	char* end;
	float float_value= strtof(string, &end);
	if (end != string) {
		if (out) *out= float_value;
		result= true;
	}
	return result;
}

static const char* true_strings[]= {
	"true",
	"yes",
	"on",
};

static const char* false_strings[]= {
	"false",
	"no",
	"off",
};

bool try_parse_bool(const char* string, bool* out)
{
	bool result= false;
	bool parsed_value= false;
	int32 int_value= 0;
	if (contains(true_strings, ARRAY_COUNT(true_strings), string)) {
		result= true;
		parsed_value= true;
	} else if (contains(false_strings, ARRAY_COUNT(false_strings), string)) {
		result= true;
		parsed_value= false;
	} else if (try_parse_int(string, &int_value)) {
		result= true;
		parsed_value= (int_value != 0);
	}

	if (result && out) {
		*out= parsed_value;
	}
	return result;
}

static int index_of(const char** strings, int count, const char* search)
{
	int result= -1;
	for (int index= 0; index < count; index++) {
		if (strncasecmp(strings[index], search, strlen(search)) == 0) {
			result= index;
			break;
		}
	}
	return result;
}

const char* toggled_bool_string(const char* bool_string)
{
	const char* result= NULL;
	int true_index= index_of(true_strings, ARRAY_COUNT(true_strings), bool_string);
	if (true_index >= 0) {
		result= false_strings[true_index];
	}
	int false_index= index_of(false_strings, ARRAY_COUNT(false_strings), bool_string);
	if (false_index >= 0) {
		result= true_strings[false_index];
	}
	return result;
}

static bool contains(const char** strings, int count, const char* search)
{
	return index_of(strings, count, search) >= 0;
}

static uint32 hash_section_and_property(int section, int property)
{
	uint32 result= INVALID_HASH;
	if (section >= 0 && property >= 0) {
		result= ((section & 0xFFFF) << 16) | (property & 0xFFFF);
	}
	return result;
}

static void build_property_map(s_config* config)
{
	hmfree(config->property_type_map);
	hmdefault(config->property_type_map, _config_property_type_string);

	ini_t* ini= config->ini;
	int section_count= ini_section_count(ini);
	for (int section_index= 0; section_index < section_count; section_index++) {
		int property_count= ini_property_count(ini, section_index);
		for (int property_index= 0; property_index < property_count; property_index++) {
			uint32 property_hash= hash_section_and_property(section_index, property_index);
			const char* property_value= ini_property_value(ini, section_index, property_index);

			e_config_property_type property_type= _config_property_type_string;
			int32 int_value;
			float float_value;
			bool is_number= try_parse_int(property_value, &int_value) | try_parse_float(property_value, &float_value);

			if (is_number) {
				float int_float_value= (float)int_value;
				if (int_float_value == float_value) {
					const char* c= property_value;
					property_type= _config_property_type_integer;
					
					// as an additional check if both values are equal check for float specific characters in the string
					while (*c) {
						if (*c == '.' || *c == 'e') {
							property_type= _config_property_type_float;
							break;
						}
						c++;
					}
				} else {
					property_type= _config_property_type_float;
				}
			} else if (try_parse_bool(property_value, NULL)) {
				property_type= _config_property_type_bool;
			}

			hmput(config->property_type_map, property_hash, property_type);
		}
	}
}