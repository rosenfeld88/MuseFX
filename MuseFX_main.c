/*
 * Authors: Taylor Rosenfeld and Yanwen Xiong
 * Date: Project began on 1/8/18
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include "stdio.h"
#include "evmdm6437.h"
#include "aic33_functions.h"
#include "evmdm6437_aic33.h"

/////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//         DON'T change the following setting for EDMA         //
/////////////////////////////////////////////////////////////////
/* EDMA Registers for 6437*/
#define		PaRAM_OPT	0	// Channel Options Parameter
#define		PaRAM_SRC	1	// Channel Source Address
#define		PaRAM_BCNT	2	// Count for 2nd Dimension (BCNT) | Count for 1st Dimension (ACNT)
#define		PaRAM_DST	3	// Channel Destination Address
#define		PaRAM_BIDX	4	// Destination BCNT Index | Source BCNT Index
#define		PaRAM_RDL	5	// BCNT Reload (BCNTRLD) | Link Address (LINK)
#define		PaRAM_CIDX	6	// Destination CCNT Index | Source CCNT Index
#define		PaRAM_CCNT	7	// Count for 3rd Dimension (CCNT)

/* EDMA Registers for 6437*/
#define		EDMA_IPR	*(volatile int *)0x01C01068	// EDMA Channel interrupt pending low register
#define		EDMA_IPRH	*(volatile int *)0x01C0106C	// EDMA Channel interrupt pending high register
#define		EDMA_IER	*(volatile int *)0x01C01050	// EDMA Channel interrupt enable low register
#define		EDMA_IERH	*(volatile int *)0x01C01054	// EDMA Channel interrupt enable high register
#define		EDMA_ER 	*(volatile int *)0x01C01000	// EDMA Event low register
#define		EDMA_ERH	*(volatile int *)0x01C01004	// EDMA Event high register
#define		EDMA_EER	*(volatile int *)0x01C01020	// EDMA Event enable low register
#define		EDMA_EERH	*(volatile int *)0x01C01024	// EDMA Event enable high register
#define		EDMA_ECR	*(volatile int *)0x01C01008	// EDMA Event clear low register
#define		EDMA_ECRH	*(volatile int *)0x01C0100C	// EDMA Event clear high register
#define		EDMA_ESR	*(volatile int *)0x01C01010	// EDMA Event set low register
#define		EDMA_ESRH	*(volatile int *)0x01C01014	// EDMA Event set high register
/////////////////////////////////////////////

extern far cregister volatile unsigned int IER;
extern far cregister volatile unsigned int CSR;
extern far cregister volatile unsigned int ICR;
extern far cregister volatile unsigned int ISTP;
extern far cregister volatile unsigned int ISR;
extern far cregister volatile unsigned int IFR;

/////////////////////////////////////////////////////////////////

extern Int16 video_loopback_test();

/************************************************** Defines *****************************************************************/

////////////////////////////////////////////////////////// VIDEO ////////////////////////////////////////////////////////////

// size for buffer_in: 720 * 480 / 2, the reason is explained below. 
#define Pixels 172800

// Resolution 720 * 480 (NTSC mode)
#define vWidth 720
#define vHeight 480

// CAN change the internal blocksize here, the example is 60 * 120
#define INTERNAL_BLK_WIDTH 12
#define INTERNAL_BLK_HEIGHT 12

// Hex color values
#define WHITE 0xEB80EB80 		// i.e. value of a white pixel.
#define BLACK 0x10801080
#define RED 0x52F0525A
#define BLUE 0x296E29F0
#define GREEN 0x5151515B
#define L_BRN 0x8FA68F5B
#define YELLOW 0xD292D210
#define GRAY 0x89808980

// Color marker value
#define WHITE_MARK 1
#define BLACK_MARK 0
#define RED_MARK 2
#define BLUE_MARK 3
#define GREEN_MARK 4  
#define L_BRN_MARK 5
#define YELLOW_MARK 6
#define GRAY_MARK 7

#define MAX_STRING 50 			// Max string length.

// Convex Hull constants
#define MIN_HULL 3 				// Minimum number of points in a valid convex hull.
#define HULL_VAL 2 				// Label for hull pts in frame

// General Graphics Dimensions
#define LETTER_HEIGHT 5
#define LETTER_WIDTH 3
#define FX_NAME_SPACING 10
#define SPACING 4
#define B_SIDE 6
#define B_SHIFT 0
#define FADER_TRK_W 6
#define FADER_TRK_H 29
#define FADER_W 6
#define FADER_H 2
#define FADER_TRK_HOR_PAD 10
#define FADER_TRK_VERT_PAD 3

////////////////////////////////////////////////////////// AUDIO ////////////////////////////////////////////////////////////

// Distortion Parameter Indices
#define DIST_GAIN 0
#define DIST_SAT 1
//#define DIST_TONE 2
#define DIST_LVL 2
#define DIST_MIX 3

// Phaser Parameter Indices
#define PHASER_RATE 0
#define PHASER_MIX 1

// Flanger Parameter Indices
#define FLANGER_RATE 0
#define FLANGER_DELAY 1
#define FLANGER_MIX 2

// Echo Parameter Indices
#define ECHO_DELAY 0
#define ECHO_REPEATS 1
#define ECHO_MIX 2

// Effect defines
#define MIN_LFO_RATE 0.1 		// Minimum LFO rate 
#define MAX_LFO_RATE 4 			// Maximum LFO rate 
#define AP_LENGTH 3 			// Length of all pass filter arrays
#define SAMPLE_RATE 48000 		// Sample rate (44.1 kHz)
#define LFO_SIZE 100 		// Number of samples in lookup table
#define M_PI 3.14159 			// Pi

#define MAX_BUF_SIZE 24000 		// Max buffer size for echo and flanger effect
#define MAX_DELAY_SAMPLE 480 	// Max delayed of flanger
//#define LFO_SIZE 100 	// # of samples in lfo for flanger

//////////////////////////////////////////////// FLAGS/LABELS ////////////////////////////////////////////////////////

#define FRAMES2SWITCH 8		// period (in num of frames) to gather information and make decision
#define FRAMES2TUNE 8		// num of frames for one tuning move
#define FRAMES2STOP 50		// num of idle frames until automatically off
#define FRAMES2CALIBRATE 30 // number of frames to calibrate 
#define NUM_GESTURES 5 // number of gestures to recognize
#define SYS_ON 5 // system on label
#define SYS_OFF 0 // system off label
#define ON 1 // general on label
#define OFF 0 // general off label
#define DIST 1 // distortion effect label
#define PHASER 2 // phaser effect label
#define FLANGER 3 // flanger effect label
#define ECHO 10 // echo effect label
#define TWO 1 // two gesture label 
#define THREE 2 // three gesture label 
#define FOUR 3 // four gesture label
#define FIVE 4 // five gesture label
#define SIX 5 // dud gesture label

/************************************************ Global Variables **********************************************************/
// Program flags, DIPS, Codec handle, etc.
unsigned char flag=0;
AIC33_CodecHandle aic33handle;
int maxBlkRow;
int minBlkRow;
int maxBlkCol;
int minBlkCol;
int faderBottom; // height screen bottom to fader track bottom.
int faderDispW; // total usable width for displaying faders
float scale_factor = 1.0;

// Define a space on memory for save the information input and output (Interface data)
Uint32 buffer_out[Pixels]; //from 0x80000000
Uint32 buffer_in[Pixels]; //from 0x800A8C00, which is the same as 4 (bytes for integer) * Pixels
unsigned char binary_hand[(vHeight/INTERNAL_BLK_HEIGHT + 2) * (vWidth/INTERNAL_BLK_WIDTH + 2)];
unsigned char binary_boundary[(vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH)];
int display[(vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH)];

// Declare the internal buffer
Uint32 internal_buffer_1D[INTERNAL_BLK_HEIGHT * INTERNAL_BLK_WIDTH / 2];

//////////////////////////////////////////////////// Letters ///////////////////////////////////////////////////////////
int size_d = 10;
int d_x[] = {0, 1, 0, 2, 0, 2, 0, 2, 0, 1};
int d_y[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4};

int size_p = 10;
int p_x[] = {0, 0, 0, 1, 2, 0, 2, 0, 1, 2};
int p_y[] = {0, 1, 2, 2, 2, 3, 3, 4, 4, 4};

int size_f = 8;
int f_x[] = {0, 0, 0, 1, 0, 0, 1, 2};
int f_y[] = {0, 1, 2, 2, 3, 4, 4, 4};

int size_e = 10;
int e_x[] = {0, 1, 2, 0, 0, 1, 0, 0, 1, 2};
int e_y[] = {0, 0, 0, 1, 2, 2, 3, 4, 4, 4};

int size_b = 12;
int b_x[] = {0, 1, 2, 0, 2, 0, 1, 0, 2, 0, 1, 2};
int b_y[] = {0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4};

int size_a = 10;
int a_x[] = {0, 2, 0, 2, 0, 1, 2, 0, 2, 1};
int a_y[] = {0, 0, 1, 1, 2, 2, 2, 3, 3, 4};

int size_c = 9;
int c_x[] = {0, 1, 2, 0, 0, 0, 0, 1, 2};
int c_y[] = {0, 0, 0, 1, 2, 3, 4, 4, 4};

int size_k = 10;
int k_x[] = {0, 2, 0, 2, 0, 1, 0, 2, 0, 2};
int k_y[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4};

int size_m = 14;
int m_x[] = {0, 4, 0, 4, 0, 2, 4, 0, 2, 4, 0, 1, 3, 4};
int m_y[] = {0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4};

int size_r = 8;
int r_x[] = {0, 0, 0, 2, 0, 1, 2, 0};
int r_y[] = {0, 1, 2, 2, 3, 3, 3, 4};

int size_d_lc = 10;
int d_lc_x[] = {0, 1, 2, 0, 2, 0, 1, 2, 2, 2};
int d_lc_y[] = {0, 0, 0, 1, 1, 2, 2, 2, 3, 4};

int size_g = 12;
int g_x[] = {0, 1, 2, 2, 0, 1, 2, 0, 2, 0, 1, 2};
int g_y[] = {0, 0, 0, 1, 2, 2, 2, 3, 3, 4, 4, 4};

int size_l = 5;
int l_x[] = {0, 0, 0, 0, 0};
int l_y[] = {0, 1, 2, 3, 4};

int size_L = 7;
int L_x[] = {0, 1, 2, 0, 0, 0, 0};
int L_y[] = {0, 0, 0, 1, 2, 3, 4};

int size_t_let = 7;
int t_x[] = {0, 0, -1, 0, 1, 0, 0};
int t_y[] = {0, 1, 2, 2, 2, 3, 4};

int size_s = 11;
int s_x[] = {0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2};
int s_y[] = {0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4};

int size_zero = 10;
int zero_x[] = {0, 1, -1, 2, -1, 2, -1, 2, 0, 1};
int zero_y[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4};

/////////////////////////////////////////////////// Flags //////////////////////////////////////////////////////////////

unsigned char flag_systemOn = 0;
unsigned char flag_selectFx = 0;
unsigned char flag_tuneFx = 0;
unsigned char flag_resetGesture = 0;

///////////////////////////////////////////// Effects Parameters ///////////////////////////////////////////////////////

Int16 preAmp = 1; // Preamp gain

// DISTORTION

float max_val = 32000.0; // Max sample value for clipping
float prev_filt_in = 0;
float prev_filt_out = 0;
float period = (float) 1/SAMPLE_RATE;
float gamma;
float a;
float b;

// PHASER

// LFO Parameters
float lfo_gain = 1; // LFO Amplitude
float lfo[LFO_SIZE]; // LFO lookup table (base frequency of MIN_LFO_RATE)
int phaser_lfo_count = 0;
int flanger_lfo_count = 0; // LFO lookup index

// Other System Parameters
float G_fb = 0.75; // Feedback gain
float depth = 1; // Phaser effect depth
int loop_count = 0; // Tracks number of interrupt executions

// All Pass Filters (Designed in MATLAB)

// Stage 1
float w1 = 400;
float p1;
	
// Stage 2
float w2 = 800;
float p2;
	
// Stage 3
float w3 = 1500;
float p3;
	
// Stage 4
float w4 = 5000;
float p4;

// Previous Values
short ap1_prev = 0;
short ap2_prev = 0;
short ap3_prev = 0;
short ap4_prev = 0;
short in_prev = 0;

// ECHO
int playCnt = 0;
float gain_ff = 0.8, gain_fb = 0.8;
short buffer_echo[MAX_BUF_SIZE];

// FLANGER
short buffer_flanger[MAX_DELAY_SAMPLE];
float sine_mod;
/*************************************************** Struct Definitions **************************************************/
/*
 * Definition of point struct. Origin is at the bottom left corner of the screen.
 * For video frame matrix indices to point coordinates conversion, the x-coordinate
 * corresponds to the frame column and the y-coordinate is frame height - row.
 * 
 * @field x X coordinate.
 * @field y Y coordinate.
 * @field angle Flipped and shifted cosine of pixel angle relative to start pixel (esp.
 * for Graham's Scan). Defined as: [-<a,b> / (||a|| ||b||)] + 1, where a is the vector from the 
 * start pixel to the point and b is an arbitrary magnitude horizontal vector from the start 
 * pixel. All of the b terms cancel and this reduces to (-x_a / sqrt(x_a^2 + y_a^2)).
 */
typedef struct Point{
	int x;
	int y;
	float angle;
} Point;

/*
 * Component struct definition. Defines the four corners of graphics components placed on the screen.
 * 
 * @field top_left Top left component corner.
 * @field top_right Top right component corner.
 * @field bottom_left Bottom left component corner.
 * @field bottom_right Bottom right component corner.
 */
 typedef struct Component{
 	Point bottom_left;
 	int height;
 	int width;
 } Component;
 
 /*
  * Button struct defintion. Combines a component with a state.
  * 
  * @field Comp Component defining button position.
  * @field state Button state (on/off, 1/0).
  */
typedef struct Button{
	Component comp;
	int state;
} Button;

/*
 * Stores the state of each effect. 
 * 
 * @field numParams Number of tunable parameters in effect.
 * @field faders Graphics components storing the positioning parameter faders.
 * The fader y-coordinate is used to map to parameter values.
 * @field param_vals Stores the actual value of each parameter.
 * @field status On/off status of effect.
 * @field param_ranges Range of values for each parameter.
 */
typedef struct Effect{
	int numParams;
	Component* faders;
	float* param_vals;
	Uint8 status;
	float* param_ranges;
} Effect;

// Effects
Effect dist;
Effect phaser;
Effect flanger;
Effect echo;

// Point data structure
Point hand[(vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH)];
Point convex_hull[(vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH)];
int hullSize = 0;
Point pot_pts[(vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH)];

// Important points
Point centroid; // convex hull centroid
Point start; // Graham Scan start point
Point curCentroid;
Point recentCentroids[FRAMES2STOP];
int lett_start_x;
int d_start_y;
int p_start_y;
int f_start_y;
int e_start_y;

// Graphics components
Button sys_tog;

//Gestures curGesture;	// current gesture
int gesture_ct[NUM_GESTURES];	// count frequency of gestures in 10 frames
int recentGestures[FRAMES2STOP];
int curGesture = 5;
int inactiveFrames = 0;
int finger_ct = 0;	// number of fingers in the frame
int frame_ct = 0;	// count number of frames have been collected
int resetGesture_ct = 0;
int x_thresh_fist = 12;
int y_thresh_fist = 14;
int x_thresh_five = 20;

/****************************************************** Memory Allocations ***************************************************/

// Define the position of the data (refer to linker.cmd)
// Internal memory L2RAM ".l2ram" 
// External memory DDR2 ".ddr2"
#pragma DATA_SECTION(buffer_out,".ddr2")
#pragma DATA_SECTION(buffer_in,".ddr2")
#pragma DATA_SECTION(hand, ".ddr2")
#pragma DATA_SECTION(convex_hull, ".ddr2")
#pragma DATA_SECTION(pot_pts, ".ddr2")
#pragma DATA_SECTION(buffer_echo, ".ddr2")

// buffer_in represents one input frame which consists of two interleaved frames.
// Each 32 bit data has the information for two adjacent pixels in a row.
// Thus, the buffer holds 720/2 integer data points for each row of 2D image and there exist 480 rows.
//
// Format: yCbCr422 ( y1 | Cr | y0 | Cb )
// Each of y1, Cr, y0, Cb has 8 bits
// For each pixel in the frame, it has y, Cb, Cr components
//
// You can generate a lookup table for color conversion if you want to convert to different color space such as RGB.
// Could refer to http://www.fourcc.org/fccyvrgb.php for conversion between yCbCr and RGB



/************************************** QUICK SORT IMPLEMENTATION *****************************************/
/*
 * Swap method. Swaps two array elements.
 *
 * @param x Pointer to first element to be swapped.
 * @param y Pointer to second element to be swapped.
 */
void swap(Point* x, Point* y){
	Point temp = *x;
	*x = *y;
	*y = temp;
}

/*
 * Partition method. Selects end element as pivot and partitions sub-array
 * into halves greater and less than the pivot.
 *
 * @param start Start index of sub-array.
 * @param end End index of sub-array.
 * @return Index of splitter element.
 */
int partition(int start, int end){
	int smaller = start - 1;
	int pivot_index = end;
	int larger;
	for(larger = start; larger <= end - 1; larger++){
		if((hand + larger)->angle <= (hand + pivot_index)->angle){
			smaller++;
			swap(&(*(hand + smaller)), &(*(hand + larger))); // Conquer
		}
	}
	swap(&(*(hand + pivot_index)), &(*(hand + smaller + 1))); // Conquer
	return smaller + 1;
}

/*
 * Recursive quick sort method.
 *
 * @param start Start index of sub-array.
 * @param end End index of sub-array.
 */
void quickSort(int start, int end){
	if(start < end){
		int part_index = partition(start, end);
		quickSort(start, part_index - 1); // Divide
		quickSort(part_index + 1, end); // Divide
	}
}

/************************************** Graham Scan Implementation ***************************************/\

/*
 * Finds the convex hull of the data.
 * Uses Graham Scan's logic for identifying points of the convex hull.
 * Beginning at starting point, the algorithm scans through sorted hand pixels.
 * If a point constitutes counter-clockwise motion it is added to the stack.
 * Otherwise, the point is skipped.
 *
 * @param size Number of data points for which convex hull is to be found.
 * @param hand Data for which convex hull is to be found.
 * @param start Start point for Graham Scan.
 * @return Pointer to stack containing convex hull points.
 */
int getConvexHull(int size){
	// Find closed path around the points.
	// No same angle points => keep the one farthest from start
	int end = 0; // tracks the index of last element in pot_pts
	pot_pts[end] = hand[0];
	float thresh = 0.1; // for handling numerical precision issues
	int i;
	for(i = 1; i < size; i++){
		Point prev = pot_pts[end];
		Point curr = hand[i];
		int prev_len = ((prev.x - start.x)*(prev.x - start.x)) + ((prev.y - start.y)*(prev.y - start.y));
		int curr_len = ((curr.x - start.x)*(curr.x - start.x)) + ((curr.y - start.y)*(curr.y - start.y));
		float angle_diff = curr.angle - prev.angle;
		if(angle_diff < 0){
			printf("NEGATIVE ANGLE DIFF\n");
		}
		if(angle_diff > thresh){
			pot_pts[++end] = curr;
		} else if(curr_len > prev_len){
			pot_pts[end] = curr;
		}
	}
	
	// Remove concavities from path.
	pot_pts[end + 1] = start;
	int hull_ct = 1;
	convex_hull[hull_ct - 1] = start;
	convex_hull[hull_ct] = pot_pts[0];
	int turn_thresh = 10;
	for(i = 1; i < end + 1; i++){
		Point next = pot_pts[i];
		Point curr = convex_hull[hull_ct];
		Point prev = convex_hull[hull_ct - 1];
		if(next.x != start.x || next.y != start.y){
			int turn = ((next.x - curr.x)*(prev.y - curr.y)) - ((next.y - curr.y)*(prev.x - curr.x));
			while(turn < turn_thresh){
				if(--hull_ct < 0){
					hull_ct = 0;
					break;
				}
				curr = convex_hull[hull_ct];
				prev = convex_hull[hull_ct - 1];
				turn = ((next.x - curr.x)*(prev.y - curr.y)) - ((next.y - curr.y)*(prev.x - curr.x));
			}
			convex_hull[++hull_ct] = next;
		}
	}
	return hull_ct + 1;
}

/************************************** Utility Functions (Prints, Initializations, etc.) *****************************/

/*
 * Initializes global data structures.
 */
void initDataStructures_Blk(void){
	int idx;
	for(idx = 0; idx < (vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH); idx++){
		// Convex hull
		convex_hull[idx].x = 0;
		convex_hull[idx].y = 0;
		convex_hull[idx].angle = 0;
		
		// Hand points
		hand[idx].x = 0;
		hand[idx].y = 0;
		hand[idx].angle = 0;
		
		// Potential convex hull points
		pot_pts[idx].x = 0;
		pot_pts[idx].y = 0;
		pot_pts[idx].angle = 0;
		
	}
	
	for(idx = 0; idx < (vHeight/INTERNAL_BLK_HEIGHT + 2) * (vWidth/INTERNAL_BLK_WIDTH + 2); idx++){
		binary_hand[idx] = 0;
	}
}

/*
 * Initializes graphics buttons.
 */
void initButtons(void){
	Component* comp = (Component*) malloc(sizeof(Component));
	// System toggle
	Point* bl = (Point*) malloc(sizeof(Point));
	bl->x = 0;	 
	bl->y = maxBlkRow - B_SIDE + 1;
	comp->bottom_left = *bl;
	comp->height = B_SIDE;
	comp->width = B_SIDE;
	sys_tog.comp = *comp;
	sys_tog.state = OFF;
}

/************************************************* Video Processing ***************************************************/

/*
 * Count number of fingers
 * 
 * @param hull_size Number of points in convex hull
 */
void getFingerNum(int hull_size)
{
	int i;
	int x_dist_sum = 0, y_dist = 0;
	for (i = 0; i < hull_size - 1; ++i){
		if (convex_hull[i].x - convex_hull[i+1].x > 0){
			++finger_ct;
			x_dist_sum += convex_hull[i].x - convex_hull[i+1].x;
		}
		if (convex_hull[i+1].y - convex_hull[0].y > y_dist){
			y_dist = convex_hull[i+1].y - convex_hull[0].y;
		}
	}
	if (x_dist_sum < x_thresh_fist && y_dist < y_thresh_fist) finger_ct = 0;
	else if (flag_resetGesture == ON) finger_ct = 6;
	else ++finger_ct;
	
	if (finger_ct == 4 && x_dist_sum > x_thresh_five) ++finger_ct;
}

/*
 * Finds the convex hull centroid.
 * 
 * @param hull_size Number of points in convex hull.
 */ 
void findCentroid(int hull_size){
	int x_sum = 0;
	int y_sum = 0;
	int i;
	for(i = 0; i < hull_size; i++){
		x_sum += convex_hull[i].x;
		y_sum += convex_hull[i].y;	
	}
	centroid.x = x_sum/hull_size;
	centroid.y = y_sum/hull_size;
	centroid.angle = 0;
}

/*
 * Processes the frame for hand detection.
 */
void processHand(void){
	int i, x, y, countWhite;
	int blkIdx, blkNum = 0;
	int *Event1;
	int blkRow, blkCol;

	// DON'T change the following setting except the source and destination address
	// Event[PaRAM_SRC] is the source data
	// Event[PaRAM_DST] is the destination data
	//
	// Setup a channel for EDMA transfer from External to Internal
	Event1			 = (int *)(0x01C04000 + 32 * 9);
	Event1[PaRAM_OPT] = 0x0010000C;
	Event1[PaRAM_SRC] = (int)buffer_in;				// Source address
	Event1[PaRAM_BCNT]= ((INTERNAL_BLK_HEIGHT) << 16) | (INTERNAL_BLK_WIDTH/2 * 4);
//	Event1[PaRAM_DST] = (int)internal_buffer_2D;  	// Destination address
	Event1[PaRAM_DST] = (int)internal_buffer_1D;
	Event1[PaRAM_BIDX]= ((INTERNAL_BLK_WIDTH/2 * 4) << 16) | (vWidth/2 * 4);
	Event1[PaRAM_RDL] = 0x0000FFFF;
	Event1[PaRAM_CIDX]= 0x00000000;
	Event1[PaRAM_CCNT]= 0x00000001;

	// Block-based processing
	blkNum = vHeight * vWidth / (INTERNAL_BLK_WIDTH * INTERNAL_BLK_HEIGHT);
	for(blkIdx = 0; blkIdx < blkNum; ++blkIdx){
		blkRow = blkIdx / (vWidth/INTERNAL_BLK_WIDTH);
		blkCol = blkIdx % (vWidth/INTERNAL_BLK_WIDTH);
		binary_hand[(blkRow+1) * (vWidth/INTERNAL_BLK_WIDTH+2) + (blkCol+1)] = 0;

		// DON'T change the following setting
		// DMA transfer of a block from external memory to internal memory
		Event1[PaRAM_SRC] = (int)(&buffer_in[
		blkRow * INTERNAL_BLK_HEIGHT * vWidth/2 +
		blkCol * INTERNAL_BLK_WIDTH/2]);
		
		for(i=0;i<500;i++)
			if(EDMA_IPR&0x400 == 0) break; // Waiting for EDMA channel 10 transfer complete		
		EDMA_IPR = 0x200;             // Clear CIP9
		EDMA_ESR = EDMA_ESR | 0x200;    // Start channel 9 EDMA transfer

		// Do your processing based on the blocks in internal memory
		// CAN change the size of blocks (check the DEFINE statement at the beginning of this file)
		//
		
		// thresholding
		countWhite = 0;
		int morph_thresh = 9;
		for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
			for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
				if (
				(0xFF000000 & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) >= 0x3C000000 &&
				(0xFF000000 & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) <= 0xFF000000 &&
				(0x00FF0000 & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) >= 0x00870000 &&
				(0x00FF0000 & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) <= 0x00BB0000 &&
				(0x0000FF00 & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) >= 0x00003C00 &&
				(0x0000FF00 & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) <= 0x0000FF00 &&
				(0x000000FF & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) >= 0x00000064 &&
				(0x000000FF & internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y]) <= 0x0000007D){
					++countWhite;
				}
				if (countWhite > morph_thresh) break;
			}
			if (countWhite > morph_thresh) {
				binary_hand[(blkRow+1) * (vWidth/INTERNAL_BLK_WIDTH+2) + (blkCol+1)] = 1;
				break;
			}
		}			
		
	}
	// Get all the hand pixels
	int end = -1;
	int numBlack;
	for (blkRow = 1; blkRow < vHeight/INTERNAL_BLK_HEIGHT + 1; ++blkRow){
		for (blkCol = 1; blkCol < vWidth/INTERNAL_BLK_WIDTH + 1; ++blkCol){
			
			// If block belongs to hand, check neighbors
			
			binary_boundary[(blkRow-1) * (vWidth/INTERNAL_BLK_WIDTH) + (blkCol-1)] = 0;
			if (binary_hand[blkRow * (vWidth/INTERNAL_BLK_WIDTH+2) + blkCol] == 1){
				numBlack = 0;
				
				// check south
				if (binary_hand[(blkRow+1) * (vWidth/INTERNAL_BLK_WIDTH+2) + blkCol] == 0) ++numBlack;
				
				// check north
				if (binary_hand[(blkRow-1) * (vWidth/INTERNAL_BLK_WIDTH+2) + blkCol] == 0) ++numBlack;
				
				// check east
				if (binary_hand[blkRow * (vWidth/INTERNAL_BLK_WIDTH+2) + blkCol + 1] == 0) ++numBlack;
				
				// check west
				if (binary_hand[blkRow * (vWidth/INTERNAL_BLK_WIDTH+2) + blkCol - 1] == 0) ++numBlack;
				
				// keep point if it is a border pixel
				if (numBlack > 0 && numBlack < 4){
					end++;
					binary_boundary[(blkRow-1) * (vWidth/INTERNAL_BLK_WIDTH) + (blkCol-1)] = 1;
					hand[end].x = blkCol - 1;
					hand[end].y = maxBlkRow - blkRow;
					hand[end].angle = 0;
				}
			}
		}
	}
	// Find start block
	start.x = 0;
	start.y = maxBlkRow - 1;
	start.angle = 0;
	for(i = 0; i < end + 1; i++){
		Point pt = hand[i];
		if(pt.y < start.y || (pt.y == start.y && pt.x > start.x)){
			start.x = pt.x;
			start.y = pt.y;
		}
	}
	// Add angles
	for(i = 0; i < end + 1; i++){
		float x_a = hand[i].x - start.x;
		float y_a = hand[i].y - start.y;
		if(x_a != 0 || y_a != 0){
			hand[i].angle = (-x_a)/sqrt((x_a * x_a) + (y_a * y_a)) + 1;
		}
	}
	
	quickSort(0, end + 1);
	
	// Get the convex hull
	hullSize = getConvexHull(end + 1);
	
	// if hullSize < 4, there's no valid gesture to be detected
	finger_ct = 0;
	if (hullSize >= 4){
		// get number of finger tips then update
		// recentGestures array
		getFingerNum(hullSize);
		switch(finger_ct){
			case 0:
				recentGestures[frame_ct] = 0;
				break;
			case 2:
				recentGestures[frame_ct] = TWO;
				break;
			case 3:
				recentGestures[frame_ct] = THREE;
				break;
			case 4:
				recentGestures[frame_ct] = FOUR;
				break;
			case 5:
				recentGestures[frame_ct] = FIVE;
				break;
			default:
				recentGestures[frame_ct] = SIX;
		}
		
		findCentroid(hullSize);
		recentCentroids[frame_ct] = centroid;
	}
	else {
		recentGestures[frame_ct] = 5;
		++inactiveFrames;
	}
	frame_ct++;
}

/*
 * Determine which gesture is presented
 * 
 * @param traceback Number of frames to look back for determining current gesture.
 */
int getGesture(int traceback)
{
	int i, maxFreq = 0;
	int mostFreqGesture = 5;
	// reset gesture_ct array
	for (i = 0; i < NUM_GESTURES; ++i){
		gesture_ct[i] = 0;
	}

	for (i = frame_ct - traceback; i < frame_ct; ++i){
		switch(recentGestures[i]){
			// gestures are are represented by 0~4
			case 0:
				++gesture_ct[0];
				break;
			case 1:
				++gesture_ct[1];
				break;
			case 2:
				++gesture_ct[2];
				break;
			case 3:
				++gesture_ct[3];
				break;
			case 4:
				++gesture_ct[4];
				break;
		}
	}
	for (i = 0; i < NUM_GESTURES; ++i){
		if (gesture_ct[i] > maxFreq){
			maxFreq = gesture_ct[i];
			mostFreqGesture = i;
		}
	}
	return mostFreqGesture;
}
/******************************************** Display Functions **************************************************/
/*
 * Clears display before painting.
 */
void clearDisplay(void){
	int idx;
	for(idx = 0; idx < (vHeight/INTERNAL_BLK_HEIGHT) * (vWidth/INTERNAL_BLK_WIDTH); idx++){
		display[idx] = 0;
	}
}

/*
 * Outputs the display.
 */
void outputDisplay(void){
	int i, x, y;
	int blkIdx, blkNum = 0;
	int *Event2;
	int blkRow, blkCol;
	// Setup a channel for EDMA transfer from Internal to External
	Event2 			 = (int *)(0x01C04000 + 32 * 10);
	Event2[PaRAM_OPT] = 0x0010000C;
//	Event2[PaRAM_SRC] = (int)internal_buffer_2D;	// Source address
	Event2[PaRAM_SRC] = (int)internal_buffer_1D;
	Event2[PaRAM_BCNT]= ((INTERNAL_BLK_HEIGHT) << 16) | (INTERNAL_BLK_WIDTH/2 * 4); 
	Event2[PaRAM_DST] = (int)buffer_out;  			// Destination address
	Event2[PaRAM_BIDX]= ((vWidth/2 * 4) << 16) | (INTERNAL_BLK_WIDTH/2 * 4);
	Event2[PaRAM_RDL] = 0x0000FFFF;
	Event2[PaRAM_CIDX]= 0x00000000;
	Event2[PaRAM_CCNT]= 0x00000001;
	
	blkNum = vHeight * vWidth / (INTERNAL_BLK_WIDTH * INTERNAL_BLK_HEIGHT);
	
	for(blkIdx = 0; blkIdx < blkNum; ++blkIdx){
		blkRow = blkIdx / (vWidth/INTERNAL_BLK_WIDTH);
		blkCol = blkIdx % (vWidth/INTERNAL_BLK_WIDTH);
		
		// Output white points
		if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == WHITE_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = WHITE;
				}
			}
		// Output black points
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == BLACK_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = BLACK;
				}
			}
		// Output red points
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == RED_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = RED;
				}
			}
		// Output blue points
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == BLUE_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = BLUE;
				}
			}
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == L_BRN_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = L_BRN;
				}
			}
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == GREEN_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = GREEN;
				}
			}
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == YELLOW_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = YELLOW;
				}
			}
		} else if(display[blkRow * (vWidth/INTERNAL_BLK_WIDTH) + blkCol] == GRAY_MARK){
			for (x = 0; x < INTERNAL_BLK_HEIGHT; ++x){
				for (y = 0; y < INTERNAL_BLK_WIDTH/2; ++y){
					internal_buffer_1D[x * INTERNAL_BLK_WIDTH/2 + y] = GRAY;
				}
			}
		}
			
		// DON'T change the following setting
		// DMA transfer of a block from internal memory to external memory
		Event2[PaRAM_DST] = (int)(&buffer_out[
		blkRow * INTERNAL_BLK_HEIGHT * vWidth/2 +
		blkCol * INTERNAL_BLK_WIDTH/2]);
		
		for(i=0;i<500;i++)
			if(EDMA_IPR&0x200 == 0) break; // Waiting for EDMA channel 9 transfer complete		

		EDMA_IPR = 0x400;              // Clear CIP10
		EDMA_ESR = EDMA_ESR | 0x400;    // Start channel 10 EDMA transfer
	}
	
}

/*
 * Paints the effect name labels.
 */
void paintEffectNames(void){
	int i;
	// Paint "D"
	for(i = 0; i < size_d; i++){
		int row = maxBlkRow - (d_start_y + d_y[i]);
		int col = lett_start_x + d_x[i]; 
		if(dist.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == DIST) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}
	
	// Paint "P"
	for(i = 0; i < size_p; i++){
		int row = maxBlkRow - (p_start_y + p_y[i]);
		int col = lett_start_x + p_x[i]; 
		if(phaser.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == PHASER) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}
	
	// Paint "F"
	for(i = 0; i < size_f; i++){
		int row = maxBlkRow - (f_start_y + f_y[i]);
		int col = lett_start_x + f_x[i]; 
		if(flanger.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == FLANGER) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}

	// Paint "E"
	for(i = 0; i < size_e; i++){
		int row = maxBlkRow - (e_start_y + e_y[i]);
		int col = lett_start_x + e_x[i]; 
		if(echo.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == ECHO) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}
}

/*
 * Paints system toggle
 */
void paintSystemToggle(void){
	int i;
	for(i = 0; i < B_SIDE; i++){
		int row = B_SIDE - i;
		int j;
		for(j = 0; j < B_SIDE; j++){
			int col = j;
			if(sys_tog.state == ON){
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK;
			} else{
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = RED_MARK;
			}
		}
	}
}

/*
 * Paints the convex hull and convex hull centroid.
 */
void paintConvexHull(void){
	int i;
	// Paint centroid
	int row = maxBlkRow - centroid.y;
	int col = centroid.x;
	display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = YELLOW_MARK;

	// Paint convex hull
	for(i = 0; i < hullSize; i++){
		Point blk = convex_hull[i];
		display[(maxBlkRow - blk.y - 1) * (vWidth/INTERNAL_BLK_WIDTH) + (blk.x)] = GRAY_MARK;
	}
}
 
/*
 * Paints the distortion effect.
 */
void paintDist(void){
	int i, j;
	
	// Paint FX Toggle
	for(i = 0; i < B_SIDE; i++){
		int row = maxBlkRow - 1 - i - B_SHIFT;
		for(j = 0; j < B_SIDE; j++){
			int col = j + B_SHIFT;
			if(dist.status == ON){
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK;
			} else{
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = RED_MARK;
			}
		}
	}
	
	// Paint fader banks
	for(i = 0; i < dist.numParams; i++){
		Component fader = dist.faders[i];
		for(j = 0; j < FADER_TRK_H; j++){
			int k;
			for(k = 0; k < FADER_TRK_W; k++){
				int row = maxBlkRow - faderBottom - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
			}
		}
	}
	
	// Paint faders
	for(i = 0; i < dist.numParams; i++){
		Component fader = dist.faders[i];
		for(j = 0; j < FADER_H; j++){
			int k;
			for(k = 0; k < FADER_W; k++){
				int row = maxBlkRow - fader.bottom_left.y - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = L_BRN_MARK;
			}
		}
	}
	
	// Paint Gain Label
	Component fader = dist.faders[DIST_GAIN];
	for(i = 0; i < size_g; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + g_y[i]) + 1;
		int col = fader.bottom_left.x + g_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}	 
	
	// Paint Saturation Label
	fader = dist.faders[DIST_SAT];
	for(i = 0; i < size_s; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + s_y[i]) + 1;
		int col = fader.bottom_left.x + s_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}

	// Paint Level Label
	fader = dist.faders[DIST_LVL];
	for(i = 0; i < size_l; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + l_y[i]) + 1;
		int col = fader.bottom_left.x + l_x[i] + (int)FADER_W/2 - 1; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}

	// Paint Mix Label
	fader = dist.faders[DIST_MIX];
	for(i = 0; i < size_m; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + m_y[i]) + 1;
		int col = fader.bottom_left.x + m_x[i]; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
}

/*
 * Paints the phaser effect.
 */
void paintPhaser(void){
	int i;
	
	// Paint FX Toggle
	for(i = 0; i < B_SIDE; i++){
		int row = maxBlkRow - 1 - i - B_SHIFT;
		int j;
		for(j = 0; j < B_SIDE; j++){
			int col = j + B_SHIFT;
			if(phaser.status == ON){
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK;
			} else{
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = RED_MARK;
			}
		}
	}
	
	// Paint fader banks
	for(i = 0; i < phaser.numParams; i++){
		Component fader = phaser.faders[i];
		int j;
		for(j = 0; j < FADER_TRK_H; j++){
			int k;
			for(k = 0; k < FADER_TRK_W; k++){
				int row = maxBlkRow - faderBottom - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
			}
		}
	}
	
	// Paint faders
	for(i = 0; i < phaser.numParams; i++){
		Component fader = phaser.faders[i];
		int j;
		for(j = 0; j < FADER_H; j++){
			int k;
			for(k = 0; k < FADER_W; k++){
				int row = maxBlkRow - fader.bottom_left.y - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = L_BRN_MARK;
			}
		}
	}
	
	// Paint Rate Label
	Component fader = phaser.faders[PHASER_RATE];
	for(i = 0; i < size_r; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + r_y[i]) + 1;
		int col = fader.bottom_left.x + r_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}	 
	
	// Paint Mix Label
	fader = phaser.faders[PHASER_MIX];
	for(i = 0; i < size_m; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + m_y[i]) + 1;
		int col = fader.bottom_left.x + m_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
}

/*
 * Paints the flanger effect.
 */
void paintFlanger(void){
	int i;
	
	// Paint FX Toggle
	for(i = 0; i < B_SIDE; i++){
		int row = maxBlkRow - 1 - i - B_SHIFT;
		int j;
		for(j = 0; j < B_SIDE; j++){
			int col = j + B_SHIFT;
			if(flanger.status == ON){
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK;
			} else{
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = RED_MARK;
			}
		}
	}
	
	// Paint fader banks
	for(i = 0; i < flanger.numParams; i++){
		Component fader = flanger.faders[i];
		int j;
		for(j = 0; j < FADER_TRK_H; j++){
			int k;
			for(k = 0; k < FADER_TRK_W; k++){
				int row = maxBlkRow - faderBottom - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
			}
		}
	}
	
	// Paint faders
	for(i = 0; i < flanger.numParams; i++){
		Component fader = flanger.faders[i];
		int j;
		for(j = 0; j < FADER_H; j++){
			int k;
			for(k = 0; k < FADER_W; k++){
				int row = maxBlkRow - fader.bottom_left.y - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = L_BRN_MARK;
			}
		}
	}
	
	// Paint Rate Label
	Component fader = flanger.faders[FLANGER_RATE];
	for(i = 0; i < size_r; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + r_y[i]) + 1;
		int col = fader.bottom_left.x + r_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
	// Paint Delay Label
	fader = flanger.faders[FLANGER_DELAY];
	for(i = 0; i < size_d_lc; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + d_lc_y[i]) + 1;
		int col = fader.bottom_left.x + d_lc_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}	 
	
	// Paint Mix Label
	fader = flanger.faders[FLANGER_MIX];
	for(i = 0; i < size_m; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + m_y[i]) + 1;
		int col = fader.bottom_left.x + m_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
}

/*
 * Paints the echo effect.
 */
void paintEcho(void){
	int i;
	
	// Paint FX Toggle
	for(i = 0; i < B_SIDE; i++){
		int row = maxBlkRow - 1 - i - B_SHIFT;
		int j;
		for(j = 0; j < B_SIDE; j++){
			int col = j + B_SHIFT;
			if(echo.status == ON){
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK;
			} else{
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = RED_MARK;
			}
		}
	}
	
	// Paint fader banks
	for(i = 0; i < echo.numParams; i++){
		Component fader = echo.faders[i];
		int j;
		for(j = 0; j < FADER_TRK_H; j++){
			int k;
			for(k = 0; k < FADER_TRK_W; k++){
				int row = maxBlkRow - faderBottom - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
			}
		}
	}
	
	// Paint faders
	for(i = 0; i < echo.numParams; i++){
		Component fader = echo.faders[i];
		int j;
		for(j = 0; j < FADER_H; j++){
			int k;
			for(k = 0; k < FADER_W; k++){
				int row = maxBlkRow - fader.bottom_left.y - j;
				int col = fader.bottom_left.x + k;
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = L_BRN_MARK;
			}
		}
	}
	
	// Paint Delay Label
	Component fader = echo.faders[ECHO_DELAY];
	for(i = 0; i < size_d_lc; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + d_lc_y[i]) + 1;
		int col = fader.bottom_left.x + d_lc_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
	
	// Paint Repeats Label
	fader = echo.faders[ECHO_REPEATS];
	for(i = 0; i < size_r; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + r_y[i]) + 1;
		int col = fader.bottom_left.x + r_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
	 
	
	// Paint Mix Label
	fader = echo.faders[ECHO_MIX];
	for(i = 0; i < size_m; i++){
		int row = maxBlkRow - (faderBottom - LETTER_HEIGHT + m_y[i]) + 1;
		int col = fader.bottom_left.x + m_x[i] + (int)FADER_W/2 - 2; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
}

/*
 * Paints the calibration screen.
 */
void paintCal(int gest){
 	int row, col;
 	int start_x = (int) maxBlkCol/2 - 6;
 	int start_y = (int) maxBlkRow/3;
 	int hor_spacing  = LETTER_WIDTH + 1;
 	
 	// Paint "C"
	int i;
	for(i = 0; i < size_c; i++){
		row = start_y - c_y[i];
		col = start_x + c_x[i]; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
	
	// Paint "A"
	start_x += hor_spacing;
	for(i = 0; i < size_a; i++){
		row = start_y - a_y[i];
		col = start_x + a_x[i];
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK; 
	}

	// Paint "L"
	start_x += hor_spacing;
	for(i = 0; i < size_L; i++){
		row = start_y - L_y[i];
		col = start_x + L_x[i]; 
		display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
	}
	
	start_x = (int) maxBlkCol/2 - 2;
	start_y += LETTER_HEIGHT + 1;
	
	// Print Gesture Number
	if(gest == FIVE){
		for(i = 0; i < size_s; i++){
			row = start_y - s_y[i];
			col = start_x + s_x[i]; 
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
	} else{
		for(i = 0; i < size_zero; i++){
			row = start_y - zero_y[i];
			col = start_x + zero_x[i]; 
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
	}		
	
	// Paint centroid
	row = maxBlkRow - centroid.y;
	col = centroid.x;
	display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = YELLOW_MARK;

	// Paint convex hull
	for(i = 0; i < hullSize; i++){
		Point blk = convex_hull[i];
		row = maxBlkRow - blk.y - 1;
		col = blk.x;
		display[(row) * (vWidth/INTERNAL_BLK_WIDTH) + (col)] = GRAY_MARK;
	}
}



/*
 * Paints effect names, system toggle, and convex hull.
 */
 void paint(void){
 	int i;	
 	
 	clearDisplay();
 	
 	// Paint "D"
	for(i = 0; i < size_d; i++){
		int row = maxBlkRow - (d_start_y + d_y[i]);
		int col = lett_start_x + d_x[i]; 
		if(dist.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == DIST) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}
	
	// Paint "P"
	for(i = 0; i < size_p; i++){
		int row = maxBlkRow - (p_start_y + p_y[i]);
		int col = lett_start_x + p_x[i]; 
		if(phaser.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == PHASER) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}
	
	// Paint "F"
	for(i = 0; i < size_f; i++){
		int row = maxBlkRow - (f_start_y + f_y[i]);
		int col = lett_start_x + f_x[i]; 
		if(flanger.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == FLANGER) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}

	// Paint "E"
	for(i = 0; i < size_e; i++){
		int row = maxBlkRow - (e_start_y + e_y[i]);
		int col = lett_start_x + e_x[i]; 
		if(echo.status == ON){
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK; 
		} else{
			display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = WHITE_MARK;
		}
		if(flag_tuneFx == ECHO) display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = BLUE_MARK;
	}
	
	// Paint System Toggle
	for(i = 0; i < B_SIDE; i++){
		int row = B_SIDE - i;
		int j;
		for(j = 0; j < B_SIDE; j++){
			int col = j;
			if(sys_tog.state == ON){
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = GREEN_MARK;
			} else{
				display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = RED_MARK;
			}
		}
	}
	
	// Paint Effects
	if(flag_tuneFx == DIST){
		paintDist();
	} 
	else if(flag_tuneFx == PHASER){
		paintPhaser();
	}
	else if(flag_tuneFx == FLANGER){
		paintFlanger();
	}
	else if(flag_tuneFx == ECHO){
		paintEcho();
	}
	
	// Paint centroid
	int row = maxBlkRow - centroid.y;
	int col = centroid.x;
	display[row * (vWidth/INTERNAL_BLK_WIDTH) + col] = YELLOW_MARK;

	// Paint convex hull
	for(i = 0; i < hullSize; i++){
		Point blk = convex_hull[i];
		display[(maxBlkRow - blk.y - 1) * (vWidth/INTERNAL_BLK_WIDTH) + (blk.x)] = GRAY_MARK;
	}
 }

/*
 * Updates effect faders.
 * 
 * @param fx Pointer to an effect.
 */
void updateFaders(Effect* fx){
	int i;
	for(i = 0; i < fx->numParams; i++){
		int isFader = centroid.x >= fx->faders[i].bottom_left.x && centroid.x <= fx->faders[i].bottom_left.x + fx->faders[i].width;
		if(isFader && curGesture == TWO){
			fx->faders[i].bottom_left.y = centroid.y;
			if(fx->faders[i].bottom_left.y >= faderBottom + FADER_TRK_H - 2){
				fx->faders[i].bottom_left.y = faderBottom + FADER_TRK_H - 2;
			} else if(fx->faders[i].bottom_left.y < faderBottom){
				fx->faders[i].bottom_left.y = faderBottom;
			}
			int fader_level = fx->faders[i].bottom_left.y - faderBottom;
			fx->param_vals[i] = fader_level * fx->param_ranges[i]/(FADER_TRK_H - 1);
		}
	}
				
}

/*
 * This method handles most of the program flow as a result of hand gesture control.
 * Switching between pages, selecting effects, toggling buttons, etc. happens here.
 */
void frameProcessing()
{
	int traceback, x_cur, y_move;
	if (!sys_tog.state){
		if (frame_ct % FRAMES2SWITCH == 0 && frame_ct != 0){
			traceback = FRAMES2SWITCH;
			// now getGesture function will first check recentGestures
			// to get the most frequent gesture, may not need gesture_ct anymore
			// recentGestures array should be initialized by 5 (meaningless gesture)
			curGesture = getGesture(traceback); 
			
			if (curGesture == FIVE){
				sys_tog.state = ON;
				flag_selectFx = ON;
				flag_resetGesture = ON;
				flag_tuneFx = OFF;
			}
			
			// reset gesture_ct array (done in getGesture function)
			
			// reset frame_ct will happen when frame_ct == 50
		}
	}
	else if (flag_selectFx){
		if (frame_ct % FRAMES2SWITCH == 0 && frame_ct != 0){
			traceback = FRAMES2SWITCH;
			curGesture = getGesture(traceback);
			
			if (curGesture == OFF){
				sys_tog.state = OFF;
				flag_selectFx = OFF;
				flag_tuneFx = OFF;
			}
			else if (curGesture == TWO){
				flag_tuneFx = DIST;
				flag_selectFx = OFF;
			}
			else if (curGesture == THREE){
				flag_tuneFx = PHASER;
				flag_selectFx = OFF;
			}
			else if (curGesture == FOUR){
				flag_tuneFx = FLANGER;
				flag_selectFx = OFF;
			}
			else if (curGesture == FIVE){
				flag_tuneFx = ECHO;
				flag_selectFx = OFF;
				flag_resetGesture = ON;
			}
		}
	}
	else if (flag_tuneFx == DIST){
		if (frame_ct % FRAMES2SWITCH == 0 && frame_ct != 0){
			traceback = FRAMES2SWITCH;
			curGesture = getGesture(traceback);
			
			if (curGesture == OFF){
				sys_tog.state = OFF;
				flag_selectFx = OFF;
				flag_tuneFx = OFF;
			}
			else if (curGesture == FIVE){
				flag_selectFx = ON;
				flag_resetGesture = ON;
				flag_tuneFx = OFF;
			}
			else if (curGesture == TWO){
				x_cur = centroid.x;
				y_move =
				centroid.y -
				recentCentroids[frame_ct - traceback + 1].y;
				updateFaders(&dist);
				// check which slider x_cur is in, then tune the corresponding parameter according to y
			}
			else if (curGesture == THREE){
				dist.status = ON;
			}
			else if (curGesture == FOUR){
				dist.status = OFF;
			}
		}
	}
	else if (flag_tuneFx == PHASER){
		if (frame_ct % FRAMES2SWITCH == 0 && frame_ct != 0){
			traceback = FRAMES2SWITCH;
			curGesture = getGesture(traceback);
			
			if (curGesture == OFF){
				sys_tog.state = OFF;
				flag_selectFx = OFF;
				flag_tuneFx = OFF;
			}
			else if (curGesture == FIVE){
				flag_selectFx = ON;
				flag_resetGesture = ON;
				flag_tuneFx = OFF;
			}
			else if (curGesture == TWO){
				x_cur = centroid.x;
				y_move =
				centroid.y -
				recentCentroids[frame_ct - traceback + 1].y;
				updateFaders(&phaser);
				// check which slider x_cur is in, then tune the corresponding parameter according to y
			}
			else if (curGesture == THREE){
				phaser.status = ON;
			}
			else if (curGesture == FOUR){
				phaser.status = OFF;
			}
		}
	}
	else if (flag_tuneFx == FLANGER){
		if (frame_ct % FRAMES2SWITCH == 0 && frame_ct != 0){
			traceback = FRAMES2SWITCH;
			curGesture = getGesture(traceback);
			
			if (curGesture == OFF){
				sys_tog.state = OFF;
				flag_selectFx = OFF;
				flag_tuneFx = OFF;
			}
			else if (curGesture == FIVE){
				flag_selectFx = ON;
				flag_resetGesture = ON;
				flag_tuneFx = OFF;
			}
			else if (curGesture == TWO){
				x_cur = centroid.x;
				y_move =
				centroid.y -
				recentCentroids[frame_ct - traceback + 1].y;
				updateFaders(&flanger);
				// check which slider x_cur is in, then tune the corresponding parameter according to y
			}
			else if (curGesture == THREE){
				flanger.status = ON;
			}
			else if (curGesture == FOUR){
				flanger.status = OFF;
			}
		}
	}
	else if (flag_tuneFx == ECHO){
		if (frame_ct % FRAMES2SWITCH == 0 && frame_ct != 0){
			traceback = FRAMES2SWITCH;
			curGesture = getGesture(traceback);
			
			if (curGesture == OFF){
				sys_tog.state = OFF;
				flag_selectFx = OFF;
				flag_tuneFx = OFF;
			}
			else if (curGesture == FIVE){
				flag_selectFx = ON;
				flag_resetGesture = ON;
				flag_tuneFx = OFF;
			}
			else if (curGesture == TWO){
				x_cur = centroid.x;
				y_move =
				centroid.y -
				recentCentroids[frame_ct - traceback + 1].y;
				updateFaders(&echo);
				// check which slider x_cur is in, then tune the corresponding parameter according to y
			}
			else if (curGesture == THREE){
				echo.status = ON;
			}
			else if (curGesture == FOUR){
				echo.status = OFF;
			}
		}
	}
	processHand();
	
	// reset frame_ct might happen here
	if (flag_resetGesture == ON && resetGesture_ct < FRAMES2SWITCH){
		++resetGesture_ct;
	}
	else if (flag_resetGesture == ON && resetGesture_ct >= FRAMES2SWITCH){
		flag_resetGesture = OFF;
		resetGesture_ct = 0;
	}
	
	if (frame_ct >= FRAMES2STOP && sys_tog.state == 1){
		frame_ct = 0;
		if (inactiveFrames >= FRAMES2STOP){
			sys_tog.state = OFF;
			flag_selectFx = OFF;
			flag_tuneFx = OFF;
		}
		inactiveFrames = 0;
	}
}

/*********************************************** Audio Effects *******************************************************/

///////////////////////////////////////////////// Distortion ////////////////////////////////////////////////////////////

/*
 * Applies distortion effect.
 * 
 * @param in_sample Sample to be distorted.
 */
void apply_dist(short* in_sample){
	//float dry_sample;
	float gain_sample;
	short dist_sample;
	short G = (short) dist.param_vals[DIST_GAIN];
	float sat = dist.param_vals[DIST_SAT];
	float level = dist.param_vals[DIST_LVL];
	float mix = dist.param_vals[DIST_MIX];
	gain_sample = G * (*in_sample);
	
	if(gain_sample >= sat){
		gain_sample = sat;
	} else if(gain_sample < -sat){
		gain_sample = -sat;
	}
	dist_sample = (short) (level * ((*in_sample) + (mix * gain_sample)));	
	
	*in_sample = dist_sample;
		
}

/*
 * Initializes distortion effect (based on default parameter values).
 */
void initDist(void){
	int i;
	dist.numParams = 4; // Number of distortion parameters
	//int fader_spacing = (int)(faderDispW - (dist.numParams * FADER_TRK_W))/(dist.numParams - 1);
	//short center = (int) maxBlkCol/2;
	short disp_w = dist.numParams * FADER_TRK_W + (dist.numParams - 1) * SPACING;
	dist.faders = (Component*) malloc(dist.numParams * sizeof(Component)); 
	dist.param_vals = (float*) malloc(dist.numParams * sizeof(float));
	dist.param_ranges = (float*) malloc(dist.numParams * sizeof(float));
	dist.param_vals[DIST_GAIN] = 5;
	dist.param_vals[DIST_SAT] = 32000;
	//dist.param_vals[DIST_TONE] = 5000;
	dist.param_vals[DIST_LVL] = 0.75;
	dist.param_vals[DIST_MIX] = 0.75;
	dist.param_ranges[DIST_GAIN] = 20;
	dist.param_ranges[DIST_SAT] = 32000;
	//dist.param_ranges[DIST_TONE] = 15000;
	dist.param_ranges[DIST_LVL] = 1;
	dist.param_ranges[DIST_MIX] = 1;

	dist.status = OFF;
	Point pt;
	for(i = 0; i < dist.numParams; i++){
		float range = dist.param_ranges[i];
		//pt.x = FADER_TRK_HOR_PAD + i * (FADER_TRK_W + SPACING);
		pt.x = ((int) maxBlkCol/2) - ((int) disp_w/2) + i * (FADER_TRK_W + SPACING);
		pt.y = faderBottom + (int)((FADER_TRK_H - 1) * (dist.param_vals[i]/range));
		dist.faders[i].bottom_left = pt;
		dist.faders[i].height = FADER_H;
		dist.faders[i].width = FADER_W;
	}
	
			
}
///////////////////////////////////////////////// Phaser ////////////////////////////////////////////////////////////////

/*
 * Applies phaser effect.
 * 
 * @param input_sample Sample to which phaser will be applied.
 */
void apply_phase(short* input_sample){
    // 32bit audio IO. 16 bits per channel, highword|lowword.
    float dry_sample; // Input sample
    float phase_sample; // Output of wet branch
    float wet_input; // Input to wet branch (in_sample + feedback)
	float rate = phaser.param_vals[PHASER_RATE];
    float mix = phaser.param_vals[PHASER_MIX];
    int lfo_interval = (int) (SAMPLE_RATE/(rate*LFO_SIZE));
	dry_sample = (float) *input_sample;
	wet_input = dry_sample + (G_fb*ap4_prev); // Calculate input to first all-pass stage
   	   
	float ap1_out = p1 * (wet_input + ap1_prev) - in_prev; // Stage 1
	float ap2_out = p2 * (ap1_out + ap2_prev) - ap1_prev; // Stage 2
	float ap3_out = p3 * (ap2_out + ap3_prev) - ap2_prev; // Stage 3
	float ap4_out = p4 * (ap3_out + ap4_prev) - ap3_prev; // Stage 4
    
    //Update Buffers
	in_prev = wet_input; // Input Buffer
    ap1_prev = ap1_out; //Stage 1
    ap2_prev = ap2_out; //Stage 2
    ap3_prev = ap3_out; //Stage 3
    ap4_prev = ap4_out; //Stage 4
    
    // Apply LFO and Update Filters
	if(loop_count % lfo_interval == 0){		
		p1 = 1 - ((w1 + 0.25 * w1 * lfo[phaser_lfo_count])/SAMPLE_RATE); // Stage 1		
		p2 = 1 - ((w2 + 0.25 * w2 * lfo[phaser_lfo_count])/SAMPLE_RATE); // Stage 2		
		p3 = 1 - ((w3 + 0.75 * w3 * lfo[phaser_lfo_count])/SAMPLE_RATE); // Stage 3		
		p4 = 1 - ((w4 + 0.75 * w4 * lfo[phaser_lfo_count])/SAMPLE_RATE); // Stage 4
		
		phaser_lfo_count++;
	}
	if(phaser_lfo_count > LFO_SIZE - 1) phaser_lfo_count = 0;
	phase_sample = ((1.0 - mix)*dry_sample) + (depth*mix*ap4_out); // Calculate Output						
    
    *input_sample = (short) phase_sample;
}

/*
 * Create the LFO lookup table. 
 * Base frequency is 0.1 Hz (MIN_LFO_RATE).
 * All other LFO rates are derived from this 
 * table by altering the sampling interval.
 */
void init_lfo(){
	int fs_sine = 10;
	int x;
	for(x = 0; x < LFO_SIZE; x++){
		lfo[x] = lfo_gain*sinf((2*M_PI*MIN_LFO_RATE*x/fs_sine));
	}
}

/*
 * Initializes the all pass filters (design via MATLAB).
 * Only the first 3 coefficients are needed due to the
 * all pass filter coefficient symmetry.
 */
void init_filters(){
    p1 = 1 - (w1/SAMPLE_RATE); // Stage 1
	p2 = 1 - (w2/SAMPLE_RATE); // Stage 2
	p3 = 1 - (w3/SAMPLE_RATE); // Stage 3
	p4 = 1 - (w4/SAMPLE_RATE); // Stage 4
}

/*
 * Initializes phaser effect (based on default parameter values).
 */
void initPhaser(void){
	int i;
	phaser.numParams = 2; // Number of distortion parameters
	int disp_w = phaser.numParams * FADER_TRK_W + (phaser.numParams - 1) * SPACING;
	phaser.faders = (Component*) malloc(phaser.numParams * sizeof(Component)); 
	phaser.param_vals = (float*) malloc(phaser.numParams * sizeof(float));
	phaser.param_ranges = (float*) malloc(phaser.numParams * sizeof(float));
	phaser.param_vals[PHASER_RATE] = 0.8;
	phaser.param_vals[PHASER_MIX] = 0.75;
	phaser.param_ranges[PHASER_RATE] = 4;
	phaser.param_ranges[PHASER_MIX] = 1;
	
	phaser.status = OFF;
	Point pt;
	for(i = 0; i < phaser.numParams; i++){
		float range = phaser.param_ranges[i];
		pt.x = ((int) maxBlkCol/2) - ((int) disp_w/2) + i * (FADER_TRK_W + SPACING);
		pt.y = faderBottom + (int)((FADER_TRK_H - 1) * (phaser.param_vals[i]/range));
		phaser.faders[i].bottom_left = pt;
		phaser.faders[i].height = FADER_H;
		phaser.faders[i].width = FADER_W;
	}
	
			
}
//////////////////////////////////////////////////// Echo ////////////////////////////////////////////////////////////////
/*
 * Applies echo effect.
 * 
 * @param input Sample to which echo will be applied.
 */
void apply_echo(short* input)
{
	short delayed, output;
	float mix = echo.param_vals[ECHO_MIX];
	float repeats = echo.param_vals[ECHO_REPEATS];
	int delay_time = (int) echo.param_vals[ECHO_DELAY];
	delayed = buffer_echo[playCnt];
	output = (*input) + (short)(mix * delayed);
	buffer_echo[playCnt] = (*input) + (short)(repeats * delayed);
	
	if (++playCnt >= MAX_BUF_SIZE) playCnt = MAX_BUF_SIZE - delay_time;
	
	*input = output;
}

/*
 * Initializes echo effect (based on default parameter values).
 */
void initEcho(void){
	int i;
	echo.numParams = 3; // Number of distortion parameters
	int disp_w = echo.numParams * FADER_TRK_W + (echo.numParams - 1) * SPACING;
	echo.faders = (Component*) malloc(echo.numParams * sizeof(Component)); 
	echo.param_vals = (float*) malloc(echo.numParams * sizeof(float));
	echo.param_ranges = (float*) malloc(echo.numParams * sizeof(float));
	echo.param_vals[ECHO_DELAY] = 12000;
	echo.param_vals[ECHO_REPEATS] = 0.8;
	echo.param_vals[ECHO_MIX] = 0.75;
	echo.param_ranges[ECHO_DELAY] = MAX_BUF_SIZE;
	echo.param_ranges[ECHO_REPEATS] = 1;
	echo.param_ranges[ECHO_MIX] = 1;

	echo.status = OFF;
	Point pt;
	for(i = 0; i < echo.numParams; i++){
		float range = echo.param_ranges[i];
		pt.x = ((int) maxBlkCol/2) - ((int) disp_w/2) + i * (FADER_TRK_W + SPACING);
		pt.y = faderBottom + (int)((FADER_TRK_H - 1) * (echo.param_vals[i]/range));
		echo.faders[i].bottom_left = pt;
		echo.faders[i].height = FADER_H;
		echo.faders[i].width = FADER_W;
	}		
}
///////////////////////////////////////////////// Flanger ////////////////////////////////////////////////////////////////
/*
 * Applies flanger effect.
 * 
 * @param input Sample to which flanger will be applied.
 */
void apply_flange(short* input)
{
	float delayed; 
	short output;
	float mod_rate = flanger.param_vals[FLANGER_RATE];
	float mix = flanger.param_vals[FLANGER_MIX];
	float delay_samps = flanger.param_vals[FLANGER_DELAY];
	int bufsize_flanger = (int) delay_samps;
	int lfo_interval = (int)(SAMPLE_RATE/(LFO_SIZE*mod_rate));
	short cur_delay = (short)(bufsize_flanger * sine_mod);

	
	if (loop_count % bufsize_flanger - cur_delay < 0)
		delayed = (float)buffer_flanger[bufsize_flanger + loop_count % bufsize_flanger - cur_delay];
	else
		delayed = (float)buffer_flanger[loop_count % bufsize_flanger - cur_delay];
		
	output = (short)((1.0 - mix) * (*input) + mix * delayed);
	buffer_flanger[loop_count % bufsize_flanger] = *input;
	
	if(loop_count % lfo_interval == 0){ 
		sine_mod = 0.5*lfo[flanger_lfo_count++] + 0.5;
	}
	if(flanger_lfo_count > LFO_SIZE - 1) flanger_lfo_count = 0;
	
	*input = output;
}


/*
 * Initializes flanger effect (based on default parameter values).
 */
void initFlanger(void){
	int i;
	flanger.numParams = 3; // Number of distortion parameters
	int disp_w = flanger.numParams * FADER_TRK_W + (flanger.numParams - 1) * SPACING;
	flanger.faders = (Component*) malloc(flanger.numParams * sizeof(Component)); 
	flanger.param_vals = (float*) malloc(flanger.numParams * sizeof(float));
	flanger.param_ranges = (float*) malloc(flanger.numParams * sizeof(float));
	flanger.param_vals[FLANGER_RATE] = 1;
	flanger.param_vals[FLANGER_DELAY] = 240;
	flanger.param_vals[FLANGER_MIX] = 0.5;
	flanger.param_ranges[FLANGER_RATE] = 2;
	flanger.param_ranges[FLANGER_DELAY] = MAX_DELAY_SAMPLE;
	flanger.param_ranges[FLANGER_MIX] = 1;
	sine_mod = 0;
	flanger.status = OFF;
	Point pt;
	for(i = 0; i < flanger.numParams; i++){
		float range = flanger.param_ranges[i];
		pt.x = ((int) maxBlkCol/2) - ((int) disp_w/2) + i * (FADER_TRK_W + SPACING);
		pt.y = faderBottom + (int)((FADER_TRK_H - 1) * (flanger.param_vals[i]/range));
		flanger.faders[i].bottom_left = pt;
		flanger.faders[i].height = FADER_H;
		flanger.faders[i].width = FADER_W;
	}			
}
/*********************************************** Program Driver ******************************************************/

void main(void)
{
  	short sample_data = 0;
  	
  	EVMDM6437_init( );
  	
  	CSR=0x100;		/* disable all interrupts            */
  	IER=1;			/* disable all interrupts except NMI */
  	ICR=0xffff;		/* clear all pending interrupts      */   
  	
	ISTP = 0x10800400; // pointer to the address of interrupt service table (refer to link.cmd)
	
    MCBSP1_SPCR = 0x03130013; // SPCR
    
    INTC_EVTCLR0 = 0xFFFFFFFF; // Event Clear Register 0 (refer to spru871k)
    INTC_EVTCLR1 = 0xFFFFFFFF; 
    INTC_EVTCLR2 = 0xFFFFFFFF; 
    INTC_EVTCLR3 =  0xFFFFFFFF;   
   
    INTC_EVTMASK3 = 0xFFFFFFFF; // Event Mask Register 3
    INTC_EVTMASK2 = 0xFFFFFFFF;
//   INTC_EVTMASK1 = 0xFFFFFFF7;   // 0xFFFF | 1111111111110111 b // 51 McBSP1 Receive
    INTC_EVTMASK1 = 0xFFFFFFFB;     // 0xFFFF | 1111111've 111111011 b // 50 McBSP1 Transmit
    INTC_EVTMASK0 = 0xFFFFFFFF; 
   
    INTC_INTMUX3 = 0x00320000; // Interrupt Mux Register 3, 50 McBSP1 Transmit
//   INTC_INTMUX3 = 0x00330000; // Interrupt Mux Register 3, 51 McBSP1 Receive
   
    IER |= 0x00004002; // Enable Interrupt 14
   
    CSR=0x01 ;		/* enable all interrupts            */
   
    // Initialize the DIP Switches & LEDs if needed 
   
    video_loopback_test();
    
    // Initialize Data Structures 
	initDataStructures_Blk();
	
	// Initialize recentGesture array 
	for (playCnt = 0; playCnt < FRAMES2STOP; ++playCnt){
		recentGestures[playCnt] = 5; 
	}
    
    // Initialize graphics 
    maxBlkRow = vHeight/INTERNAL_BLK_HEIGHT;
	maxBlkCol = vWidth/INTERNAL_BLK_WIDTH;
    faderBottom = maxBlkRow - FADER_TRK_VERT_PAD - FADER_TRK_H;
	faderDispW = maxBlkCol - (2 * FADER_TRK_HOR_PAD);
	lett_start_x = maxBlkCol - LETTER_WIDTH - 2;
	d_start_y = maxBlkRow - LETTER_HEIGHT - 1;
	p_start_y = d_start_y - FX_NAME_SPACING;
	f_start_y = p_start_y - FX_NAME_SPACING;
	e_start_y = f_start_y - FX_NAME_SPACING;
	initButtons();
	clearDisplay();
	
    // Grab AIC33 handle 
    aic33handle = 0;
    
    // Effect initializations 
    init_lfo(); // LFO initializations
    init_filters(); // All-pass filter initializations
    initDist();
    initPhaser();
    initFlanger();
    initEcho();

    // Initialize echo buffer *
    for (playCnt = 0; playCnt < MAX_BUF_SIZE; ++playCnt){
    	buffer_echo[playCnt] = 0;
    }
    
    // Initialize flanger buffer 
    for (playCnt = 0; playCnt < MAX_DELAY_SAMPLE; ++playCnt){
    	buffer_flanger[playCnt] = 0;
    }
    playCnt = 0;

	// open audio codec
	if (aic33handle) EVMDM6437_AIC33_closeCodec( aic33handle );
	aic33handle = EVMDM6437_AIC33_openCodec( AIC33_MCBSP_ID, &aic33config_linein_loopback );
	
	// Calibrate fist
	printf("Calibrate gesture 0\n");
	int sum_fist_width = 0, sum_fist_height = 0, calib_frames = 0, i;
	while (calib_frames < FRAMES2CALIBRATE){
		processHand();
		if (hullSize >= 4){
			int currentFistWidth = 0, currentFistHeight = 0;
			for (i = 0; i < hullSize - 1; ++i){
				if (convex_hull[i].x - convex_hull[i+1].x > 0){
					currentFistWidth += convex_hull[i].x - convex_hull[i+1].x;
				}
				if (convex_hull[i+1].y - convex_hull[0].y > currentFistHeight){
					currentFistHeight = convex_hull[i+1].y - convex_hull[0].y;
				}
			}
			sum_fist_width += currentFistWidth;
			sum_fist_height += currentFistHeight;
			printf("Current fist width: %d, current fist height: %d\n", currentFistWidth, currentFistHeight);
			++calib_frames;
		}
		paintCal(0);
		outputDisplay();
	}
	x_thresh_fist = sum_fist_width / FRAMES2CALIBRATE + 2;
	y_thresh_fist = sum_fist_height / FRAMES2CALIBRATE + 2;
	printf("Fist width threshold: %d, fist height threshold: %d\n", x_thresh_fist, y_thresh_fist);
	
	// Calibrate gesture 5
	clearDisplay();
	printf("Calibrate gesture 5\n");
	int sum_palm_width = 0;
	calib_frames = 0;
	while (calib_frames < FRAMES2CALIBRATE){
		processHand();
		if (finger_ct == 5){
			int currentPalmWidth = 0;
			for (i = 0; i < hullSize - 1; ++i){
				if (convex_hull[i].x - convex_hull[i+1].x > 0){
					currentPalmWidth += convex_hull[i].x - convex_hull[i+1].x;
				}
			}
			sum_palm_width += currentPalmWidth;
			printf("Current palm width: %d\n", currentPalmWidth);
			++calib_frames;
		}
		paintCal(FIVE);
		outputDisplay();
	}
	x_thresh_five = sum_palm_width / FRAMES2CALIBRATE - 2;
	printf("Palm width threshold: %d\n", x_thresh_five);
	
	for (i = 0; i < FRAMES2STOP; ++i){
		recentGestures[i] = SIX;
	}
	
   // Main program functionality
    while(1)
    {  
	   EVMDM6437_AIC33_read16( aic33handle, &sample_data );
       EVMDM6437_AIC33_write16( aic33handle, sample_data );
       
       // gesture recognition
       frameProcessing();
       
       // paint and output display
       paint();
       outputDisplay();
   }
   
}

   
/*********************************
 * Interrupt Service Routine
 * *******************************/
interrupt void extint14_isr(void)
{
	short sample_data = 0;
	flag = 1;
	 	  
	EVMDM6437_AIC33_read16( aic33handle, &sample_data );
  
	// Apply enabled effects
	if(dist.status == ON){
	  	apply_dist(&sample_data);
	}
	if(phaser.status == ON){
	  	apply_phase(&sample_data);
	}
	if(flanger.status == ON){
	  	apply_flange(&sample_data);
	}
	if(echo.status == ON){
	  	apply_echo(&sample_data);
	}
	loop_count++;
	if(loop_count > SAMPLE_RATE - 1) loop_count = 0;
	EVMDM6437_AIC33_write16( aic33handle, sample_data );
  
	return;
}
