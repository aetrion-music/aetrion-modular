#pragma once

#include "plugin.hpp"

#define PI 3.141592f

json_t* json_bool(bool value);

json_t* json_floatArray(float * array, int length);
void json_floatArray_value(json_t* jArray, float * array, int length);

json_t* json_boolArray(bool * array, int length);
void json_boolArray_value(json_t* jArray, bool * array, int length);

float mod_0_max(float val, float max);
int mod_0_max(int val, int max);