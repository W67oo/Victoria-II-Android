

#include <fstream>
#include "Parsing/parsing.hpp"
#include <unordered_set>
#include <iostream>

static std::unordered_set<std::string> dcon_ids{};

const std::string src_newline = "\\n";

const std::string container_1_name = "container_1";
const std::string container_2_name = "container_2";
const std::string load_record_name = "record";
const std::string final_report_name = "report";
const std::string dcon_obj_instance_name = "obj";
const std::string vector_obj_name_1 = "vec_1";
const std::string vector_obj_name_2 = "vec_2";
const std::string dcon_obj_id = "obj_id";
const std::string dcon_array_index_name = "arr_index";
const std::string for_loop_iterator_name_1 = "i";
const std::string bitfield_type = "bitfield_type";


std::string get_comment_header(const std::string& dcon_file_path) {
	return "// This file was automatically generated from: " + dcon_file_path + "\n" +
		"// EDIT AT YOUR OWN RISK; all changes will be lost upon regeneration\n" +
		"// NOT SUITABLE FOR USE IN CRITICAL SOFTWARE WHERE LIVES OR LIVELIHOODS DEPEND ON THE CORRECT OPERATION\n";
}

std::string get_start_namespace() {
	return "namespace dcon\n";
}

std::string add_indent(uint32_t indent) {
	return std::string(indent, '\t');
}

std::string get_start_curly_bracket(uint32_t indent) {
	return add_indent(indent) + "{\n";
}
std::string get_end_curly_bracket(uint32_t indent) {
	return add_indent(indent) + "}\n";
}

std::string get_generic_for_loop_start(const std::string& end, uint32_t indent) {
	return add_indent(indent) + "for(size_t " + for_loop_iterator_name_1 + " = 0;i<" + end + ";i++)\n";
}

std::string get_dcon_typename(const std::string object_name) {
	std::string result{ };
	if (!object_name.starts_with("dcon::")) {
		result += "dcon::";
	}
	result += object_name;
	return result;
}
std::string cast_expression_to_index_type(const std::string& type, const std::string& expression) {
	return type + "(" + expression + ")";
}

std::string data_resetter_get_foreach_obj_clear_data_loop(const relationship_object_def& obj_def, const property_def& prop_def, uint32_t& indent) {
	std::string result;
	result.reserve(300);
	result += add_indent(indent) + "for(auto "  + dcon_obj_instance_name + " : " + container_1_name + ".in_" + obj_def.name + ")\n";
	result += get_start_curly_bracket(indent);
	indent++;
	switch (prop_def.type) {
	case property_type::special_vector: {
		result += add_indent(indent) + dcon_obj_instance_name + "."  + "get_" + prop_def.name + "().clear();\n";
		break;
	}
	case property_type::array_bitfield:
	case property_type::array_other: {
		// iterate over array and reset every index
		result += get_generic_for_loop_start(container_1_name + "." + obj_def.name + "_get_" + prop_def.name + "_size()", indent);
		result += get_start_curly_bracket(indent);
		indent++;
		if (dcon_ids.contains(prop_def.array_index_type)) {
			result += add_indent(indent) + get_dcon_typename(prop_def.array_index_type) + " " + dcon_array_index_name + "{" + get_dcon_typename(prop_def.array_index_type) + "::value_base_t(" + for_loop_iterator_name_1 + ")};\n";
		}
		else {
			result += add_indent(indent) + "auto " + dcon_array_index_name + " = " + cast_expression_to_index_type(prop_def.array_index_type, for_loop_iterator_name_1) + ";\n";
		}
		const std::string& data_type = (prop_def.type == property_type::array_bitfield ? "bool" : prop_def.data_type);
		result += add_indent(indent) + dcon_obj_instance_name + "." + "set_" + prop_def.name + "(" + dcon_array_index_name + ", " + data_type + "{}" + ");\n";
		indent--;
		result += get_end_curly_bracket(indent);
		break;
	}
	case property_type::bitfield: {
		result += add_indent(indent) + dcon_obj_instance_name + "." + "set_" + prop_def.name + "(bool{}" + ");\n";
		break;
	}
	// objects which can be defaulted by simply default constructing a new one in its place
	default: {
		result += add_indent(indent) + dcon_obj_instance_name + "." + "set_" + prop_def.name + "(" + prop_def.data_type + "{}" + ");\n";
		break;
	}

	}
	indent--;
	result += get_end_curly_bracket(indent);
	return result;

}

std::string data_resetter_set_dcon_size_zero(const relationship_object_def& obj_def, uint32_t indent) {
	return add_indent(indent) + container_1_name + "." + obj_def.name + "_resize(0);\n";
}

std::string add_continue(uint32_t indent) {
	return add_indent(indent) + "continue;\n";
}

std::string get_else_statement(uint32_t indent) {
	return add_indent(indent) + "else\n";
}

std::string get_if_statement(const std::string& expression, uint32_t indent) {
	return add_indent(indent) + "if(" + expression + ")\n";
}


std::string get_dcon_typename_with_id(const std::string object_name) {
	std::string result{ };
	if (!object_name.starts_with("dcon::")) {
		result += "dcon::";
	}
	result += object_name;
	if (!object_name.ends_with("_id")) {
		result += "_id";
	}
	return result;
}


std::string add_compare_function_call(const std::string& value_1, const std::string& value_2) {
	return "compare_values(" + value_1 + ", " + value_2 + ")";
}

std::string get_dcon_array_element(const std::string& object_name, const property_def& property, const std::string& container_name, bool integer_index) {
	if (integer_index) {
		return container_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ", " + cast_expression_to_index_type(property.array_index_type, dcon_array_index_name) + ")";
	}
	else {
		return container_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ", " + dcon_array_index_name + ")";
	}
}

std::string get_dcon_object_variable(const std::string& container, const std::string& object_name, const std::string& property_name, const std::string& dcon_object_variable) {
	return container + "." + object_name + "_get_" + property_name + "(" + dcon_object_variable + ")";
}

std::string get_dcon_object_variable_w_index(const std::string& container, const std::string& object_name, const std::string& property_name, const std::string& dcon_object_variable, const std::string& index_var) {
	return container + "." + object_name + "_get_" + property_name + "(" + dcon_object_variable + ", " + index_var + ")";
}

std::string get_dcon_array_size(const std::string& object_name, const property_def& property, const std::string& container_name) {
	return container_name + "." + object_name + "_get_" + property.name + "_size()";
}
std::string get_dcon_array_element_if_comparison(const std::string& object_name, const property_def& property, uint32_t indent, bool integer_index) {
	if (integer_index) {
		std::string dcon_expression = object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ", " + cast_expression_to_index_type(property.array_index_type, dcon_array_index_name) + ")";
		return add_indent(indent) + "if(!" +  add_compare_function_call( container_1_name + "." + dcon_expression,  container_2_name + "." + dcon_expression) + ")\n";
	}
	else {
		std::string dcon_expression = object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ", " + dcon_array_index_name + ")";
		return add_indent(indent) + "if(!" + add_compare_function_call(container_1_name + "." + dcon_expression, container_2_name + "." + dcon_expression)  +")\n";
	}
}

std::string get_dcon_array_size_if_comparison(const std::string& object_name, const property_def& property, uint32_t indent) {
	return add_indent(indent) + "if(!" + add_compare_function_call( get_dcon_array_size(object_name, property, container_1_name), get_dcon_array_size(object_name, property, container_2_name)) + ")\n";
}


std::string get_property_if_comparison(const std::string& object_name, const property_def& property, uint32_t indent) {
	std::string dcon_expression = object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ")";
	return add_indent(indent) + "if(!" + add_compare_function_call(container_1_name + "." + dcon_expression, container_2_name + "." + dcon_expression) + ")\n";
}

std::string get_variable_if_comparison(const std::string& var_1, const std::string& var_2, uint32_t indent) {
	return add_indent(indent) + "if(!" + add_compare_function_call(var_1, var_2) + ")\n";
}

std::string get_relationship_index_if_comparison(const relationship_object_def& object, uint32_t indent, uint8_t index_to_check) {
	std::string dcon_expression = object.name + "_get_" + object.indexed_objects[index_to_check].property_name + "(" + dcon_obj_instance_name + ")";
	return add_indent(indent) + "if(!" + add_compare_function_call(container_1_name + "." + dcon_expression, container_2_name + "." + dcon_expression) + ")\n";
}

std::string get_relationship_composite_index_if_comparison(const relationship_object_def& object, uint32_t indent, uint8_t index_to_check, const std::string& composite_index_var) {
	std::string dcon_expression = object.name + "_get_" + object.indexed_objects[index_to_check].property_name + "(" + dcon_obj_instance_name + ", " + composite_index_var + ")";
	return add_indent(indent) + "if(!" + add_compare_function_call(container_1_name + "." + dcon_expression, container_2_name + "." + dcon_expression) + ")\n";
}

std::string source_get_includes(const std::string& header_filename) {
	return "#include \"dcon_generated.hpp\"\n#include \"" + header_filename + "\"\n#include \"debug_string_convertions.hpp\"\n";
}

std::string source_get_initial_var_declariations(uint32_t indent) {
	// reserve stringbuffer of 100 MB for the entire report (it may be quite big if there are alot of mismatches
	return add_indent(indent) + "std::string " + final_report_name + ";\n" +
		   add_indent(indent) + final_report_name + ".reserve(104857600);\n";
}
std::string source_get_partial_var_declariations(uint32_t indent) {
	// reserve stringbuffer of 10 MB for the part of the report (it may be quite big if there are alot of mismatches
	return add_indent(indent) + "std::string " + final_report_name + ";\n" +
		add_indent(indent) + final_report_name + ".reserve(10485760);\n";
}
std::string header_get_forward_declarations() {
	return "struct load_record;\nclass data_container;\n";
}
std::string get_oos_reporter_function_signature() {
	return "std::string generate_oos_report(const dcon::data_container& " + container_1_name + ", const dcon::data_container& " + container_2_name + ", const dcon::load_record& "  + load_record_name +  ")";
}
std::string get_oos_reporter_subfunction_signature(std::string object_name) {
	return "std::string generate_oos_report_" + object_name + "(const dcon::data_container& " + container_1_name + ", const dcon::data_container& " + container_2_name + ", const dcon::load_record& "  + load_record_name +")";
}
std::string call_oos_reporter_subfunction(uint32_t indent, std::string object_name) {
	return add_indent(indent) + final_report_name + " += generate_oos_report_" + object_name + "(" + container_1_name + ", " + container_2_name + ", " + load_record_name + ");\n";
}

std::string get_data_reset_function_signature() {
	return "void reset_data(dcon::data_container& " + container_1_name + ", const dcon::load_record& " + load_record_name + ")";
}

std::string get_data_reset_subfunction_signature(std::string object_name) {
	return "void reset_data_" + object_name + "(dcon::data_container& " + container_1_name + ", const dcon::load_record& " + load_record_name + ")";
}

std::string call_data_reset_subfunction(uint32_t indent, std::string object_name) {
	return add_indent(indent) + "reset_data_" + object_name + "(" + container_1_name + ", " + load_record_name +  ");\n";
}

std::string get_semicolon_newline() {
	return ";\n";
}

std::string return_report(uint32_t indent) {
	return add_indent(indent) + "return " + final_report_name + get_semicolon_newline();
}

std::string get_load_record_if_statement(const std::string& record_member, uint32_t indent) {
	return get_if_statement(load_record_name + "." + record_member, indent);
}
std::string get_neg_load_record_if_statement(const std::string& record_member, uint32_t indent) {
	return get_if_statement("!" + load_record_name + "." + record_member, indent);
}



std::string get_compare_function(uint32_t indent) {
	return add_indent(indent) + "template<typename T>\n" +
		add_indent(indent) + "bool compare_values(const T& val_1, const T& val_2)\n" +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + "if constexpr(std::is_trivially_copyable<T>::value)\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return std::memcmp(&val_1, &val_2, sizeof(T)) == 0;\n" +
		get_end_curly_bracket(indent + 1) +
		add_indent(indent + 1) + "else\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return val_1 == val_2;\n" +
		get_end_curly_bracket(indent + 1) +
		get_end_curly_bracket(indent);
}


std::string get_string_template_function_definition(uint32_t indent) {
	return add_indent(indent) + "template<typename T>\n" +
		add_indent(indent) + "std::string get_string(T input)\n" +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + "if constexpr(std::is_same<T, bool>::value)\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return input ? \"true\" : \"false\";\n" +
		get_end_curly_bracket(indent + 1) +
		add_indent(indent + 1) + "else if constexpr(requires{ sys::to_debug_string(input); })\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return sys::to_debug_string(input);\n" +
		get_end_curly_bracket(indent + 1) +
		add_indent(indent + 1) + "else if constexpr(requires{ std::to_string(input); } )\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return std::to_string(input);\n" +
		get_end_curly_bracket(indent + 1) +
		add_indent(indent + 1) + "else if constexpr(requires{ std::to_string(input.value); })\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return std::to_string(input.value);\n" +
		get_end_curly_bracket(indent + 1) +
		add_indent(indent + 1) + "else\n" +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "return \"\";\n" +
		get_end_curly_bracket(indent + 1) +
		get_end_curly_bracket(indent);

}

std::string get_string_func_call(const std::string& variable) {
	return "get_string(" + variable + ")";
}


std::string append_to_final_report(const std::string& to_append, uint32_t indent, bool header) {
	if(header) {
		return add_indent(indent) + final_report_name + " += std::string(" + to_append + ") + \"\\n\";\n";
	}
	else {
		return add_indent(indent) + final_report_name + " += \"\\t\" + std::string(" + to_append + ") + \"\\n\";\n";
	}
}

std::string get_report_object_header(const std::string& object_name, uint32_t indent) {
	std::string result { object_name };
	for (uint32_t i = 0; i < result.size(); i++) {
		result[i] = std::toupper(result[i]);
	}
	return append_to_final_report("\"" + result + "\"", indent, true);
}

std::string get_object_size_check(const std::string& object_name, uint32_t indent) {
	return add_indent(indent) + "if(!" + add_compare_function_call(container_1_name + "." + object_name + "_size()", container_2_name + "." + object_name + "_size()") + ")\n" +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + "std::string cont_1_size = std::to_string(" + container_1_name + "." + object_name + "_size());\n" +
		add_indent(indent + 1) + "std::string cont_2_size = std::to_string(" + container_2_name + "." + object_name + "_size());\n" +
		append_to_final_report("\"Size mismatch in object: 1st size: \" + cont_1_size + \" 2nd size: \" + cont_2_size", indent + 1, false) +
		get_end_curly_bracket(indent);

}

std::string get_dconarray_size_check(const std::string& object_name, const property_def& property, uint32_t indent) {
	return get_dcon_array_size_if_comparison(object_name, property, indent) +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + "std::string cont_1_size = std::to_string(" + get_dcon_array_size(object_name, property, container_1_name) + ");\n" +
		add_indent(indent + 1) + "std::string cont_2_size = std::to_string(" + get_dcon_array_size(object_name, property, container_2_name) + "); \n" +
		append_to_final_report("\"Size mismatch in dcon array property: " + property.name + ", 1st size: \" + cont_1_size + \" 2nd size: \" + cont_2_size", indent + 1, false) +
		get_end_curly_bracket(indent);
}

std::string get_specialvector_size_check(const std::string& object_name, const property_def& property , uint32_t indent) {
	return add_indent(indent) + "if(!" + add_compare_function_call(vector_obj_name_1 + "." + "size()", vector_obj_name_2 + "." + "size()") + ")\n" +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + "std::string cont_1_size = std::to_string(" + vector_obj_name_1 + "." + "size());\n" +
		add_indent(indent + 1) + "std::string cont_2_size = std::to_string(" + vector_obj_name_2 + "." + "size());\n" +
		append_to_final_report("\"Size mismatch in vector property: " + property.name + ", 1st size: \" + cont_1_size + \" 2nd size: \" + cont_2_size", indent + 1, false) +
		get_end_curly_bracket(indent);

}

std::string get_dconarray_items_check(const std::string& object_name, const property_def& property, uint32_t& indent) {
	std::string result{};
	// we dont expect the total string to be larger than this, so pre-allocate
	result.reserve(360);

	result += add_indent(indent) + "uint32_t min_arr_size = uint32_t(std::min(" + get_dcon_array_size(object_name, property, container_1_name) + ", " + get_dcon_array_size(object_name, property, container_2_name) + "));\n" +
		add_indent(indent) + "for(uint32_t j = 0;j < min_arr_size;j++)\n" +
		get_start_curly_bracket(indent);
	bool integer_index = false;
	if (dcon_ids.contains(property.array_index_type)) {
		result += add_indent(indent + 1) + get_dcon_typename(property.array_index_type) + " " + dcon_array_index_name + "{" + get_dcon_typename(property.array_index_type) + "::value_base_t(j)};\n";
	}
	else {
		result += add_indent(indent + 1) + "auto " + dcon_array_index_name + " = j;\n";
		integer_index = true;
	}

	if (dcon_ids.contains(property.data_type)) {
		result += add_indent(indent + 1) + get_dcon_typename(property.data_type) + " cont_1_arr_val = " + get_dcon_array_element(object_name, property, container_1_name, integer_index) + ";\n" +
			add_indent(indent + 1) + get_dcon_typename(property.data_type) + " cont_2_arr_val = " + get_dcon_array_element(object_name, property, container_2_name, integer_index) + ";\n";
	}
	else {
		result += add_indent(indent + 1) + "const auto& cont_1_arr_val = " + get_dcon_array_element(object_name, property, container_1_name, integer_index) + ";\n" +
			add_indent(indent + 1) + "const auto& cont_2_arr_val = " + get_dcon_array_element(object_name, property, container_2_name, integer_index) + ";\n";
	}
	result += get_variable_if_comparison("cont_1_arr_val", "cont_2_arr_val", indent + 1) +
		get_start_curly_bracket(indent + 1) +
		add_indent(indent + 2) + "std::string cont_1_arr_str = " + get_string_func_call("cont_1_arr_val") + ";\n" +
		add_indent(indent + 2) + "std::string cont_2_arr_str = " + get_string_func_call("cont_2_arr_val") + ";\n" +
		append_to_final_report("\"ID: \" + std::to_string(" + dcon_obj_id + ") + \" property: \" + \"" + property.name + ", array index: \" + std::to_string(j) + \": \" + cont_1_arr_str + \", \" + cont_2_arr_str", indent + 2, false) +
		get_end_curly_bracket(indent + 1);

	result += get_end_curly_bracket(indent);
	return result;
}


std::string get_specialvector_items_check(const std::string& object_name, const property_def& property, uint32_t& indent) {
	std::string result{};
	// we dont expect the total string to be larger than this, so pre-allocate
	result.reserve(360);

	result += add_indent(indent) + "uint32_t min_arr_size = uint32_t(std::min(" + vector_obj_name_1 + "." + "size(), " + vector_obj_name_2 + "." + "size()));\n" +
		add_indent(indent) + "for(uint32_t j = 0;j < min_arr_size;j++)\n" +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + "if(!" + add_compare_function_call(vector_obj_name_1 + "[j]", vector_obj_name_2 + "[j]") + ")\n" +
		get_start_curly_bracket(indent + 1);
	if (dcon_ids.contains(property.data_type)) {
		result += add_indent(indent + 2) + get_dcon_typename(property.data_type) + " cont_1_arr_val = " + vector_obj_name_1 + "[j];\n" +
			add_indent(indent + 2) + get_dcon_typename(property.data_type) + " cont_2_arr_val = " + vector_obj_name_2 + "[j];\n";
	}
	else {
		result += add_indent(indent + 2) + "const auto& cont_1_arr_val = " + vector_obj_name_1 + "[j];\n" +
			add_indent(indent + 2) + "const auto& cont_2_arr_val = " + vector_obj_name_2 + "[j];\n";
	}
	result += add_indent(indent + 2) + "std::string cont_1_arr_str = " + get_string_func_call("cont_1_arr_val") + ";\n" +
		add_indent(indent + 2) + "std::string cont_2_arr_str = " + get_string_func_call("cont_2_arr_val") + ";\n" +
		append_to_final_report("\"ID: \" + std::to_string(" + dcon_obj_id + ") + \" property: \" + \"" + property.name + ", vector index: \" + std::to_string(j) + \": \" + cont_1_arr_str + \", \" + cont_2_arr_str", indent + 2, false);

	result += get_end_curly_bracket(indent + 1);
	result += get_end_curly_bracket(indent);
	return result;
}


std::string get_object_property_check(const std::string& object_name, const property_def& property, uint32_t& indent) {
	std::string result{};

	if (property.type == property_type::other || property.type == property_type::bitfield) {
		// handle references to other dcon ids. They must be casted to the "basic" dcon type instead of the "fat" type to compare properly
		if (dcon_ids.contains(property.data_type)) {
			result += add_indent(indent) + get_dcon_typename(property.data_type) + " cont_1_val = " + container_1_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n" +
				add_indent(indent) + get_dcon_typename(property.data_type) + " cont_2_val = " + container_2_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n";
		}
		// handle std::array fields as if they were specialvectors, and return early
		else if (property.data_type.starts_with("std::array<")) {
			result +=
				add_indent(indent) + "const auto& " + vector_obj_name_1 + " = " + container_1_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n" +
				add_indent(indent) + "const auto& " + vector_obj_name_2 + " = " + container_2_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n" +
				get_specialvector_size_check(object_name, property, indent) +
				get_specialvector_items_check(object_name, property, indent);
			return result;
		}
		else {
			result += add_indent(indent) + "const auto& cont_1_val = " + container_1_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n" +
				add_indent(indent) + "const auto& cont_2_val = " + container_2_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n";
		}

		result += get_variable_if_comparison("cont_1_val", "cont_2_val", indent) +
			get_start_curly_bracket(indent) +
			add_indent(indent + 1) + "std::string cont_1_str = " + get_string_func_call("cont_1_val") + ";\n" +
			add_indent(indent + 1) + "std::string cont_2_str = " + get_string_func_call("cont_2_val") + ";\n" +
			append_to_final_report("\"ID: \" + std::to_string(" + dcon_obj_id + ") + \" property: \" + \"" + property.name + ": \" + cont_1_str + \", \" + cont_2_str", indent + 1, false) +
			get_end_curly_bracket(indent);
	}
	else if (property.type == property_type::special_vector) {
		result +=
			add_indent(indent) + "auto " + vector_obj_name_1 + " = " + container_1_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n" +
			add_indent(indent) + "auto " + vector_obj_name_2 + " = " + container_2_name + "." + object_name + "_get_" + property.name + "(" + dcon_obj_instance_name + ");\n" +
			get_specialvector_size_check(object_name, property, indent) +
			get_specialvector_items_check(object_name, property, indent);

	}
	else if (property.type == property_type::array_other || property.type == property_type::array_bitfield) {
		result += get_dconarray_size_check(object_name, property, indent) +
			get_dconarray_items_check(object_name, property, indent);
	}
	else {
		result += add_indent(indent) + "// Unsupported type id " + std::to_string(int(property.type)) + "\n";
	}
	return result;




}

std::string get_dcon_object_loop_start(const std::string& object_name, uint32_t& indent) {
	std::string ret = add_indent(indent) + "uint32_t size = std::min(" + container_1_name + "." + object_name + "_size(), " + container_2_name + "." + object_name + "_size());\n" +
		add_indent(indent) + "for(uint32_t i = 0; i < size;i++)\n" +
		get_start_curly_bracket(indent) +
		add_indent(indent + 1) + get_dcon_typename_with_id(object_name) + " " + dcon_obj_instance_name + "{" + get_dcon_typename_with_id(object_name) + "::value_base_t(i)};\n" +
		add_indent(indent + 1) + "auto " + dcon_obj_id + " = " + dcon_obj_instance_name + ".value;\n";
	indent++;
	return ret;

}

std::string get_dcon_relations_check(const relationship_object_def& object, uint32_t indent) {
	std::string result{};
	result.reserve(500);
	for (uint32_t i = 0; i < object.indexed_objects.size(); i++) {
		result += get_load_record_if_statement(object.name + "_" + object.indexed_objects[i].property_name, indent) +
			get_start_curly_bracket(indent);
		if (object.indexed_objects[i].multiplicity > 1) {

			result += add_indent(indent + 1) + "for(uint32_t k = 0;k < " + std::to_string(object.indexed_objects[i].multiplicity) + ";k++)\n" +
				get_start_curly_bracket(indent + 1) +
				get_relationship_composite_index_if_comparison(object, indent + 2, i, "k") +
				get_start_curly_bracket(indent + 2) +
				add_indent(indent + 3) + "auto cont_1_val = std::to_string(" + get_dcon_object_variable_w_index(container_1_name, object.name, object.indexed_objects[i].property_name, dcon_obj_instance_name, "k") + ".value);\n" +
				add_indent(indent + 3) + "auto cont_2_val = std::to_string(" + get_dcon_object_variable_w_index(container_2_name, object.name, object.indexed_objects[i].property_name, dcon_obj_instance_name, "k") + ".value);\n" +
				append_to_final_report("\"ID: \" + std::to_string(" + dcon_obj_id + ") + \" relation: \" + \"" + object.indexed_objects[i].property_name + ": \" + cont_1_val + \", \" + cont_2_val", indent + 3, false) +
				get_end_curly_bracket(indent + 2) +
				get_end_curly_bracket(indent + 1);
		}
		else {
			result += get_relationship_index_if_comparison(object, indent + 1, i) +
				get_start_curly_bracket(indent + 1) +
				add_indent(indent + 2) + "auto cont_1_val = std::to_string(" + get_dcon_object_variable(container_1_name, object.name, object.indexed_objects[i].property_name, dcon_obj_instance_name) + ".value);\n" +
				add_indent(indent + 2) + "auto cont_2_val = std::to_string(" + get_dcon_object_variable(container_2_name, object.name, object.indexed_objects[i].property_name, dcon_obj_instance_name) + ".value);\n" +
				append_to_final_report("\"ID: \" + std::to_string(" + dcon_obj_id + ") + \" relation: \" + \"" + object.indexed_objects[i].property_name + ": \" + cont_1_val + \", \" + cont_2_val", indent + 2, false) +
				get_end_curly_bracket(indent + 1);
		}
		result += get_end_curly_bracket(indent);
	}
	return result;

}


int main(int argc, char* argv[])
{
	if (argc < 4) {
		printf("[1]: PROJECT NAME, [2]: DCON DEFINITION FILE, [3]: CPP OUTPUT FILE, [4]: HPP OUTPUT FILE, [5]: OUTPUT FOLDER");
		return 1;
	}

	const std::string project_name = argv[1];
	const std::string project_prefix = project_name + "_";

	std::fstream input_file;
	std::string input_file_name(argv[2]);
	input_file.open(argv[2], std::ios::in);

	const std::string source_file_name = argv[3];
	const std::string header_file_name = argv[4];
	const std::string dest_folder = argv[5];
	const std::string source_path = dest_folder + "/" + source_file_name;
	const std::string header_path = dest_folder + "/" + header_file_name;

	error_record err(input_file_name);

	if (!input_file.is_open()) {
		err.add(row_col_pair{ 0, 0 }, 1000, "Could not open input file");
		std::cout << err.accumulated;
		return -1;
	}

	std::string file_contents((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());

	file_def parsed_file = parse_file(file_contents.c_str(), file_contents.c_str() + file_contents.length(), err);

	for (auto obj : parsed_file.relationship_objects) {
		dcon_ids.insert(obj.name + "_id");
		dcon_ids.insert("dcon::" + obj.name + "_id");
	}
	for (auto obj : parsed_file.extra_ids) {
		dcon_ids.insert(obj.name);
		dcon_ids.insert("dcon::" + obj.name);
	}

	input_file.close();


	std::string source_output{};
	// reserve 1 MB for the source file, shouldn't be bigger than that
	source_output.reserve(1048576);
	std::string header_output;

	header_output += get_comment_header(input_file_name);
	// start namespace
	header_output += get_start_namespace();
	header_output += get_start_curly_bracket(0);
	header_output += header_get_forward_declarations();
	header_output += get_oos_reporter_function_signature();
	header_output += get_semicolon_newline();
	header_output += get_data_reset_function_signature();
	header_output += get_semicolon_newline();
	// end namespace
	header_output += get_end_curly_bracket(0);

	source_output += get_comment_header(input_file_name);
	source_output += source_get_includes(header_file_name);
	// start namespace
	source_output += get_start_namespace();
	source_output += get_start_curly_bracket(0);

	source_output += get_string_template_function_definition(0);
	source_output += get_compare_function(0);

	for (auto& dcon_obj : parsed_file.relationship_objects) {
		uint32_t indent = 0;
		source_output += get_oos_reporter_subfunction_signature(dcon_obj.name);
		source_output += get_start_curly_bracket(indent);
		indent++;

		source_output += source_get_partial_var_declariations(indent);

		source_output += get_load_record_if_statement(dcon_obj.name, indent);
		source_output += get_start_curly_bracket(indent);
		indent++;
		source_output += get_report_object_header(dcon_obj.name, indent);
		source_output += get_object_size_check(dcon_obj.name, indent);
		source_output += get_dcon_object_loop_start(dcon_obj.name, indent);
		if (dcon_obj.is_relationship) {
			source_output += get_dcon_relations_check(dcon_obj, indent);
		}

		for (auto& dcon_property : dcon_obj.properties) {
			source_output += get_load_record_if_statement(dcon_obj.name + "_" + dcon_property.name, indent);
			source_output += get_start_curly_bracket(indent);
			indent++;
			source_output += get_object_property_check(dcon_obj.name, dcon_property, indent);
			indent--;
			// property load record if statement end
			source_output += get_end_curly_bracket(indent);

		}
		indent--;
		// dcon object loop end
		source_output += get_end_curly_bracket(indent);
		indent--;
		// object load record if statement end
		source_output += get_end_curly_bracket(indent);

		source_output += return_report(indent);

		indent--;
		source_output += get_end_curly_bracket(indent);
	}

	// start construction oos reporting function
	source_output += get_oos_reporter_function_signature();
	uint32_t indent = 1;
	source_output += get_start_curly_bracket(indent);
	source_output += source_get_initial_var_declariations(indent);

	for (auto& dcon_obj : parsed_file.relationship_objects) {
		source_output += call_oos_reporter_subfunction(indent, dcon_obj.name);
	}

	source_output += return_report(indent);

	// function end
	indent--;
	source_output += get_end_curly_bracket(indent);


	for (auto& dcon_obj : parsed_file.relationship_objects) {
		source_output += get_data_reset_subfunction_signature(dcon_obj.name);
		source_output += get_start_curly_bracket(indent);
		indent++;

		source_output += get_neg_load_record_if_statement(dcon_obj.name, indent);
		source_output += get_start_curly_bracket(indent);
		indent++;
		source_output += data_resetter_set_dcon_size_zero(dcon_obj, indent);
		indent--;
		source_output += get_end_curly_bracket(indent);
		// start of else
		source_output += get_else_statement(indent);
		source_output += get_start_curly_bracket(indent);
		indent++;
		for (auto& dcon_property : dcon_obj.properties) {
			source_output += get_neg_load_record_if_statement(dcon_obj.name + "_" + dcon_property.name, indent);
			source_output += get_start_curly_bracket(indent);
			indent++;
			source_output += data_resetter_get_foreach_obj_clear_data_loop(dcon_obj, dcon_property, indent);
			indent--;
			source_output += get_end_curly_bracket(indent);

		}
		// end of else
		indent--;
		source_output += get_end_curly_bracket(indent);

		indent--;
		source_output += get_end_curly_bracket(indent);
	}


	// start constructing data reset function here
	source_output += get_data_reset_function_signature();
	source_output += get_start_curly_bracket(indent);
	indent++;

	for (auto& dcon_obj : parsed_file.relationship_objects) {
		source_output += call_data_reset_subfunction(indent, dcon_obj.name);
	}

	// function end
	indent--;
	source_output += get_end_curly_bracket(indent);

	// end namespace
	source_output += get_end_curly_bracket(0);




	std::fstream header_out;
	header_out.open(header_path, std::ios::out);
	if (header_out.is_open()) {
		header_out << header_output;
		header_out.close();
	}
	else {
		std::abort();
	}

	std::fstream source_out;
	source_out.open(source_path, std::ios::out);
	if (source_out.is_open()) {
		source_out << source_output;
		source_out.close();
	}
	else {
		std::abort();
	}





}
