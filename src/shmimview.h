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
	int colormapinit;
	
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

	int pressed_button1_status;
	int pressed_button3_status;
	float pressed_pos_X;
	float pressed_pos_Y;

} app_widgets;




// Image viewing data

typedef struct {
	
    
    int dispmap; // 1 if using a display map
    
    
	// pointer to GtkImage
	GtkImage *gtkimage;

	// gtkimage points to pbview, the pixel buffer
	GdkPixbuf *pbview;                  
	

	int        update;
	int        update_minmax;


	// ==============================================
	// =========== VIEW =============================
	// ==============================================
	
	// view size is at most equal to zoom x stream size
	// created/updated by resize_PixelBufferView()
	// pbview points to viewpixels
	guchar    *viewpixels;              // pixel view buffer, (re-)allocated by resize_PixelBufferVie()
	int        allocated_viewpixels;    // 1 if viewpixels is allocated
	// size of viewpixels buffer
	int        xviewsize;               // size of pixel view area
	int        yviewsize;               // size of pixel view area
	int        stride;	
	// part of view that can be displayed on current display (view coordinates)
	int        xviewmin;
	int        xviewmax;
	int        yviewmin;
	int        yviewmax;         



	// ==============================================
	// =========== STREAM ===========================
	// ==============================================
	// image index
	int        imindex;
	int        dispmap_imindex;
	int        active;
	// This is the raw image data
	char       imname[200];
	int        imtype;
	int        naxis;	
	// image size, pixel coordinates
	int        ysize;
	int        xsize;

	// part of image displayed (pixel coordinates)
	long       iimin;
	long       iimax;
	long       jjmin;
	long       jjmax;            

	// arrays used for internal computation
	int        computearrayinit;
	float *    computearray;   // compute array - same size as image. This is where pixel computations are done
	uint16_t * computearray16; // 16-bit mapped computearray




	// ==============================================
	// =========== MAPPING ==========================
	// ==============================================
	// VIEW->STREAM MAPPING
	long      *PixelIndexRaw_array;           // pixel mapping from view coord to stream coord, array size is viewXsize x viewYsize
	int       *PixelIndexViewBuff_array;      // pixel mapping from view coord to viewpixel buffer


	
	

	// ==============================================
	// =========== VIEW OPTIONS AND SETTINGS ========
	// ==============================================
	// view
	int        view_streaminfo;
	int        view_pixinfo;
	int        view_imstat;
	int        view_timing;
	// zoom
	float      zoomFact;

	// INTENSITY SCALE
	int        scale_type;
	float      scale_coeff;      
	float    (*scalefunc)(float, float);
	float      scale_range_perc;    // percentile for min/max
	float      vmin;
	float      vmax;                // min and max scale vales

	float      bscale_slope;
	float      bscale_center;
	float      bscale_center_ref;
	float      bscale_slope_ref;

	// COLORMAP
	int        colormap;
	int        colormapinit;
	unsigned char * COLORMAP_RVAL;
	unsigned char * COLORMAP_GVAL;
	unsigned char * COLORMAP_BVAL;

	int        showsaturated_min;
	int        showsaturated_max;



	// ==============================================
	// =========== GUI VARIABLES AND SETTINGS =======
	// ==============================================

	float      pointerXpos; // current X position (image pixel unit)
	float      pointerYpos; // current Y position (image pixel unit)
	float      pointerPixValue; // current pixel value


	// size of eventbox
	int        eventbox_xsize;
	int        eventbox_ysize;

	// mouse
	float      mouseXpos;
	float      mouseYpos;                             // current mouse coord (in image pix units)
	int        iisel;
	int        jjsel;                                       // selected pixel

	int        button1pressed;                                     // is button 1 pressed
	float      mouseXpos_pressed1;
	int        mouseYpos_pressed1;           // coordinates last time button 1 pressed (in image pix units)
	float      mouseXpos_pressed1_view;
	float      mouseYpos_pressed1_view; // coordinates last time button 1 pressed (in view pix units)

	float      x0view_ref;
	float      y0view_ref;
	float      x1view_ref;
	float      y1view_ref;


	int        button3pressed;                                     // is button 1 pressed
	float      mouseXpos_pressed3;
	float      mouseYpos_pressed3;           // coordinates last time button 1 pressed (in image pix units)
	float      mouseXpos_pressed3_view;
	float      mouseYpos_pressed3_view; // coordinates last time button 1 pressed (in view pix units)



} IMAGEDATAVIEW;



#define NB_IMDATAVIEW_MAX 10


#define NB_IMAGES_MAX 10



int precompute_cmap();


int open_shm_image(
    char *streamname,
    int index
);


int close_shm_image(
    int viewindex
);



gboolean  on_window_main_key_press_event(GtkWidget *widget, GdkEventKey *event, void *data);


#endif

