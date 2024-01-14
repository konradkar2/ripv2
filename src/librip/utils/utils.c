#include "utils.h"
#include <stdlib.h>

float get_random_float(float min, float max)
{
	float scale = rand() / (float)RAND_MAX;
	return min + scale * (max - min);
}
