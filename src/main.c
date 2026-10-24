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
	v3 *positions_obj = malloc((radius_resolution * layer_count * 5) * sizeof(v3));
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
	int cap_subdivisions = 4;

	for(int z = 0; z < layer_count; ++z) {
		int layer = z * (radius_resolution+1) * 2;
		int n_layer = layer + radius_resolution + 1;
		//fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
		for(int i = 1; i <= radius_resolution; ++i) {
			int vert1 = i + 1;
			int vert2 = i % radius_resolution + 2;
			//fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert1 + n_layer, vert2 + n_layer);
			fprintf(obj, "f %i %i %i %i\n", vert2 + n_layer, vert1 + n_layer, vert1 + layer, vert2 + layer);
		}

		//for(int i = 0; i <= 1; ++i) {
			//if(i == 1) layer = z * (radius_resolution+1) * 2;
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
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
						fprintf(obj, "f %i %i %i\n", layer+1, vert_id, vert_id - 1);
					} else {
						fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + 1, vert_id, layer+1);
					}
				} else if (subdivision == cap_subdivisions) {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					if(i != radius_resolution) {
						fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
					} else {
						//fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution + i + 1 - radius_resolution, vert_id - radius_resolution + i, vert1 + layer, vert2 + layer);
						fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1 - radius_resolution);
					}
				} else {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert2 + layer - 1];

					if(i == 1) {
						v3 top_stick = positions_obj[vert1 + layer - 1];

						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
						fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution-1, vert_id - radius_resolution, vert_id, vert_id - 1);
						//fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution, vert_id - radius_resolution + 1, vert_id, vert_id - 1);
					} else {
						fprintf(obj, "f %i %i %i %i\n",   vert_id - radius_resolution + 1, vert_id, vert_id - radius_resolution, vert_id - radius_resolution - radius_resolution + 1);
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
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
						//fprintf(obj, "f %i %i %i\n", vert_id, vert_id - 1, layer+1);
						fprintf(obj, "f %i %i %i\n", layer+1, vert_id-1, vert_id);
					} else {
						//fprintf(obj, "f %i %i %i\n", vert_id, layer+1, vert_id - radius_resolution + 1);
						fprintf(obj, "f %i %i %i\n", vert_id - radius_resolution + 1, layer+1, vert_id);
					}
				} else if (subdivision == cap_subdivisions) {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					if(i != radius_resolution) {
						//fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1);
						fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution + i + 1, vert_id - radius_resolution + i, vert1 + layer, vert2 + layer);
					} else {
						fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution + i + 1 - radius_resolution, vert_id - radius_resolution + i, vert1 + layer, vert2 + layer);
						//fprintf(obj, "f %i %i %i %i\n", vert2 + layer, vert1 + layer, vert_id - radius_resolution + i, vert_id - radius_resolution + i + 1 - radius_resolution);
					}
				} else {
					int vert1 = i + 1;
					int vert2 = i % radius_resolution + 2;

					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert2 + layer - 1];

					if(i == 1) {
						v3 top_stick = positions_obj[vert1 + layer - 1];

						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
					}
					if(i != radius_resolution) {
						v3 mid = v3_lerp(top_base, top_stick, lerp_factor);
						fprintf(obj, "v %f %f %f\n", mid.x, mid.y, mid.z);
						vert_id++;
						//fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution-1, vert_id - radius_resolution, vert_id, vert_id - 1);
						fprintf(obj, "f %i %i %i %i\n", vert_id - 1, vert_id, vert_id - radius_resolution, vert_id - radius_resolution-1);
						//fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution, vert_id - radius_resolution + 1, vert_id, vert_id - 1);
					} else {
						//fprintf(obj, "f %i %i %i %i\n",   vert_id - radius_resolution + 1, vert_id, vert_id - radius_resolution, vert_id - radius_resolution - radius_resolution + 1);
						fprintf(obj, "f %i %i %i %i\n", vert_id - radius_resolution - radius_resolution + 1, vert_id - radius_resolution, vert_id, vert_id - radius_resolution + 1);
					}
				}
			}
		}
		//}
	}
	/*for(int z = 0; z < layer_count; ++z) {
		for(int circle_index = 1; circle_index <= radius_resolution; ++circle_index) {
			int layer = z * (radius_resolution+1) * 2;
			int n_layer = layer + radius_resolution + 1;

			int vert1 = circle_index + 1;
			int vert2 = circle_index % radius_resolution + 2;
			for(int i = 1; i <= cap_subdivisions; ++i) {
				float lerp_factor = (float)i / (cap_subdivisions+1);
				float lerp_factor_n = (float)(i+1) / (cap_subdivisions+1);
				if(i == 1) {
					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert1 + layer - 1];
					if(circle_index == 1) {
						v3 top_new = v3_lerp(top_base, top_stick, lerp_factor_n);
						fprintf(obj, "v %f %f %f\n", top_new.x, top_new.y, top_new.z);
						vert_id++;
					}

					v3 top_stick2 = positions_obj[vert2 + layer - 1];
					v3 top_new2 = v3_lerp(top_base, top_stick2, lerp_factor_n);
					fprintf(obj, "v %f %f %f\n", top_new2.x, top_new2.y, top_new2.z);
					vert_id++;



					if(circle_index == 1) {
						fprintf(obj, "f %i %i %i\n", vert_id, vert_id-1, 1 + layer);
					} else {
						fprintf(obj, "f %i %i %i\n", vert_id, vert_id-cap_subdivisions- 1, 1 + layer);
					}

					//fprintf(obj, "f %i %i %i\n", vert_id-3, vert_id-2, vert1+layer);
					//fprintf(obj, "f %i %i %i\n", vert_id-2, vert2+layer, vert1+layer);
				} else if(i == cap_subdivisions) {
					//fprintf(obj, "f %i %i %i\n", vert_id-1, vert_id, vert1+layer);
					//fprintf(obj, "f %i %i %i\n", vert_id, vert2+layer, vert1+layer);
				} else{
					v3 top_base = positions_obj[layer];
					v3 top_stick = positions_obj[vert1 + layer - 1];

					v3 top_stick2 = positions_obj[vert2 + layer - 1];


					v3 top_next_new2 = v3_lerp(top_base, top_stick2, lerp_factor_n);
					fprintf(obj, "v %f %f %f\n", top_next_new2.x, top_next_new2.y, top_next_new2.z);
					vert_id++;

					fprintf(obj, "f %i %i %i\n", vert_id-cap_subdivisions-3, vert_id-2, vert_id-cap_subdivisions-1);
					fprintf(obj, "f %i %i %i\n", vert_id-2, vert_id, vert_id-cap_subdivisions-1);

				}
			}

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
	}*/

	free(values);
	free(positions);
	free(positions_obj);
	fclose(f);
	fclose(obj);
}
