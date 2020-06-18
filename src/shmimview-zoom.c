#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>


#include "ImageStreamIO/ImageStruct.h"
#include "ImageStreamIO/ImageStreamIO.h"


#include "shmimview.h"
#include "shmimview-process.h"


#define BYTES_PER_PIXEL 3


// GTK widget pointers
extern app_widgets *widgets;


extern IMAGEDATAVIEW *imdataview;


// Images
extern IMAGE *imarray;

extern gboolean verbose;






// zoom




void on_zoombin4_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        if(verbose) {
            printf("zoom 1/4\n");
        }
        imdataview[viewindex].zoomFact = 0.25;
        resize_PixelBufferView(
            imdataview[viewindex].xsize/4,
            imdataview[viewindex].ysize/4);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;

        imdataview[viewindex].update = 1;
    }
}



void on_zoombin2_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        if(verbose) {
            printf("zoom 1/2\n");
        }
        imdataview[viewindex].zoomFact = 0.5;
        resize_PixelBufferView(
            imdataview[viewindex].xsize/2,
            imdataview[viewindex].ysize/2);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;

        imdataview[viewindex].update = 1;
    }
}

void on_zoom1_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        if(verbose) {
            printf("zoom 1\n");
        }
        imdataview[viewindex].zoomFact = 1;
        resize_PixelBufferView(
            imdataview[viewindex].xsize,
            imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;

        imdataview[viewindex].update = 1;
    }
}

void on_zoom2_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        if(verbose) {
            printf("zoom 2\n");
        }
        imdataview[viewindex].zoomFact = 2;
        resize_PixelBufferView(
            2*imdataview[viewindex].xsize,
            2*imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;

        imdataview[viewindex].update = 1;
    }
}

void on_zoom4_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        if(verbose) {
            printf("zoom 4\n");
        }
        imdataview[viewindex].zoomFact = 4;
        resize_PixelBufferView(
            4*imdataview[viewindex].xsize,
            4*imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;

        imdataview[viewindex].update = 1;
    }
}

void on_zoom8_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        if(verbose) {
            printf("zoom 8\n");
        }
        imdataview[viewindex].zoomFact = 8;
        resize_PixelBufferView(
            8*imdataview[viewindex].xsize,
            8*imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;

        imdataview[viewindex].update = 1;
    }
}








