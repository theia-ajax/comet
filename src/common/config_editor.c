#include "config_editor.h"

// #include <cimgui.h>

#include "Util.h"
#include "config.h"
#include "ini.h"

struct {
	h_config config_handle;
	bool has_unsaved_changes;
	// ImGuiTextFilter* filter;
} g_config_window;

void config_editor_initialize(void)
{
	// g_config_window.filter = ImGuiTextFilter_ImGuiTextFilter("");
}

void config_editor_shutdown(void)
{
	// ImGuiTextFilter_destroy(g_config_window.filter);
}

void config_editor_set_config(h_config config_handle)
{
	g_config_window.config_handle = config_handle;
	g_config_window.has_unsaved_changes = false;
}

void snake_case_to_label(const char* snake_string, char* out_label, size_t max_length)
{
	bool next_upper = true;

	const char* source_char = snake_string;
	char* dest_char = out_label;

	while ((dest_char - out_label) < (ptrdiff_t)max_length - 1 && *source_char) {
		if (*source_char == '_') {
			next_upper = true;
			*dest_char = ' ';
		} else {
			char next = *source_char;
			if (next_upper) {
				next_upper = false;
				next = toupper(next);
			}
			*dest_char = next;
		}
		source_char++;
		dest_char++;
	}
	*dest_char = '\0';
}

void config_editor_show_window()
{
	static const char* property_type_names[k_config_property_type_count] = {
		"string",
		"float",
		"integer",
		"boolean",
	};

	static bool show_config_editor = false;
#if 0
	if (igBegin("Config", &show_config_editor, ImGuiWindowFlags_None)) {
		if (HANDLE_IS_VALID(g_config_window.config_handle)) {
			ImGuiTextFilter_Draw(g_config_window.filter, "Search", 0.0f);
			ini_t* ini = config_get_ini(g_config_window.config_handle);
			s_config_property_type_entry* properties =
				config_get_property_type_map(g_config_window.config_handle);

			igPushItemFlag(ImGuiItemFlags_Disabled, !g_config_window.has_unsaved_changes);
			igSameLine(0, -1);
			if (igButton("Save", (ImVec2){0, 0})) {
				config_save(g_config_window.config_handle);
				g_config_window.has_unsaved_changes = false;
			}
			igPopItemFlag();

			int section_count = ini_section_count(ini);
			for (int section_index = 0; section_index < section_count; section_index++) {
				const char* section_name = ini_section_name(ini, section_index);
				int property_count = ini_property_count(ini, section_index);

				// If the section name passes the filter show the entire section
				bool show_entire_section =
					ImGuiTextFilter_PassFilter(g_config_window.filter, section_name, NULL);
				bool show_section = show_entire_section;

				if (!show_entire_section) {
					// If any of the property names pass the filter we will need to show the section
					for (int property_index = 0; property_index < property_count; property_index++)
					{
						const char* property_name =
							ini_property_name(ini, section_index, property_index);
						if (ImGuiTextFilter_PassFilter(g_config_window.filter, property_name, NULL))
						{
							show_section = true;
							break;
						}
					}
				}

				if (show_section) {
					igPushStyleColor_U32(ImGuiCol_Text, IM_COL32(200, 255, 255, 255));
					igText("%s", section_name);
					igPopStyleColor(1);

					for (int property_index = 0; property_index < property_count; property_index++)
					{
						const char* property_name =
							ini_property_name(ini, section_index, property_index);
						if (show_entire_section
							|| ImGuiTextFilter_PassFilter(
								g_config_window.filter, property_name, NULL))
						{
							int id = ((section_index & 0xFFFF) << 16) + (property_index & 0xFFFF);
							igPushID_Int(id);

							e_config_property_type property_type =
								properties_map_get_type(properties, section_index, property_index);

							{
								igSetNextItemWidth(80.0f);
								int property_type_index = (int)property_type;
								if (igCombo_Str_arr(
										"##property_type",
										&property_type_index,
										property_type_names,
										k_config_property_type_count,
										4))
								{
									property_type = (e_config_property_type)property_type_index;
									properties_map_set_type(
										properties, section_index, property_index, property_type);
								}
							}

							igSameLine(0, -1);

							fixed_buffer(label_buffer, 256);
							snake_case_to_label(property_name, label_buffer, label_buffer_size);

							const char* value_string =
								ini_property_value(ini, section_index, property_index);

							fixed_buffer(property_buffer, 256);
							strncpy(property_buffer, value_string, property_buffer_size);
							bool update_ini_property = false;

							igSetNextItemWidth(300.0f);

							switch (property_type) {
								case _config_property_type_integer:
									{
										int32 int_value;
										try_parse_int(property_buffer, &int_value);
										if (igInputInt(
												label_buffer,
												&int_value,
												1,
												10,
												ImGuiInputTextFlags_EnterReturnsTrue))
										{
											snprintf(
												property_buffer,
												property_buffer_size,
												"%d",
												int_value);
											update_ini_property = true;
										}
									}
									break;
								case _config_property_type_float:
									{
										float float_value;
										try_parse_float(property_buffer, &float_value);
										if (igInputFloat(
												label_buffer,
												&float_value,
												0.1f,
												1.0f,
												"%f",
												ImGuiInputTextFlags_EnterReturnsTrue))
										{
											snprintf(
												property_buffer,
												property_buffer_size,
												"%f",
												float_value);
											update_ini_property = true;
										}
									}
									break;
								case _config_property_type_bool:
									{
										bool bool_value;
										try_parse_bool(property_buffer, &bool_value);
										if (igCheckbox(label_buffer, &bool_value)) {
											snprintf(
												property_buffer,
												property_buffer_size,
												"%s",
												toggled_bool_string(property_buffer));
											update_ini_property = true;
										}
									}
									break;
								default:
								case _config_property_type_string:
									if (igInputText(
											label_buffer,
											property_buffer,
											property_buffer_size,
											ImGuiInputTextFlags_EnterReturnsTrue,
											NULL,
											NULL))
									{
										update_ini_property = true;
									}
									break;
							}

							if (update_ini_property) {
								ini_property_value_set(
									ini,
									section_index,
									property_index,
									property_buffer,
									(int)property_buffer_size);
								config_apply(g_config_window.config_handle);
								g_config_window.has_unsaved_changes = true;
							}

							igPopID();
						}
					}
				}

				igSeparator();
			}
		} else {
			igPushStyleColor_U32(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			igText("No config set.");
			igPopStyleColor(1);
		}
		igEnd();
	}
#endif
}
