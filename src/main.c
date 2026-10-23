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

v3 v3_div(v3 a, v3 b){
	return (v3){a.x / b.x, a.y / b.y, a.z / b.z}; 
}

v3 v3_fdiv(v3 a, float b){
	return (v3){a.x / b, a.y / b, a.z / b}; 
}

v3 v3_add(v3 a, v3 b){
	return (v3){a.x + b.x, a.y + b.y, a.z + b.z}; 
}

v3 v3_sub(v3 a, v3 b){
	return (v3){a.x - b.x, a.y - b.y, a.z - b.z}; 
}
float v3_len(v3 a){
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
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
float noise3( v3 p )
{
	return hash( p.x + p.y * 57.1235 + p.z * 502.352); // just guessing for the second scalar
}

float get_blurred_at_coord(v3 p, v3 res, int radius) {
	float blurred_count = 0;
	float blur_value = 0;
	for(int z = -radius; z <= radius; ++z) {
		for(int x = -radius; x <= radius; ++x) {
			for(int y = -radius; y <= radius; ++y) {
				v3 offset = {x, y, 0};
				v3 p_floor = {floor(p.x), floor(p.y), floor(p.z)};
				//v3 p_floor = p;
				//v2 uv = v2_add(v2_div(p, res), v2_div(offset, res));
				v3 uv = v3_div(v3_add(p_floor, offset), res);
				//if(uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1) continue;
				++blurred_count;
				blur_value += noise3(uv);
			}
		}
	}

	v3 p_2d = p;
	p_2d.z = 0;
	float noise_scalar = pow(v3_len(v3_div(p_2d, res)), 0.2);

	return (blur_value / blurred_count) * noise_scalar;
}

float get_thresholded_at_coord(v3 p, v3 res, int radius, float threshold) {
	return get_blurred_at_coord(p, res, radius) > threshold;
	//return 1;
}

float get_blurred_thresholded_at_coord(v3 p, v3 res, int radius1, float threshold1, int radius2, float threshold2) {
	float blurred_count = 0;
	float blur_value = 0;
	for(int z = -radius2; z <= radius2; ++z) {
		for(int x = -radius2; x <= radius2; ++x) {
			for(int y = -radius2; y <= radius2; ++y) {
				v3 offset = {x, y, z};
				//v3 p_floor = {floor(p.x), floor(p.y), floor(p.z)};
				//v2 uv = v2_add(v2_div(p, res), v2_div(offset, res));
				//v3 uv = v3_add(p, v3_div(offset, res));
				v3 uv = v3_add(p, offset);
				//v3 uv = v3_div(v3_add(p_floor, offset), res);
				//if(uv.x < 0 || uv.y < 0 || uv.x > res.x || uv.y > res.y) continue;
				++blurred_count;
				//blur_value += get_biased_at_coord(uv);
				blur_value += get_thresholded_at_coord(uv, res, radius1, threshold1);

			}
		}
	}
	return (blur_value/blurred_count) > threshold2;
}


int main() {
	const int resolution = 100;
	FILE *f = fopen("rand.ppm", "w");
	FILE *obj = fopen("layer.obj", "w");
	fprintf(f, "P6 %i %i 255\n", resolution, resolution);
	v3 res = {resolution,resolution,resolution};
	v3 halfres = {resolution / 2., resolution / 2., 0.};
	_Bool *values = calloc(resolution*resolution,sizeof(_Bool));


/*#pragma omp parallel for
	for(int i = 0; i < resolution * resolution; ++i) {
		float width = i % resolution;
		float height = floor((float)i / resolution);

		v3 p = {width, height, 0};

		values[i] = get_thresholded_at_coord(v3_sub(p, halfres), res, 6, 0.35) > 0.5;
	}*/


	const int radius_resolution = 50;
	const int ray_resolution=75;
	int layer_count = 30;
	float layer_step = 1;
	v3 *positions = malloc(radius_resolution * layer_count * sizeof(v3));
	v3 *positions_obj = malloc((radius_resolution * layer_count * 3) * sizeof(v3));
	const float step_size = 1.;
	for(int z = 0; z < layer_count; ++z) {
#pragma omp parallel for
	for(int i = 0; i < radius_resolution; ++i) {
		float angle = ((float)i / radius_resolution) * 2 * M_PI;
		v3 pos = (v3){0, 0, z * layer_step};
		for(int step = 0; step < ray_resolution; ++step) {
			pos = v3_add(pos, (v3){cos(angle) * step_size, sin(angle) * step_size, 0});
			if(get_blurred_thresholded_at_coord(pos, res, 3, 0.35, 2, 0.35) > 0.5) {
				break;
			}
		} 
		//int x = pos.x * res.x,y = pos.y * res.y;
		int index = (int)(pos.y + res.y/2) * (int)res.x + (int)(pos.x + res.x/2);
		positions[i + radius_resolution * z] = pos;
		if(index >= resolution * resolution || index < 0) continue;
		values[(int)(pos.y + res.y/2) * (int)res.x + (int)(pos.x + res.x/2)] = 1;
	}
	}
	for(int i = 0; i < resolution*resolution; ++i) {
		fprintf(f, "%c", (int)values[i] * 255);
		fprintf(f, "%c", (int)values[i] * 255);
		fprintf(f, "%c", (int)values[i] * 255);
	}
	int vert_id = 0;

	for(int z = 0; z < layer_count; ++z) {
		fprintf(obj, "v %f %f %f\n", 0., 0., (float)z * layer_step);
		positions_obj[vert_id] = (v3){0., 0., (float)z * layer_step};

		vert_id++;
		float scale_min = 1;
		float scale_drop = 2; 
		v2 scale = (v2){(1-pow(hash(z * layer_count * 32), scale_drop) + scale_min)/(scale_min+1), (1-pow(hash(z * layer_count * 2409), scale_drop) + scale_min)/(scale_min+1)};
		//v2 scale = (v2){1, 1};

		for(int i = 0; i < radius_resolution; ++i) {
			v3 pos = positions[i + radius_resolution * z];
			fprintf(obj, "v %f %f %f\n", pos.x * scale.x, pos.y * scale.y, (float)z*layer_step);
			positions_obj[vert_id] = (v3){pos.x * scale.x, pos.y * scale.y, (float)z * layer_step};
			vert_id++;
			//fprintf(obj, "v %f %f %f\n", positions[i].x, positions[i].y, 1.);
		}

		fprintf(obj, "v %f %f %f\n", 0., 0., (float)(z+1)*layer_step);
		positions_obj[vert_id] = (v3){0., 0., (float)(z+1) * layer_step};
		vert_id++;
		for(int i = 0; i < radius_resolution; ++i) {
			v3 pos = positions[i + radius_resolution * z];
			fprintf(obj, "v %f %f %f\n", pos.x * scale.x, pos.y * scale.y, (float)(z+1)*layer_step);
			positions_obj[vert_id] = (v3){pos.x * scale.x, pos.y * scale.y, (float)(z+1) * layer_step};
			vert_id++;
			//fprintf(obj, "v %f %f %f\n", positions[i].x, positions[i].y, 1.);
		}
	}
	//fprintf(obj, "v %f %f %f\n", 0., 0., 1.);

	for(int z = 0; z < layer_count; ++z) {
		for(int i = 1; i <= radius_resolution; ++i) {
			int layer = z * (radius_resolution+1) * 2;
			int n_layer = layer + radius_resolution + 1;

			int vert1 = i + 1;
			int vert2 = i % radius_resolution + 2;
			//int layer = 0;
			


			// top cap
			v3 top_base = positions_obj[layer];
			v3 top_stick = positions_obj[vert1 + layer - 1];
			v3 top_new = v3_fdiv(v3_add(top_base, top_stick), 2);
			fprintf(obj, "v %f %f %f\n", top_new.x, top_new.y, top_new.z);
			vert_id++;

			v3 top_stick2 = positions_obj[vert2 + layer - 1];
			v3 top_new2 = v3_fdiv(v3_add(top_base, top_stick2), 2);
			fprintf(obj, "v %f %f %f\n", top_new2.x, top_new2.y, top_new2.z);
			vert_id++;

			fprintf(obj, "f %i %i %i\n", vert_id, vert_id-1, 1 + layer);

			fprintf(obj, "f %i %i %i\n", vert_id-1, vert_id, vert1+layer);
			fprintf(obj, "f %i %i %i\n", vert_id, vert2+layer, vert1+layer);

			// bottom cap
			v3 bottom_base = positions_obj[n_layer];
			v3 bottom_stick = positions_obj[vert1 + n_layer - 1];
			v3 bottom_new = v3_fdiv(v3_add(bottom_base, bottom_stick), 2);
			fprintf(obj, "v %f %f %f\n", bottom_new.x, bottom_new.y, bottom_new.z);
			vert_id++;

			v3 bottom_stick2 = positions_obj[vert2 + n_layer - 1];
			v3 bottom_new2 = v3_fdiv(v3_add(bottom_base, bottom_stick2), 2);
			fprintf(obj, "v %f %f %f\n", bottom_new2.x, bottom_new2.y, bottom_new2.z);
			vert_id++;

			fprintf(obj, "f %i %i %i\n", 1 + n_layer, vert_id-1, vert_id);

			fprintf(obj, "f %i %i %i\n", vert1+n_layer, vert_id, vert_id-1);
			fprintf(obj, "f %i %i %i\n", vert1+n_layer, vert2+n_layer, vert_id);

			// walls
			fprintf(obj, "f %i %i %i \n", vert1 + layer, vert2 + layer, vert2 + n_layer); 
			fprintf(obj, "f %i %i %i \n", vert1 + layer, vert2 + n_layer, vert1 + n_layer); 
		}
	}

	free(values);
	free(positions);
	free(positions_obj);
	fclose(f);
	fclose(obj);
}
