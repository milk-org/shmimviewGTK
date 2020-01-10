#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ImageStruct.h"
#include "ImageStreamIO.h"




#ifndef _SHMIMVIEW_H
#define _SHMIMVIEW_H



#define BYTES_PER_PIXEL 3



#define SCALE_LOG    0
#define SCALE_POWER  1
#define SCALE_LINEAR 2
#define SCALE_SQRT   3
#define SCALE_SQUARE 4
#define SCALE_POW4   5
#define SCALE_POW8   5


#define COLORMAP_GREY   0
#define COLORMAP_HEAT   1
#define COLORMAP_COOL   2
#define COLORMAP_BRY    3
#define COLORMAP_RGB    4




typedef struct {
	
	int NBlevel;
	
	unsigned char * COLORMAP_GREY_RVAL;
	unsigned char * COLORMAP_GREY_GVAL;
	unsigned char * COLORMAP_GREY_BVAL;
	
	unsigned char * COLORMAP_HEAT_RVAL;
	unsigned char * COLORMAP_HEAT_GVAL;
	unsigned char * COLORMAP_HEAT_BVAL;	
	
	unsigned char * COLORMAP_COOL_RVAL;
	unsigned char * COLORMAP_COOL_GVAL;
	unsigned char * COLORMAP_COOL_BVAL;
	
	unsigned char * COLORMAP_BRY_RVAL;
	unsigned char * COLORMAP_BRY_GVAL;
	unsigned char * COLORMAP_BRY_BVAL;
	
	unsigned char * COLORMAP_RGB_RVAL;
	unsigned char * COLORMAP_RGB_GVAL;
	unsigned char * COLORMAP_RGB_BVAL;
	
} COLORMAPDATA;




// GTK widget pointers

typedef struct {
	
	GtkWidget *filechooserdialog;
	
    GtkWidget *mainwindow;
    GtkWidget *imscrollwindow;
    GtkWidget *imviewport;
    GtkWidget *imareageventbox;

	GtkWidget *label_streaminfo;
	GtkWidget *label_pixinfo;
	GtkWidget *label_imstat;
	GtkWidget *label_timing;
	


    GtkWidget *w_img_main;              // Pointer to image widget


	// zoom
/*	GtkWidget *zoombin4;
	GtkWidget *zoombin2;
	GtkWidget *zoom1;
	GtkWidget *zoom2;
	GtkWidget *zoom4;
	GtkWidget *zoom8;
	
*/

    // Intensity scale
/*    GtkWidget *scale_log1;
    GtkWidget *scale_log2;
    GtkWidget *scale_log3;
    GtkWidget *scale_log4;
    GtkWidget *scale_power01;
    GtkWidget *scale_power02;
    GtkWidget *scale_power05;
    GtkWidget *scale_linear;
    GtkWidget *scale_power20;
    GtkWidget *scale_power40;
    GtkWidget *scale_power80;

    GtkWidget *scale_rangeminmax;
    GtkWidget *scale_range005;
    GtkWidget *scale_range01;
    GtkWidget *scale_range02;
    GtkWidget *scale_range03;
    GtkWidget *scale_range05;
    GtkWidget *scale_range10;
    GtkWidget *scale_range20;

    GtkWidget *colormap_grey;
    GtkWidget *colormap_heat;
    GtkWidget *colormap_cool;
*/
	int pressed_button1_status;
	int pressed_button3_status;
	float pressed_pos_X;
	float pressed_pos_Y;

} app_widgets;




// Image viewing data

typedef struct {
	// image data
    
    
    int dispmap; // 1 if using a display map
    
    
    int stride;

	
	// pinter to GtkImage
	GtkImage *gtkimage;

	int computearrayinit;
	float *    computearray;   // compute array - same size as image. This is where pixel computations are done
	uint16_t * computearray16; // 16-bit mapped computearray


	float pointerXpos; // current X position (image pixel unit)
	float pointerYpos; // current Y position (image pixel unit)
	float pointerPixValue; // current pixel value

	// image index
	int imindex;
	int dispmap_imindex;
	int active;

	// size of eventbox
	int eventbox_xsize;
	int eventbox_ysize;

	// size of viewing area (= pix buff size)
	int viewXsize, viewYsize;
	// part of image displayed (view coordinates)
	int xviewmin, xviewmax, yviewmin, yviewmax;           

	// pixel buffer
	GdkPixbuf *pbview;
	
	guchar *viewpixels;
	int allocated_viewpixels;
  	long *PixelRaw_array; // mapping
	int *PixelBuff_array; // buffer  


	char imname[200];

	
	int imtype;
	
	int naxis;	
	// image size, pixel coordinates
	int ysize, xsize;
	// part of image displayed (pixel coordinates)
	long  iimin, iimax, jjmin, jjmax;               

	int naxisdisp;
	// image size, pixel coordinates
	int ysizedisp, xsizedisp;
	// part of image displayed (pixel coordinates)
	long  iimindisp, iimaxdisp, jjmindisp, jjmaxdisp;       


	// view
	int view_streaminfo;
	int view_pixinfo;
	int view_imstat;
	int view_timing;


	// zoom
	float zoomFact;



	// INTENSITY SCALE
	
	int   scale_type;
	float scale_coeff;      
	float (*scalefunc)(float, float);

	float scale_range_perc;    // percentile for min/max
	float vmin, vmax;                           // min and max scale vales
	
	
	// COLORMAP
	
	int colormap;
	
	unsigned char * COLORMAP_RVAL;
	unsigned char * COLORMAP_GVAL;
	unsigned char * COLORMAP_BVAL;
	
	
	
	float bscale_slope;
	float bscale_center;
	float bscale_center_ref;
	float bscale_slope_ref;
	
	
	
	// mouse
	float mouseXpos, mouseYpos;                             // current mouse coord (in image pix units)
	int iisel, jjsel;                                       // selected pixel

	int button1pressed;                                     // is button 1 pressed
	float mouseXpos_pressed1, mouseYpos_pressed1;           // coordinates last time button 1 pressed (in image pix units)
	float mouseXpos_pressed1_view, mouseYpos_pressed1_view; // coordinates last time button 1 pressed (in view pix units)

	float x0view_ref, y0view_ref;
	float x1view_ref, y1view_ref;


	int button3pressed;                                     // is button 1 pressed
	float mouseXpos_pressed3, mouseYpos_pressed3;           // coordinates last time button 1 pressed (in image pix units)
	float mouseXpos_pressed3_view, mouseYpos_pressed3_view; // coordinates last time button 1 pressed (in view pix units)





	int showsaturated_min;
	int showsaturated_max;

	int update;
	int update_minmax;

} IMAGEDATAVIEW;


#define NB_IMDATAVIEW_MAX 10


#define NB_IMAGES_MAX 10




int open_shm_image(char *streamname, int index);
int close_shm_image(int viewindex);



gboolean  on_window_main_key_press_event(GtkWidget *widget, GdkEventKey *event, void *data);


#endif

