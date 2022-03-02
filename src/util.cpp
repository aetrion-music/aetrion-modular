#include "util.hpp"

json_t* json_bool(bool value){
	return value ? json_true() : json_false();
}


json_t* json_floatArray(float * array, int length){
	json_t *jArray = json_array();
	for(int i = 0; i < length; i++){
		json_array_insert_new(jArray, i, json_real(array[i]));
	}
	return jArray;
}
void json_floatArray_value(json_t* jArray, float * array, int length){
	for(int i = 0; i < length; i++){
		array[i] = json_real_value(json_array_get(jArray, i));
	}
}

json_t* json_boolArray(bool * array, int length){
	json_t *jArray = json_array();
	for(int i = 0; i < length; i++){
		json_array_insert_new(jArray, i, json_bool(array[i]));
	}
	return jArray;
}
void json_boolArray_value(json_t* jArray, bool * array, int length){
	for(int i = 0; i < length; i++){
		array[i] = json_is_true(json_array_get(jArray, i));
	}
}