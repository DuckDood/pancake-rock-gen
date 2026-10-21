#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	float x,y;
} v2;

typedef struct {
	float x,y,z;
} v3;

v2 v2_div(v2 a, v2 b){
	return (v2){a.x / b.x, a.y / b.y}; 
}
v2 v2_add(v2 a, v2 b){
	return (v2){a.x + b.x, a.y + b.y}; 
}

v2 v2_sub(v2 a, v2 b){
	return (v2){a.x - b.x, a.y - b.y}; 
}
float v2_len(v2 a){
	return sqrt(a.x * a.x + a.y * a.y);
}

// i forgot which shadertoy this random number generator is from, sorry!
//
float fract(float n) {
	return n - floor(n);
}

float hash( float n )
{
	return fract( sin(n)*54671.57391);
}

float noise2( v2 p )
{
	return hash( p.x + p.y * 57.1235);
}

float get_biased_at_coord(v2 p) {
	float noise_amount =  noise2((p));
	float noise_scale = v2_len(v2_sub(p, (v2){0.5, 0.5}));

	return noise_amount * noise_scale;
}

float get_blurred_at_coord(v2 p, v2 res, int radius) {
	float blurred_count = 0;
	float blur_value = 0;
	for(int x = -radius; x <= radius; ++x) {
		for(int y = -radius; y <= radius; ++y) {
			v2 offset = {x, y};
			//v2 uv = v2_add(v2_div(p, res), v2_div(offset, res));
			v2 uv = v2_div(v2_add(p, offset), res);
			if(uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1) continue;
			++blurred_count;
			blur_value += get_biased_at_coord(uv);
		}
	}

	return pow(blur_value / blurred_count, 0.5);
}

float get_thresholded_at_coord(v2 p, v2 res, int radius, float threshold) {
	return get_blurred_at_coord(p, res, radius) > threshold;
	//return 1;
}

float get_blurred_thresholded_at_coord(v2 p, v2 res, int radius1, float threshold1, int radius2, float threshold2) {
	float blurred_count = 0;
	float blur_value = 0;
	for(int x = -radius2; x <= radius2; ++x) {
		for(int y = -radius2; y <= radius2; ++y) {
			v2 offset = {x, y};
			//v2 uv = v2_add(v2_div(p, res), v2_div(offset, res));
			v2 uv = v2_div(v2_add(p, offset), res);
			if(uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1) continue;
			++blurred_count;
			//blur_value += get_biased_at_coord(uv);
			blur_value += get_thresholded_at_coord(p, res, radius1, threshold1);

		}
	}
	return blur_value > threshold2;
}


int main() {
	const int resolution = 100;
	FILE *f = fopen("rand.ppm", "w");
	FILE *obj = fopen("layer.obj", "w");
	fprintf(f, "P6 %i %i 255\n", resolution, resolution);
	v2 res = {resolution,resolution};
	v2 halfres = {resolution / 2., resolution / 2.};
	_Bool *values = calloc(resolution*resolution,sizeof(_Bool));
/*
#pragma omp parallel for
	for(int i = 0; i < resolution * resolution; ++i) {
		float width = i % resolution;
		float height = floor((float)i / resolution);

		v2 p = {width, height};

		values[i] = get_blurred_thresholded_at_coord(p, res, 7, 0.3, 3, 0.4) * 255;
	}*/
	const int radius_resolution = 100;
	const int ray_resolution=200;
	v2 *positions = malloc(radius_resolution * sizeof(v2));
	const float step_size = 1/2.;
#pragma omp parallel for
	for(int i = 0; i < radius_resolution; ++i) {
		float angle = ((float)i / radius_resolution) * 2 * M_PI;
		v2 pos = (v2){0};
		for(int step = 0; step < ray_resolution; ++step) {
			pos = v2_add(pos, (v2){cos(angle) * step_size, sin(angle) * step_size});
			if(get_thresholded_at_coord(v2_add(pos,halfres), res, 6, 0.4) > 0.5) {
				break;
			}
		} 
		//int x = pos.x * res.x,y = pos.y * res.y;
		values[(int)(pos.y + res.y/2) * (int)res.x + (int)(pos.x + res.x/2)] = 1;
		positions[i] = v2_div(pos, res);
	}
	for(int i = 0; i < resolution*resolution; ++i) {
		fprintf(f, "%c", (int)values[i] * 255);
		fprintf(f, "%c", (int)values[i] * 255);
		fprintf(f, "%c", (int)values[i] * 255);
	}
	for(int i = 0; i < radius_resolution; ++i) {
		fprintf(obj, "v %f %f %f\n", positions[i].x, positions[i].y, 0.);
		fprintf(obj, "v %f %f %f\n", positions[i].x, positions[i].y, 1.);
	}
	fprintf(obj, "v %f %f %f\n", 0., 0., 0.);
	fprintf(obj, "v %f %f %f\n", 0., 0., 1.);

	for(int i = 1; i <= radius_resolution; ++i) {
		fprintf(obj, "f %i %i %i\n", radius_resolution*2+1, 2*(i)-1, 2*((i) % (radius_resolution) + 1)-1);
		fprintf(obj, "f %i %i %i\n", radius_resolution*2+2, 2*(i), 2*((i) % (radius_resolution) + 1));

		fprintf(obj, "f %i %i %i %i\n", 2*(i)-1, 2*((i) % (radius_resolution) + 1)-1, 2*((i) % (radius_resolution) + 1), 2*i);
	}

	free(values);
	free(positions);
	fclose(f);
	fclose(obj);
}
