#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>


#include "ImageStruct.h"
#include "ImageStreamIO.h"


#include "shmimview.h"





// GTK widget pointers
extern app_widgets *widgets;


extern IMAGEDATAVIEW *imdataview;


// Images
extern IMAGE *imarray;










// Intensity scale

float scalefunction_log (
    float val,
    float coeff)
{
	float val1;
	
	val1 = (log10(val+coeff) - log10(coeff) ) / (log10(1.0+coeff) - log10(coeff));
	
    return (val1);
}



float scalefunction_linear (
    float val,
    __attribute__((unused)) float coeff)
{
    return (val);
}



float scalefunction_sqrt (
    float val,
    __attribute__((unused)) float coeff)
{
    return (sqrt(val));
}




float scalefunction_square (
    float val,
    __attribute__((unused)) float coeff)
{
    return (val*val);
}



float scalefunction_pow4 (
    float val,
    __attribute__((unused)) float coeff)
{
    float val2;

    val2 = val*val;

    return (val2*val2);
}



float scalefunction_pow8 (
    float val,
    __attribute__((unused)) float coeff)
{
    float val2, val4;

    val2 = val*val;
    val4 = val2*val2;

    return (val4*val4);
}



float scalefunction_power (
    float val,
    float coeff) {
    return ( pow(val,coeff) );
}







void on_scale_log4_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Log 4\n");
        imdataview[viewindex].scale_type = SCALE_LOG;
        imdataview[viewindex].scalefunc = *scalefunction_log;
        imdataview[viewindex].scale_coeff = 0.0001;
        imdataview[viewindex].update = 1;
    }
}



void on_scale_log3_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Log 3\n");
        imdataview[viewindex].scale_type = SCALE_LOG;
        imdataview[viewindex].scalefunc = *scalefunction_log;
        imdataview[viewindex].scale_coeff = 0.001;
        imdataview[viewindex].update = 1;
    }
}


void on_scale_log2_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Log 2\n");
        imdataview[viewindex].scale_type = SCALE_LOG;
        imdataview[viewindex].scalefunc = *scalefunction_log;
        imdataview[viewindex].scale_coeff = 0.01;
        imdataview[viewindex].update = 1;
    }
}

void on_scale_log1_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Log 1\n");
        imdataview[viewindex].scale_type = SCALE_LOG;
        imdataview[viewindex].scalefunc = *scalefunction_log;
        imdataview[viewindex].scale_coeff = 0.1;
        imdataview[viewindex].update = 1;
    }
}




void on_scale_linear_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
	gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	
	if (T) {
		printf("Scale Linear\n");
		imdataview[viewindex].scale_type = SCALE_LINEAR;
		imdataview[viewindex].scalefunc = *scalefunction_linear;
		imdataview[viewindex].update = 1;
	}
}

void on_scale_power01_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Power 0.1\n");
        imdataview[viewindex].scale_type = SCALE_POWER;
        imdataview[viewindex].scalefunc = *scalefunction_power;
        imdataview[viewindex].scale_coeff = 0.1;
        imdataview[viewindex].update = 1;
    }
}


void on_scale_power02_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Power 0.2\n");
        imdataview[viewindex].scale_type = SCALE_POWER;
        imdataview[viewindex].scalefunc = *scalefunction_power;
        imdataview[viewindex].scale_coeff = 0.2;
        imdataview[viewindex].update = 1;
    }
}


void on_scale_power05_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Power 0.5\n");
        imdataview[viewindex].scale_type = SCALE_SQRT;
        imdataview[viewindex].scalefunc = *scalefunction_sqrt;
        imdataview[viewindex].update = 1;
    }
}


void on_scale_power20_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Power 2\n");
        imdataview[viewindex].scale_type = SCALE_SQUARE;
        imdataview[viewindex].scalefunc = *scalefunction_square;
        imdataview[viewindex].update = 1;
    }
}


void on_scale_power40_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Power 4\n");
        imdataview[viewindex].scale_type = SCALE_POW4;
        imdataview[viewindex].scalefunc = *scalefunction_pow4;
        imdataview[viewindex].update = 1;
    }
}


void on_scale_power80_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("Scale Power 8\n");
        imdataview[viewindex].scale_type = SCALE_POW8;
        imdataview[viewindex].scalefunc = *scalefunction_pow8;
        imdataview[viewindex].update = 1;
    }
}









void on_scale_rangeminmax_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range minmax\n");
		imdataview[viewindex].scale_range_perc = 0.0;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range005_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 005\n");
		imdataview[viewindex].scale_range_perc = 0.005;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range01_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 01\n");
		imdataview[viewindex].scale_range_perc = 0.01;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range02_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 02\n");
		imdataview[viewindex].scale_range_perc = 0.02;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range03_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 03\n");
		imdataview[viewindex].scale_range_perc = 0.03;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range05_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 05\n");
		imdataview[viewindex].scale_range_perc = 0.05;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range10_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 10\n");
		imdataview[viewindex].scale_range_perc = 0.1;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_range20_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range 20\n");
		imdataview[viewindex].scale_range_perc = 0.2;
		imdataview[viewindex].update = 1;
    }
}

void on_scale_rangecustom_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("Scale range custom\n");
		imdataview[viewindex].scale_range_perc = 0.0;
		imdataview[viewindex].autominmax = 0;
		imdataview[viewindex].update = 1;
    } else {
		imdataview[viewindex].autominmax = 1;
	}
}







void on_colormap_grey_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("color grey\n");
		imdataview[viewindex].colormap = COLORMAP_GREY;
		imdataview[viewindex].update = 1;
    }
}


void on_colormap_heat_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("color heat\n");
		imdataview[viewindex].colormap = COLORMAP_HEAT;
		imdataview[viewindex].update = 1;
    }
}


void on_colormap_cool_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("color cool\n");
		imdataview[viewindex].colormap = COLORMAP_COOL;
		imdataview[viewindex].update = 1;
    }
}

void on_colormap_bry_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
	int viewindex = 0;
    
    if (T) {
        printf("color cool\n");
		imdataview[viewindex].colormap = COLORMAP_BRY;
		imdataview[viewindex].update = 1;
    }
}













