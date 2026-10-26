#include <float.h>
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
v3 v3_fmul(v3 a, float b){
	return (v3){a.x * b, a.y * b, a.z * b}; 
}

v3 v3_add(v3 a, v3 b){
	return (v3){a.x + b.x, a.y + b.y, a.z + b.z}; 
}

v3 v3_sub(v3 a, v3 b){
	return (v3){a.x - b.x, a.y - b.y, a.z - b.z}; 
}

v3 v3_lerp(v3 a, v3 b, float t) {
	return v3_add(v3_fmul(a, 1-t), v3_fmul(b, t));
}

float v3_len(v3 a){
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

float v3_dot(v3 a, v3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

v3 v3_cross(v3 a, v3 b) {
	v3 crossed;

	crossed.x = a.y * b.z - a.z * b.y;
	crossed.y = a.z * b.x - a.x * b.z;
	crossed.z = a.x * b.y - a.y * b.x;

	return crossed;
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
	//FILE *f = fopen("rand.ppm", "w");
	FILE *obj = fopen("layer.obj", "w");
	//fprintf(f, "P6 %i %i 255\n", resolution, resolution);
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


	const int radius_resolution = 75;
	const int ray_resolution=75;
	int layer_count = 20;
	float layer_step = 1;
	int cap_subdivisions = 30;
	v3 *positions = malloc(radius_resolution * layer_count * sizeof(v3));
	v3 *positions_obj = malloc((radius_resolution * layer_count + radius_resolution * layer_count * cap_subdivisions + radius_resolution) * 2 * sizeof(v3));
	int *triangle_indices = malloc((radius_resolution * cap_subdivisions * layer_count * 2 * 2 + radius_resolution * 2) * 3 * sizeof(int));
	int triangle_ind_id = 0;
	const float step_size = 1.;
	printf("Generating:\n");
	for(int z = 0; z < layer_count; ++z) {
#pragma omp parallel for
	for(int i = 0; i < radius_resolution; ++i) {
		if(i % 10 == 0) printf("\r%i%%", (int)ceil(100 * (float)(z*radius_resolution + i)/(layer_count * radius_resolution))), fflush(stdout);
		float angle = ((float)i / radius_resolution) * 2 * M_PI;
		float z_pos = (z - (double)layer_count/2) * layer_step;
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
	/*for(int i = 0; i < resolution*resolution; ++i) {
		fprintf(f, "%c", (int)values[i] * 255);
		fprintf(f, "%c", (int)values[i] * 255);
		fprintf(f, "%c", (int)values[i] * 255);
	}*/
	int vert_id = 0;

	for(int z = 0; z < layer_count; ++z) {

		//fprintf(obj, "v %f %f %f\n", 0., 0., (float)z * layer_step);
		float z_pos = (z - (double)layer_count/2) * layer_step;
		float z_pos_n = ((z+1) - (double)layer_count/2) * layer_step;

		positions_obj[vert_id] = (v3){0., 0., (float)z_pos};

		vert_id++;
		float scale_min = 1;
		float scale_drop = 2; 
		v2 scale = (v2){(1-pow(hash(z * layer_count * 32), scale_drop) + scale_min)/(scale_min+1), (1-pow(hash(z * layer_count * 2409), scale_drop) + scale_min)/(scale_min+1)};
		//v2 scale = (v2){1, 1};

		for(int i = 0; i < radius_resolution; ++i) {
			v3 pos = positions[i + radius_resolution * z];
			//fprintf(obj, "v %f %f %f\n", pos.x * scale.x, pos.y * scale.y, (float)z*layer_step);
			positions_obj[vert_id] = (v3){pos.x * scale.x, pos.y * scale.y, z_pos};
			vert_id++;
			////fprintf(obj, "v %f %f %f\n", positions[i].x, positions[i].y, 1.);
		}

		//fprintf(obj, "v %f %f %f\n", 0., 0., (float)(z+1)*layer_step);
		positions_obj[vert_id] = (v3){0., 0., z_pos_n};
		vert_id++;
		for(int i = 0; i < radius_resolution; ++i) {
			v3 pos = positions[i + radius_resolution * z];
			//fprintf(obj, "v %f %f %f\n", pos.x * scale.x, pos.y * scale.y, (float)(z+1)*layer_step);
			positions_obj[vert_id] = (v3){pos.x * scale.x, pos.y * scale.y, z_pos_n};
			vert_id++;
			////fprintf(obj, "v %f %f %f\n", positions[i].x, positions[i].y, 1.);
		}
	}
	////fprintf(obj, "v %f %f %f\n", 0., 0., 1.);

	printf("\nSubdividing:\n");
	for(int z = 0; z < layer_count; ++z) {
		printf("\r%i%%", (int)ceil((float)z/(layer_count-1) * 100)), fflush(stdout);
		int layer = z * (radius_resolution+1) * 2;
		int n_layer = layer + radius_resolution + 1;
		////fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
		for(int i = 1; i <= radius_resolution; ++i) {
			int vert1 = i + 1;
			int vert2 = i % radius_resolution + 2;
			////fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert1 + n_layer, vert2 + n_layer);
			////fprintf(obj, "f %i %i %i %i\n", vert2 + n_layer, vert1 + n_layer, vert1 + layer, vert2 + layer);


			//fprintf(obj, "f %i %i %i\n", vert2 + n_layer, vert1 + n_layer, vert1 + layer);
			triangle_indices[triangle_ind_id++] = vert2 + n_layer;
			triangle_indices[triangle_ind_id++] = vert1 + n_layer;
			triangle_indices[triangle_ind_id++] = vert1 + layer;

			//fprintf(obj, "f %i %i %i\n", vert2 + n_layer, vert1 + layer, vert2 + layer);
			triangle_indices[triangle_ind_id++] = vert2 + n_layer;
			triangle_indices[triangle_ind_id++] = vert1 + layer;
			triangle_indices[triangle_ind_id++] = vert2 + layer;
		}

		//for(int i = 0; i <= 1; ++i) {
			//if(i == 1) layer = z * (radius_resolution+1) * 2;
		for(int subdivision = 1; subdivision <= cap_subdivisions; ++subdivision) {
			float lerp_factor = (float)subdivision / (cap_subdivisions+1);
			float lerp_factor_next = (float)(subdivision+1) / (cap_subdivisions+1);
			lerp_factor = lerp_factor_next;

			for(int i = 1; i <= radius_resolution; ++i) {
				if(subdivision == 1) {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert2 + layer - 1];

					if(i == 1) {
						v3 top_stick = positions_obj[vert1 + layer - 1];

						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
						//fprintf(obj, "f %i %i %i\n", layer+1, vert_id, vert_id - 1);
						triangle_indices[triangle_ind_id++] = layer+1;
						triangle_indices[triangle_ind_id++] = vert_id;
						triangle_indices[triangle_ind_id++] = vert_id - 1;
					} else {
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + 1, vert_id, layer+1);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + 1;
						triangle_indices[triangle_ind_id++] = vert_id;
						triangle_indices[triangle_ind_id++] = layer+1;
					}
				} else if (subdivision == cap_subdivisions) {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					if(i != radius_resolution) {
						////fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
						//fprintf(obj, "f %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i);
						triangle_indices[triangle_ind_id++] = vert2 + layer;
						triangle_indices[triangle_ind_id++] = vert1 + layer;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i;
						//fprintf(obj, "f %i %i %i\n", vert2 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
						triangle_indices[triangle_ind_id++] = vert2 + layer;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i + 1;
					} else {
						////fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution + i + 1 - radius_resolution, vert_id - radius_resolution + i, vert1 + layer, vert2 + layer);
						////fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1 - radius_resolution);
						//fprintf(obj, "f %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i);
						triangle_indices[triangle_ind_id++] = vert2 + layer;
						triangle_indices[triangle_ind_id++] = vert1 + layer;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i;
						//fprintf(obj, "f %i %i %i\n", vert2 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1 - radius_resolution);
						triangle_indices[triangle_ind_id++] = vert2 + layer;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i + 1 - radius_resolution;
					}
				} else {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert2 + layer - 1];

					if(i == 1) {
						v3 top_stick = positions_obj[vert1 + layer - 1];

						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
						////fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution-1, vert_id - radius_resolution, vert_id, vert_id - 1);
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution-1, vert_id - radius_resolution, vert_id);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution-1;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution;
						triangle_indices[triangle_ind_id++] = vert_id;
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution-1, vert_id, vert_id - 1);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution-1;
						triangle_indices[triangle_ind_id++] = vert_id;
						triangle_indices[triangle_ind_id++] = vert_id - 1;
					} else {
						////fprintf(obj, "f %i %i %i %i\n",   vert_id - radius_resolution + 1, vert_id, vert_id - radius_resolution, vert_id - radius_resolution - radius_resolution + 1);
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + 1, vert_id, vert_id - radius_resolution);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + 1;
						triangle_indices[triangle_ind_id++] = vert_id;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution;
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + 1, vert_id - radius_resolution, vert_id - radius_resolution - radius_resolution + 1);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + 1;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution - radius_resolution + 1;
					}
				}
			}
		}



		layer = n_layer;
		for(int subdivision = 1; subdivision <= cap_subdivisions; ++subdivision) {
			float lerp_factor = (float)subdivision / (cap_subdivisions+1);
			float lerp_factor_next = (float)(subdivision+1) / (cap_subdivisions+1);

			for(int i = 1; i <= radius_resolution; ++i) {
				if(subdivision == 1) {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert2 + layer - 1];

					if(i == 1) {
						v3 top_stick = positions_obj[vert1 + layer - 1];

						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
						////fprintf(obj, "f %i %i %i\n", vert_id, vert_id - 1, layer+1);
						//fprintf(obj, "f %i %i %i\n", layer+1, vert_id-1, vert_id);
						triangle_indices[triangle_ind_id++] = layer+1;
						triangle_indices[triangle_ind_id++] = vert_id-1;
						triangle_indices[triangle_ind_id++] = vert_id;
					} else {
						////fprintf(obj, "f %i %i %i\n", vert_id, layer+1, vert_id - radius_resolution + 1);
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + 1, layer+1, vert_id);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + 1;
						triangle_indices[triangle_ind_id++] = layer+1;
						triangle_indices[triangle_ind_id++] = vert_id;
					}
				} else if (subdivision == cap_subdivisions) {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					if(i != radius_resolution) {
						////fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
						////fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution + i + 1, vert_id - radius_resolution + i, vert1 + layer, vert2 + layer);
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + i + 1, vert_id - radius_resolution + i, vert1 + layer);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i + 1;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i;
						triangle_indices[triangle_ind_id++] = vert1 + layer;
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + i + 1, vert1 + layer, vert2 + layer);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i + 1;
						triangle_indices[triangle_ind_id++] = vert1 + layer;
						triangle_indices[triangle_ind_id++] = vert2 + layer;
					} else {
						////fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution + i + 1 - radius_resolution, vert_id - radius_resolution + i, vert1 + layer, vert2 + layer);
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + i + 1 - radius_resolution, vert_id - radius_resolution + i, vert1 + layer);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i + 1 - radius_resolution;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i;
						triangle_indices[triangle_ind_id++] = vert1 + layer;
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + i + 1 - radius_resolution, vert1 + layer, vert2 + layer);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + i + 1 - radius_resolution;
						triangle_indices[triangle_ind_id++] = vert1 + layer;
						triangle_indices[triangle_ind_id++] = vert2 + layer;
					}
				} else {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert2 + layer - 1];

					if(i == 1) {
						v3 top_stick = positions_obj[vert1 + layer - 1];

						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						//fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						positions_obj[vert_id] = (v3){mid.x, mid.y, mid.z};
						vert_id++;
						////fprintf(obj, "f %i %i %i %i\n", vert_id - 1, vert_id, vert_id - radius_resolution, vert_id - radius_resolution-1);
						//fprintf(obj, "f %i %i %i\n", vert_id - 1, vert_id, vert_id - radius_resolution);
						triangle_indices[triangle_ind_id++] = vert_id - 1;
						triangle_indices[triangle_ind_id++] = vert_id;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution;
						//fprintf(obj, "f %i %i %i\n", vert_id - 1, vert_id - radius_resolution, vert_id - radius_resolution-1);
						triangle_indices[triangle_ind_id++] = vert_id - 1;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution-1;
					} else {
						////fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution - radius_resolution + 1, vert_id - radius_resolution, vert_id, vert_id - radius_resolution + 1);
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution - radius_resolution + 1, vert_id - radius_resolution, vert_id);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution - radius_resolution + 1;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution;
						triangle_indices[triangle_ind_id++] = vert_id;
						//fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution - radius_resolution + 1, vert_id, vert_id - radius_resolution + 1);
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution - radius_resolution + 1;
						triangle_indices[triangle_ind_id++] = vert_id;
						triangle_indices[triangle_ind_id++] = vert_id - radius_resolution + 1;
					}
				}
			}
		}
	}
	
	printf("\nEroding:\n");
	int erosion_ray_count = 50000;
	int eroded_count = 0;
#pragma omp parallel for
	for(int i = 0; i < erosion_ray_count; ++i) {
		eroded_count++;
		if(i % 10 == 0) printf("\r%i%%", (int)ceil(100 * (float)((float)eroded_count/(erosion_ray_count-1)))), fflush(stdout);
		v3 origin;
		v3 direction;
		float random;
		
		random = sqrt(-2 * log(((double)rand() / RAND_MAX))) * cos(2 * M_PI * ((double)rand() / RAND_MAX));
		origin.x = random;
		random = sqrt(-2 * log(((double)rand() / RAND_MAX))) * cos(2 * M_PI * ((double)rand() / RAND_MAX));
		origin.y = random;
		random = random;
		origin.z = 1/sqrt(2*M_PI) * exp(-(random/2)) - 0.5;
		//printf("origin: %f %f %f\n", origin.x, origin.y, origin.z);

		origin = v3_fdiv(origin, v3_len(origin)); // normalize
		origin = v3_fmul(origin, 3000);

		random = sqrt(-2 * log(((double)rand() / RAND_MAX))) * cos(2 * M_PI * ((double)rand() / RAND_MAX));
		direction.x = random;
		random = sqrt(-2 * log(((double)rand() / RAND_MAX))) * cos(2 * M_PI * ((double)rand() / RAND_MAX));
		direction.y = random;
		random = sqrt(-2 * log(((double)rand() / RAND_MAX))) * cos(2 * M_PI * ((double)rand() / RAND_MAX));
		direction.z = random;

		origin = v3_fdiv(origin, v3_len(origin)); // normalize
		origin = v3_fmul(origin, 30);

		direction = v3_fdiv(direction, v3_len(direction)); // normalize

		float closest = INFINITY;
		v3 *vert_a_p = NULL;
		v3 *vert_b_p = NULL;
		v3 *vert_c_p = NULL;
		float v_u,v_v,v_w;
		for(int i = 0; i < triangle_ind_id; i+=3) {
			v3 vert_a = positions_obj[triangle_indices[i]-1];
			v3 vert_b = positions_obj[triangle_indices[i+1]-1];
			v3 vert_c = positions_obj[triangle_indices[i+2]-1];

			float epsilon = FLT_EPSILON;

			v3 edge1 = v3_sub(vert_b, vert_a);
			v3 edge2 = v3_sub(vert_c, vert_a);

			v3 ray_cross_e2 = v3_cross(direction, edge2);
			float det = v3_dot(edge1, ray_cross_e2);
			if(fabs(det) < epsilon) continue;

			float inv_det = 1/det;

			v3 s = v3_sub(origin, vert_a);
			float u = inv_det * v3_dot(s, ray_cross_e2);

			if(u < epsilon || u - 1 > epsilon) continue;

			v3 s_cross_e1 = v3_cross(s, edge1);
			float v = inv_det * v3_dot(direction, s_cross_e1);

			if(v < epsilon || u + v - 1 > epsilon) continue;

			float t = inv_det * v3_dot(edge2, s_cross_e1);
			if(t < closest) {
				closest = t;
				float w = (1 - u - v);

				vert_a_p = &positions_obj[triangle_indices[i]-1];
				vert_b_p = &positions_obj[triangle_indices[i+1]-1];
				vert_c_p = &positions_obj[triangle_indices[i+2]-1];
				v_w = w;
				v_v = v;
				v_u = u;

				/**vert_a_p = v3_add(*vert_a_p, v3_fmul(direction, w * 0.1));
				*vert_b_p = v3_add(*vert_b_p, v3_fmul(direction, u * 0.1));
				*vert_c_p = v3_add(*vert_c_p, v3_fmul(direction, v * 0.1));*/
			}
			
		}
		if(closest != INFINITY) {
			*vert_a_p = v3_add(*vert_a_p, v3_fmul(direction, v_w * 0.3));
			*vert_b_p = v3_add(*vert_b_p, v3_fmul(direction, v_u * 0.3));
			*vert_c_p = v3_add(*vert_c_p, v3_fmul(direction, v_v * 0.3));
		}
	}


	for(int i = 0; i < vert_id; ++i) {
		v3 current_vertex = positions_obj[i];
		fprintf(obj, "v %f %f %f\n", current_vertex.x, current_vertex.y, current_vertex.z);
	}
	for(int i = 0; i < triangle_ind_id; i+=3) {
		fprintf(obj, "f %i %i %i\n", triangle_indices[i], triangle_indices[i+1], triangle_indices[i+2]);
	}

	free(values);
	free(positions);
	free(positions_obj);
	free(triangle_indices);
	//fclose(f);
	fclose(obj);
}
