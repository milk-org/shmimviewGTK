#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>


#include "ImageStruct.h"
#include "ImageStreamIO.h"


#include "shmimview.h"
#include "shmimview-process.h"



#define BYTES_PER_PIXEL 3


// GTK widget pointers
extern app_widgets *widgets;


extern IMAGEDATAVIEW *imdataview;


// Images
extern IMAGE *imarray;

extern gboolean verbose;




int viewWindow_check_values()
{
    int viewindex = 0;


    // Change bounds as needed

    if(imdataview[viewindex].xviewmin < 0)
        imdataview[viewindex].xviewmin = 0;

    if(imdataview[viewindex].yviewmin < 0)
        imdataview[viewindex].yviewmin = 0;


    if( imdataview[viewindex].xviewmax > imdataview[viewindex].viewXsize )
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;

    if( imdataview[viewindex].yviewmax > imdataview[viewindex].viewYsize )
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;



    if( imdataview[viewindex].iimindisp < 0 )
        imdataview[viewindex].iimindisp = 0;

    if( imdataview[viewindex].jjmindisp < 0 )
        imdataview[viewindex].jjmindisp = 0;


    if( imdataview[viewindex].iimaxdisp > imdataview[viewindex].xsizedisp )
        imdataview[viewindex].iimaxdisp = imdataview[viewindex].xsizedisp;

    if( imdataview[viewindex].jjmaxdisp > imdataview[viewindex].ysizedisp )
        imdataview[viewindex].jjmaxdisp = imdataview[viewindex].ysizedisp;



    // Recompute zoom

    float zfactX, zfactY;

    zfactX = 1.0*(imdataview[viewindex].iimaxdisp - imdataview[viewindex].iimindisp) / (imdataview[viewindex].xviewmax - imdataview[viewindex].xviewmin);
    zfactY = 1.0*(imdataview[viewindex].jjmaxdisp - imdataview[viewindex].jjmindisp) / (imdataview[viewindex].yviewmax - imdataview[viewindex].yviewmin);


    if(zfactX>zfactY)
        imdataview[viewindex].zoomFact = zfactX;
    else
        imdataview[viewindex].zoomFact = zfactY;



    if(verbose) {
        printf(" -> %8.4f ===\n", imdataview[viewindex].zoomFact);
    }

    return 0;
}
















gboolean on_imgareaeventbox_scroll_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventScroll * event,
    __attribute__((unused)) gpointer        data)
{
    if(verbose) {
        g_print ("Event - scroll %d >   %f  %f\n", (int) event->direction, event->x, event->y);
    }
    int viewindex = 0;


    float zfact = 1.0;


    switch (event->direction)
    {

    case GDK_SCROLL_UP :
        if(verbose) {
            printf("SCROLL UP\n");
        }
        zfact = 1.1;
        break;

    case GDK_SCROLL_DOWN :
        if(verbose) {
            printf("SCROLL DOWN\n");
        }
        zfact = 1.0/1.1;
        break;

    case GDK_SCROLL_RIGHT :
        if(verbose) {
            printf("SCROLL RIGHT\n");
        }
        break;

    case GDK_SCROLL_LEFT :
        if(verbose) {
            printf("SCROLL LEFT\n");
        }
        break;

    case GDK_SCROLL_SMOOTH :
        if(verbose) {
            printf("SCROLL SMOOTH\n");
            printf("  delta_x = %f\n", (float) event->delta_x );
            printf("  delta_y = %f\n", (float) event->delta_y );
        }
        if(event->delta_y>0) {
            zfact = 1.0/1.1;
        }
        else {
            zfact = 1.1;
        }
        break;

    }



    // min/max limits to zoomFact
    float zoom_min = 0.1;
    float zoom_max = 8.0;

    float tmpz = imdataview[viewindex].zoomFact * zfact;
    if(tmpz < zoom_min)
        zfact = zoom_min/imdataview[viewindex].zoomFact;

    if(tmpz > zoom_max)
        zfact = zoom_max/imdataview[viewindex].zoomFact;

    imdataview[viewindex].zoomFact *= zfact;


    resize_PixelBufferView(
        (int) (imdataview[viewindex].xsizedisp*imdataview[viewindex].zoomFact),
        (int) (imdataview[viewindex].ysizedisp*imdataview[viewindex].zoomFact));
    imdataview[viewindex].xviewmin = 0;
    imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
    imdataview[viewindex].yviewmin = 0;
    imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;

    imdataview[viewindex].update = 1;

    return TRUE;
}








gboolean on_img_main_size_allocate(
    __attribute__((unused)) GtkWidget *widget,
    GtkAllocation *allocation,
    __attribute__((unused)) void *data)
{

    if(verbose) {
        printf("Event - mainwin  width = %d, height = %d\n", allocation->width, allocation->height);
    }
    int viewindex = 0;

    imdataview[viewindex].viewXsize = allocation->width;
    imdataview[viewindex].viewYsize = allocation->height;

    return TRUE;
}







/*
 * Update active area
 *
 */
gboolean on_imgareaeventbox_size_allocate(
    __attribute__((unused)) GtkWidget *widget,
    GtkAllocation *allocation,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;

    if(verbose) {
        printf("Event - eventbox width = %d, height = %d\n", allocation->width, allocation->height);
    }
    imdataview[viewindex].eventbox_xsize = allocation->width;
    imdataview[viewindex].eventbox_ysize = allocation->height;

    return TRUE;
}









gboolean on_imgareaeventbox_button_press_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventButton *event,
    __attribute__((unused)) void *data)
{
    if(verbose) {
        g_print ("Event - box clicked at coordinates %f,%f\n",
                 event->x, event->y);
    }

	int viewindex = 0;

	if ( (int)event->button == 1 ) {
		widgets->pressed_button1_status = 1;
	}
	if ( (int)event->button == 3 ) {
		widgets->pressed_button3_status = 1;
		
        imdataview[viewindex].bscale_center_ref = imdataview[viewindex].bscale_center;
        imdataview[viewindex].bscale_slope_ref = imdataview[viewindex].bscale_slope;		
	}

    widgets->pressed_pos_X = event->x;
    widgets->pressed_pos_Y = event->y;

    GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
    double hval = gtk_adjustment_get_value (hadj);
    if(verbose) {
        g_print ("scroll h value = %f\n", hval);
    }
    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
    double vval = gtk_adjustment_get_value (vadj);
    if(verbose) {
        g_print ("scroll v value = %f\n", vval);
    }

    return TRUE;
}





gboolean on_imgareaeventbox_motion_notify_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventButton *event,
    __attribute__((unused)) void *data)
{
    char string_pixinfo[200];
    float pixXpos = 0.0;
    float pixYpos = 0.0;
    int viewindex = 0;


    pixXpos = event->x;
    pixYpos = event->y;

    float offsetx =  0.5 * (imdataview[viewindex].eventbox_xsize - imdataview[viewindex].viewXsize);
    float offsety =  0.5 * (imdataview[viewindex].eventbox_ysize - imdataview[viewindex].viewYsize);

    if(offsetx < 0.0) {
        offsetx = 0.0;
    }
    if(offsety < 0.0) {
        offsety = 0.0;
    }

    pixXpos -= offsetx;
    pixYpos -= offsety;

    pixXpos /= imdataview[viewindex].zoomFact;
    pixYpos /= imdataview[viewindex].zoomFact;



    if(imdataview[viewindex].view_pixinfo == 1) {
        float pixval = 0.0;
        int ii, jj;

        ii = (int) (pixXpos+0.5);
        jj = (int) (pixYpos+0.5);
        int imindex = imdataview[viewindex].imindex;
		int imindexdisp = imdataview[viewindex].dispmap_imindex;

        if( (ii>-1) && (ii<imdataview[viewindex].xsizedisp) && (jj>-1) && (jj<imdataview[viewindex].ysizedisp)) {
            if(imdataview[viewindex].dispmap == 1) 
            {
				int zone = imarray[imindexdisp].array.SI32[jj*imdataview[viewindex].xsizedisp+ii];
				pixval = imarray[imindex].array.F[zone];
				sprintf(string_pixinfo, "%8.1f %8.1f\npixval[%d] = %.2g",
                    pixXpos,
                    pixYpos,
                    zone,
                    pixval);
			}
			else
			{
				pixval = imarray[imindex].array.F[jj*imdataview[viewindex].xsize+ii];
				sprintf(string_pixinfo, "%8.1f %8.1f\npixval = %.2g",
                    pixXpos,
                    pixYpos,
                    pixval);
			}
            
            

            imdataview[viewindex].pointerXpos = pixXpos;
            imdataview[viewindex].pointerYpos = pixYpos;
            imdataview[viewindex].pointerPixValue = pixval;
        }
        else
        {
            sprintf(string_pixinfo, "---");
        }

        gtk_label_set_text ( GTK_LABEL(widgets->label_pixinfo), string_pixinfo);
    }
    else {
        gtk_label_set_text ( GTK_LABEL(widgets->label_pixinfo), "");
    }

	

    if(widgets->pressed_button1_status == 1) {
        float dx = event->x - widgets->pressed_pos_X;
        float dy = event->y - widgets->pressed_pos_Y;		
        if(verbose) {
            printf("BT1 VECTOR : %f %f\n", dx, dy);
        }
        
        GtkAllocation alloc;
        gtk_widget_get_allocation(widgets->imviewport, &alloc);
        
        if(verbose) {
			printf("----- widget size is currently %d x %d\n", alloc.width, alloc.height);
		}
        
        
        GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
        double hval = gtk_adjustment_get_value (hadj);
        double hval1 = hval - dx;
        if(verbose) {
            g_print ("scroll h value = %f -> %f\n", hval, hval1);
        }
        gtk_adjustment_set_value (hadj, hval1);

        GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
        double vval = gtk_adjustment_get_value (vadj);
        double vval1 = vval - dy;
        if(verbose) {
            g_print ("scroll v value = %f -> %f\n", vval, vval1);
        }
        gtk_adjustment_set_value (vadj, vval1);

    }

    if(widgets->pressed_button3_status == 1) {
        float dx = event->x - widgets->pressed_pos_X;
        float dy = event->y - widgets->pressed_pos_Y;	
        if(verbose) {
            printf("BT3 VECTOR : %f %f\n", dx, dy);
        }
        
        imdataview[viewindex].bscale_center = imdataview[viewindex].bscale_center_ref + dx / imdataview[viewindex].viewXsize;
        imdataview[viewindex].bscale_slope = imdataview[viewindex].bscale_slope_ref + dy / imdataview[viewindex].viewYsize; 
        imdataview[viewindex].update = 1;
	}


    return TRUE;
}



gboolean on_imgareaeventbox_button_release_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventButton *event,
    __attribute__((unused)) void *data)
{
	 int viewindex = 0;
    /*    g_print ("Event button release at coordinates %f,%f\n",
                 event->x, event->y);
    	*/

    if ( (int)event->button == 1 ) {
		imdataview[viewindex].update = 1;
        imdataview[viewindex].update_minmax = 0;
        		
        widgets->pressed_button1_status = 0;
    }
    if ( (int)event->button == 3 ) {
        widgets->pressed_button3_status = 0;
    }


    float dx = event->x - widgets->pressed_pos_X;
    float dy = event->y - widgets->pressed_pos_Y;

    if(verbose) {
        printf("VECTOR : %f %f\n", dx, dy);
    }


    return TRUE;
}






gboolean on_window_main_key_press_event(
    __attribute__((unused)) GtkWidget *widget,
    GdkEventKey *event,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;

    if(verbose) {
        g_print ("WINDOWMAIN KEY PRESS callback\n"); //TEST
    }

    float dx = 10.0;
    float dy = 10.0;
    double vval;
    double vval1;
    double hval;
    double hval1;
    GtkAdjustment *vadj;
    GtkAdjustment *hadj;

    switch (event->keyval)
    {

    case GDK_KEY_u:
        if(verbose) {
            printf("Updating display\n");
        }
        imdataview[viewindex].update = 1;
        break;

    case GDK_KEY_m: // set scale min to current pixel value
        imdataview[viewindex].vmin = imdataview[viewindex].pointerPixValue;
        imdataview[viewindex].update = 1;
        break;

    case GDK_KEY_M: // set scale max to current pixel value
        imdataview[viewindex].vmax = imdataview[viewindex].pointerPixValue;
        imdataview[viewindex].update = 1;
        break;

    case GDK_KEY_b: // reset bscale
        imdataview[viewindex].bscale_slope = 1.0;
        imdataview[viewindex].bscale_center = 0.5;
        imdataview[viewindex].update = 1;
        break;

    case GDK_KEY_Left:
        hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
        hval = gtk_adjustment_get_value (hadj);
        hval1 = hval - dx;
        gtk_adjustment_set_value (hadj, hval1);
        break;

    case GDK_KEY_Up:
        vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
        vval = gtk_adjustment_get_value (vadj);
        vval1 = vval - dy;
        gtk_adjustment_set_value (vadj, vval1);
        break;

    case GDK_KEY_Right:
        hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
        hval = gtk_adjustment_get_value (hadj);
        hval1 = hval + dx;
        gtk_adjustment_set_value (hadj, hval1);
        break;

    case GDK_KEY_Down:
        vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
        vval = gtk_adjustment_get_value (vadj);
        vval1 = vval + dy;
        gtk_adjustment_set_value (vadj, vval1);
        break;


    default:
        return FALSE;
    }

    return GDK_EVENT_PROPAGATE;
}




gboolean on_menuitem_clear_activate(
    __attribute__((unused)) GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    if(verbose) {
        g_print("button streamclear clicked\n");
    }
    int viewindex = 0;
    close_shm_image(viewindex);

    return TRUE;
}











void on_view_streaminfo_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
    int viewindex = 0;

    if (T) {
        imdataview[viewindex].view_streaminfo = 1;
    }
    else {
        imdataview[viewindex].view_streaminfo = 0;
    }
    imdataview[viewindex].update = 1;

}


void on_view_pixinfo_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
    int viewindex = 0;

    if (T) {
        imdataview[viewindex].view_pixinfo = 1;
    }
    else {
        imdataview[viewindex].view_pixinfo = 0;
    }
    imdataview[viewindex].update = 1;
}

void on_view_imstat_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
    int viewindex = 0;

    if (T) {
        imdataview[viewindex].view_imstat = 1;
    }
    else {
        imdataview[viewindex].view_imstat = 0;
    }
    imdataview[viewindex].update = 1;
}

void on_view_timing_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );
    int viewindex = 0;

    if (T) {
        imdataview[viewindex].view_timing = 1;
    }
    else {
        imdataview[viewindex].view_timing = 0;
    }
    imdataview[viewindex].update = 1;
}














