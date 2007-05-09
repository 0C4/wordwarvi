/* 
    (C) Copyright 2005,2006, Stephen M. Cameron.

    This file is part of wordwarvi.

    wordwarvi is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    wordwarvi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with wordwarvi; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <gtk/gtk.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <gdk/gdkkeysyms.h>

/* For Audio stuff... */
#include <math.h>
#include "portaudio.h"
#include <sndfile.h>

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (1024)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TWOPI (M_PI * 2.0)
#define NCLIPS (16)
#define MAX_CONCURRENT_SOUNDS (16)
#define CLIPLEN (12100) 
int add_sound(int which_sound, int which_slot);
#define ANY_SLOT (-1)
#define MUSIC_SLOT (0)

#define PLAYER_LASER_SOUND 0
#define BOMB_IMPACT_SOUND 1
#define ROCKET_LAUNCH_SOUND 2
#define FLAK_FIRE_SOUND 3
#define LARGE_EXPLOSION_SOUND 4
#define ROCKET_EXPLOSION_SOUND 5
#define LASER_EXPLOSION_SOUND 6
#define GROUND_SMACK_SOUND 7
#define INSERT_COIN_SOUND 8
#define MUSIC_SOUND 9
#define SAM_LAUNCH_SOUND 10 
#define NHUMANOIDS 4
#define HUMANOID_PICKUP_SOUND 8 /* FIXME, this will do for now */

/* ...End of audio stuff */


#define TERRAIN_LENGTH 1000
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define WORLDWIDTH (SCREEN_WIDTH * 40)
#define KERNEL_Y_BOUNDARY (-1000)

#define LARGE_SCALE_ROUGHNESS (0.04)
#define SMALL_SCALE_ROUGHNESS (0.09)
#define MAXOBJS 6500
#define NFLAK 20
#define LASER_DAMAGE 5;
#define NROCKETS 70 
// #define NROCKETS 0
#define LAUNCH_DIST 500
#define MAX_ROCKET_SPEED -32
#define SAM_LAUNCH_DIST 300
#define SAM_LAUNCH_CHANCE 15 
#define PLAYER_SPEED 8 
#define MAX_VX 15
#define MAX_VY 25
#define LASER_FIRE_CHANCE 3
#define LASERLEAD (11)
#define LASER_SPEED 40
#define LASER_PROXIMITY 300 /* square root of 300 */
#define BOMB_PROXIMITY 10000 /* square root of 30000 */
#define BOMB_X_PROXIMITY 100
#define LINE_BREAK (-999)
#define NBUILDINGS 40
#define NBRIDGES 7
#define MAXBUILDING_WIDTH 9
#define NFUELTANKS 20
#define NSAMS 5
#define BOMB_SPEED 10
#define NBOMBS 100
#define MAX_ALT 100
#define MIN_ALT 50
#define MAXHEALTH 100
#define NAIRSHIPS 3
#define NBALLOONS 3 
#define MAX_BALLOON_HEIGHT 300
#define MIN_BALLOON_HEIGHT 50
#define MAX_MISSILE_VELOCITY 12 
#define MISSILE_DAMAGE 20
#define MISSILE_PROXIMITY 10
#define HUMANOID_PICKUP_SCORE 100
#define HUMANOID_DIST 10

/* Scoring stuff */
#define ROCKET_SCORE 20
#define BRIDGE_SCORE 5
#define FLAK_SCORE 25


int game_pause = 0;
int attract_mode = 0;
int credits = 0;
int toggle = 0;
int timer = 0;
int next_timer = 0;
int timer_event = 0;
#define BLINK_EVENT 1
#define READY_EVENT 2
#define SET_EVENT 3
#define GO_EVENT 4
#define BLANK_EVENT 5
#define GAME_OVER_EVENT 6
#define BLANK_GAME_OVER_1_EVENT 7
#define INSERT_COIN_EVENT 8
#define BLANK_GAME_OVER_2_EVENT 9
#define GAME_ENDED_EVENT 10
#define GAME_ENDED_EVENT_2 11
#define CREDITS1_EVENT 12
#define CREDITS2_EVENT 13
#define INTRO1_EVENT 14
#define INTRO2_EVENT 15

#define NCOLORS 8

GdkColor huex[NCOLORS];

#define WHITE 0
#define BLUE 1
#define BLACK 2
#define GREEN 3
#define YELLOW 4
#define RED 5
#define ORANGE 6
#define CYAN 7


/* Object types */
#define OBJ_TYPE_AIRSHIP 'a'
#define OBJ_TYPE_BALLOON 'B'
#define OBJ_TYPE_BUILDING 'b'
#define OBJ_TYPE_CHAFF 'c'
#define OBJ_TYPE_FUEL 'f'
#define OBJ_TYPE_GUN 'g'
#define OBJ_TYPE_HUMAN 'h'
#define OBJ_TYPE_LASER 'L'
#define OBJ_TYPE_MISSILE 'm'
#define OBJ_TYPE_ROCKET 'r'
#define OBJ_TYPE_SOCKET 'x'
#define OBJ_TYPE_SAM_STATION 'S'
#define OBJ_TYPE_SPARK 's'
#define OBJ_TYPE_BRIDGE 'T'

int current_level = 0;
struct level_parameters_t {
	int random_seed;
	int nrockets;
	int nbridges;
	int nflak;
	int nfueltanks;
	int nsams;
	int nhumanoids;
	int nbuildings;
	int nbombs;
	int laser_fire_chance;
	double large_scale_roughness;
	double small_scale_roughness;
} level = {
	31415927,
	NROCKETS,
	NBRIDGES,
	NFLAK,
	NFUELTANKS,
	NSAMS,
	NHUMANOIDS,
	NBUILDINGS,
	NBOMBS,
	LASER_FIRE_CHANCE,
	LARGE_SCALE_ROUGHNESS,
	SMALL_SCALE_ROUGHNESS,
};

/**** LETTERS and stuff *****/


typedef unsigned char stroke_t;

/**************************

	Here's how this works, there's a 3x7 grid. on which the
	letters are drawn.  You list the sequence of strokes.
	Use 21 to lift the pen, 99 to mark the end.

	Inspired by Hofstadters' Creative Concepts and Creative Analogies.

                               letter 'A'    can be compactly represented by:

		0   1   2      *   *   *    { 6, 8, 14, 12, 9, 11, 99 }

                3   4   5      *   *   *

                6   7   8      *---*---*
                                       |
                9  10  11      *---*---*
                               |       |
               12  13  14      *---*---*

               15  16  17      *   *   *

               18  19  20      *   *   *
The grid numbers can be decoded into (x,y) coords like:
	x = ((n % 3) * xscale);
	y = (x/3) * yscale;     // truncating division.
***************************/
stroke_t glyph_Z[] = { 0, 2, 12, 14, 99 };
stroke_t glyph_Y[] = { 0, 7, 2, 21, 7, 13, 99 };
stroke_t glyph_X[] = { 0, 14, 21, 12, 2, 99 };
stroke_t glyph_W[] = { 0, 12, 10, 14, 2, 21, 10, 1, 99 };
stroke_t glyph_V[] = { 0, 13, 2, 99 };
stroke_t glyph_U[] = { 0, 9, 13, 11, 2, 99 };
stroke_t glyph_T[] = { 13, 1, 21, 0, 2, 99 };
stroke_t glyph_S[] = { 9, 13, 11, 3, 1, 5, 99 };
stroke_t glyph_R[] = { 12, 0, 1, 5, 7, 6, 21, 7, 14, 99 };
stroke_t glyph_Q[] = { 13, 9, 3, 1, 5, 11, 13, 21, 10, 14, 99 };
stroke_t glyph_P[] = { 12, 0, 1, 5, 7, 6, 99 };
stroke_t glyph_O[] = { 13, 9, 3, 1, 5, 11, 13, 99 };
stroke_t glyph_N[] = { 12, 0, 14, 2, 99 };
stroke_t glyph_M[] = { 12, 0, 4, 2, 14, 99 };
stroke_t glyph_L[] = { 0, 12, 14, 99};
stroke_t glyph_K[] = { 0, 12, 21, 6, 7, 11, 14, 21, 7, 5, 2, 99};
stroke_t glyph_J[] = { 9, 13, 11, 2, 99}; 
stroke_t glyph_I[] = { 12, 14, 21, 13, 1, 21, 0, 2, 99 }; 
stroke_t glyph_H[] = { 0, 12, 21, 2, 14, 21, 6, 8, 99 };
stroke_t glyph_G[] = { 7, 8, 11, 13, 9, 3, 1, 5, 99 };
stroke_t glyph_F[] = { 12, 0, 2, 21, 8, 7, 99 };
stroke_t glyph_E[] = { 14, 12, 0, 2, 21, 6, 7, 99 };
stroke_t glyph_D[] = { 12, 13, 11, 5, 1, 0, 12, 99 };
stroke_t glyph_C[] = { 11, 13, 9, 3, 1, 5, 99 };
stroke_t glyph_B[] = { 0, 12, 13, 11, 5, 1, 0, 21, 6, 8, 99 };
stroke_t glyph_A[] = { 12, 3, 0, 5, 14, 21, 8, 6, 99 };
stroke_t glyph_slash[] = { 12, 2, 99 };
stroke_t glyph_que[] = { 13, 10, 21, 7, 5, 2, 0, 3, 99 };
stroke_t glyph_bang[] = { 10, 13, 21, 1, 7, 99};
stroke_t glyph_colon[] = { 6, 7, 21, 12, 13, 99 };
stroke_t glyph_leftparen[] = { 2, 4, 10, 14,99 };
stroke_t glyph_rightparen[] = { 0, 4, 10, 12,99 };
stroke_t glyph_space[] = { 99 };
stroke_t glyph_plus[] = { 6, 8, 21, 7, 13, 99 };
stroke_t glyph_dash[] = { 6, 8, 99 };
stroke_t glyph_0[] = { 12, 0, 2, 14, 12, 2, 99 };
stroke_t glyph_9[] = { 8, 6, 0, 2, 14, 99 };
stroke_t glyph_8[] = { 0, 2, 14, 12, 0, 21, 6, 8, 99 };
stroke_t glyph_7[] = { 0, 2, 12, 99 };
stroke_t glyph_6[] = { 2, 0, 12, 14, 8, 6, 99 };
stroke_t glyph_5[] = { 2, 0, 6, 8, 14, 12, 99 };
stroke_t glyph_4[] = { 0, 6, 8, 21, 2, 14, 99 };
stroke_t glyph_3[] = { 0, 2, 14, 12, 21, 6, 8, 99 };
stroke_t glyph_2[] = { 0, 2, 5, 9, 12, 14, 99 };
stroke_t glyph_1[] = { 1, 13, 99 };
stroke_t glyph_comma[] = { 13, 16, 99 };
stroke_t glyph_period[] = { 12, 13, 99 };
stroke_t glyph_z[] = { 6, 8, 12, 14, 99 };
stroke_t glyph_y[] = { 6, 12, 14, 21, 8, 17, 19, 18,  99};
stroke_t glyph_x[] = { 12, 8, 21, 6, 14, 99 };
stroke_t glyph_w[] = { 6, 12, 14, 8, 21, 7, 13, 99 };
stroke_t glyph_v[] = { 6, 13, 8, 99 };
stroke_t glyph_u[] = { 6, 12, 14, 8, 99 };
stroke_t glyph_t[] = { 14, 13, 4, 21, 6, 8, 99 };
stroke_t glyph_s[] = { 8, 6, 9, 11, 14,12, 99 };

stroke_t glyph_a[] = { 6, 8, 14, 12, 9, 11, 99 };
stroke_t glyph_b[] = { 0, 12, 14, 8, 6, 99 };
stroke_t glyph_c[] = { 8, 6, 12, 14, 99 };
stroke_t glyph_d[] = { 8, 6, 12, 14, 2, 99 };
stroke_t glyph_e[] = { 9, 11, 8, 6, 12, 14, 99 };
stroke_t glyph_f[] = { 13, 1, 2, 21, 6, 8, 99 };
stroke_t glyph_g[] = { 11, 12, 6, 8, 20, 18, 99 };
stroke_t glyph_h[] = { 0, 12, 21, 6, 8, 14, 99 };
stroke_t glyph_i[] = { 13, 7, 21, 4, 2, 99 };
stroke_t glyph_j[] = { 18, 16, 7, 21, 4, 2, 99 };
stroke_t glyph_k[] = { 0, 12, 21, 6, 7, 11, 14, 21, 7, 5, 99 };
stroke_t glyph_l[] = { 1, 13, 99 };
stroke_t glyph_m[] = { 12, 6, 8, 14, 21, 7, 13, 99 };
stroke_t glyph_n[] = { 12, 6, 7, 13, 99 };
stroke_t glyph_o[] = { 12, 6, 8, 14, 12, 99 };
stroke_t glyph_p[] = { 18, 6, 8, 14, 12, 99 };
stroke_t glyph_q[] = { 14, 12, 6, 8, 20, 99 };
stroke_t glyph_r[] = { 12, 6, 21, 9,7,8,99 };



struct target_t;

struct my_point_t {
	int x,y;
};


struct my_point_t decode_glyph[] = {
	{ 0, -4 },
	{ 1, -4 },
	{ 2, -4 },
	{ 0, -3 },
	{ 1, -3 },
	{ 2, -3 },
	{ 0, -2 },
	{ 1, -2 },
	{ 2, -2 },
	{ 0, -1 },
	{ 1, -1 },
	{ 2, -1 },
	{ 0, -0 },
	{ 1, -0 },
	{ 2, -0 },
	{ 0, 1 },
	{ 1, 1 },
	{ 2, 1 },
	{ 0, 2 },
	{ 1, 2 },
	{ 2, 2 },
	{ 0, 3 },
	{ 1, 3 },
	{ 2, 3 },
};
/**** end of LETTERS and stuff */

#define MAX_VELOCITY_TO_COMPUTE 20 
#define V_MAGNITUDE (20.0)
struct my_point_t vxy_2_dxy[MAX_VELOCITY_TO_COMPUTE+1][MAX_VELOCITY_TO_COMPUTE+1];

struct my_point_t humanoid_points[] = {
	{ -5, 0 },
	{ -3, -5 },
	{ 3, -5, },
	{ 3, 0 },
	{ LINE_BREAK, LINE_BREAK },
	{ 3, -5 },
	{ 3, -9 },
	{ 1, -9 },
	{ 1, -12 },
	{ -1, -12 },
	{ -1, -9 },
	{ -3, -9 },
	{ LINE_BREAK, LINE_BREAK },
	{ 6, -5 },
	{ 3, -9 },
	{ -3, -9 },
	{ -6, -5 },
};

struct my_point_t SAM_station_points[] = {
	{ -5, 0 },   /* Bottom base */
	{ -5, -10 },
	{ 5, 0 },
	{ 5, -10 },
	{ -5, 0 },
	{ 5, 0 }, 	
	{ LINE_BREAK, LINE_BREAK },
	{ -5, -10 },   /* middle base */
	{ -5, -20 },
	{ 5, -10 },
	{ 5, -20 },
	{ -5, -10 },
	{ 5, -10 }, 	
	{ LINE_BREAK, LINE_BREAK },
	{ -5, -20 },   /* Top base */
	{ -5, -30 },
	{ 5, -20 },
	{ 5, -30 },
	{ -5, -20 },
	{ 5, -20 }, 	
	{ LINE_BREAK, LINE_BREAK },
	{ -5, -30 },
	{ 5, -30 }, 	
	{ LINE_BREAK, LINE_BREAK },
	{ -3, -30 }, /* Base of radar */
	{ 0, -35, },
	{ 3, -30 },
	{ LINE_BREAK, LINE_BREAK },
	{ 0, -35, }, /* Radar dish */
	{ 10, -45 },
	{ 13, -59 },
	{ LINE_BREAK, LINE_BREAK },
	{ 0, -35, }, /* Radar dish */
	{ -10, -25, },
	{ -25, -22, },
	{ 13, -59 },
	{ LINE_BREAK, LINE_BREAK },
	{ 0, -35, }, /* Radar dish */
	{ -15, -50, }, /* Radar dish */
	{ LINE_BREAK, LINE_BREAK },
	{ 20, 0 }, /* Little building */
	{ 20, -50 },
	{ 30, -50 },
	{ 30, 0 },

};

struct my_point_t airship_points[] = {
	{ -70, -50 }, /* tip of nose */
	{ -60, -60 },
	{ -50, -65 },
	{ -40, -67 },
	{ -30, -70 }, /* top left */
	{  30, -70 }, /* top right */
	{  40, -67 },
	{  50, -65 },
	{  60, -60 },
	{  70, -50 },
	{  LINE_BREAK, LINE_BREAK },
	/* Now the same shape, but with y displacement to make bottom of blimp */
	{ -70, -50 }, /* tip of nose */
	{ -60, -40 },
	{ -50, -35 },
	{ -40, -33 },
	{ -30, -30 }, /* bottom left */
	{  30, -30 }, /* bottom right */
	{  40, -33 },
	{  50, -35 },
	{  60, -40 },
	{  70, -50 }, /* back point */
	{  LINE_BREAK, LINE_BREAK },
	{  60, -60 }, /* top tail */
	{  70, -70 },
	{  90, -70 },
	{  90, -60 },
	{  70, -50 }, /* back point */
	{  90, -40 },
	{  90, -30 },
	{  70, -30 },
	{  60, -40 },
	{  LINE_BREAK, LINE_BREAK },  /* central tail fin */
	{  60, -50 } ,
	{  90, -50 },
	{  LINE_BREAK, LINE_BREAK },  /* Gondola */
	{  -10, -30 },
	{    -5, -20 },
	{  10, -20 },
	{  10, -30 },
};

struct my_point_t balloon_points[] = {
	{ 0, -100 },
	{ -20, -98 }, 
	{ -35, -90 },
	{ -45, -80 },
	{ -47, -70 },
	{ -47, -60 },
	{ -40, -50 },
	{ -8, -20 },
	{ -8, -10 },
	{  8, -10 },
	{  8, -20 },
	{  40, -50 }, 
	{  47, -60 },
	{  47, -70 },
	{  45, -80 },
	{  35, -90 },
	{  20, -98 }, 
	{ 0, -100 },
	{  LINE_BREAK, LINE_BREAK },  /* Gondola strings */
	{ -8, -10 },
	{ -5, 5, },
	{ 5, 5, },
	{ 8, -10 },
	{  LINE_BREAK, LINE_BREAK },  /* Gondola */
	{ -5, 5,},
	{ -5, 0,},
	{  5, 0,},
	{  5, 5,},
};


struct my_point_t spark_points[] = {
	{ -1,-1 },
	{ -1, 1},
	{ 1, 1 },
	{ 1, -1 },
	{ -1, -1 },
#if 0
	{ 0, 0 },
	{ 0, 10 },
	{ 10, 10 },
	{ 10, 0 },
	{ 0, 0 },
#endif
};

struct my_point_t fuel_points[] = {
	{ -30, -10 },
	{ 30, -10 },
	{ 30, 30 },
	{ -30, 30 },
	{ -30, -10 },
	{ LINE_BREAK, LINE_BREAK },
	{ -25, 25 },
	{ -25, -5 },
	{ -15, -5 },
	{ LINE_BREAK, LINE_BREAK },
	{ -25, 15 },
	{ -15, 15 },
	{ LINE_BREAK, LINE_BREAK },
	{ -10, -5 },
	{ -10, 25 },
	{ 0, 25 },
	{ 0, -5 },
	{ LINE_BREAK, LINE_BREAK },
	{ 15, -5 },
	{ 5, -5 },
	{ 5, 25 },
	{ 15, 25 },
	{ LINE_BREAK, LINE_BREAK },
	{ 5, 15 },
	{ 10, 15 },
	{ LINE_BREAK, LINE_BREAK },
	{ 18, -25 },
	{ 18, 25 },
	{ 25, 25 },
};

struct my_point_t right_laser_beam_points[] = {
	{ -100, 0 },
	{ -80, 0 },
	{ LINE_BREAK, LINE_BREAK },
	{ -75, 0 },
	{ -50, 0 },
	{ LINE_BREAK, LINE_BREAK },
	{ -45, 0 },
	{ 0, 0 },
};

struct my_point_t player_ship_points[] = {
	{ 9, 0 }, /* front of hatch */
	{ 0,0 },
	{ -3, -6 }, /* top of hatch */
	{ -12, -12 },
	{ -18, -12 },
	{ -15, -4 }, /* bottom of tail */
	{ -3, -4 }, /* bottom of tail */
	{ -15, -4 }, /* bottom of tail */
	{ -15, 3 }, /* back top of wing */
	{ 0, 3 }, /* back top of wing */
	{ -15, 3 }, /* back top of wing */
	{ -18, 9 }, /* back bottom of wing */
	{ -12, 9 }, /* back front of wing */
	{ 0, 3 },
	{ -6, 6 },
	{ 20, 4 },
	{ 24, 2 }, /* tip of nose */
	{ 9, 0 }, /* front of hatch */
	{ -3, -6 }, /* top of hatch */ 
#if 0
	{ LINE_BREAK, LINE_BREAK }, /* Just for testing */
	{ -30, -20 },
	{ 30, -20 },
	{ 30, 20 },
	{ -30, 20 },
	{ -30, -20 },
#endif
};

struct my_point_t socket_points[] = {
	{ 9, 0 }, /* front of hatch */
	{ -3, -6 }, /* top of hatch */
	{ -12, -12 },
	{ -18, -12 },
	{ -15, -4 }, /* bottom of tail */
	// { -3, -4 }, /* bottom of tail */
	// { -15, -4 }, /* bottom of tail */
	{ -15, 3 }, /* back top of wing */
	//{ 0, 3 }, /* back top of wing */
	//{ -15, 3 }, /* back top of wing */
	{ -18, 9 }, /* back bottom of wing */
	{ -12, 9 }, /* back front of wing */
	// { LINE_BREAK, LINE_BREAK },
	// { 0, 3 },
	{ -6, 6 },
	{ 20, 4 },
	{ 24, 2 }, /* tip of nose */
	{ 9, 0 }, /* front of hatch */
	{ LINE_BREAK, LINE_BREAK },
	{ -30, -25 },
	{ 30, -25 },
	{ 30, 25 },
	{ -30, 25 },
	{ -30, -25 },
	// { -3, -6 }, /* top of hatch */ 
#if 0
	{ LINE_BREAK, LINE_BREAK }, /* Just for testing */
	{ -30, -20 },
	{ 30, -20 },
	{ 30, 20 },
	{ -30, 20 },
	{ -30, -20 },
#endif
};

struct my_point_t left_player_ship_points[] = {
	/* Data is reversed algorithmically */
	{ 9, 0 }, /* front of hatch */
	{ 0,0 },
	{ -3, -6 }, /* top of hatch */
	{ -12, -12 },
	{ -18, -12 },
	{ -15, -4 }, /* bottom of tail */
	{ -3, -4 }, /* bottom of tail */
	{ -15, -4 }, /* bottom of tail */
	{ -15, 3 }, /* back top of wing */
	{ 0, 3 }, /* back top of wing */
	{ -15, 3 }, /* back top of wing */
	{ -18, 9 }, /* back bottom of wing */
	{ -12, 9 }, /* back front of wing */
	{ 0, 3 },
	{ -6, 6 },
	{ 20, 4 },
	{ 24, 2 }, /* tip of nose */
	{ 9, 0 }, /* front of hatch */
	{ -3, -6 }, /* top of hatch */ 
#if 0
	{ LINE_BREAK, LINE_BREAK }, /* Just for testing */
	{ -30, -20 },
	{ 30, -20 },
	{ 30, 20 },
	{ -30, 20 },
	{ -30, -20 },
#endif
};

struct my_point_t bomb_points[] = {
	{ -2, -6 },
	{ 2, -6 },
	{ 2, -4 },
	{ 0, -3 },
	{ -2, -4 },
	{ -2, -6 },
	{ LINE_BREAK, LINE_BREAK },
	{ 0, -3 },
	{ 2, -1 },
	{ 2, 8 },
	{ 0, 9 },
	{ -2, 8 },
	{ -2, -1 },
	{ 0, -3 },
};

struct my_point_t flak_points[] = {
	{ -10, 5 },
	{ -5, -3 },
	{ 5, -3},
	{ 10, 5 },
	{ LINE_BREAK, LINE_BREAK },
	{ -3, -3 },
	{ -3, -5},
	{ 3, -5},
	{ 3, -3},
};

struct my_point_t rocket_points[] = {
	{ -2, 3 },
	{ -4, 7 },
	{ -2, 7 },
	{ -2, -8 },
	{ 0, -10 },
	{ 2, -8 },
	{ 2, 7},
	{ 4, 7},
	{ 2, 3}, 
};

struct my_point_t bridge_points[] = {  /* square with an x through it, 8x8 */
	{ -4, -4 },
	{ -4, 4 },
	{ 4, 4 },
	{ -4, -4 },
	{ 4, -4 },
	{ -4, 4 },
	{ LINE_BREAK, LINE_BREAK },
	{ 4, -4 },
	{ 4, 4 },
};

struct my_vect_obj {
	int npoints;
	struct my_point_t *p;	
};

struct my_vect_obj player_vect;
struct my_vect_obj left_player_vect;
struct my_vect_obj rocket_vect;
struct my_vect_obj spark_vect;
struct my_vect_obj right_laser_vect;
struct my_vect_obj fuel_vect;
struct my_vect_obj bomb_vect;
struct my_vect_obj bridge_vect;
struct my_vect_obj flak_vect;
struct my_vect_obj airship_vect;
struct my_vect_obj balloon_vect;
struct my_vect_obj SAM_station_vect;
struct my_vect_obj humanoid_vect;
struct my_vect_obj socket_vect;

struct my_vect_obj **gamefont[2];
#define BIG_FONT 0
#define SMALL_FONT 1
#define BIG_FONT_SCALE 14 
#define SMALL_FONT_SCALE 5 
#define BIG_LETTER_SPACING (10)
#define SMALL_LETTER_SPACING (5)
#define MAXTEXTLINES 20
int current_color = WHITE;
int current_font = BIG_FONT;
int cursorx = 0;
int cursory = 0;
int livecursorx = 0;
int livecursory = 0;
int font_scale[2] = { BIG_FONT_SCALE, SMALL_FONT_SCALE };
int letter_spacing[2] = { BIG_LETTER_SPACING, SMALL_LETTER_SPACING };

int ntextlines = 0;
struct text_line_t {
	int x, y, font;
	char string[80];
} textline[20];

#define FINAL_MSG1 "Where is your"
#define FINAL_MSG2 "editor now???"

/* text line entries are just fixed... */
#define GAME_OVER 1
#define CREDITS 0

void init_vxy_2_dxy()
{
	int x, y;
	double dx, dy, angle;

	/* Given a velocity, (vx, vy), precompute offsets */
	/* for a fixed magnitude */

	for (x=0;x<=MAX_VELOCITY_TO_COMPUTE;x++) {
		for (y=0;y<=MAX_VELOCITY_TO_COMPUTE;y++) {
			if (x == 0) {
				vxy_2_dxy[x][y].y = 
					((double) y / (double) MAX_VELOCITY_TO_COMPUTE) * V_MAGNITUDE;
				vxy_2_dxy[x][y].x = 0;
				continue;
			}
			dx = x;
			dy = y;
			angle = atan(dy/dx);
			dx = cos(angle) * V_MAGNITUDE;
			dy = -sin(angle) * V_MAGNITUDE;
			vxy_2_dxy[x][y].x = (int) dx;
			vxy_2_dxy[x][y].y = (int) dy;
		}
	}
}

static inline int dx_from_vxy(int vx, int vy) 
{
	if ((abs(vx) > MAX_VELOCITY_TO_COMPUTE) || (abs(vy) > MAX_VELOCITY_TO_COMPUTE))
		return 0;
	return (vx < 0) ?  -vxy_2_dxy[abs(vx)][abs(vy)].x : vxy_2_dxy[abs(vx)][abs(vy)].x;
}

static inline int dy_from_vxy(int vx, int vy) 
{
	if ((abs(vx) > MAX_VELOCITY_TO_COMPUTE) || (abs(vy) > MAX_VELOCITY_TO_COMPUTE))
		return 0;
	return (vy < 0) ?  -vxy_2_dxy[abs(vx)][abs(vy)].y : vxy_2_dxy[abs(vx)][abs(vy)].y;
}

void set_font(int fontnumber)
{
	current_font = fontnumber;
}

void gotoxy(int x, int y)
{
	cursorx = (x+1) * font_scale[current_font]*3;
	cursory = (y+1) * font_scale[current_font]*7;
}

void cleartext()
{
	ntextlines = 0;
}

void gameprint(char *s)
{
	int n;

	/* printf("Printing '%s'\n", s); */
	n = ntextlines;
	if (n>=MAXTEXTLINES)
		n = 0;
	textline[n].x = cursorx;
	textline[n].y = cursory;
	textline[n].font = current_font;
	strcpy(textline[n].string, s);
	ntextlines++;
	if (ntextlines >=MAXTEXTLINES)
		ntextlines = MAXTEXTLINES-1;
}

int current_font_scale = BIG_FONT_SCALE;

struct game_obj_t;

typedef void obj_move_func(struct game_obj_t *o);
typedef void obj_draw_func(struct game_obj_t *o, GtkWidget *w);

struct game_obj_t {
	obj_move_func *move;
	obj_draw_func *draw;
	struct my_vect_obj *v;
	struct target_t *target;
	int x, y;
	int vx, vy;
	int color;
	int alive;
	int otype;
	struct game_obj_t *bullseye;
};

GtkWidget *score_label;
GtkWidget *bombs_label;

struct target_t {
	struct game_obj_t *o;
	struct target_t *prev, *next;
} *target_head = NULL;

struct terrain_t {
	int npoints;
	int x[TERRAIN_LENGTH];
	int y[TERRAIN_LENGTH];
} terrain;

struct game_state_t {
	int x;
	int y;
	int last_x1, last_x2;
	int vx;
	int vy;
	int lives;
	int nobjs;
	int direction;
	int health;
	int score;
	int prev_score;
	int nbombs;
	int prev_bombs;
	int humanoids;
	struct game_obj_t go[MAXOBJS];
} game_state = { 0, 0, 0, 0, PLAYER_SPEED, 0, 0 };

struct game_obj_t *player = &game_state.go[0];

GdkGC *gc = NULL;
GtkWidget *main_da;
gint timer_tag;

struct target_t *add_target(struct game_obj_t *o)
{
	struct target_t *t;

	t = malloc(sizeof(struct target_t));
	if (t == NULL) {
		printf("add target failed.\n");
		return NULL;
	}

	t->o = o;
	t->prev = NULL;
	if (target_head == NULL) { 
		target_head = t;
		t->next = NULL;
	} else {
		t->next = target_head;
		target_head->prev = t;
		target_head = t;
	}
	return target_head;
}

void print_target_list()
{
	struct target_t *t;
	printf("Targetlist:\n");
	for (t=target_head; t != NULL;t=t->next) {
		printf("%c: %d,%d\n", t->o->otype, t->o->x, t->o->y);
	}
	printf("end of list.\n");
}
struct target_t *remove_target(struct target_t *t)
{

	struct target_t *next;
	if (!t)
		return NULL;

	next = t->next;
	if (t == target_head)
		target_head = t->next;
	if (t->next)
		t->next->prev = t->prev;
	if (t->prev)
		t->prev->next = t->next;
	t->next = NULL;
	t->prev = NULL;
	free(t);
	return next;
}

int randomn(int n)
{
	return (int) (((random() + 0.0) / (RAND_MAX + 0.0)) * (n + 0.0));
}

int randomab(int a, int b)
{
	int x, y;
	if (a > b) {
		x = b;
		y = a;
	} else {
		x = a;
		y = b;
	}
	return (int) (((random() + 0.0) / (RAND_MAX + 0.0)) * (y - x + 0.0)) + x;
}

void explode(int x, int y, int ivx, int ivy, int v, int nsparks, int time);

void move_laserbolt(struct game_obj_t *o)
{
	int dy;
	if (!o->alive)
		return;
	dy = (o->y - player->y);
	if (dy < -1000) {
		o->alive = 0;
		return;
	}
	if (abs(dy) < 9 && abs(player->x - o->x) < 15) {
		explode(o->x, o->y, o->vx, 1, 70, 20, 20);
		add_sound(LASER_EXPLOSION_SOUND, ANY_SLOT);
		game_state.health -= LASER_DAMAGE;
		o->alive = 0;	
	}
	o->x += o->vx;
	o->y += o->vy;
	o->alive--;
}

static void add_laserbolt(int x, int y, int vx, int vy, int time);
void move_flak(struct game_obj_t *o)
{
	int xdist;
	int dx, dy, bx,by;
	int x1, y1;
	xdist = abs(o->x - player->x);
	if (xdist < SCREEN_WIDTH && randomn(100) < level.laser_fire_chance) {
		dx = player->x+LASERLEAD*player->vx - o->x;
		dy = player->y+LASERLEAD*player->vy - o->y;

		add_sound(FLAK_FIRE_SOUND, ANY_SLOT);
		if (dy >= 0) {
			if (player->x+player->vx*LASERLEAD < o->x)
				bx = -20;
			else
				bx = 20;
			by = 0;
		} else if (dx == 0) {
			bx = -0;
			by = -20;
		} else if (abs(dx) > abs(dy)) {
			if (player->x+player->vx*LASERLEAD < o->x)
				bx = -20;
			else
				bx = 20;
			by = -abs((20*dy)/dx);
		} else {
			by = -20;
			/* if (player->x < o->x)
				bx = -20;
			else
				bx = 20; */
			bx = (-20*dx)/dy;
		}
		x1 = o->x-5;
		y1 = o->y-5;  
		add_laserbolt(x1, y1, bx, by, 50);
		add_laserbolt(x1+10, y1, bx, by, 50);
	}
}

static void add_missile(int x, int y, int vx, int vy, 
	int time, int color, struct game_obj_t *bullseye);

void move_rocket(struct game_obj_t *o)
{
	int xdist, ydist;
	if (!o->alive)
		return;

	xdist = abs(o->x - player->x);
	if (xdist < LAUNCH_DIST) {
		ydist = o->y - player->y;
		if ((xdist <= ydist && ydist > 0) || o->vy != 0) {
			if (o->vy == 0)
				add_sound(ROCKET_LAUNCH_SOUND, ANY_SLOT);
			if (o->vy > MAX_ROCKET_SPEED)
				o->vy--;
		}

		if ((ydist*ydist + xdist*xdist) < 400) {
			add_sound(ROCKET_EXPLOSION_SOUND, ANY_SLOT);
			explode(o->x, o->y, o->vx, 1, 70, 150, 20);
			o->alive = 0;
			game_state.health -= 20;
			remove_target(o->target);
			return;
		}
	}
	o->x += o->vx;
	o->y += o->vy;
	if (o->vy != 0)
		explode(o->x, o->y, 0, 9, 8, 7, 13);
	if (o->y - player->y < -1000 && o->vy != 0) {
		o->alive = 0;
		remove_target(o->target);
	}
}

void sam_move(struct game_obj_t *o)
{
	int xdist, ydist;
	if (!o->alive)
		return;

	xdist = abs(o->x - player->x);
	if (xdist < SAM_LAUNCH_DIST) {
		ydist = o->y - player->y;
		if (ydist > 0 && randomn(1000) < SAM_LAUNCH_CHANCE) {
			add_sound(SAM_LAUNCH_SOUND, ANY_SLOT);
			add_missile(o->x+20, o->y-30, 0, 0, 300, GREEN, player);
		}
	}
}

void humanoid_move(struct game_obj_t *o)
{
	int xdist, ydist;
	if (!o->alive)
		return;

	xdist = abs(o->x - player->x);
	ydist = abs(o->y - player->y);
	if (xdist < HUMANOID_DIST && ydist < HUMANOID_DIST) {
		add_sound(HUMANOID_PICKUP_SOUND, ANY_SLOT);
		o->x = -1000; /* take him off screen. */
		o->y = -1000;
		game_state.score += HUMANOID_PICKUP_SCORE;
		game_state.humanoids++;
	}
}

void advance_level();
void socket_move(struct game_obj_t *o)
{
	int xdist, ydist;
	if (!o->alive)
		return;

	xdist = abs(o->x - player->x);
	ydist = abs(o->y - player->y);
	/* HUMANOID_DIST is close enough */
	if (xdist < HUMANOID_DIST && ydist < HUMANOID_DIST) {
		add_sound(HUMANOID_PICKUP_SOUND, ANY_SLOT);
		advance_level();
	}
}

int find_free_obj();

void laser_move(struct game_obj_t *o);

void player_fire_laser()
{
	int i;
	struct game_obj_t *o, *p;

	i = find_free_obj();
	o = &game_state.go[i];
	p = &game_state.go[0];

	if (p != player) {
		printf("p != player!\n");
	} 

	o->x = p->x+(30 * game_state.direction);
	o->y = p->y;
	o->vx = p->vx + LASER_SPEED * game_state.direction;
	o->vy = 0;
	o->v = &right_laser_vect;
	o->move = laser_move;
	o->otype = OBJ_TYPE_LASER;
	o->color = GREEN;
	o->alive = 20;
	add_sound(PLAYER_LASER_SOUND, ANY_SLOT);
}

int interpolate(int x, int x1, int y1, int x2, int y2)
{
	/* return corresponding y on line x1,y1,x2,y2 for value x */
	/*
		(y2 -y1)/(x2 - x1) = (y - y1) / (x - x1)
		(x -x1) * (y2 -y1)/(x2 -x1) = y - y1
		y = (x - x1) * (y2 - y1) / (x2 -x1) + y1;
	*/
	if (x2 == x1)
		return y1;
	else
		return (x - x1) * (y2 - y1) / (x2 -x1) + y1;
}

#define GROUND_OOPS 64000
int ground_level(int x)
{
	/* Find the level of the ground at position x */
	int deepest, i;

	/* Detect smashing into the ground */
	deepest = GROUND_OOPS;
	for (i=0;i<TERRAIN_LENGTH-1;i++) {
		if (x >= terrain.x[i] && x < terrain.x[i+1]) {
			deepest = interpolate(x, terrain.x[i], terrain.y[i],
					terrain.x[i+1], terrain.y[i+1]);
			break;
		}
	}
	return deepest;
}

void bridge_move(struct game_obj_t *o);
void no_move(struct game_obj_t *o);

void bomb_move(struct game_obj_t *o)
{
	struct target_t *t;
	int deepest;
	int dist2;
	int removed;

	if (!o->alive)
		return;
	o->x += o->vx;
	o->y += o->vy;
	o->vy++;

	for (t=target_head;t != NULL;) {
		if (!t->o->alive) {
			t=t->next;
			continue;
		}
		removed = 0;
		switch (t->o->otype) {
			case OBJ_TYPE_ROCKET:
			case OBJ_TYPE_FUEL:
			case OBJ_TYPE_GUN:
			case OBJ_TYPE_SAM_STATION:  {
				dist2 = (o->x - t->o->x)*(o->x - t->o->x) + 
					(o->y - t->o->y)*(o->y - t->o->y);
				if (dist2 < LASER_PROXIMITY) { /* a hit */
					game_state.score += ROCKET_SCORE;
					add_sound(BOMB_IMPACT_SOUND, ANY_SLOT);
					explode(t->o->x, t->o->y, t->o->vx, 1, 70, 150, 20);
					t->o->alive = 0;
					t = remove_target(t);
					removed = 1;
					o->alive = 0;
				}
			}
			default:
				break;
		}
		if (!removed)
			t=t->next;
	}

	/* Detect smashing into the ground */
	deepest = ground_level(o->x);
	if (deepest != GROUND_OOPS && o->y > deepest) {
		o->alive = 0;
		add_sound(BOMB_IMPACT_SOUND, ANY_SLOT);
		explode(o->x, o->y, o->vx, 1, 90, 150, 20);
		/* find nearby targets */
		for (t=target_head;t != NULL;) {
			if (!t->o->alive) {
				t=t->next;
				continue;
			}
			removed = 0;
			switch (t->o->otype) {
				case OBJ_TYPE_ROCKET:
				case OBJ_TYPE_GUN:
				case OBJ_TYPE_SAM_STATION:
				case OBJ_TYPE_FUEL: {
					dist2 = (o->x - t->o->x)*(o->x - t->o->x) + 
						(o->y - t->o->y)*(o->y - t->o->y);
					if (dist2 < BOMB_PROXIMITY) { /* a hit */
						if (o->otype == OBJ_TYPE_ROCKET)
							game_state.score += ROCKET_SCORE;
						explode(t->o->x, t->o->y, t->o->vx, 1, 70, 150, 20);
						t->o->alive = 0;
						if (t->o->otype == OBJ_TYPE_FUEL) {
							game_state.health += 10;
							if (game_state.health > MAXHEALTH)
								game_state.health = MAXHEALTH;
						}
						t = remove_target(t);
						removed = 1;
					}
				}
				break;

				case OBJ_TYPE_BRIDGE:
					if (abs(o->x - t->o->x) < BOMB_X_PROXIMITY) { /* a hit */
						/* "+=" instead of "=" in case multiple bombs */
						if (t->o->move == no_move) /* only get the points once. */
							game_state.score += BRIDGE_SCORE;
						t->o->vx += ((t->o->x < o->x) ? -1 : 1) * randomn(6);
						t->o->vy += ((t->o->y < o->y) ? -1 : 1) * randomn(6);
						t->o->move = bridge_move;
						t->o->alive = 20;
					}
					break;
				default:
					break;
			}
			if (!removed)
				t = t->next;
		}
	}
	if (!o->alive) {
		remove_target(o->target);
		o->target = NULL;
	}
}

void chaff_move(struct game_obj_t *o)
{
	int deepest;

	if (!o->alive)
		return;
	o->x += o->vx;
	o->y += o->vy;
	o->vy++;
	o->alive--;

	if (o->vx > 0)
		o->vx--;
	else if (o->vx < 0);
		o->vx++;

	explode(o->x, o->y, 0, 0, 10, 7, 19);
	/* Detect smashing into the ground */
	deepest = ground_level(o->x);
	if (deepest != GROUND_OOPS && o->y > deepest) {
		o->alive = 0;
	}
	if (!o->alive) {
		remove_target(o->target);
		o->target = NULL;
	}
}

void drop_chaff()
{
	int j, i[3];
	struct game_obj_t *o;
	struct target_t *t;

	for (j=0;j<3;j++) {
		i[j] = find_free_obj();
		if (i[j] < 0)
			continue;
		o = &game_state.go[i[j]];
		o->x = player->x;
		o->y = player->y;
		o->vx = player->vx + ((j-1) * 7);
		o->vy = player->vy + 7;
		o->v = &spark_vect;
		o->move = chaff_move;
		o->otype = OBJ_TYPE_CHAFF;
		o->target = add_target(o);
		o->color = ORANGE;
		o->alive = 25;
	}
	/* find any missiles in the vicinity */
	for (t=target_head;t != NULL;t=t->next) {
		if (t->o->otype != OBJ_TYPE_MISSILE)
			continue;
		if (t->o->bullseye == player) {
			j = randomn(3);
			if (j >= 0 && j <= 3 && i[j] > 0 && randomn(100) < 50)
				t->o->bullseye = &game_state.go[i[j]];
		}
	}
	/* Bug: when (bullseye->alive == 0) some new object will allocate there
	   and become the new target... probably a spark. */
}

void drop_bomb()
{
	int i;
	struct game_obj_t *o;

	if (game_state.nbombs == 0)
		return;
	game_state.nbombs--;
	
	i = find_free_obj();
	if (i < 0)
		return;

	o = &game_state.go[i];
	o->x = player->x+(5 * game_state.direction);
	o->y = player->y;
	o->vx = player->vx + BOMB_SPEED * game_state.direction;
	o->vy = player->vy;;
	o->v = &bomb_vect;
	o->move = bomb_move;
	o->otype = OBJ_TYPE_BALLOON;
	o->target = add_target(o);
	o->color = ORANGE;
	o->alive = 20;
}

static void add_debris(int x, int y, int vx, int vy, int r, struct game_obj_t **victim);
void no_draw(struct game_obj_t *o, GtkWidget *w);
void move_player(struct game_obj_t *o)
{
	int i;
	int deepest;
	static int was_healthy = 1;
	o->x += o->vx;
	o->y += o->vy;

	if (game_state.health > 0) {
		if (credits > 0 && o->vy > 0) /* damp y velocity. */
			o->vy--;
		if (credits > 0 && o->vy < 0)
			o->vy++;
		was_healthy = 1;
	} else if (was_healthy) {
		was_healthy = 0;
		player->move = bridge_move;
		explode(player->x, player->y, player->vx, player->vy, 90, 350, 30);
		player->draw = no_draw;
		add_debris(o->x, o->y, o->vx, o->vy, 20, &player);
		add_sound(LARGE_EXPLOSION_SOUND, ANY_SLOT);
		printf("decrementing lives %d.\n", game_state.lives);
		game_state.lives--;
		sprintf(textline[CREDITS].string, "Credits: %d Lives: %d", 
			credits, game_state.lives);
		if (game_state.lives <= 0 || credits <= 0) {
			if (credits > 0) 
				credits--;
			if (credits <= 0) {
				timer_event = GAME_ENDED_EVENT;
				next_timer = timer + 30;
			} else {
				timer_event = READY_EVENT;
				game_state.lives = 3;
				next_timer = timer + 30;
			}
		} else {
			next_timer = timer + 30;
			timer_event = READY_EVENT;
		}
	} 
	if (abs(o->vx) < 5 || game_state.health <= 0) {
		o->vy+=1;
		if (o->vy > MAX_VY)
			o->vy = MAX_VY;
		if (was_healthy)
			explode(o->x-(11 * game_state.direction), o->y, -(7*game_state.direction), 0, 7, 10, 9);
	} else
		if (was_healthy)
			explode(o->x-(11 * game_state.direction), o->y, -((abs(o->vx)+7)*game_state.direction), 0, 10, 10, 9);
	if (game_state.direction == 1) {
		if (player->x - game_state.x > SCREEN_WIDTH/3) {
			/* going off screen to the right... rein back in */
			/* game_state.x = player->x - 2*SCREEN_WIDTH/3; */
			game_state.vx = player->vx + 7;
		} else {
			game_state.x = player->x - SCREEN_WIDTH/3;
			game_state.vx = player->vx;
		}
	} else {
		if (player->x - game_state.x < 2*SCREEN_WIDTH/3) {
		/* going off screen to the left... rein back in */
			game_state.vx = player->vx - 7;
		} else {
			game_state.x = player->x - 2*SCREEN_WIDTH/3;
			game_state.vx = player->vx;
		}
	}

	if (player->y - game_state.y > 2*SCREEN_HEIGHT/6) {
		game_state.vy = player->vy + 7;
	} else if (player->y - game_state.y < -SCREEN_HEIGHT/6) {
		game_state.vy = player->vy - 7;
	} else
		game_state.vy = player->vy;

	/* Detect smashing into the ground */
	deepest = ground_level(player->x);
	if (deepest != GROUND_OOPS && player->y > deepest) {
		player->y = deepest - 5;
		if (abs(player->vy) > 7) 
			player->vy = -0.65 * abs(player->vy);
		else
			player->vy = 0;
		player->vx = 0.65 * player->vx;
		if (player->vy < -15) {
			player->vy = -15;
		}
		if (abs(player->vx) > 5 || abs(player->vy) > 5) {
			add_sound(GROUND_SMACK_SOUND, ANY_SLOT);
			explode(player->x, player->y, player->vx*1.5, 1, 20, 20, 15);
			game_state.health -= 4 - player->vy * 0.3 -abs(player->vx) * 0.1;
		}
	}

	/* Detect smashing into sides and roof */
	if (player->y < KERNEL_Y_BOUNDARY) {
		player->y = KERNEL_Y_BOUNDARY + 10;
		if (abs(player->vy) > 7) 
			player->vy = 0.65 * abs(player->vy);
		else
			player->vy = 0;
		player->vx = 0.65 * player->vx;
		if (player->vy > 15)
			player->vy = 15;
		if (abs(player->vx) > 5 || abs(player->vy) > 5) {
			add_sound(GROUND_SMACK_SOUND, ANY_SLOT);
			explode(player->x, player->y, player->vx*1.5, 1, 20, 20, 15);
			game_state.health -= 4 - player->vy * 0.3 -abs(player->vx) * 0.1;
		}
	}
	if (player->x < 0) {
		add_sound(GROUND_SMACK_SOUND, ANY_SLOT);
		explode(player->x, player->y, player->vx*1.5, 1, 20, 20, 15);
		game_state.health -= 4 - player->vy * 0.3 -abs(player->vx) * 0.1;
		player->x = 20;
		player->vx = 5;
	} else if (player->x > terrain.x[TERRAIN_LENGTH - 1]) {
		add_sound(GROUND_SMACK_SOUND, ANY_SLOT);
		explode(player->x, player->y, player->vx*1.5, 1, 20, 20, 15);
		game_state.health -= 4 - player->vy * 0.3 -abs(player->vx) * 0.1;
		player->x = terrain.x[TERRAIN_LENGTH - 1] - 20;
		player->vx = -5;
	}

	/* Autopilot, "attract mode", if credits <= 0 */
	if (credits <= 0) {
		for (i=0;i<TERRAIN_LENGTH;i++) {
			if (terrain.x[i] - player->x > 100 && (terrain.x[i] - player->x) < 300) {
				if (terrain.y[i] - player->y > MAX_ALT) {
					player->vy += 1;
					if (player->vy > 9)
						player->vy = 9;
				} else if (terrain.y[i] - player->y < MIN_ALT) {
					player->vy -= 1;
					if (player->vy < -9)
					player->vy = -9;
				} else if (player->vy > 0) player->vy--;
				else if (player->vy < 0) player->vy++;
				game_state.vy = player->vy;
				break;
			}
			if (player->vx < PLAYER_SPEED)
				player->vx++; 
		}
		if (randomn(40) < 4)
			player_fire_laser();
		if (randomn(100) < 2)
			drop_bomb();
	}
	/* End attract mode */
}

void bridge_move(struct game_obj_t *o) /* move bridge pieces when hit by bomb */
{
	int i;
	int deepest;
	int slope;
	o->x += o->vx;
	o->y += o->vy;
	o->vy++;
	if (o->alive >1)
		o->alive--;

	/* Detect smashing into the ground */
	deepest = 64000;
	for (i=0;i<TERRAIN_LENGTH-1;i++) {
		if (o->x >= terrain.x[i] && o->x < terrain.x[i+1]) {
			deepest = interpolate(o->x, terrain.x[i], terrain.y[i],
					terrain.x[i+1], terrain.y[i+1]);
			slope = (100*(terrain.y[i+1] - terrain.y[i])) / 
					(terrain.x[i+1] - terrain.x[i]);
			break;
		}
	}
	if (deepest != 64000 && o->y > deepest) {
		o->y = deepest-2;
		o->vx = 0;
		o->vy = 0;
		if (slope > 25 && o->alive > 1) {
			o->vx = 3;
			o->vy += 1;
		} else if (slope < -25 && o->alive > 1) {
			o->vx = -3;
			o->vy += 1;
		}
		if (o->alive == 1) 
			o->move = NULL;
	}
}

void no_move(struct game_obj_t *o)
{
	return;
}

void laser_move(struct game_obj_t *o)
{
	struct target_t *t;
	int dist2;
	int removed;

	if (!o->alive)
		return;
	o->x += o->vx;
	o->y += o->vy;

	for (t=target_head;t != NULL;) {
		if (!t->o->alive) {
			t = t->next;
			continue;
		}
		removed = 0;
		switch (t->o->otype) {
			case OBJ_TYPE_ROCKET:
			case OBJ_TYPE_FUEL:
			case OBJ_TYPE_GUN:
			case OBJ_TYPE_SAM_STATION:
			case OBJ_TYPE_MISSILE:{
				dist2 = (o->x - t->o->x)*(o->x - t->o->x) + 
					(o->y - t->o->y)*(o->y - t->o->y);
				// printf("dist2 = %d\n", dist2);
				if (dist2 < LASER_PROXIMITY) { /* a hit */
					if (t->o->otype == OBJ_TYPE_ROCKET)
						game_state.score += ROCKET_SCORE;
					add_sound(LASER_EXPLOSION_SOUND, ANY_SLOT);
					explode(t->o->x, t->o->y, t->o->vx, 1, 70, 150, 20);
					t->o->alive = 0;
					if (t->o->otype == OBJ_TYPE_FUEL) {
						game_state.health += 10;
						if (game_state.health > MAXHEALTH)
							game_state.health = MAXHEALTH;
					}
					t = remove_target(t);
					removed = 1;
					o->alive = 0;
				}
			}
			break;
			default:
				break;
		}
		if (!removed)
			t=t->next;
	}
	if (o->alive)
		o->alive--;
	if (!o->alive) {
		remove_target(o->target);
		o->target = NULL;
	}
}

void move_obj(struct game_obj_t *o)
{
	o->x += o->vx;
	o->y += o->vy;
}

void draw_spark(struct game_obj_t *o, GtkWidget *w)
{
	int x1, y1, x2, y2;

	gdk_gc_set_foreground(gc, &huex[o->color]);
	x1 = o->x - o->vx - game_state.x;
	y1 = o->y - o->vy - game_state.y + (SCREEN_HEIGHT/2);  
	x2 = o->x - game_state.x; 
	y2 = o->y + (SCREEN_HEIGHT/2) - game_state.y;
	gdk_draw_line(w->window, gc, x1, y1, x2, y2); 
}

void draw_missile(struct game_obj_t *o, GtkWidget *w)
{
	int x1, y1;
	int dx, dy;

	x1 = o->x - game_state.x;
	y1 = o->y - game_state.y + (SCREEN_HEIGHT/2);  
	dx = dx_from_vxy(o->vx, o->vy);
	dy = -dy_from_vxy(o->vx, o->vy);
	gdk_gc_set_foreground(gc, &huex[o->color]);
	gdk_draw_line(w->window, gc, x1, y1, x1+dx, y1+dy); 
}

void move_missile(struct game_obj_t *o)
{
	struct game_obj_t *target_obj;
	int dx, dy, desired_vx, desired_vy;
	int exvx,exvy;

	/* move one step... */
	o->x += o->vx;
	o->y += o->vy;
	
	o->alive--;
	if (o->alive <= 0)
		return;

	/* Figure out where we're trying to go */
	target_obj = o->bullseye;
	dx = target_obj->x + target_obj->vx - o->x;
	dy = target_obj->y + target_obj->vy - o->y;

	if ((abs(dx) < MISSILE_PROXIMITY) && (abs(dy) < MISSILE_PROXIMITY)) {
		/* We've hit the target */
		if (player == o->bullseye)
			game_state.health -= MISSILE_DAMAGE;
		else
			target_obj->alive -= MISSILE_DAMAGE;
		o->alive = 0;
		target_obj->alive -= MISSILE_DAMAGE;
		add_sound(ROCKET_EXPLOSION_SOUND, ANY_SLOT);
		explode(o->x, o->y, o->vx, o->vy, 70, 150, 20);
		return;
	}

	/* by similar triangles, find desired vx/vy from dx/dy... */
	if (abs(dx) < abs(dy)) {
		desired_vy = ((dy < 0) ? -1 : 1 ) * MAX_MISSILE_VELOCITY;
		if (dy != 0)
			desired_vx = desired_vy * dx / dy;
		else /* shouldn't happen */
			desired_vx = ((dx < 0) ? -1 : 1 ) * MAX_MISSILE_VELOCITY;
	} else {
		desired_vx = ((dx < 0) ? -1 : 1 ) * MAX_MISSILE_VELOCITY;
		if (dx != 0)
			desired_vy = desired_vx * dy / dx;
		else /* shouldn't happen */
			desired_vy = ((dy < 0) ? -1 : 1 ) * MAX_MISSILE_VELOCITY;
	}

	/* Try to get to desired vx,vy */
	if (o->vx < desired_vx)
		o->vx++;
	else if (o->vx > desired_vx)
		o->vx--;
	if (o->vy < desired_vy)
		o->vy++;
	else if (o->vy > desired_vy)
		o->vy--;

	/* make some exhaust sparks. */
	exvx = -dx_from_vxy(o->vx, o->vy);
	exvy = dy_from_vxy(o->vx, o->vy);
	explode(o->x, o->y, exvx, exvy, 4, 4, 9);
}

static void add_generic_object(int x, int y, int vx, int vy,
	obj_move_func *move_func,
	obj_draw_func *draw_func,
	int color, 
	struct my_vect_obj *vect, 
	int target, char otype, int alive)
{
	int j;
	struct game_obj_t *o;

	j = find_free_obj();
	if (j < 0)
		return;
	o = &game_state.go[j];
	o->x = x;
	o->y = y;
	o->vx = vx;
	o->vy = vy;
	o->move = move_func;
	o->draw = draw_func;
	o->color = color;
	if (target)
		o->target = add_target(o);
	else
		o->target = NULL;
	o->v = vect;
	o->otype = otype;
	o->alive = alive;
	o->bullseye = NULL;
}

static void add_missile(int x, int y, int vx, int vy, 
	int time, int color, struct game_obj_t *bullseye)
{
	int i;
	struct game_obj_t *o;

	i = find_free_obj();
	if (i<0)
		return;
	o = &game_state.go[i];
	o->x = x;
	o->y = y;
	o->vx = vx;
	o->vy = vy;
	o->move = move_missile;
	o->draw = draw_missile;
	o->bullseye = bullseye;
	o->target = add_target(o);
	o->otype = OBJ_TYPE_MISSILE;
	o->color = color;
	o->alive = time;
}

void balloon_move(struct game_obj_t *o)
{
	int deepest;

	o->x += o->vx;
	o->y += o->vy;

	if (randomn(1000) < 5) {
		o->vx = randomab(1, 3) - 2;
		o->vy = randomab(1, 3) - 2;
	}

	deepest = ground_level(o->x);
	if (deepest != GROUND_OOPS && o->y > deepest - MIN_BALLOON_HEIGHT)
		o->vy = -1;
	else if (deepest != GROUND_OOPS && o->y < deepest - MAX_BALLOON_HEIGHT)
		o->vy = 1;
}

void airship_move(struct game_obj_t *o)
{
	balloon_move(o);
}

void move_spark(struct game_obj_t *o)
{
	if (o->alive < 0) {
		o->alive = 0;
		o->draw = NULL;
	}
	if (!o->alive) {
		o->draw = NULL;
		return;
	}

	// printf("x=%d,y=%d,vx=%d,vy=%d, alive=%d\n", o->x, o->y, o->vx, o->vy, o->alive);
	o->x += o->vx;
	o->y += o->vy;
	o->alive--;
	if (!o->alive) {
		o->draw = NULL;
		return;
	}
	// printf("x=%d,y=%d,vx=%d,vy=%d, alive=%d\n", o->x, o->y, o->vx, o->vy, o->alive);
	
	if (o->vx > 0)
		o->vx--;
	else if (o->vx < 0)
		o->vx++;
	if (o->vy > 0)
		o->vy--;
	else if (o->vy < 0)
		o->vy++;

	if (o->vx == 0 && o->vy == 0) {
		o->alive = 0;
		o->draw = NULL;
	}
	if (abs(o->y - player->y) > 2000 || o->x > 2000+WORLDWIDTH || o->x < -2000) {
		o->alive = 0;
		o->draw = NULL;
	}
}

static void add_spark(int x, int y, int vx, int vy, int time);

void explode(int x, int y, int ivx, int ivy, int v, int nsparks, int time)
{
	int vx, vy, i;

	for (i=0;i<nsparks;i++) {
		vx = (int) ((-0.5 + random() / (0.0 + RAND_MAX)) * (v + 0.0) + (0.0 + ivx));
		vy = (int) ((-0.5 + random() / (0.0 + RAND_MAX)) * (v + 0.0) + (0.0 + ivy));
		add_spark(x, y, vx, vy, time); 
		/* printf("%d,%d, v=%d,%d, time=%d\n", x,y, vx, vy, time); */
	}
}

struct my_vect_obj *prerender_glyph(stroke_t g[], int xscale, int yscale)
{
	int i, x, y;
	int npoints = 0;
	struct my_point_t scratch[100];
	struct my_vect_obj *v;

	printf("Prerendering glyph..\n");

	for (i=0;g[i] != 99;i++) {
		if (g[i] == 21) {
			printf("LINE_BREAK\n");
			x = LINE_BREAK;
			y = LINE_BREAK;
		} else {
			// x = ((g[i] % 3) * xscale);
			// y = ((g[i]/3)-4) * yscale ;     // truncating division.
			x = decode_glyph[g[i]].x * xscale;
			y = decode_glyph[g[i]].y * yscale;
			printf("x=%d, y=%d\n", x,y);
		}
		scratch[npoints].x = x;
		scratch[npoints].y = y;
		npoints++;
	}

	v = (struct my_vect_obj *) malloc(sizeof(struct my_vect_obj));
	v->npoints = npoints;
	if (npoints != 0) {
		v->p = (struct my_point_t *) malloc(sizeof(struct my_point_t) * npoints);
		memcpy(v->p, scratch, sizeof(struct my_point_t) * npoints);
	} else
		v->p = NULL;
	return v;
}

int make_font(struct my_vect_obj ***font, int xscale, int yscale) 
{
	struct my_vect_obj **v;

	v = malloc(sizeof(**v) * 256);
	if (!v) {
		if (v) free(v);
		return -1;
	}
	memset(v, 0, sizeof(**v) * 256);
	v['A'] = prerender_glyph(glyph_A, xscale, yscale);
	v['B'] = prerender_glyph(glyph_B, xscale, yscale);
	v['C'] = prerender_glyph(glyph_C, xscale, yscale);
	v['D'] = prerender_glyph(glyph_D, xscale, yscale);
	v['E'] = prerender_glyph(glyph_E, xscale, yscale);
	v['F'] = prerender_glyph(glyph_F, xscale, yscale);
	v['G'] = prerender_glyph(glyph_G, xscale, yscale);
	v['H'] = prerender_glyph(glyph_H, xscale, yscale);
	v['I'] = prerender_glyph(glyph_I, xscale, yscale);
	v['J'] = prerender_glyph(glyph_J, xscale, yscale);
	v['K'] = prerender_glyph(glyph_K, xscale, yscale);
	v['L'] = prerender_glyph(glyph_L, xscale, yscale);
	v['M'] = prerender_glyph(glyph_M, xscale, yscale);
	v['N'] = prerender_glyph(glyph_N, xscale, yscale);
	v['O'] = prerender_glyph(glyph_O, xscale, yscale);
	v['P'] = prerender_glyph(glyph_P, xscale, yscale);
	v['Q'] = prerender_glyph(glyph_Q, xscale, yscale);
	v['R'] = prerender_glyph(glyph_R, xscale, yscale);
	v['S'] = prerender_glyph(glyph_S, xscale, yscale);
	v['T'] = prerender_glyph(glyph_T, xscale, yscale);
	v['U'] = prerender_glyph(glyph_U, xscale, yscale);
	v['V'] = prerender_glyph(glyph_V, xscale, yscale);
	v['W'] = prerender_glyph(glyph_W, xscale, yscale);
	v['X'] = prerender_glyph(glyph_X, xscale, yscale);
	v['Y'] = prerender_glyph(glyph_Y, xscale, yscale);
	v['Z'] = prerender_glyph(glyph_Z, xscale, yscale);
	v['!'] = prerender_glyph(glyph_bang, xscale, yscale);
	v['/'] = prerender_glyph(glyph_slash, xscale, yscale);
	v['?'] = prerender_glyph(glyph_que, xscale, yscale);
	v[':'] = prerender_glyph(glyph_colon, xscale, yscale);
	v['('] = prerender_glyph(glyph_leftparen, xscale, yscale);
	v[')'] = prerender_glyph(glyph_rightparen, xscale, yscale);
	v['a'] = prerender_glyph(glyph_a, xscale, yscale);
	v[' '] = prerender_glyph(glyph_space, xscale, yscale);
	v['b'] = prerender_glyph(glyph_b, xscale, yscale);
	v['c'] = prerender_glyph(glyph_c, xscale, yscale);
	v['d'] = prerender_glyph(glyph_d, xscale, yscale);
	v['e'] = prerender_glyph(glyph_e, xscale, yscale);
	v['f'] = prerender_glyph(glyph_f, xscale, yscale);
	v['g'] = prerender_glyph(glyph_g, xscale, yscale);
	v['h'] = prerender_glyph(glyph_h, xscale, yscale);
	v['i'] = prerender_glyph(glyph_i, xscale, yscale);
	v['j'] = prerender_glyph(glyph_j, xscale, yscale);
	v['k'] = prerender_glyph(glyph_k, xscale, yscale);
	v['l'] = prerender_glyph(glyph_l, xscale, yscale);
	v['m'] = prerender_glyph(glyph_m, xscale, yscale);
	v['n'] = prerender_glyph(glyph_n, xscale, yscale);
	v['o'] = prerender_glyph(glyph_o, xscale, yscale);
	v['p'] = prerender_glyph(glyph_p, xscale, yscale);
	v['q'] = prerender_glyph(glyph_q, xscale, yscale);
	v['r'] = prerender_glyph(glyph_r, xscale, yscale);
	v['s'] = prerender_glyph(glyph_s, xscale, yscale);
	v['t'] = prerender_glyph(glyph_t, xscale, yscale);
	v['u'] = prerender_glyph(glyph_u, xscale, yscale);
	v['v'] = prerender_glyph(glyph_v, xscale, yscale);
	v['w'] = prerender_glyph(glyph_w, xscale, yscale);
	v['x'] = prerender_glyph(glyph_x, xscale, yscale);
	v['y'] = prerender_glyph(glyph_y, xscale, yscale);
	v['z'] = prerender_glyph(glyph_z, xscale, yscale);
	v['0'] = prerender_glyph(glyph_0, xscale, yscale);
	v['1'] = prerender_glyph(glyph_1, xscale, yscale);
	v['2'] = prerender_glyph(glyph_2, xscale, yscale);
	v['3'] = prerender_glyph(glyph_3, xscale, yscale);
	v['4'] = prerender_glyph(glyph_4, xscale, yscale);
	v['5'] = prerender_glyph(glyph_5, xscale, yscale);
	v['6'] = prerender_glyph(glyph_6, xscale, yscale);
	v['7'] = prerender_glyph(glyph_7, xscale, yscale);
	v['8'] = prerender_glyph(glyph_8, xscale, yscale);
	v['9'] = prerender_glyph(glyph_9, xscale, yscale);
	v['-'] = prerender_glyph(glyph_dash, xscale, yscale);
	v['+'] = prerender_glyph(glyph_plus, xscale, yscale);
	v[','] = prerender_glyph(glyph_comma, xscale, yscale);
	v['.'] = prerender_glyph(glyph_period, xscale, yscale);
	*font = v;
	return 0;
}

void init_vects()
{
	int i;

	/* memset(&game_state.go[0], 0, sizeof(game_state.go[0])*MAXOBJS); */
	player_vect.p = player_ship_points;
	player_vect.npoints = sizeof(player_ship_points) / sizeof(player_ship_points[0]);
	left_player_vect.p = left_player_ship_points;
	left_player_vect.npoints = sizeof(left_player_ship_points) / sizeof(left_player_ship_points[0]);
	for (i=0;i<left_player_vect.npoints;i++)
		left_player_ship_points[i].x *= -1;
	rocket_vect.p = rocket_points;
	rocket_vect.npoints = sizeof(rocket_points) / sizeof(rocket_points[0]);
	spark_vect.p = spark_points;
	spark_vect.npoints = sizeof(spark_points) / sizeof(spark_points[0]);
	right_laser_vect.p = right_laser_beam_points;
	right_laser_vect.npoints = sizeof(right_laser_beam_points) / sizeof(right_laser_beam_points[0]);
	fuel_vect.p = fuel_points;
	fuel_vect.npoints = sizeof(fuel_points) / sizeof(fuel_points[0]);
	bomb_vect.p = bomb_points;
	bomb_vect.npoints = sizeof(bomb_points) / sizeof(bomb_points[0]);
	bridge_vect.p = bridge_points;
	bridge_vect.npoints = sizeof(bridge_points) / sizeof(bridge_points[0]);
	flak_vect.p = flak_points;
	flak_vect.npoints = sizeof(flak_points) / sizeof(flak_points[0]);
	airship_vect.p = airship_points;
	airship_vect.npoints = sizeof(airship_points) / sizeof(airship_points[0]);
	for (i=0;i<airship_vect.npoints;i++) {
		if (airship_vect.p[i].x != LINE_BREAK) {
			airship_vect.p[i].x *= 3;
			airship_vect.p[i].y *= 3;
		}
	}
	balloon_vect.p = balloon_points;
	balloon_vect.npoints = sizeof(balloon_points) / sizeof(balloon_points[0]);
	SAM_station_vect.p = SAM_station_points;
	SAM_station_vect.npoints = sizeof(SAM_station_points) / sizeof(SAM_station_points[0]);
	humanoid_vect.p = humanoid_points;
	humanoid_vect.npoints = sizeof(humanoid_points) / sizeof(humanoid_points[0]);
	socket_vect.p = socket_points;
	socket_vect.npoints = sizeof(socket_points) / sizeof(socket_points[0]);

	make_font(&gamefont[BIG_FONT], font_scale[BIG_FONT], font_scale[BIG_FONT]);
	make_font(&gamefont[SMALL_FONT], font_scale[SMALL_FONT], font_scale[SMALL_FONT]);
	set_font(BIG_FONT);
}

void no_draw(struct game_obj_t *o, GtkWidget *w)
{
	return;
}

void draw_generic(struct game_obj_t *o, GtkWidget *w)
{
	int j;
	int x1, y1, x2, y2;
	gdk_gc_set_foreground(gc, &huex[o->color]);
	for (j=0;j<o->v->npoints-1;j++) {
		if (o->v->p[j+1].x == LINE_BREAK) /* Break in the line segments. */
			j+=2;
		x1 = o->x + o->v->p[j].x - game_state.x;
		y1 = o->y + o->v->p[j].y - game_state.y + (SCREEN_HEIGHT/2);  
		x2 = o->x + o->v->p[j+1].x - game_state.x; 
		y2 = o->y + o->v->p[j+1].y+(SCREEN_HEIGHT/2) - game_state.y;
		gdk_draw_line(w->window, gc, x1, y1, x2, y2); 
	}
}

void draw_flak(struct game_obj_t *o, GtkWidget *w)
{
	int dx, dy, bx,by;
	int x1, y1;
	draw_generic(o, w);

	/* Draw the gun barrels... */
	dx = player->x+LASERLEAD*player->vx - o->x;
	dy = player->y+LASERLEAD*player->vy - o->y;

	if (dy >= 0) {
		if (player->x + LASERLEAD*player->vx < o->x)
			bx = -20;
		else
			bx = 20;
		by = 0;
	} else if (dx == 0) {
		bx = -0;
		by = -20;
	} else if (abs(dx) > abs(dy)) {
		if (player->x+LASERLEAD*player->vx < o->x)
			bx = -20;
		else
			bx = 20;
		by = -abs((20*dy)/dx);
	} else {
		by = -20;
		/* if (player->x < o->x)
			bx = -20;
		else
			bx = 20; */
		bx = (-20*dx)/dy;
	}
	x1 = o->x-5 - game_state.x;
	y1 = o->y-5 - game_state.y + (SCREEN_HEIGHT/2);  
	gdk_draw_line(w->window, gc, x1, y1, x1+bx, y1+by); 
	gdk_draw_line(w->window, gc, x1+10, y1, x1+bx+6, y1+by); 
}

void draw_objs(GtkWidget *w)
{
	int i, j, x1, y1, x2, y2;

	for (i=0;i<MAXOBJS;i++) {
		struct my_vect_obj *v = game_state.go[i].v;
		struct game_obj_t *o = &game_state.go[i];

		if (!o->alive)
			continue;

		if (o->x < (game_state.x - (SCREEN_WIDTH/3)))
			continue;
		if (o->x > (game_state.x + 4*(SCREEN_WIDTH/3)))
			continue;
		if (o->y < (game_state.y - (SCREEN_HEIGHT)))
			continue;
		if (o->y > (game_state.y + (SCREEN_HEIGHT)))
			continue;
#if 0
		if (o->v == &spark_vect)
			printf("s");
		if (o->v == &player_vect)
			printf("p");
		if (o->v == &rocket_vect)
			printf("r");
#endif
		if (o->draw == NULL) {
			gdk_gc_set_foreground(gc, &huex[o->color]);
			for (j=0;j<v->npoints-1;j++) {
				if (v->p[j+1].x == LINE_BREAK) /* Break in the line segments. */
					j+=2;
				x1 = o->x + v->p[j].x - game_state.x;
				y1 = o->y + v->p[j].y - game_state.y + (SCREEN_HEIGHT/2);  
				x2 = o->x + v->p[j+1].x - game_state.x; 
				y2 = o->y + v->p[j+1].y+(SCREEN_HEIGHT/2) - game_state.y;
				gdk_draw_line(w->window, gc, x1, y1, x2, y2); 
			}
		} else
			o->draw(o, w);
	}
}

static void draw_letter(GtkWidget *w, struct my_vect_obj **font, unsigned char letter)
{
	int i, x1, y1, x2, y2;

	if (letter == ' ') {
		livecursorx += font_scale[current_font]*2 + letter_spacing[current_font];
		return;
	}
	if (letter == '\n') {
		livecursorx = font_scale[current_font];
		livecursory += font_scale[current_font];
		return;
	}
	if (font[letter] == NULL) {
		return;
	}

	for (i=0;i<font[letter]->npoints-1;i++) {
		if (font[letter]->p[i+1].x == LINE_BREAK)
			i+=2;
		x1 = font[letter]->p[i].x;
		x1 = livecursorx + x1;
		y1 = livecursory + font[letter]->p[i].y;
		x2 = livecursorx + font[letter]->p[i+1].x;
		y2 = livecursory + font[letter]->p[i+1].y;
		gdk_draw_line(w->window, gc, x1, y1, x2, y2); 
	}
	livecursorx += font_scale[current_font]*2 + letter_spacing[current_font];
}

static void draw_string(GtkWidget *w, unsigned char *s) 
{

	char *i;

	for (i=s;*i;i++)
		draw_letter(w, gamefont[current_font], *i);  
}

static void draw_strings(GtkWidget *w)
{
	int i;

	gdk_gc_set_foreground(gc, &huex[WHITE]); //&huex[current_color]);
	for (i=0;i<ntextlines;i++) {
		/* printf("Drawing '%s' color=%d, x=%d, y=%d\n", 
			textline[i].string, current_color, 
			textline[i].x, textline[i].y); */
		livecursorx = textline[i].x;
		livecursory = textline[i].y;
		set_font(textline[i].font);
		draw_string(w, textline[i].string);
	}
}

static void add_laserbolt(int x, int y, int vx, int vy, int time)
{
	add_generic_object(x, y, vx, vy, move_laserbolt, draw_spark,
		CYAN, &spark_vect, 0, OBJ_TYPE_SPARK, time);
}

static void add_spark(int x, int y, int vx, int vy, int time)
{
	add_generic_object(x, y, vx, vy, move_spark, draw_spark,
		YELLOW, &spark_vect, 0, OBJ_TYPE_SPARK, time);
}

void perturb(int *value, int lower, int upper, double percent)
{
	double perturbation;

	perturbation = percent * (lower - upper) * ((0.0 + random()) / (0.0 + RAND_MAX) - 0.5);
	*value += perturbation;
	return;
}

void generate_sub_terrain(struct terrain_t *t, int xi1, int xi2)
{
	int midxi;
	int y1, y2, y3, tmp;
	int x1, x2, x3;


	midxi = (xi2 - xi1) / 2 + xi1;
	if (midxi == xi2 || midxi == xi1)
		return;

	y1 = t->y[xi1];
	y2 = t->y[xi2];
	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}
	y3 = ((y2 - y1) / 2) + y1;

	x1 = t->x[xi1];
	x2 = t->x[xi2];
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	x3 = ((x2 - x1) / 2) + x1;

	if ((x2 - x1) > 1000) {	
		perturb(&x3, x2, x1, level.large_scale_roughness);
		perturb(&y3, x2, x1, level.large_scale_roughness);
	} else {
		perturb(&x3, x2, x1, level.small_scale_roughness);
		perturb(&y3, x2, x1, level.small_scale_roughness);
	}

	t->x[midxi] = x3;
	t->y[midxi] = y3;
	// printf("gst %d %d\n", x3, y3);

	generate_sub_terrain(t, xi1, midxi);
	generate_sub_terrain(t, midxi, xi2);
}

void generate_terrain(struct terrain_t *t)
{
	t->npoints = TERRAIN_LENGTH;
	t->x[0] = 0;
	t->y[0] = 100;
	t->x[t->npoints-1] = WORLDWIDTH;
	t->y[t->npoints-1] = t->y[0];

	generate_sub_terrain(t, 0, t->npoints-1);
}

static struct my_vect_obj *make_debris_vect()
{
	int i, n;
	struct my_point_t *p;
	struct my_vect_obj *v;

	/* FIXME, this malloc'ing is a memory leak, */
	
	n = randomab(5,10);
	v = (struct my_vect_obj *) malloc(sizeof(*v));
	p = (struct my_point_t *) malloc(sizeof(*p) * n);
	if (!v || !p) {
		if (v)
			free(v);
		if (p)
			free (p);
		return NULL;
	}

	v->p = &p[0];
	v->npoints = n;
	
	for (i=0;i<n;i++) {
		p[i].x = randomn(20)-10;
		p[i].y = randomn(10)-5;
	}
	return v;
}

static void add_debris(int x, int y, int vx, int vy, int r, struct game_obj_t **victim)
{
	int i, z; 
	struct game_obj_t *o;

	for (i=0;i<=12;i++) {
		z = find_free_obj();
		if (z < 0)
			return;
		o = &game_state.go[z];
		o->x = x;
		o->y = y;
		o->move = bridge_move;
		o->draw = draw_generic;
		o->alive = 30;	
		o->color = WHITE;
		o->vx = (int) ((-0.5 + random() / (0.0 + RAND_MAX)) * (r + 0.0) + (0.0 + vx));
		o->vy = (int) ((-0.5 + random() / (0.0 + RAND_MAX)) * (r + 0.0) + (0.0 + vy));
		o->target = NULL;
		o->v = make_debris_vect();
		if (o->v == NULL)
			o->draw = no_draw;
		if (i==0)
			*victim = o;
	}
}

static void add_flak_guns(struct terrain_t *t)
{
	int i, xi;
	for (i=0;i<level.nflak;i++) {
		xi = (int) (((0.0 + random()) / RAND_MAX) * 
			(TERRAIN_LENGTH - 40) + 40);
		add_generic_object(t->x[xi], t->y[xi] - 7, 0, 0, 
			move_flak, draw_flak, GREEN, &flak_vect, 1, OBJ_TYPE_GUN, 1);
	}
}

static void add_rockets(struct terrain_t *t)
{
	int i, xi;
	for (i=0;i<level.nrockets;i++) {
		xi = (int) (((0.0 + random()) / RAND_MAX) * (TERRAIN_LENGTH - 40) + 40);
		add_generic_object(t->x[xi], t->y[xi] - 7, 0, 0, 
			move_rocket, NULL, WHITE, &rocket_vect, 1, OBJ_TYPE_ROCKET, 1);
	}
}

/* Insert the points in one list into the middle of another list */
static void insert_points(struct my_point_t host_list[], int *nhost, 
		struct my_point_t injection[], int ninject, 
		int injection_point)
{
	/* make room for the injection */
	memmove(&host_list[injection_point + ninject], 
		&host_list[injection_point], 
		(*nhost - injection_point) * sizeof(host_list[0]));

	/* make the injection */
	memcpy(&host_list[injection_point], &injection[0], (ninject * sizeof(injection[0])));
	*nhost += ninject;
}

static void embellish_roof(struct my_point_t *building, int *npoints, int left, int right);

static void peak_roof(struct my_point_t *building, int *npoints, int left, int right)
{
	struct my_point_t p;
	int height;
	height = randomab(20,140);
	height = (int) (height * (building[right].x-building[left].x) / 100.0);
	if (height > 60)
		height = 60;

	p.x = ((building[right].x - building[left].x) / 2) + building[left].x;
	p.y = building[right].y - height;

	insert_points(building, npoints, &p, 1, right);
	
	return;
}

static void turret_roof(struct my_point_t *building, int *npoints, int left, int right)
{
	struct my_point_t p[8];
	int height, indent, width;
	int old_npoints;

	width = randomab(15,30);
	height = randomab(20,60);
	indent = randomab(0,10);
	height = (int) (height * (building[right].x-building[left].x) / 100.0);
	indent = (int) (indent * (building[right].x-building[left].x) / 100.0);
	width = (int) (width * (building[right].x-building[left].x) / 100.0);
	
	p[0].x = building[left].x - indent;
	p[0].y = building[left].y - indent;

	p[1].x = p[0].x;
	p[1].y = p[0].y - height;

	p[2].x = p[1].x + width;
	p[2].y = p[1].y;

	p[3].x = p[2].x;
	p[3].y = p[0].y;

	p[4].x = building[right].x - width + indent; 
	p[4].y = p[3].y;

	p[5].x = p[4].x; 
	p[5].y = p[4].y - height;

	p[6].x = p[5].x + width; 
	p[6].y = p[5].y;

	p[7].x = p[6].x; 
	p[7].y = p[6].y + height;

	insert_points(building, npoints, p, 8, right);
	old_npoints = *npoints;

	embellish_roof(building, npoints, left+2, left+3);

	embellish_roof(building, npoints, left+6 + *npoints - old_npoints, left+7 + *npoints -old_npoints);
	embellish_roof(building, npoints, left+4 + *npoints - old_npoints, left+5 + *npoints -old_npoints);

	return;
}

static void indent_roof(struct my_point_t *building, int *npoints, int left, int right)
{
	struct my_point_t p[4];
	int indent;
	int height;
	indent = randomab(5,25);
	height = randomab(-20,30);

	height = (int) (height * (building[right].x-building[left].x) / 100.0);
	indent = (int) (indent * (building[right].x-building[left].x) / 100.0);
	
	p[0].x = building[left].x + indent;
	p[0].y = building[left].y;

	p[1].x = p[0].x;
	p[1].y = building[left].y - height;
	p[2].x = building[right].x - indent;
	p[2].y = p[1].y;
	p[3].x = p[2].x;
	p[3].y = building[right].y;

	insert_points(building, npoints, p, 4, right);
	embellish_roof(building, npoints, left+4, left+5);
	embellish_roof(building, npoints, left+2, left+3);

	return;
}

static void addtower_roof(struct my_point_t *building, int *npoints, int left, int right)
{

	return;
}

static void crenellate_roof(struct my_point_t *building, int *npoints, int left, int right)
{
	return;
}


static void embellish_roof(struct my_point_t *building, int *npoints, int left, int right)
{
	int r = randomn(10);

	switch (r) {
		case 1: peak_roof(building, npoints, left, right);
			break;
		case 2:turret_roof(building, npoints, left, right);
			break;
		case 3:
		case 4:
		case 5: indent_roof(building, npoints, left, right);
			break;
		case 8: crenellate_roof(building, npoints, left, right);
			break;
		case 9: addtower_roof(building, npoints, left, right);
			break;
		default:
			break;
	}
}

static void add_window(struct my_point_t *building, int *npoints, 
		int x1, int y1, int x2, int y2)
{
	printf("aw: npoints = %p\n", npoints); fflush(stdout);
	if (*npoints > 1000) {
		printf("npoints = %d\n", *npoints); fflush(stdout);
		return;
	}
	building[*npoints].x = LINE_BREAK; 
	building[*npoints].y = LINE_BREAK; *npoints += 1;
	building[*npoints].x = x1; 
	building[*npoints].y = y1; *npoints += 1;
	building[*npoints].x = x1; 
	building[*npoints].y = y2; *npoints += 1;
	building[*npoints].x = x2; 
	building[*npoints].y = y2; *npoints += 1;
	building[*npoints].x = x2; 
	building[*npoints].y = y1; *npoints += 1;
	building[*npoints].x = x1; 
	building[*npoints].y = y1; *npoints += 1;
}

static void add_windows(struct my_point_t *building, int *npoints, 
		int x1, int y1, int x2, int y2)
{
	int nwindows;
	int xindent, yindent;
	int spacing;
	int width;
	int i;

	printf("aws: npoints = %p\n", npoints); fflush(stdout);
	xindent = randomab(5, 20);
	yindent = randomab(5, 20);
	spacing = randomab(3, 15);
	x2 -= xindent;
	x1 += xindent;
	y1 -= yindent;
	y2 += yindent;
	printf("add_windows, %d,%d  %d,%d\n", x1, y1, x2, y2);

	if (x2 - x1 < 30)
		return;
	if (y1 - y2 < 20)
		return;

	nwindows = randomab(1, 5);
	width = (x2-x1) / nwindows; 
	printf("adding %d windows, *npoints = %d\n", nwindows, *npoints);
	for (i=0;i<nwindows;i++) {
		printf("Adding window -> %d, %d, %d, %d, npoints = %d\n",
			x1 + (i*width), y1, x1 + (i+1)*width - spacing, y2, *npoints);
		fflush(stdout);
		add_window(building, npoints,
			x1 + (i*(width)), y1, x1 + (i+1)*(width) - spacing, y2);
	}
	return;
}

static void embellish_building(struct my_point_t *building, int *npoints)
{
	int x1, y1, x2, y2;

	x1 = building[0].x;
	y1 = building[0].y;
	x2 = building[2].x;
	y2 = building[2].y;
	embellish_roof(building, npoints, 1, 2);
	add_windows(building, npoints, x1, y1, x2, y2);
	return;
}

int find_free_obj()
{
	int i;
	for (i=0;i<MAXOBJS;i++)
		if (!game_state.go[i].alive)
			return i;
	return -1;
}


static void add_building(struct terrain_t *t, int xi)
{
	int npoints = 0;
	int height;
	int width;
	struct my_point_t scratch[1000];
	struct my_point_t *building;
	struct my_vect_obj *bvec; 
	int objnum;
	struct game_obj_t *o;
	int i, x, y;

	memset(scratch, 0, sizeof(scratch[0]) * 1000);
	objnum = find_free_obj();
	if (objnum == -1)
		return;

	height = randomab(50, 100);
	width = randomab(5,MAXBUILDING_WIDTH);
	scratch[0].x = t->x[xi];	
	scratch[0].y = t->y[xi];	
	scratch[1].x = scratch[0].x;
	scratch[1].y = scratch[0].y - height;
	scratch[2].x = t->x[xi+width];
	scratch[2].y = scratch[1].y; /* make roof level, even if ground isn't. */
	scratch[3].x = scratch[2].x;
	scratch[3].y = t->y[xi+width];
	npoints = 4;

	y = scratch[1].y;
	x = ((scratch[2].x - scratch[0].x) / 2) + scratch[0].x;

	for (i=0;i<npoints;i++) {
		scratch[i].x -= x;
		scratch[i].y -= y;
	}

	embellish_building(scratch, &npoints);

	building = malloc(sizeof(scratch[0]) * npoints);
	bvec = malloc(sizeof(bvec));
	if (building == NULL || bvec == NULL)
		return;

	memcpy(building, scratch, sizeof(scratch[0]) * npoints);
	bvec->p = building;
	bvec->npoints = npoints;

	o = &game_state.go[objnum];
	o->x = x;
	o->y = y;
	o->vx = 0;
	o->vy = 0;
	o->v = bvec;
	o->move = no_move;
	o->otype = OBJ_TYPE_BUILDING;
	o->target = add_target(o);
	o->color = BLUE;
	o->alive = 1;
	printf("b, x=%d, y=%d\n", x, y);
}

static void add_buildings(struct terrain_t *t)
{
	int xi, i;

	for (i=0;i<level.nbuildings;i++) {
		xi = randomn(TERRAIN_LENGTH-MAXBUILDING_WIDTH-1);
		add_building(t, xi);
	}
}

static int find_dip(struct terrain_t *t, int n, int *px1, int *px2, int minlength)
{
	/*	Find the nth dip in the terrain, over which we might put a bridge. */

	int i, j, x1, x2, found, highest, lowest;
	printf("top of find_dip, n=%d\n", n);

	found=0;
	x1 = 0;
	for (i=0;i<n;i++) {
		for (;x1<TERRAIN_LENGTH-100;x1++) {
			for (x2=x1+5; t->x[x2] - t->x[x1] < minlength; x2++)
				/* do nothing in body of for loop */ 
				;
			highest = 60000; /* highest altitude, lowest y value */
			lowest = -1; /* lowest altitude, highest y value */
			
			for (j=x1+1; j<x2-1;j++) {
				if (t->y[j] < highest)
					highest = t->y[j];
				if (t->y[j] > lowest)
					lowest = t->y[j];
			}

			/* Lowest point between x1,x2 must be at least 100 units deep */
			if (lowest - t->y[x1] < 100 || lowest - t->y[x2] < 100)
				continue; /* not a dip */

			/* highest point between x1,x2 must be lower than both */
			if (t->y[x1] >= highest || t->y[x2] >= highest)
				continue; /* not a dip */
	
			printf("found a dip x1=%d, x2=%d.\n", x1, x2);
			break;
		}
		if (x1 >= TERRAIN_LENGTH-100)
			break;
		found++;
		if (found == n)
			break;
		x1 = x2;
	}
	printf("found %d dips.\n", found);
	if (found == n) {
		*px1 = x1;
		*px2 = x2;
		return 0;
	}
	return -1;
}

static void add_bridge_piece(int x, int y)
{
	add_generic_object(x, y, 0, 0, no_move, NULL, RED, &bridge_vect, 1, OBJ_TYPE_BRIDGE, 1);
}

static void add_bridge_column(struct terrain_t *t, 
	int rx, int ry,  /* real coords */
	int x1, int x2) /* index into terrain */
{
	int i, terminal_y;
	i = x1;

	printf("add_bridge_column, rx =%d, ry=%d, x1 = %d, x2=%d\n", rx, ry, x1, x2);
	while (t->x[i] <= rx && i <= x2)
		i++;
	if (i>x2)  /* we're at the rightmost end of the bridge, that is, done. */
		return;

	terminal_y = interpolate(rx, t->x[i-1], t->y[i-1], t->x[i], t->y[i]);
	printf("term_y = %d, intr(%d, %d, %d, %d, %d)\n", 
		terminal_y, rx, t->y[i-1], t->x[i-1], t->y[i], t->x[i]);

	if (ry > terminal_y) /* shouldn't happen... */
		return;

	do {
		add_bridge_piece(rx, ry);
		ry += 8;
	} while (ry <= terminal_y);
	return;
}

static void add_bridge(struct terrain_t *t, int x1, int x2)
{
	int x, y;
	int rx1, rx2, ry1, ry2;

	ry1 = t->y[x1], 
	rx1 = t->x[x1], 
	ry2 = t->y[x2], 
	rx2 = t->x[x2], 

	/* Make the bridge span */
	x = rx1 - (rx1 % 40);
	while (x < rx2) {
		y = interpolate(x, rx1, ry1, rx2, ry2); 
		add_bridge_piece(x, y);
		if ((x % 40) == 32) {
			add_bridge_column(t, x, y+8, x1, x2);
		}
		x += 8;
	}
}

static void add_bridges(struct terrain_t *t)
{
	int i, n;
	int x1, x2;

	for (i=0;i<level.nbridges;i++) {
		n = find_dip(t, i+1, &x1, &x2, 600);
		if (n != 0) 
			n = find_dip(t, i+1, &x1, &x2, 500);
		if (n != 0)
			n = find_dip(t, i+1, &x1, &x2, 400);
		if (n != 0)
			n = find_dip(t, i+1, &x1, &x2, 300);
		if (n != 0)
			n = find_dip(t, i+1, &x1, &x2, 200);
		if (n == 0)
			add_bridge(t, x1, x2);
		else
			break;
	}
}

static void add_fuel(struct terrain_t *t)
{
	int xi, i;
	for (i=0;i<level.nfueltanks;i++) {
		xi = randomn(TERRAIN_LENGTH-MAXBUILDING_WIDTH-1);
		add_generic_object(t->x[xi], t->y[xi]-30, 0, 0, 
			no_move, NULL, ORANGE, &fuel_vect, 1, OBJ_TYPE_FUEL, 1);
	}
}

static void add_SAMs(struct terrain_t *t)
{
	int xi, i;
	for (i=0;i<level.nsams;i++) {
		xi = randomn(TERRAIN_LENGTH-MAXBUILDING_WIDTH-1);
		add_generic_object(t->x[xi], t->y[xi], 0, 0, 
			sam_move, NULL, GREEN, &SAM_station_vect, 1, OBJ_TYPE_SAM_STATION, 1);
	}
}

static void add_humanoids(struct terrain_t *t)
{
	int xi, i;
	for (i=0;i<level.nhumanoids;i++) {
		xi = randomn(TERRAIN_LENGTH-MAXBUILDING_WIDTH-1);
		add_generic_object(t->x[xi], t->y[xi], 0, 0, 
			humanoid_move, NULL, GREEN, &humanoid_vect, 1, OBJ_TYPE_HUMAN, 1);
	}
}

static void add_airships(struct terrain_t *t)
{
	int xi, i;
	for (i=0;i<NAIRSHIPS;i++) {
		xi = randomn(TERRAIN_LENGTH-MAXBUILDING_WIDTH-1);
		add_generic_object(t->x[xi], t->y[xi]-50, 0, 0, 
			airship_move, NULL, CYAN, &airship_vect, 1, OBJ_TYPE_AIRSHIP, 1);
	}
}

static void add_socket(struct terrain_t *t)
{
	add_generic_object(t->x[TERRAIN_LENGTH-1] - 250, t->y[TERRAIN_LENGTH-1] - 250, 
		0, 0, socket_move, NULL, CYAN, &socket_vect, 0, OBJ_TYPE_SOCKET, 1);
}

static void add_balloons(struct terrain_t *t)
{
	int xi, i;
	for (i=0;i<NBALLOONS;i++) {
		xi = randomn(TERRAIN_LENGTH-MAXBUILDING_WIDTH-1);
		add_generic_object(t->x[xi], t->y[xi]-50, 0, 0, 
			balloon_move, NULL, CYAN, &balloon_vect, 1, OBJ_TYPE_BALLOON, 1);
	}
}

static void draw_strings(GtkWidget *w);

static int main_da_expose(GtkWidget *w, GdkEvent *event, gpointer p)
{
	int i;
	int sx1, sx2;
	static int last_lowx = 0, last_highx = TERRAIN_LENGTH-1;
	char score_str[100];
	// int last_lowx = 0, last_highx = TERRAIN_LENGTH-1;

	

	sx1 = game_state.x - SCREEN_WIDTH / 3;
	sx2 = game_state.x + 4*SCREEN_WIDTH/3;


	while (terrain.x[last_lowx] < sx1 && last_lowx+1 < TERRAIN_LENGTH)
		last_lowx++;
	while (terrain.x[last_lowx] > sx1 && last_lowx > 0)
		last_lowx--;
	while (terrain.x[last_highx] > sx2 && last_highx > 0)
		last_highx--;
	while (terrain.x[last_highx] < sx2 && last_highx+1 < TERRAIN_LENGTH) {
		last_highx++;
	}

	gdk_gc_set_foreground(gc, &huex[RED]);

	for (i=last_lowx;i<last_highx;i++) {
#if 0
		if (terrain.x[i] < sx1 && terrain.x[i+1] < sx1) /* offscreen to the left */
			continue;
		if (terrain.x[i] > sx2 && terrain.x[i+1] > sx2) /* offscreen to the right */
			continue;
#endif
#if 0
		if (zz < 5) {
				if (game_state.y < terrain.y[i+1] - 150) {
					game_state.vy = 3; 
					game_state.go[0].vy = 3;
				} else if (game_state.y > terrain.y[i+1] - 50) {
					game_state.vy = -3;
					game_state.go[0].vy = -3;
				} else {
					game_state.vy = 0;
					game_state.go[0].vy = 0;
				}
			zz++;
			printf(".\n");
		}
#endif
		gdk_draw_line(w->window, gc, terrain.x[i] - game_state.x, terrain.y[i]+(SCREEN_HEIGHT/2) - game_state.y,  
					 terrain.x[i+1] - game_state.x, terrain.y[i+1]+(SCREEN_HEIGHT/2) - game_state.y);
	}

	/* draw "system memory boundaries" (ha!) */
	if (game_state.x > terrain.x[0] - SCREEN_WIDTH)
		gdk_draw_line(w->window, gc, terrain.x[0] - game_state.x, 0, 
			terrain.x[0] - game_state.x, SCREEN_HEIGHT);
	if (game_state.x > terrain.x[TERRAIN_LENGTH-1] - SCREEN_WIDTH)
		gdk_draw_line(w->window, gc, terrain.x[TERRAIN_LENGTH-1] - game_state.x, 0, 
			 terrain.x[TERRAIN_LENGTH-1] - game_state.x, SCREEN_HEIGHT); 
	set_font(SMALL_FONT);
	if (game_state.y < KERNEL_Y_BOUNDARY + SCREEN_HEIGHT) {
		gdk_draw_line(w->window, gc, 0, KERNEL_Y_BOUNDARY  - game_state.y + SCREEN_HEIGHT/2, 
			SCREEN_WIDTH, KERNEL_Y_BOUNDARY - game_state.y + SCREEN_HEIGHT/2);
		livecursorx = (SCREEN_WIDTH - abs(game_state.x) % SCREEN_WIDTH);
		livecursory = KERNEL_Y_BOUNDARY - game_state.y + SCREEN_HEIGHT/2 - 10;
		draw_string(w, "Kernel Space");
	}

	if (game_state.x > terrain.x[TERRAIN_LENGTH] - SCREEN_WIDTH)
	/* draw health bar */
	if (game_state.health > 0)
		gdk_draw_rectangle(w->window, gc, TRUE, 10, 10, 
			((SCREEN_WIDTH - 20) * game_state.health / 100), 30);
	draw_objs(w);
		sprintf(textline[CREDITS].string, "Credits: %d Lives: %d Score: %d Humans:%d/%d ", 
			credits, game_state.lives, game_state.score, 
			game_state.humanoids, level.nhumanoids);
	draw_strings(w);

	if (game_state.prev_score != game_state.score) {
		sprintf(score_str, "Score: %06d", game_state.score);
		game_state.prev_score = game_state.score;
		gtk_label_set_text(GTK_LABEL(score_label), score_str);
	}
	if (game_state.prev_bombs != game_state.nbombs) {
		sprintf(score_str, "Bombs: %02d", game_state.nbombs);
		game_state.prev_bombs = game_state.nbombs;
		gtk_label_set_text(GTK_LABEL(bombs_label), score_str);
	}

	return 0;
}
static void do_game_pause( GtkWidget *widget,
                   gpointer   data )
{
	if (game_pause)
		game_pause = 0;
	else
		game_pause = 1;
}

/* This is a callback function. The data arguments are ignored
 * in this example. More on callbacks below. */
static void destroy_event( GtkWidget *widget,
                   gpointer   data )
{
    g_print ("Bye bye.\n");
	exit(1); /* bad form to call exit here... */
}

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    /* If you return FALSE in the "delete_event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete_event". */

    return TRUE;
}

/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}

void setup_text();
void game_ended();
void start_level();
void timer_expired()
{
	static int game_over_count = 0;

	printf("timer expired, %d\n", timer_event);
	switch (timer_event) {
	case BLINK_EVENT:
		setup_text();
		if (credits >= 1) {
			game_over_count = 0;
			next_timer = timer + 1;
			timer_event = GAME_ENDED_EVENT;
			strcpy(textline[GAME_OVER].string, " Game Over");
			break;
		}
		game_over_count++;
		timer_event = BLANK_GAME_OVER_1_EVENT;
		next_timer = timer + 20;
		strcpy(textline[GAME_OVER].string, " Game Over");
		break;
	case BLANK_GAME_OVER_1_EVENT:
		timer_event = INSERT_COIN_EVENT;
		next_timer = timer + 20;
		strcpy(textline[GAME_OVER].string, "");
		break;
	case INSERT_COIN_EVENT:
		timer_event = BLANK_GAME_OVER_2_EVENT;
		next_timer = timer + 20;
		strcpy(textline[GAME_OVER].string, "Insert Coin");
		break;
	case BLANK_GAME_OVER_2_EVENT:
		if (game_over_count >= 3) {
			if (randomn(10) < 5)
				timer_event = CREDITS1_EVENT;
			else
				timer_event = INTRO1_EVENT;
		} else
			timer_event = BLINK_EVENT;
		next_timer = timer + 20;
		strcpy(textline[GAME_OVER].string, "");
		break;
	case CREDITS1_EVENT: {
		int yline = 7;
		int x = 12;
		ntextlines = 1;
		set_font(SMALL_FONT);
		gotoxy(x,yline++);
		gameprint("Credits:");
		gotoxy(x,yline++);
		gameprint("Programming:   Stephen Cameron");
		gotoxy(x,yline++);
		gameprint("Game design:   Stephen Cameron");
		gotoxy(x,yline++);
		gameprint("Music:         Stephen Cameron");
		gotoxy(x,yline++);
		gameprint("               and Marty Kiel");
		gotoxy(x,yline++);
		gameprint("Sound effects: Freesound users:");
		gotoxy(x,yline++);
		gameprint("   dobroide, inferno and oniwe");
		gotoxy(x,yline++);
		timer_event = CREDITS2_EVENT;
		next_timer = timer + 100;
		game_over_count = 0;
		break;
		}
	case CREDITS2_EVENT:
		ntextlines = 1;
		setup_text();
		timer_event = BLINK_EVENT;
		timer_event = BLANK_GAME_OVER_1_EVENT;
		next_timer = timer + 1;
		break;

	case INTRO1_EVENT: {
		int yline = 7;
		int x = 12;
		ntextlines = 1;
		set_font(SMALL_FONT);
		gotoxy(x,yline++);
		gameprint("In the beginning, there was ed."); gotoxy(x,yline++);
		gameprint("Ed is the standard text editor."); gotoxy(x,yline++);
		gameprint("Then there was vi, and it was good."); gotoxy(x,yline++);
		gameprint("Then came emacs, and disharmony."); gotoxy(x,yline++);
		gameprint("Your mission is to traverse core"); gotoxy(x,yline++);
		gameprint("memory and rid the host of emacs."); gotoxy(x,yline++);
		gameprint("It will not be an easy mission, as"); gotoxy(x,yline++);
		gameprint("there are many emacs friendly"); gotoxy(x, yline++);
		gameprint("processes."); gotoxy(x,yline++);
		timer_event = INTRO2_EVENT;
		next_timer = timer + 350;
		game_over_count = 0;
		break;
		}
	case INTRO2_EVENT: {
		ntextlines = 1;
		setup_text();
		timer_event = BLINK_EVENT;
		timer_event = BLANK_GAME_OVER_1_EVENT;
		next_timer = timer + 1;
		break;
		}
	case READY_EVENT:
		start_level();
		add_sound(MUSIC_SOUND, MUSIC_SLOT);
		sprintf(textline[CREDITS].string, "Credits: %d Lives: %d Score: %d Humans:%d/%d ", 
			credits, game_state.lives, game_state.score, 
			game_state.humanoids, level.nhumanoids);
		strcpy(textline[GAME_OVER].string, "Ready...");
		next_timer += 30;
		timer_event = SET_EVENT;
		ntextlines = 2;
		game_state.x = 0;
		game_state.y = 0;
		game_state.vy = 0;
		game_state.vx = 0;
		break;
	case SET_EVENT:
		strcpy(textline[GAME_OVER].string, "Set...");
		next_timer += 30;
		timer_event = GO_EVENT;
		break;
	case GO_EVENT:
		strcpy(textline[GAME_OVER].string, "Prepare to die!");
		next_timer += 30;
		timer_event = BLANK_EVENT;
		break;
	case BLANK_EVENT:
		ntextlines = 1;
		game_state.vx = PLAYER_SPEED;
		break;
	case GAME_ENDED_EVENT:
		timer_event = GAME_ENDED_EVENT_2;
		next_timer = timer + 30;
		if (credits <= 0) {
			setup_text();
			timer_event = GAME_ENDED_EVENT_2;
			next_timer = timer + 30;
			strcpy(textline[GAME_OVER+1].string, FINAL_MSG1);
			strcpy(textline[GAME_OVER].string, FINAL_MSG2);
			ntextlines = 4;
			next_timer = timer + 120;
		} else {
			strcpy(textline[GAME_OVER].string, "");
			timer_event = GAME_ENDED_EVENT_2;
			ntextlines = 2;
		}
		break;
	case GAME_ENDED_EVENT_2:
		next_timer = timer + 1;
		if (credits <= 0) {
			game_ended();
			timer_event = BLINK_EVENT;
			strcpy(textline[GAME_OVER].string, FINAL_MSG1);
			strcpy(textline[GAME_OVER+1].string, FINAL_MSG2);
			ntextlines = 4;
			next_timer = timer + 60;
		} else {
			timer_event = READY_EVENT; 
			ntextlines = 2;
		}
		game_ended();
		break;
	default: 
		break;
	}
}

gint advance_game(gpointer data)
{
	int i, ndead, nalive;

	gdk_threads_enter();
	ndead = 0;
	nalive = 0;
	game_state.x += game_state.vx;
	game_state.y += game_state.vy; 

	timer++;
	if (timer == next_timer)
		timer_expired();

	if (game_pause == 1)
		return TRUE;

	for (i=0;i<MAXOBJS;i++) {
		if (game_state.go[i].alive) {
			// printf("%d ", i);
			nalive++;
		} else
			ndead++;
		if (game_state.go[i].alive && game_state.go[i].move != NULL)
			game_state.go[i].move(&game_state.go[i]);
		// if (game_state.go[i].alive && game_state.go[i].move == NULL)
			// printf("NULL MOVE!\n");
	}
	gtk_widget_queue_draw(main_da);
	// printf("ndead=%d, nalive=%d\n", ndead, nalive);
	gdk_threads_leave();
	if (WORLDWIDTH - game_state.x < 100)
		return FALSE;
	else
		return TRUE;
}

void setup_text()
{
	cleartext();
	set_font(SMALL_FONT);
	gotoxy(0,0);
	gameprint("Credits: 0 Lives: 3");
	set_font(BIG_FONT);
	gotoxy(4,3);
	gameprint(" Game Over\n");
	gotoxy(4,2);
	gameprint("Word War vi\n");
	set_font(SMALL_FONT);
	gotoxy(13,15);
	gameprint("(c) 2007 Stephen Cameron\n");
	timer_event = BLINK_EVENT;
	next_timer = timer + 30;
#if 0
	gotoxy(1,6);
	gameprint("abcdefghijklmn");
	gotoxy(1,7);
	gameprint("opqrstuvwxyz");
	gotoxy(1,8);
	gameprint("0123456789,.+-");
#endif
}

void initialize_game_state_new_level()
{
	game_state.lives = 3;
	game_state.score = 0;
	game_state.humanoids = 0;
	game_state.prev_score = 0;
	game_state.health = MAXHEALTH;
	game_state.nbombs = level.nbombs;
	game_state.prev_bombs = -1;
}

void start_level()
{
	int i;


	for (i=0;i<MAXOBJS;i++) {
		game_state.go[i].alive = 0;
		game_state.go[i].vx = 0;
		game_state.go[i].vy = 0;
		game_state.go[i].move = move_obj;
	}
	memset(&game_state.go[0], 0, sizeof(game_state.go));

	game_state.humanoids = 0;
	game_state.direction = 1;
	player = &game_state.go[0];
	player->draw = NULL;
	player->move = move_player;
	player->v = (game_state.direction == 1) ? &player_vect : &left_player_vect;
	player->x = 200;
	player->y = -100;
	player->vx = PLAYER_SPEED;
	player->vy = 0;
	player->target = add_target(player);
	player->alive = 1;
	game_state.health = MAXHEALTH;
	game_state.nbombs = level.nbombs;
	game_state.prev_bombs = -1;
	game_state.nobjs = MAXOBJS-1;
	game_state.x = 0;
	game_state.y = 0;

	srandom(level.random_seed);
	generate_terrain(&terrain);
	add_rockets(&terrain);
	add_buildings(&terrain);
	add_fuel(&terrain);
	add_SAMs(&terrain);
	add_humanoids(&terrain);
	add_bridges(&terrain);
	add_flak_guns(&terrain);
	add_airships(&terrain);
	add_balloons(&terrain);
	add_socket(&terrain);

	if (credits == 0)
		setup_text();

}

void init_levels_to_beginning()
{
	level.nrockets = NROCKETS;
	level.nbridges = NBRIDGES;
	level.nflak = NFLAK;
	level.nfueltanks = NFUELTANKS;
	level.nsams = NSAMS;
	level.nsams = NHUMANOIDS;
	level.nbuildings = NBUILDINGS;
	level.nbombs = NBOMBS;
	if (credits > 0) {
		level.random_seed = 31415927;
		level.laser_fire_chance = LASER_FIRE_CHANCE;
		level.large_scale_roughness = LARGE_SCALE_ROUGHNESS;
		level.small_scale_roughness = SMALL_SCALE_ROUGHNESS;;
	} else {
		level.random_seed = random();
		level.laser_fire_chance = LASER_FIRE_CHANCE;
		level.large_scale_roughness = LARGE_SCALE_ROUGHNESS;
		level.small_scale_roughness = 0.45;
		level.nflak = NFLAK + 20;
		level.nrockets = NROCKETS + 20;
	}
}

void game_ended()
{
	init_levels_to_beginning();
	initialize_game_state_new_level();
	start_level();
}

void cancel_sound(int queue_entry);
void advance_level()
{
	/* This is harsh. */
	srandom(level.random_seed);
	level.random_seed = random(); /* deterministic */
	level.nrockets += 10;
	level.nbridges += 1;
	level.nflak += 10;
	level.nfueltanks += 2;
	level.nsams += 2;
	level.nhumanoids += 1;
	level.large_scale_roughness+= (0.03);
	if (level.large_scale_roughness > 0.3)
		level.large_scale_roughness = 0.3;
	level.small_scale_roughness+=(0.03);
	if (level.small_scale_roughness > 0.60)
		level.small_scale_roughness = 0.60;
	// level.nbuildings;
	level.nbombs -= 5;
	if (level.nbombs < 50) 
		level.nbombs = 50;
	level.laser_fire_chance++; /* this is bad. */
	if (level.laser_fire_chance > 10)
		level.laser_fire_chance = 10;

	initialize_game_state_new_level();
	start_level();
}

static gint key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
	/* char *x = (char *) data; */
#if 0
	if (event->length > 0)
		printf("The key event's string is `%s'\n", event->string);

	printf("The name of this keysym is `%s'\n", 
		gdk_keyval_name(event->keyval));
#endif
	switch (event->keyval)
	{
	case GDK_9:
		if (credits > 0)
			game_state.health = -1;
		return TRUE;
	case GDK_Escape:
		destroy_event(widget, NULL);
		return TRUE;	
	case GDK_q:
		add_sound(INSERT_COIN_SOUND, ANY_SLOT);
		credits++;
		if (credits == 1) {
			cancel_sound(MUSIC_SLOT);
			sleep(2);
			ntextlines = 1;
			game_ended();
			/* initialize_game_state_new_level();
			init_levels_to_beginning();
			start_level(); */
			timer_event = READY_EVENT;
			next_timer = timer+1;
		}
		sprintf(textline[CREDITS].string, "Credits: %d Lives: %d", credits, game_state.lives);
		return TRUE;
#if 0
	case GDK_Home:
		printf("The Home key was pressed.\n");
		return TRUE;
#endif
	case GDK_j:
	case GDK_Down:
		if (player->vy < MAX_VY && game_state.health > 0 && credits > 0)
			player->vy += 4;
		return TRUE;
	case GDK_k:
	case GDK_Up:
		if (player->vy > -MAX_VY && game_state.health > 0 && credits > 0)
			player->vy -= 4;
		return TRUE;
	case GDK_l:
	case GDK_Right:
	case GDK_period:
	case GDK_greater:
		if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		if (game_state.direction != 1) {
			game_state.direction = 1;
			player->vx = player->vx / 2;
			player->v = &player_vect;
		} else if (abs(player->vx + game_state.direction) < MAX_VX)
				player->vx += game_state.direction;
		return TRUE;
	case GDK_h:
	case GDK_Left:
	case GDK_comma:
	case GDK_less:
		if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		if (game_state.direction != -1) {
			player->vx = player->vx / 2;
			game_state.direction = -1;
			player->v = &left_player_vect;
		} else if (abs(player->vx + game_state.direction) < MAX_VX)
				player->vx += game_state.direction;
		return TRUE;
	case GDK_space:
	case GDK_z:
		if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		player_fire_laser();
		return TRUE;
		break;	
	case GDK_x:
		if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		if (abs(player->vx + game_state.direction) < MAX_VX)
			player->vx += game_state.direction;
		return TRUE;

	case GDK_c: if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		drop_chaff();
		return TRUE;
	case GDK_b: if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		drop_bomb();
		return TRUE;
	case GDK_p:
		if (game_state.health <= 0 || credits <= 0)
			return TRUE;
		do_game_pause(widget, NULL);
		return TRUE;
#if 1
/* These two just for testing... */
	case GDK_R:
		start_level();
		break;
	case GDK_A:
		advance_level();
		break;
/* The above 2 just for testing */
#endif
	default:
		break;
	}

#if 0
	printf("Keypress: GDK_%s\n", gdk_keyval_name(event->keyval));
	if (gdk_keyval_is_lower(event->keyval)) {
		printf("A non-uppercase key was pressed.\n");
	} else if (gdk_keyval_is_upper(event->keyval)) {
		printf("An uppercase letter was pressed.\n");
	}
#endif
	return FALSE;
}

/***********************************************************************/
/* Beginning of AUDIO related code                                     */
/***********************************************************************/


struct sound_clip {
	int active;
	int nsamples;
	int pos;
	double *sample;
} clip[NCLIPS];

struct sound_clip audio_queue[MAX_CONCURRENT_SOUNDS];

int nclips = 0;


int read_clip(int clipnum, char *filename)
{
	SNDFILE *f;
	SF_INFO sfinfo;
	sf_count_t nframes;

	memset(&sfinfo, 0, sizeof(sfinfo));
	f = sf_open(filename, SFM_READ, &sfinfo);
	if (f == NULL) {
		fprintf(stderr, "sf_open('%s') failed.\n", filename);
		return -1;
	}

	printf("Reading sound file: '%s'\n", filename);
	printf("frames = %lld\n", sfinfo.frames);
	printf("samplerate = %d\n", sfinfo.samplerate);
	printf("channels = %d\n", sfinfo.channels);
	printf("format = %d\n", sfinfo.format);
	printf("sections = %d\n", sfinfo.sections);
	printf("seekable = %d\n", sfinfo.seekable);

	clip[clipnum].sample = (double *) 
		malloc(sizeof(double) * sfinfo.channels * sfinfo.frames);
	if (clip[clipnum].sample == NULL) {
		printf("Can't get memory for sound data for %llu frames in %s\n", 
			sfinfo.frames, filename);
		goto error;
	}

	nframes = sf_readf_double(f, clip[clipnum].sample, sfinfo.frames);
	if (nframes != sfinfo.frames) {
		printf("Read only %llu of %llu frames from %s\n", 
			nframes, sfinfo.frames, filename);
	}
	clip[clipnum].nsamples = (int) nframes;
	if (clip[clipnum].nsamples < 0)
		clip[clipnum].nsamples = 0;

	sf_close(f);
	return 0;
error:
	sf_close(f);
	return -1;
}

/* precompute 16 2-second clips of various sine waves */
int init_clips()
{
	int i, j;
	double phase, phaseinc, sawinc, sawval;
	phaseinc = 0.01;
	sawinc = 0.01;

	memset(&audio_queue, 0, sizeof(audio_queue));

	for (i=0;i<NCLIPS;i++) {
		clip[i].nsamples = CLIPLEN;
		clip[i].pos = 0;
		clip[i].active = 0;
		sawval = 0;
		sawinc = (double) 0.01 + (double) i / 1000.0;
		phase = 0.0;
		clip[i].sample = malloc(sizeof(clip[i].sample[0]) * CLIPLEN); 
		if (clip[i].sample == NULL) {
			printf("malloc failed, i=%d\n", i);
			continue;
		}
		for (j=0;j<CLIPLEN;j++) {
			if ((i % 3) != 0) {
				clip[i].sample[j] = (double) (((CLIPLEN) - j) / ((double) 2*j+CLIPLEN)) * sin(phase);
				phase += phaseinc;
				if (phase > TWOPI) 
					phase -= TWOPI;
			} else {
				sawval += sawinc;
				clip[i].sample[j] = sawval * (double) (CLIPLEN -j) / (double) (j+CLIPLEN);
				if (sawval > 0.80 || sawval < -0.80)
					sawinc = -sawinc;
			}
		}
		phaseinc *= 1.4;
		//phaseinc *= 1.02;
	}

	read_clip(PLAYER_LASER_SOUND, "sounds/18385_inferno_laserbeam.wav");
	read_clip(BOMB_IMPACT_SOUND, "sounds/18390_inferno_plascanh.wav");
	read_clip(ROCKET_LAUNCH_SOUND, "sounds/18386_inferno_lightrl.wav");
	read_clip(FLAK_FIRE_SOUND, "sounds/18382_inferno_hvylas.wav");
	read_clip(LARGE_EXPLOSION_SOUND, "sounds/18384_inferno_largex.wav");
	read_clip(ROCKET_EXPLOSION_SOUND, "sounds/9679__dobroide__firecracker.04_modified.wav");
	read_clip(LASER_EXPLOSION_SOUND, "sounds/18399_inferno_stormplas.wav");
	read_clip(GROUND_SMACK_SOUND, "sounds/ground_smack.wav");
	read_clip(INSERT_COIN_SOUND, "sounds/us_quarter.wav");
	read_clip(MUSIC_SOUND, "sounds/lucky13-steve-mono-mix.wav");
	read_clip(SAM_LAUNCH_SOUND, "sounds/18395_inferno_rltx.wav");

	return 0;
}


/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, __attribute__ ((unused)) void *userData )
{
	// void *data = userData; /* Prevent unused variable warning. */
	float *out = (float*)outputBuffer;
	int i, j, sample, count = 0;
	(void) inputBuffer; /* Prevent unused variable warning. */
	float output = 0.0;

	for (i=0; i<framesPerBuffer; i++) {
		output = 0.0;
		count = 0;
		for (j=0; j<NCLIPS; j++) {
			if (!audio_queue[j].active || 
				audio_queue[j].sample == NULL)
				continue;
			sample = i + audio_queue[j].pos;
			count++;
			if (sample >= audio_queue[j].nsamples) {
				audio_queue[j].active = 0;
				continue;
			}
			output += audio_queue[j].sample[sample];
		}
		*out++ = (float) output / 2; /* (output / count); */
        }
	for (i=0;i<NCLIPS;i++) {
		if (!audio_queue[i].active)
			continue;
		audio_queue[i].pos += framesPerBuffer;
		if (audio_queue[i].pos >= audio_queue[i].nsamples)
			audio_queue[i].active = 0;
	}
	return 0; /* we're never finished */
}

static PaStream *stream = NULL;

void decode_paerror(PaError rc)
{
	if (rc == paNoError)
		return;
	fprintf(stderr, "An error occured while using the portaudio stream\n");
	fprintf(stderr, "Error number: %d\n", rc);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(rc));
}

void terminate_portaudio(PaError rc)
{
	Pa_Terminate();
	decode_paerror(rc);
}

int initialize_portaudio()
{
	PaStreamParameters outparams;
	PaError rc;

	init_clips();

	rc = Pa_Initialize();
	if (rc != paNoError)
		goto error;
    
	outparams.device = Pa_GetDefaultOutputDevice();  /* default output device */
	outparams.channelCount = 1;                      /* mono output */
	outparams.sampleFormat = paFloat32;              /* 32 bit floating point output */
	outparams.suggestedLatency = 
		Pa_GetDeviceInfo(outparams.device)->defaultLowOutputLatency;
	outparams.hostApiSpecificStreamInfo = NULL;

	rc = Pa_OpenStream(&stream,
		NULL,         /* no input */
		&outparams, SAMPLE_RATE, FRAMES_PER_BUFFER,
		paNoFlag, /* paClipOff, */   /* we won't output out of range samples so don't bother clipping them */
		patestCallback, NULL /* cookie */);    
	if (rc != paNoError)
		goto error;
	if ((rc = Pa_StartStream(stream)) != paNoError);
		goto error;
#if 0
	for (i=0;i<20;i++) {
		for (j=0;j<NCLIPS;j++) {
			// printf("clip[%d].pos = %d, active = %d\n", j, clip[j].pos, clip[j].active);
			Pa_Sleep( 250 );
			if (clip[j].active == 0) {
				clip[j].nsamples = CLIPLEN;
				clip[j].pos = 0;
				clip[j].active = 1;
			}
		}
		Pa_Sleep( 1500 );
	}
#endif
	return rc;
error:
	terminate_portaudio(rc);
	return rc;
}


void stop_portaudio()
{
	int rc;

	if ((rc = Pa_StopStream(stream)) != paNoError)
		goto error;
	rc = Pa_CloseStream(stream);
error:
	terminate_portaudio(rc);
	return;
}

int add_sound(int which_sound, int which_slot)
{
	int i;

	if (which_slot != ANY_SLOT) {
		if (audio_queue[which_slot].active)
			audio_queue[which_slot].active = 0;
		audio_queue[which_slot].pos = 0;
		audio_queue[which_slot].nsamples = 0;
		/* would like to put a memory barrier here. */
		audio_queue[which_slot].sample = clip[which_sound].sample;
		audio_queue[which_slot].nsamples = clip[which_sound].nsamples;
		/* would like to put a memory barrier here. */
		audio_queue[which_slot].active = 1;
		return which_slot;
	}
	for (i=1;i<MAX_CONCURRENT_SOUNDS;i++) {
		if (audio_queue[i].active == 0) {
			audio_queue[i].nsamples = clip[which_sound].nsamples;
			audio_queue[i].pos = 0;
			audio_queue[i].sample = clip[which_sound].sample;
			audio_queue[i].active = 1;
			break;
		}
	}
	return (i >= MAX_CONCURRENT_SOUNDS) ? -1 : i;
}

void cancel_sound(int queue_entry)
{
	audio_queue[queue_entry].active = 0;
}

void cancel_all_sounds()
{
	int i;
	for (i=0;i<MAX_CONCURRENT_SOUNDS;i++)
		audio_queue[i].active = 0;
}

/***********************************************************************/
/* End of AUDIO related code                                     */
/***********************************************************************/


int main(int argc, char *argv[])
{
	/* GtkWidget is the storage type for widgets */
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *vbox;
	int i;

	struct timeval tm;
	gettimeofday(&tm, NULL);
	srandom(tm.tv_usec);	

	if (initialize_portaudio() != paNoError)
		printf("Guess sound's not working...\n");

	gtk_set_locale();
	gtk_init (&argc, &argv);
   
	gdk_color_parse("white", &huex[WHITE]);
	gdk_color_parse("blue", &huex[BLUE]);
	gdk_color_parse("black", &huex[BLACK]);
	gdk_color_parse("green", &huex[GREEN]);
	gdk_color_parse("yellow", &huex[YELLOW]);
	gdk_color_parse("red", &huex[RED]);
	gdk_color_parse("orange", &huex[ORANGE]);
	gdk_color_parse("cyan", &huex[CYAN]);
 
    /* create a new window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    /* When the window is given the "delete_event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
    g_signal_connect (G_OBJECT (window), "delete_event",
		      G_CALLBACK (delete_event), NULL);
    
    /* Here we connect the "destroy" event to a signal handler.  
     * This event occurs when we call gtk_widget_destroy() on the window,
     * or if we return FALSE in the "delete_event" callback. */
    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK (destroy), NULL);
    
    /* Sets the border width of the window. */
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
   
	vbox = gtk_vbox_new(FALSE, 0); 
	main_da = gtk_drawing_area_new();
	gtk_widget_modify_bg(main_da, GTK_STATE_NORMAL, &huex[WHITE]);
	gtk_widget_set_size_request(main_da, SCREEN_WIDTH, SCREEN_HEIGHT);

	g_signal_connect(G_OBJECT (main_da), "expose_event", G_CALLBACK (main_da_expose), NULL);

    button = gtk_button_new_with_label ("Quit");
    score_label = gtk_label_new("Score: 000000");
    bombs_label = gtk_label_new("Bombs: 99");
    
    /* When the button receives the "clicked" signal, it will call the
     * function hello() passing it NULL as its argument.  The hello()
     * function is defined above. */
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (destroy_event), NULL);
    
    /* This will cause the window to be destroyed by calling
     * gtk_widget_destroy(window) when "clicked".  Again, the destroy
     * signal could come from here, or the window manager. */
    g_signal_connect_swapped (G_OBJECT (button), "clicked",
			      G_CALLBACK (gtk_widget_destroy),
                              G_OBJECT (window));
    
    /* This packs the button into the window (a gtk container). */
    gtk_container_add (GTK_CONTAINER (window), vbox);

    gtk_box_pack_start(GTK_BOX (vbox), main_da, TRUE /* expand */, FALSE /* fill */, 2);
    gtk_box_pack_start(GTK_BOX (vbox), button, FALSE /* expand */, FALSE /* fill */, 2);
    gtk_box_pack_start(GTK_BOX (vbox), score_label, FALSE /* expand */, FALSE /* fill */, 2);
    gtk_box_pack_start(GTK_BOX (vbox), bombs_label, FALSE /* expand */, FALSE /* fill */, 2);
    
	init_vects();
	init_vxy_2_dxy();
	
	g_signal_connect(G_OBJECT (window), "key_press_event",
		G_CALLBACK (key_press_cb), "window");



	// print_target_list();

	for (i=0;i<NCOLORS;i++)
		gdk_colormap_alloc_color(gtk_widget_get_colormap(main_da), &huex[i], FALSE, FALSE);
	gdk_gc_set_foreground(gc, &huex[BLUE]);
	gdk_gc_set_foreground(gc, &huex[WHITE]);
	gtk_widget_modify_bg(main_da, GTK_STATE_NORMAL, &huex[BLACK]);


	initialize_game_state_new_level();
	start_level();

    gtk_widget_show (vbox);
    gtk_widget_show (main_da);
    // gtk_widget_show (button);
    gtk_widget_show (score_label);
    gtk_widget_show (bombs_label);
    
    /* and the window */
    gtk_widget_show (window);
	gc = gdk_gc_new(GTK_WIDGET(main_da)->window);

    timer_tag = g_timeout_add(42, advance_game, NULL);
    
    /* All GTK applications must have a gtk_main(). Control ends here
     * and waits for an event to occur (like a key press or
     * mouse event). */

    g_thread_init(NULL);
    gdk_threads_init();
    gtk_main ();

    stop_portaudio();
    
    return 0;
}
