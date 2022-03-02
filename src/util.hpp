#pragma once

#include "plugin.hpp"

json_t* json_bool(bool value);

json_t* json_floatArray(float * array, int length);
void json_floatArray_value(json_t* jArray, float * array, int length);

json_t* json_boolArray(bool * array, int length);
void json_boolArray_value(json_t* jArray, bool * array, int length);