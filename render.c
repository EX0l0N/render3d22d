// gcc -O3 -Wall -Werror -pedantic-errors -std=c99 -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/include/SDL render.c mat?.c quat.c vec?.c -lSDL -lSDL_image -lm -o render
// emcc -O3 -Wall -Werror -pedantic-errors -std=c99 -D_GNU_SOURCE=1 -D_REENTRANT -s TOTAL_MEMORY=32000000 render.c mat?.c quat.c vec?.c -o index.html --use-preload-plugins --preload-file "./color_map_2048.jpg"
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "SDL_image.h"
#include <math.h>
#include "vec2.h"
#include "gl-matrix.h"
#include "vecs.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define min3(x, y, z) \
(((x) < (y)) ? (((z) < (x)) ? (z) : (x)) : (((z) < (y)) ? (z) : (y)))
#define max3(x, y, z) \
(((x) > (y)) ? (((z) > (x)) ? (z) : (x)) : (((z) > (y)) ? (z) : (y)))

#define SQ_SIZE 768
#define HALF_SQ_SIZE 383.

SDL_Surface *screen, *texture;

mat4_t vp_corr, projection, camera, model;
double eye[3] = { 0., -1., 3. };
double vert_buf[VERT_N][3];
double norm_buf[FACES][3];
int sort_buf[FACES];

void init_mats(void) {
	double scalormoon[3] = { HALF_SQ_SIZE, HALF_SQ_SIZE, .5 };
	double center[3] = { 0., 0., 0. };
	double up[3] = { 0., 1., 0. };

	vp_corr = mat4_scale(mat4_translate(mat4_identity(NULL), scalormoon, NULL), scalormoon, NULL);
	projection = mat4_perspective(45, 1., .1, 5., NULL);
	camera = mat4_lookAt(eye, center, up, NULL);
	model = mat4_identity(NULL);
}

void screen_project(void) {
	static double combined[16];
	int c;
	
	//this will write the result to combined
	mat4_multiply(mat4_multiply(mat4_multiply(vp_corr, projection, combined), camera, NULL), model, NULL);
	for (c = 0; c < VERT_N; c++) {
		mat4_multiplyVec3(combined, (double *)vert[c], vert_buf[c]);
	}

	for (c = 0; c < FACES; c++) {
		mat4_multiplyVec3(model, (double *)norm[c], norm_buf[c]);
	}
}

int cull_back_face(vec3_t normal) {
	double angle;
	angle = vec3_dot(eye, normal);
	return angle < 0.;
}

int zcompare(const void * a, const void * b) {
	static double vaz[3];
	static double vbz[3];
	double ret;

	vaz[0] = vert_buf[faces[*(int*)a][0][0] - 1][2];
	vaz[1] = vert_buf[faces[*(int*)a][1][0] - 1][2];
	vaz[2] = vert_buf[faces[*(int*)a][2][0] - 1][2];
	vbz[0] = vert_buf[faces[*(int*)b][0][0] - 1][2];
	vbz[1] = vert_buf[faces[*(int*)b][1][0] - 1][2];
	vbz[2] = vert_buf[faces[*(int*)b][2][0] - 1][2];
	
	ret = 1000. * (min3(vaz[0], vaz[1], vaz[2]) - min3(vbz[0], vbz[1], vbz[2]));

	return -ret;
}

void fragment_shader(double * vert, double * tex) {
	static double P[2], v0[2], v1[2], v2[2], tvv[2], tvu[2], tvvt[2], tvut[2], tvt[2];
	vec2_t A = vert, B = vert + 2, C = vert + 4;
	vec2_t tA = tex, tB = tex + 2, tC = tex + 4;
	double dot00, dot01, dot02, dot11, dot12, invDenom, u, v;
	int x, y, minx, maxx, miny, maxy, uv;
	int valid_tx_cords, source[2];
	Uint32 pixel;
	Uint8 r, g, b;

	// calculate bounding box
	minx = (int)min3(A[0], B[0], C[0]);
	maxx = (int)max3(A[0], B[0], C[0]);
	miny = (int)min3(A[1], B[1], C[1]);
	maxy = (int)max3(A[1], B[1], C[1]);

	// test for broken texcords
	valid_tx_cords = ( max3(tA[0], tB[0], tC[0]) < 1. ) && ( max3(tA[1], tB[1], tC[1]) < 1. );

	// prepare constant vecs
	vec2_subtract(C, A, v0);
	vec2_subtract(B, A, v1);
	if (valid_tx_cords) {
		vec2_subtract(tC, tA, tvu);
		vec2_subtract(tB, tA, tvv);
	}

	// prepare constant dots
	dot00 = vec2_dot(v0, v0);
	dot01 = vec2_dot(v0, v1);
	dot11 = vec2_dot(v1, v1);
	invDenom = 1 / (dot00 * dot11 - dot01 * dot01);

	for (y = miny; y <= maxy; y++) {
		for (x = minx; x <= maxx; x++) {
			P[0] = x;
			P[1] = y;

			vec2_subtract(P, A, v2);
			dot02 = vec2_dot(v0, v2);
			dot12 = vec2_dot(v1, v2);

			u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			uv = (int)((1 - u - v) * 255.);
			if ((u >= 0.) && (v >= 0.) && (u + v < 1.)) {
				if (valid_tx_cords) {
					vec2_add(vec2_scale(tvu, u, tvut), vec2_scale(tvv, v, tvvt), tvt);
					vec2_add(tvt, tA, NULL);
					source[0] = (int)(tvt[0] * texture->w);
					source[1] = (int)(tvt[1] * texture->h);

					// fuck this 24 bit image
					pixel = *(Uint32*)((Uint8*)texture->pixels + source[1] * texture->w * texture->format->BytesPerPixel + source[0] * texture->format->BytesPerPixel);
					SDL_GetRGB(pixel, texture->format, &r, &g, &b);
					*((Uint32*)screen->pixels + y * SQ_SIZE + x) = SDL_MapRGBA(screen->format, r, g, b, 255);
				} else {
					*((Uint32*)screen->pixels + y * SQ_SIZE + x) = SDL_MapRGBA(screen->format, (int)(u * 255.), uv, (int)(v * 255.), 255);
				}
			}
		}
	}

	*((Uint32*)screen->pixels + (int)A[1] * SQ_SIZE + (int)A[0]) = SDL_MapRGBA(screen->format, 0, 255, 255, 255);
	*((Uint32*)screen->pixels + (int)B[1] * SQ_SIZE + (int)B[0]) = SDL_MapRGBA(screen->format, 0, 255, 255, 255);
	*((Uint32*)screen->pixels + (int)C[1] * SQ_SIZE + (int)C[0]) = SDL_MapRGBA(screen->format, 0, 255, 255, 255);
}

void vertex_shader() {
	static double triangle[3][2], texmap[3][2];
	int c, i, j = 0;
	
	if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

	for (c = 0; c < FACES; c++) {
		//lookup the normal of the first vertex at faces c | don't forget they count from 1
		if (cull_back_face(norm_buf[faces[c][0][2] - 1]))
			continue;

		// memorize frontfacing
		sort_buf[j++] = c;
	}

	//sort indices in sort_buf representing their lowest z value
	qsort(sort_buf, (size_t)j, sizeof(int), zcompare);

	for (c = 0; c < j; c++) {
		for (i = 0; i < 3; i++) {
			triangle[i][0] = vert_buf[faces[sort_buf[c]][i][0] - 1][0] / vert_buf[faces[sort_buf[c]][i][0] - 1][2];
			triangle[i][1] = vert_buf[faces[sort_buf[c]][i][0] - 1][1] / vert_buf[faces[sort_buf[c]][i][0] - 1][2];
			texmap[i][0] = texcord[faces[sort_buf[c]][i][1] - 1][0];
			texmap[i][1] = texcord[faces[sort_buf[c]][i][1] - 1][1];
		}
		fragment_shader((double *)triangle, (double *)texmap);
	}

	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}

void render() {
	static double yaxis[3] = {0., 1., 0.};
	unsigned int new;
	double angle;
#ifndef __EMSCRIPTEN__
	unsigned int next;
#endif

	new = SDL_GetTicks();
#ifndef __EMSCRIPTEN__
	next = new + 16;
#endif

	angle = M_PI * 2. * (double)(new % 50000) / 50000.;
	mat4_identity(model);
	mat4_rotate(model, angle, yaxis, NULL);

	screen_project();
	vertex_shader();

#ifndef __EMSCRIPTEN__
	new = SDL_GetTicks();

	if (new < next)
		SDL_Delay(next - new);
#endif
}

int main(int argc, char *argv[]) {
#ifndef __EMSCRIPTEN__
	SDL_Event Events;
	int run = 1;
#endif


	printf("Hello World\n");
	init_mats();

	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode(SQ_SIZE, SQ_SIZE, 32, SDL_SWSURFACE);
	texture = IMG_Load("color_map_2048.jpg");

#ifdef TEST_SDL_LOCK_OPTS
	EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(render, 0, 1);
#else
	while(run) {
		while (SDL_PollEvent(&Events))
			if (Events.type == SDL_QUIT)
				run = 0;
		render();
	}
#endif

	SDL_Quit();
	return 0;
}
