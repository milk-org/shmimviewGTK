#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>


#include "ImageStruct.h"
#include "ImageStreamIO.h"


#include "shmimview.h"



#define BYTES_PER_PIXEL 3


// GTK widget pointers
extern app_widgets *widgets;


extern IMAGEDATAVIEW *imdataview;


// Images
extern IMAGE *imarray;






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



	if( imdataview[viewindex].iimin < 0 )
		imdataview[viewindex].iimin = 0;

	if( imdataview[viewindex].jjmin < 0 )
		imdataview[viewindex].jjmin = 0;

				
	if( imdataview[viewindex].iimax > imdataview[viewindex].xsize )
		imdataview[viewindex].iimax = imdataview[viewindex].xsize;

	if( imdataview[viewindex].jjmax > imdataview[viewindex].ysize )
		imdataview[viewindex].jjmax = imdataview[viewindex].ysize;



    // Recompute zoom

    float zfactX, zfactY;

	zfactX = 1.0*(imdataview[viewindex].iimax - imdataview[viewindex].iimin) / (imdataview[viewindex].xviewmax - imdataview[viewindex].xviewmin);
	zfactY = 1.0*(imdataview[viewindex].jjmax - imdataview[viewindex].jjmin) / (imdataview[viewindex].yviewmax - imdataview[viewindex].yviewmin);
    

    if(zfactX>zfactY)
        imdataview[viewindex].zoomFact = zfactX;
    else
        imdataview[viewindex].zoomFact = zfactY;



#ifdef VERBOSE
    printf(" -> %8.4f ===\n", imdataview[viewindex].zoomFact);
#endif

    return 0;
}
















gboolean on_imgareaeventbox_scroll_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventScroll * event,
    __attribute__((unused)) gpointer        data)
{
    g_print ("Event - scroll %d >   %f  %f\n", (int) event->direction, event->x, event->y);

    int viewindex = 0;


    float zfact = 1.0;


    switch (event->direction)
    {

    case GDK_SCROLL_UP :

        printf("SCROLL UP\n");

        zfact = 1.1;
        break;

    case GDK_SCROLL_DOWN :

        printf("SCROLL DOWN\n");

        zfact = 1.0/1.1;
        break;

    case GDK_SCROLL_RIGHT :

        printf("SCROLL RIGHT\n");

        break;

    case GDK_SCROLL_LEFT :

        printf("SCROLL LEFT\n");

        break;

    case GDK_SCROLL_SMOOTH :
        printf("SCROLL SMOOTH\n");
        printf("  delta_x = %f\n", (float) event->delta_x );
        printf("  delta_y = %f\n", (float) event->delta_y );
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
        (int) (imdataview[viewindex].xsize*imdataview[viewindex].zoomFact),
        (int) (imdataview[viewindex].ysize*imdataview[viewindex].zoomFact));
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

    printf("Event - mainwin  width = %d, height = %d\n", allocation->width, allocation->height);
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
	
    printf("Event - eventbox width = %d, height = %d\n", allocation->width, allocation->height);
    imdataview[viewindex].eventbox_xsize = allocation->width;
	imdataview[viewindex].eventbox_ysize = allocation->height;

    return TRUE;
}









gboolean on_imgareaeventbox_button_press_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventButton *event,
    __attribute__((unused)) void *data)
{
    g_print ("Event - box clicked at coordinates %f,%f\n",
             event->x, event->y);

	widgets->pressed_status = 1;
    
    widgets->pressed_pos_X = event->x;
    widgets->pressed_pos_Y = event->y;
    
    GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
    double hval = gtk_adjustment_get_value (hadj);
	g_print ("scroll h value = %f\n", hval);

    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
    double vval = gtk_adjustment_get_value (vadj);
	g_print ("scroll v value = %f\n", vval);
	
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
		
		if( (ii>-1) && (ii<imdataview[viewindex].xsize) && (jj>-1) && (jj<imdataview[viewindex].ysize)) {
			pixval = imarray[imindex].array.F[jj*imdataview[viewindex].xsize+ii];
		sprintf(string_pixinfo, "%8.1f %8.1f\npixval = %.2g",
            pixXpos,
            pixYpos,
            pixval);
            
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




    if(widgets->pressed_status == 1) {
        float dx = event->x - widgets->pressed_pos_X;
        float dy = event->y - widgets->pressed_pos_Y;

        printf("VECTOR : %f %f\n", dx, dy);

        GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
        double hval = gtk_adjustment_get_value (hadj);
        double hval1 = hval - dx;
        g_print ("scroll h value = %f -> %f\n", hval, hval1);
        gtk_adjustment_set_value (hadj, hval1);

        GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
        double vval = gtk_adjustment_get_value (vadj);
        double vval1 = vval - dy;
        g_print ("scroll v value = %f -> %f\n", vval, vval1);
        gtk_adjustment_set_value (vadj, vval1);

    }

    return TRUE;
}



gboolean on_imgareaeventbox_button_release_event(
    __attribute__((unused)) GtkWidget      *widget,
    GdkEventButton *event,
    __attribute__((unused)) void *data)
{
/*    g_print ("Event button release at coordinates %f,%f\n",
             event->x, event->y);
	*/
	widgets->pressed_status = 0;

	float dx = event->x - widgets->pressed_pos_X;
	float dy = event->y - widgets->pressed_pos_Y;

	printf("VECTOR : %f %f\n", dx, dy);

    GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
    double hval = gtk_adjustment_get_value (hadj);
    double hval1 = hval - dx;
	g_print ("scroll h value = %f -> %f\n", hval, hval1);
	gtk_adjustment_set_value (hadj, hval1);

    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
    double vval = gtk_adjustment_get_value (vadj);
    double vval1 = vval - dy;
	g_print ("scroll v value = %f -> %f\n", vval, vval1);
	gtk_adjustment_set_value (vadj, vval1);
	
    return TRUE;
}






gboolean on_window_main_key_press_event(
    __attribute__((unused)) GtkWidget *widget,
    GdkEventKey *event,
    __attribute__((unused)) void *data)
{
    int viewindex = 0;

    g_print ("WINDOWMAIN KEY PRESS callback\n"); //TEST

    switch (event->keyval)
    {
    case GDK_KEY_u:
        printf("Updating display\n");
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


    default:
        return FALSE;
    }

    return FALSE;
}




gboolean on_menuitem_clear_activate(
    __attribute__((unused)) GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	g_print("button streamclear clicked\n");

	int viewindex = 0;
	close_shm_image(viewindex);
		
	return TRUE;
}




// zoom




void on_zoombin4_toggled(
    GtkWidget      *widget,
    __attribute__((unused)) void *data)
{
	int viewindex = 0;
    gboolean T = gtk_check_menu_item_get_active ( GTK_CHECK_MENU_ITEM(widget) );

    if (T) {
        printf("zoom 1/4\n");
        imdataview[viewindex].zoomFact = 0.25;
        resize_PixelBufferView(
            imdataview[viewindex].xsize/4,
            imdataview[viewindex].ysize/4);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;
        
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
        printf("zoom 1/2\n");
        imdataview[viewindex].zoomFact = 0.5;
        resize_PixelBufferView(
            imdataview[viewindex].xsize/2,
            imdataview[viewindex].ysize/2);      
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;
                
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
        printf("zoom 1\n");
        imdataview[viewindex].zoomFact = 1;
        resize_PixelBufferView(
            imdataview[viewindex].xsize,
            imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;

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
        printf("zoom 2\n");
        imdataview[viewindex].zoomFact = 2;
        resize_PixelBufferView(
            2*imdataview[viewindex].xsize,
            2*imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;

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
        printf("zoom 4\n");
        imdataview[viewindex].zoomFact = 4;
        resize_PixelBufferView(
            4*imdataview[viewindex].xsize,
            4*imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;
            
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
        printf("zoom 8\n");
        imdataview[viewindex].zoomFact = 8;
        resize_PixelBufferView(
            8*imdataview[viewindex].xsize,
            8*imdataview[viewindex].ysize);
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;
        
        imdataview[viewindex].update = 1;
    }
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
















// =======================================







/*

static gboolean mouse_moved(GtkWidget *widget, GdkEvent *event, gpointer ptr) {

    //ImageData *id = (ImageData*) ptr;

    if (event->type==GDK_MOTION_NOTIFY) {
        GdkEventMotion* e=(GdkEventMotion*)event;



        if( (e->x_root < imdataview[0].mainwindow_x+ imdataview[0].viewXsize) && (e->y_root < imdataview[0].mainwindow_y + imdataview[0].viewYsize))  // is mouse in image ?
        {

            imdataview[0].mouseXpos = imdataview[0].x0view + (1.0*e->x) / imdataview[0].zoomFact;
            imdataview[0].mouseYpos = imdataview[0].y0view + (1.0*e->y) / imdataview[0].zoomFact;



#ifdef VERBOSE
            printf("%d Mouse move coordinates : (%8.2f x %8.2f)  (%8.2f x %8.2f) %8.2f %8.2f %f -> ( %8.2f, %8.2f )\n",
                   imdataview[0].showsaturated_max,
                   1.0*e->x_root,
                   1.0*e->y_root,
                   1.0*e->x,
                   1.0*e->y,
                   imdataview[0].x0view,
                   imdataview[0].y0view,
                   imdataview[0].zoomFact,
                   imdataview[0].mouseXpos,
                   imdataview[0].mouseYpos);
#endif

            char labeltext[100];
            int ii, jj;
            ii = (int) imdataview[0].mouseXpos;
            jj = (int) imdataview[0].mouseYpos;
            if((ii>-1)&&(jj>-1)&&(ii<imdataview[0].xsize)&&(jj<imdataview[0].ysize))
            {
                imdataview[0].iisel = ii;
                imdataview[0].jjsel = jj;

// TBD                sprintf(labeltext, "%5d", ii);
//                gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelxcoord), labeltext);
//                sprintf(labeltext, "%5d", jj);
//                gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelycoord), labeltext);

                switch ( imdataview[0].streamimage[0].md[0].datatype ) {

                case _DATATYPE_FLOAT :
                    sprintf(labeltext, "%18f", imdataview[0].streamimage[0].array.F[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_DOUBLE :
                    sprintf(labeltext, "%18f", imdataview[0].streamimage[0].array.D[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_UINT8 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.UI8[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_UINT16 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.UI16[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_UINT32 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.UI32[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_UINT64 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.UI64[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_INT8 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.SI8[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_INT16 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.SI16[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_INT32 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.SI32[jj*imdataview[0].xsize+ii]);
                    break;

                case _DATATYPE_INT64 :
                    sprintf(labeltext, "%ld", (long) imdataview[0].streamimage[0].array.SI64[jj*imdataview[0].xsize+ii]);
                    break;

                default :
                    sprintf(labeltext, "ERR DATATYPE");
                    break;
                }


  // TBD              gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelpixval), labeltext);
            }

            if(imdataview[0].button1pressed == 1)
            {
                float dragx, dragy;

                dragx = 1.0*e->x - imdataview[0].mouseXpos_pressed1_view;
                dragy = 1.0*e->y - imdataview[0].mouseYpos_pressed1_view;


                imdataview[0].x0view = imdataview[0].x0view_ref - dragx / imdataview[0].zoomFact;
                imdataview[0].y0view = imdataview[0].y0view_ref - dragy / imdataview[0].zoomFact;
                imdataview[0].x1view = imdataview[0].x1view_ref - dragx / imdataview[0].zoomFact;
                imdataview[0].y1view = imdataview[0].y1view_ref - dragy / imdataview[0].zoomFact;

                viewWindow_check_values();
            }

            if(imdataview[0].button3pressed == 1)
            {
                float dragx, dragy;

                dragx = 1.0*e->x - imdataview[0].mouseXpos_pressed3_view;
                dragy = 1.0*e->y - imdataview[0].mouseYpos_pressed3_view;

                imdataview[0].bscale_center = imdataview[0].bscale_center_ref + dragx / imdataview[0].viewXsize;
                imdataview[0].bscale_slope = imdataview[0].bscale_slope_ref + dragy / imdataview[0].viewYsize;

 // TBD               gtk_range_set_value (GTK_RANGE(imdataview[0].GTKscale_bscale_slope), imdataview[0].bscale_slope);
 // TBD               gtk_range_set_value (GTK_RANGE(imdataview[0].GTKscale_bscale_center), imdataview[0].bscale_center);
            }



            imdataview[0].update = 1;
        }

    }

}

*/













/*
static gboolean UpdateLevelscallback( GtkWidget * w, GdkEventButton * event, gpointer *ptr )
{
#ifdef VERBOSE
    printf("Update levels\n");
#endif

    char tmpstring[100];

    imdataview[0].vmin = atof(gtk_entry_get_text(GTK_ENTRY(imdataview[0].GTKentry_vmin)));
    imdataview[0].vmax = atof(gtk_entry_get_text(GTK_ENTRY(imdataview[0].GTKentry_vmax)));

    sprintf(tmpstring, "%.2f", imdataview[0].vmin);
    gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabel_scale_vmin), tmpstring);

    sprintf(tmpstring, "%.2f", imdataview[0].vmax);
    gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabel_scale_vmax), tmpstring);

    imdataview[0].update = 1;

}

*/



/*
static gboolean buttonpresscallback ( GtkWidget * w,
                                      GdkEventButton * event,
                                      gpointer *ptr )
{
//#ifdef VERBOSE
    printf ( " mousebuttonDOWN %d (x,y)=(%d,%d)state %x\n", (int)event->button, (int)event->x, (int)event->y, event->state ) ;
//#endif

    if ( (int)event->button == 1 )
    {
        imdataview[0].button1pressed = 1;

        imdataview[0].mouseXpos_pressed1_view = 1.0*event->x;
        imdataview[0].mouseYpos_pressed1_view = 1.0*event->y;

        imdataview[0].mouseXpos_pressed1 = imdataview[0].x0view + (1.0*event->x) / imdataview[0].zoomFact;
        imdataview[0].mouseYpos_pressed1 = imdataview[0].y0view + (1.0*event->y) / imdataview[0].zoomFact;

        imdataview[0].x0view_ref = imdataview[0].x0view;
        imdataview[0].y0view_ref = imdataview[0].y0view;
        imdataview[0].x1view_ref = imdataview[0].x1view;
        imdataview[0].y1view_ref = imdataview[0].y1view;
    }

    if ( (int)event->button == 3 )
    {
        imdataview[0].button3pressed = 1;

        imdataview[0].mouseXpos_pressed3_view = 1.0*event->x;
        imdataview[0].mouseYpos_pressed3_view = 1.0*event->y;

        imdataview[0].mouseXpos_pressed3 = imdataview[0].x0view + (1.0*event->x) / imdataview[0].zoomFact;
        imdataview[0].mouseYpos_pressed3 = imdataview[0].y0view + (1.0*event->y) / imdataview[0].zoomFact;

        imdataview[0].bscale_center_ref = imdataview[0].bscale_center;
        imdataview[0].bscale_slope_ref = imdataview[0].bscale_slope;
    }

    return FALSE;
}

*/




/*
static gboolean buttonreleasecallback ( GtkWidget * w,
                                        GdkEventButton * event,
                                        gpointer *ptr )
{
//#ifdef VERBOSE
    printf ( " mousebuttonUP %d (x,y)=(%d,%d)state %x\n", (int)event->button, (int)event->x, (int)event->y, event->state ) ;
//#endif

    if ( (int)event->button == 1 )
        imdataview[0].button1pressed = 0;

    if ( (int)event->button == 3 )
        imdataview[0].button3pressed = 0;

    return FALSE;
}
*/




/*

static gboolean buttonscrollcallback ( GtkWidget * w,
                                       GdkEventScroll * event,
                                       gpointer *ptr )
{
    float zfact = 1.0;

#ifdef VERBOSE
    printf ( " mousescroll  ");
#endif

    //ImageData *id = (ImageData*) ptr;


    if( (event->x_root < imdataview[0].mainwindow_x+ imdataview[0].viewXsize) && (event->y_root < imdataview[0].mainwindow_y + imdataview[0].viewYsize))  // is mouse in image ?
    {
        switch (event->direction)
        {

        case GDK_SCROLL_UP :
#ifdef VERBOSE
            printf("SCROLL UP");
#endif
            zfact = 1.1;
            break;

        case GDK_SCROLL_DOWN :
#ifdef VERBOSE
            printf("SCROLL DOWN");
#endif
            zfact = 1.0/1.1;
            break;

        case GDK_SCROLL_RIGHT :
#ifdef VERBOSE
            printf("SCROLL RIGHT");
#endif
            break;

        case GDK_SCROLL_LEFT :
#ifdef VERBOSE
            printf("SCROLL LEFT");
#endif
            break;

           //case GDK_SCROLL_SMOOTH :
           // printf("SCROLL SMOOTH");
           // printf("  delta_x = %f", (float) event->delta_x );
           // printf("  delta_y = %f", (float) event->delta_y );
           // break;

        }


        // mouse coordinates on image
        imdataview[0].mouseXpos = imdataview[0].x0view + (1.0*event->x) / imdataview[0].zoomFact;
        imdataview[0].mouseYpos = imdataview[0].y0view + (1.0*event->y) / imdataview[0].zoomFact;



        float tmpz = imdataview[0].zoomFact * zfact;
        if(tmpz < 1.0)
            zfact = 1.0/imdataview[0].zoomFact;

        if(tmpz > 32.0)
            zfact = 32.0/imdataview[0].zoomFact;

        imdataview[0].zoomFact *= zfact;



#ifdef VERBOSE
        printf(" %8.4f", imdataview[0].zoomFact);
#endif


        imdataview[0].x0view += (imdataview[0].mouseXpos - imdataview[0].x0view) * (zfact - 1.0);
        imdataview[0].x1view += (imdataview[0].mouseXpos - imdataview[0].x1view) * (zfact - 1.0);

        //imdataview[0].x1view = imdataview[0].x0view + (1.0/imdataview[0].zoomFact * imdataview[0].viewXsize);

        imdataview[0].y0view += (imdataview[0].mouseYpos - imdataview[0].y0view) * (zfact - 1.0);
        imdataview[0].y1view += (imdataview[0].mouseYpos - imdataview[0].y1view) * (zfact - 1.0);

        //imdataview[0].y1view = imdataview[0].y0view + (1.0/imdataview[0].zoomFact * imdataview[0].viewYsize);

        viewWindow_check_values();

        imdataview[0].update = 1;
    }
    return FALSE;

}



*/



/*

gboolean saturation_min_toggled_callback(GtkToggleButton *toggle_button,  gpointer ptr)
{
#ifdef VERBOSE
    printf("MIN toggle\n");
#endif

    if (gtk_toggle_button_get_active (toggle_button))
        imdataview[0].showsaturated_min = 1;
    else
        imdataview[0].showsaturated_min = 0;

#ifdef VERBOSE
    printf("MIN toggle = %d\n", imdataview[0].showsaturated_min);
#endif
    imdataview[0].update = 1;
}
*/



/*
gboolean saturation_max_toggled_callback(GtkToggleButton *toggle_button,  gpointer ptr)
{

#ifdef VERBOSE
    printf("MAX toggle\n");
#endif

    if (gtk_toggle_button_get_active (toggle_button))
        imdataview[0].showsaturated_max = 1;
    else
        imdataview[0].showsaturated_max = 0;

#ifdef VERBOSE
    printf("MAX toggle = %d\n", imdataview[0].showsaturated_max);
#endif
    imdataview[0].update = 1;
}



gboolean autominmax_toggled_callback(GtkToggleButton *toggle_button,  gpointer ptr)
{

#ifdef VERBOSE
    printf("AUTO MINMAX toggle\n");
#endif

    if (gtk_toggle_button_get_active (toggle_button))
        imdataview[0].autominmax = 1;
    else
        imdataview[0].autominmax = 0;

#ifdef VERBOSE
    printf("MAX toggle = %d\n", imdataview[0].autominmax);
#endif
    imdataview[0].update = 1;
}





static void bscale_center_moved (GtkRange *range, gpointer ptr)
{

    imdataview[0].bscale_center = gtk_range_get_value (range);
    update_pic_colorbar(ptr);
    imdataview[0].update = 1;
}

static void bscale_slope_moved (GtkRange *range, gpointer ptr)
{

    imdataview[0].bscale_slope = gtk_range_get_value (range);
    update_pic_colorbar(ptr);
    imdataview[0].update = 1;
}




gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    
    g_print ("KEY PRESS callback\n"); //TEST
    g_print ("size = %d %d\n", imdataview[0].xsize, imdataview[0].ysize);//TEST

    switch (event->keyval)
    {
    case GDK_KEY_p:
        printf("key pressed: %s\n", "p");
        break;
    case GDK_KEY_S:
        printf("key pressed: %s\n", "S");
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

*/


/*

void window_configure_callback(GtkWindow *window,
                               GdkEvent *event, gpointer data) {

    int x, y;
    GString *buf;


    imdataview[0].mainwindow_x = event->configure.x;
    imdataview[0].mainwindow_y = event->configure.y;

    imdataview[0].mainwindow_width = event->configure.width;
    imdataview[0].mainwindow_height = event->configure.height;


    buf = g_string_new(NULL);
    g_string_printf(buf, "%s [%d x %d]", imdataview[0].streamimage->md[0].name, imdataview[0].xsize, imdataview[0].ysize);


    gtk_window_set_title(window, buf->str);

    g_string_free(buf, TRUE);
}

*/
