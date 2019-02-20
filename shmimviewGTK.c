#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "ImageStruct.h"
#include "ImageStreamIO.h"


// gcc -O2 shmimviewGTK.c `pkg-config --cflags --libs gtk+-3.0` -lm -lImageStreamIO -o shmimviewGTK



#define BYTES_PER_PIXEL 3
 
//#define VERBOSE 1


typedef struct {
	// image data
    IMAGE *streamimage;
    int ysize, xsize;  // image size
    int stride;

	// main window
	int mainwindow_x;
	int mainwindow_y;
	int mainwindow_width;
	int mainwindow_height;

	// viewing data
	GtkImage *gtkimage;
	int viewXsize, viewYsize;                   // size of viewing area
	float x0view, x1view, y0view, y1view;       // part of image displayed
	float zoomFact;
	float vmin, vmax;                           // min and max scale vales
	float bscale_slope;
	float bscale_center;
	
	// colorbar
	GtkImage *gtkimage_colorbar;
	int colorbar_height;
	int stride_colorbar;
	
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

	float bscale_center_ref;
	float bscale_slope_ref;
	
	
	
	
	int showsaturated_min;
	int showsaturated_max;
	
	int update;
	
	// widgets
	GtkWidget *GTKlabelxcoord;
	GtkWidget *GTKlabelycoord;
	GtkWidget *GTKlabelpixval;
	
	GtkWidget *GTKlabelxview;
	GtkWidget *GTKlabelyview;
	GtkWidget *GTKlabelzoom;
	
	GtkWidget *GTKentry_vmin;  // user input
	GtkWidget *GTKentry_vmax;  // user input
	GtkWidget *GTKlabel_scale_vmin; // current
	GtkWidget *GTKlabel_scale_vmax; // current
	GtkWidget *GTKcheck_saturation_min;
	GtkWidget *GTKcheck_saturation_max;
	
	GtkWidget *GTKscale_bscale_slope;
	GtkWidget *GTKscale_bscale_center;
	
} ImageData;




 
 
void free_pixels(guchar *pixels, gpointer data) {
    free(pixels);
}









void PixVal_to_RGB(float pixval, guchar *rval, guchar *gval, guchar *bval, gpointer ptr)
{
    float pixval1;
	int pixsat = 0;
	
	ImageData *id = (ImageData*)ptr;
	
	
	
	
	
    pixval1 = id->bscale_center + (pixval-id->bscale_center) * id->bscale_slope;



    if(pixval1 < 0.0)
    {
        pixval1 = 0.0;
       if(id->showsaturated_min == 1)
        {
            pixsat = 1;
            *rval = 0;
            *gval = 0;
            *bval = 255;
        }
    }

    if(pixval1 > 1.0)
    {
        pixval1 = 1.0;
        if(id->showsaturated_max == 1)
        {
            pixsat = 1;
            *rval = 255;
            *gval = 0;
            *bval = 0;
        }
    }

    if(pixsat==0)
    {
        *rval = (unsigned char) (pixval1*255);
        *gval = (unsigned char) (pixval1*255);
        *bval = (unsigned char) (pixval1*255);
    }	
}








int update_pic_colorbar(gpointer ptr)
{
	ImageData *id = (ImageData*)ptr;
	GdkPixbuf *pb = gtk_image_get_pixbuf(id->gtkimage_colorbar);
	guchar rval, gval, bval;
	
	guchar *array = gdk_pixbuf_get_pixels(pb);
	
	int ii, jj;
	for(ii=0; ii < id->viewXsize; ii++)
		{
			for(jj=0; jj < id->colorbar_height; jj++)
			{
				float pixval;
				
				pixval = 1.0*ii/id->viewXsize;
				PixVal_to_RGB(pixval, &rval, &gval, &bval, ptr);
				
				int pixindex = jj * id->stride_colorbar + ii * BYTES_PER_PIXEL;
				
				#ifdef VERBOSE
				printf(" %3d / %3d   %3d / %3d  (%d) %d   %f ->  %d %d %d\n", ii, id->viewXsize, jj, id->colorbar_height, id->stride_colorbar, pixindex, pixval, rval, gval, bval);
				fflush(stdout);
				#endif
				
                array[pixindex] = rval;
				array[pixindex+1] = gval;
				array[pixindex+2] = bval;
			}
		}
	gtk_image_set_from_pixbuf(GTK_IMAGE(id->gtkimage_colorbar), pb);
	
}






int update_pic(gpointer ptr) {
    static long long imcnt0 = 0;

    static int comparrayinit = 0;
    static float *comparray; // compute array - same size as image. This is where pixel computations are done
    static guchar *Rarray;
    static guchar *Garray;
    static guchar *Barray;

    static long *PixelRaw_array; // mapping
    static int *PixelBuff_array; // buffer

    int ii, jj;

    static int iimin_save = -1;
    static int iimax_save = -1;
    static int jjmin_save = -1;
    static int jjmax_save = -1;


    ImageData *id = (ImageData*) ptr;
    GdkPixbuf *pb = gtk_image_get_pixbuf(id->gtkimage);

    guchar rval, gval, bval;

    if(imcnt0 != id->streamimage->md[0].cnt0)
        id->update = 1;

    if(id->update==1)
    {
#ifdef VERBOSE
        printf("updating image\n");
        fflush(stdout);
#endif

        imcnt0 = id->streamimage->md[0].cnt0;

        guchar *array = gdk_pixbuf_get_pixels(pb);

        //id->zoomFact = 1.0 * id->xsize / (id->x1view - id->x0view);


        // compute min and max window coord in raw image

        int iimin = (long) (0.5 + id->x0view);
        int iimax = (long) (0.5 + id->x0view + id->viewXsize / id->zoomFact) + 1;
        if (iimin < 0)
            iimin = 0;
        if (iimax > id->streamimage->md[0].size[0]-1 )
            iimax = id->streamimage->md[0].size[0];

        int jjmin = (long) (0.5 + id->y0view);
        int jjmax = (long) (0.5 + id->y0view + id->viewYsize / id->zoomFact) + 1;
        if (jjmin < 0)
            jjmin = 0;
        if (jjmax > id->streamimage->md[0].size[1]-1 )
            jjmax = id->streamimage->md[0].size[1];


        if(comparrayinit==0)
        {
            comparray = (float*) malloc(sizeof(float) * id->streamimage->md[0].size[0] * id->streamimage->md[0].size[1]);
            Rarray = (guchar*) malloc(sizeof(guchar) * id->streamimage->md[0].size[0] * id->streamimage->md[0].size[1]);
            Garray = (guchar*) malloc(sizeof(guchar) * id->streamimage->md[0].size[0] * id->streamimage->md[0].size[1]);
            Barray = (guchar*) malloc(sizeof(guchar) * id->streamimage->md[0].size[0] * id->streamimage->md[0].size[1]);
            PixelRaw_array = (long*) malloc(sizeof(long) * id->viewXsize * id->viewYsize);
            PixelBuff_array = (int*) malloc(sizeof(int) * id->viewXsize * id->viewYsize);
            comparrayinit = 1;
        }

        /*		printf("active image area :  %4d x %4d   at  %4d %4d    [%4d %4d]\n", iimax-iimin, jjmax-jjmin, iimin, jjmin, id->viewXsize, id->viewYsize);*/


        // has the view window changed ?
        if( (iimin != iimin_save) || (iimax != iimax_save) || (jjmin != jjmin_save) || (jjmax != jjmax_save))
        {
            int xview, yview;

            /*	printf("VIEW WINDOW CHANGED\n");
            	printf("   %4d  %4d\n", iimin, iimin_save);
            	printf("   %4d  %4d\n", iimax, iimax_save);
            	printf("   %4d  %4d\n", jjmin, jjmin_save);
            	printf("   %4d  %4d\n", jjmax, jjmax_save);*/
            // if window changed, recompute mapping between screen pixel and image pixel
            for (yview = 0; yview < id->viewYsize; yview++)
                for (xview = 0; xview < id->viewXsize; xview++)
                {
                    long pixindexRaw;
                    long pixindexView;
                    int pixindexBuff;

                    ii = (int) (0.5 + id->x0view + xview / id->zoomFact );
                    jj = (int) (0.5 + id->y0view + yview / id->zoomFact );

                    pixindexView = yview * id->viewXsize + xview;
                    pixindexRaw = jj*id->streamimage->md[0].size[0]+ii;
                    pixindexBuff = yview * id->stride + xview * BYTES_PER_PIXEL;
                    PixelRaw_array[pixindexView] = pixindexRaw;
                    PixelBuff_array[pixindexView] = pixindexBuff;
                }
        }

        iimin_save = iimin;
        iimax_save = iimax;
        jjmin_save = jjmin;
        jjmax_save = jjmax;



        // Fill in comparray
        // this is the image area over which things will be computed
        switch ( id->streamimage->md[0].atype )
        {
			case _DATATYPE_FLOAT:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = id->streamimage->array.F[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_DOUBLE:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = id->streamimage->array.D[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_UINT8:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.UI8[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_UINT16:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.UI16[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_UINT32:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.UI32[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_UINT64:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.UI64[jj*id->streamimage->md[0].size[0]+ii];
            break;


			case _DATATYPE_INT8:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.SI8[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_INT16:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.SI16[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_INT32:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.SI32[jj*id->streamimage->md[0].size[0]+ii];
            break;

			case _DATATYPE_INT64:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*id->streamimage->md[0].size[0]+ii] = (float) id->streamimage->array.SI64[jj*id->streamimage->md[0].size[0]+ii];
            break;


        }


        // compute R, G, B values
        for (ii=0; ii<id->streamimage->md[0].size[0]*id->streamimage->md[0].size[1]; ii++)
        {
            Rarray[ii] = 0;
            Garray[ii] = 0;
            Barray[ii] = 0;
        }
        for(ii=iimin; ii<iimax; ii++)
            for(jj=jjmin; jj<jjmax; jj++)
            {
                float pixval;
                int pixindex;

                pixindex = jj*id->streamimage->md[0].size[0]+ii;
                pixval = comparray[pixindex];
                pixval = (pixval - id->vmin) / (id->vmax - id->vmin);

                PixVal_to_RGB( pixval, &Rarray[pixindex], &Garray[pixindex], &Barray[pixindex], ptr);
            }


        /*   if( (ii==id->iisel) && (jj==id->jjsel) )
           {
               rval = 0;
               gval = 255;
               bval = 0;
           }
        */





        long pixindexView;
        for(pixindexView = 0; pixindexView<id->viewXsize*id->viewYsize; pixindexView++)
        {
            long pixindexRaw = PixelRaw_array[pixindexView];
            int pixindex = PixelBuff_array[pixindexView];

            array[pixindex] = Rarray[pixindexRaw];
            array[pixindex+1] = Garray[pixindexRaw];
            array[pixindex+2] = Barray[pixindexRaw];

        }






        gtk_image_set_from_pixbuf(GTK_IMAGE(id->gtkimage), pb);
    }
    id->update = 0;

    return TRUE;
}










int viewWindow_check_values(ImageData *id)
{
 //   ImageData *id = (ImageData*) ptr;
    char labeltext[100];



	// Translate image along X

    if(id->x0view < 0)
    {
        float offset = -id->x0view;

#ifdef VERBOSE
        printf("[[ X + offset %8.2f ]]", offset);
#endif

        id->x0view += offset;
        id->x1view += offset;
    }
    if(id->x1view > id->xsize)
    {
        float offset = id->x1view - id->xsize;

#ifdef VERBOSE
        printf("[[ X - offset %8.2f ]]", offset);
#endif

        id->x0view -= offset;
        id->x1view -= offset;
    }


	// Translate image along Y

    if(id->y0view < 0)
    {
        float offset = -id->y0view;

#ifdef VERBOSE
        printf(" [[ Y + offset %8.2f ]]", offset);
#endif

        id->y0view += offset;
        id->y1view += offset;

#ifdef VERBOSE
        printf("->[ %8.2f %8.2f ]", id->y0view, id->y1view);
#endif
    }
    if(id->y1view > id->ysize)
    {
        float offset = id->y1view - id->ysize;

#ifdef VERBOSE
        printf("[[ Y - offset %8.2f ]]", offset);
#endif

        id->y0view -= offset;
        id->y1view -= offset;

#ifdef VERBOSE
        printf("->[ %8.2f %8.2f ] ", id->y0view, id->y1view);
#endif
    }




	// Change bounds as needed
	
	if(id->x0view < 0)
		id->x0view = 0;
	if(id->y0view < 0)
		id->y0view = 0;
	
	
	
	


	// Recompute zoom

    float zfactX, zfactY;

    zfactX = 1.0*id->viewXsize / (id->x1view - id->x0view);
    zfactY = 1.0*id->viewYsize / (id->y1view - id->y0view);

    if(zfactX>zfactY)
        id->zoomFact = zfactX;
    else
        id->zoomFact = zfactY;



	#ifdef VERBOSE
    printf(" -> %8.4f ===\n", id->zoomFact);
    #endif


    sprintf(labeltext, "%8.2f", id->zoomFact);
    gtk_label_set_text(GTK_LABEL(id->GTKlabelzoom), labeltext);
    sprintf(labeltext, "%8.2f - %8.2f", id->x0view, id->x1view);
    gtk_label_set_text(GTK_LABEL(id->GTKlabelxview), labeltext);
    sprintf(labeltext, "%8.2f - %8.2f", id->y0view, id->y1view);
    gtk_label_set_text(GTK_LABEL(id->GTKlabelyview), labeltext);
    
    return 0;
}







static gboolean button_press_callback (GtkWidget      *event_box,
                       GdkEventButton *event,
                       gpointer        data)
{
  g_print ("Event box clicked at coordinates %f,%f\n",
           event->x, event->y);

  // Returning TRUE means we handled the event, so the signal
  // emission should be stopped (don’t call any further callbacks
  // that may be connected). Return FALSE to continue invoking callbacks.
  return TRUE;
}
 







static gboolean mouse_moved(GtkWidget *widget, GdkEvent *event, gpointer ptr) {

    ImageData *id = (ImageData*) ptr;

    if (event->type==GDK_MOTION_NOTIFY) {
        GdkEventMotion* e=(GdkEventMotion*)event;



        if( (e->x_root < id->mainwindow_x+ id->viewXsize) && (e->y_root < id->mainwindow_y + id->viewYsize))  // is mouse in image ?
        {

            id->mouseXpos = id->x0view + (1.0*e->x) / id->zoomFact;
            id->mouseYpos = id->y0view + (1.0*e->y) / id->zoomFact;



#ifdef VERBOSE
            printf("%d Mouse move coordinates : (%8.2f x %8.2f)  (%8.2f x %8.2f) %8.2f %8.2f %f -> ( %8.2f, %8.2f )\n", id->showsaturated_max, 1.0*e->x_root, 1.0*e->y_root, 1.0*e->x, 1.0*e->y, id->x0view, id->y0view, id->zoomFact, id->mouseXpos, id->mouseYpos);
#endif

            char labeltext[100];
            int ii, jj;
            ii = (int) id->mouseXpos;
            jj = (int) id->mouseYpos;
            if((ii>-1)&&(jj>-1)&&(ii<id->xsize)&&(jj<id->ysize))
            {
                id->iisel = ii;
                id->jjsel = jj;

                sprintf(labeltext, "%5d", ii);
                gtk_label_set_text(GTK_LABEL(id->GTKlabelxcoord), labeltext);
                sprintf(labeltext, "%5d", jj);
                gtk_label_set_text(GTK_LABEL(id->GTKlabelycoord), labeltext);
                
                switch ( id->streamimage[0].md[0].atype ) {
					
					case _DATATYPE_FLOAT :            
					sprintf(labeltext, "%18f", id->streamimage[0].array.F[jj*id->xsize+ii]);
					break;
					
					case _DATATYPE_DOUBLE :            
					sprintf(labeltext, "%18f", id->streamimage[0].array.D[jj*id->xsize+ii]);
					break;

					case _DATATYPE_UINT8 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.UI8[jj*id->xsize+ii]);
					break;

					case _DATATYPE_UINT16 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.UI16[jj*id->xsize+ii]);
					break;

					case _DATATYPE_UINT32 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.UI32[jj*id->xsize+ii]);
					break;

					case _DATATYPE_UINT64 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.UI64[jj*id->xsize+ii]);
					break;

					case _DATATYPE_INT8 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.SI8[jj*id->xsize+ii]);
					break;

					case _DATATYPE_INT16 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.SI16[jj*id->xsize+ii]);
					break;

					case _DATATYPE_INT32 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.SI32[jj*id->xsize+ii]);
					break;

					case _DATATYPE_INT64 :            
					sprintf(labeltext, "%ld", (long) id->streamimage[0].array.SI64[jj*id->xsize+ii]);
					break;
					
					default :
					sprintf(labeltext, "ERR DATATYPE");
					break;
				}
					
                
                gtk_label_set_text(GTK_LABEL(id->GTKlabelpixval), labeltext);
            }

            if(id->button1pressed == 1)
            {
                float dragx, dragy;

                dragx = 1.0*e->x - id->mouseXpos_pressed1_view;
                dragy = 1.0*e->y - id->mouseYpos_pressed1_view;


                id->x0view = id->x0view_ref - dragx / id->zoomFact;
                id->y0view = id->y0view_ref - dragy / id->zoomFact;
                id->x1view = id->x1view_ref - dragx / id->zoomFact;
                id->y1view = id->y1view_ref - dragy / id->zoomFact;

                viewWindow_check_values(id);
            }

            if(id->button3pressed == 1)
            {
                float dragx, dragy;

                dragx = 1.0*e->x - id->mouseXpos_pressed3_view;
                dragy = 1.0*e->y - id->mouseYpos_pressed3_view;
                
                id->bscale_center = id->bscale_center_ref + dragx / id->viewXsize;
				id->bscale_slope = id->bscale_slope_ref + dragy / id->viewYsize;
				
				gtk_range_set_value (GTK_RANGE(id->GTKscale_bscale_slope), id->bscale_slope);
				gtk_range_set_value (GTK_RANGE(id->GTKscale_bscale_center), id->bscale_center);
            }



            id->update = 1;
        }

    }

}













static gboolean UpdateLevelscallback( GtkWidget * w, GdkEventButton * event, gpointer *ptr )
{
	ImageData *id = (ImageData*) ptr;
	
	#ifdef VERBOSE
	printf("Update levels\n");
	#endif
	
	char tmpstring[100];

	id->vmin = atof(gtk_entry_get_text(GTK_ENTRY(id->GTKentry_vmin)));
	id->vmax = atof(gtk_entry_get_text(GTK_ENTRY(id->GTKentry_vmax)));
	
	sprintf(tmpstring, "%.2f", id->vmin);
	gtk_label_set_text(GTK_LABEL(id->GTKlabel_scale_vmin), tmpstring);

	sprintf(tmpstring, "%.2f", id->vmax);
	gtk_label_set_text(GTK_LABEL(id->GTKlabel_scale_vmax), tmpstring);
	
	id->update = 1;
	
}





static gboolean buttonpresscallback ( GtkWidget * w,
                                      GdkEventButton * event,
                                      gpointer *ptr )
{
	ImageData *id = (ImageData*) ptr;
	
	#ifdef VERBOSE
    printf ( " mousebuttonDOWN %d (x,y)=(%d,%d)state %x\n", (int)event->button, (int)event->x, (int)event->y, event->state ) ;
    #endif
             
    if ( (int)event->button == 1 )
    {
		id->button1pressed = 1;
		
		id->mouseXpos_pressed1_view = 1.0*event->x;
		id->mouseYpos_pressed1_view = 1.0*event->y;
		
		id->mouseXpos_pressed1 = id->x0view + (1.0*event->x) / id->zoomFact;
		id->mouseYpos_pressed1 = id->y0view + (1.0*event->y) / id->zoomFact;
		
		id->x0view_ref = id->x0view;
		id->y0view_ref = id->y0view;
		id->x1view_ref = id->x1view;
		id->y1view_ref = id->y1view;
	}
	
	if ( (int)event->button == 3 )
    {
		id->button3pressed = 1;
		
		id->mouseXpos_pressed3_view = 1.0*event->x;
		id->mouseYpos_pressed3_view = 1.0*event->y;
		
		id->mouseXpos_pressed3 = id->x0view + (1.0*event->x) / id->zoomFact;
		id->mouseYpos_pressed3 = id->y0view + (1.0*event->y) / id->zoomFact;
		
		id->bscale_center_ref = id->bscale_center;
		id->bscale_slope_ref = id->bscale_slope;
	}
             
    return FALSE;
}




static gboolean buttonreleasecallback ( GtkWidget * w,
                                        GdkEventButton * event,
                                        gpointer *ptr )
{
	ImageData *id = (ImageData*) ptr;
	
	#ifdef VERBOSE
    printf ( " mousebuttonUP %d (x,y)=(%d,%d)state %x\n", (int)event->button, (int)event->x, (int)event->y, event->state ) ;
    #endif
    
    if ( (int)event->button == 1 )
		id->button1pressed = 0;

    if ( (int)event->button == 3 )
		id->button3pressed = 0;
    
    return FALSE;
}







static gboolean buttonscrollcallback ( GtkWidget * w,
                                       GdkEventScroll * event,
                                       gpointer *ptr )
{
    float zfact = 1.0;

#ifdef VERBOSE
    printf ( " mousescroll  ");
#endif

    ImageData *id = (ImageData*) ptr;


    if( (event->x_root < id->mainwindow_x+ id->viewXsize) && (event->y_root < id->mainwindow_y + id->viewYsize))  // is mouse in image ?
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

            /*case GDK_SCROLL_SMOOTH :
            printf("SCROLL SMOOTH");
            printf("  delta_x = %f", (float) event->delta_x );
            printf("  delta_y = %f", (float) event->delta_y );
            break;*/

        }


		// mouse coordinates on image
        id->mouseXpos = id->x0view + (1.0*event->x) / id->zoomFact;
        id->mouseYpos = id->y0view + (1.0*event->y) / id->zoomFact;



        float tmpz = id->zoomFact * zfact;
        if(tmpz < 1.0)
            zfact = 1.0/id->zoomFact;

        if(tmpz > 32.0)
            zfact = 32.0/id->zoomFact;

        id->zoomFact *= zfact;



#ifdef VERBOSE
        printf(" %8.4f", id->zoomFact);
#endif


        id->x0view += (id->mouseXpos - id->x0view) * (zfact - 1.0);
        id->x1view += (id->mouseXpos - id->x1view) * (zfact - 1.0);
        
        //id->x1view = id->x0view + (1.0/id->zoomFact * id->viewXsize);

        id->y0view += (id->mouseYpos - id->y0view) * (zfact - 1.0);
        id->y1view += (id->mouseYpos - id->y1view) * (zfact - 1.0);
        
        //id->y1view = id->y0view + (1.0/id->zoomFact * id->viewYsize);

        viewWindow_check_values(id);

        id->update = 1;
    }
    return FALSE;

}






gboolean saturation_min_toggled_callback(GtkToggleButton *toggle_button,  gpointer ptr)
{
    ImageData *id = (ImageData*) ptr;

#ifdef VERBOSE
    printf("MIN toggle\n");
#endif

    if (gtk_toggle_button_get_active (toggle_button))
        id->showsaturated_min = 1;
    else
        id->showsaturated_min = 0;

#ifdef VERBOSE
    printf("MIN toggle = %d\n", id->showsaturated_min);
#endif
    id->update = 1;
}

gboolean saturation_max_toggled_callback(GtkToggleButton *toggle_button,  gpointer ptr)
{
    ImageData *id = (ImageData*) ptr;

#ifdef VERBOSE
    printf("MAX toggle\n");
#endif

    if (gtk_toggle_button_get_active (toggle_button))
        id->showsaturated_max = 1;
    else
        id->showsaturated_max = 0;

#ifdef VERBOSE
    printf("MAX toggle = %d\n", id->showsaturated_max);
#endif
    id->update = 1;
}


static void bscale_center_moved (GtkRange *range, gpointer ptr)
{
	ImageData *id = (ImageData*) ptr;
	
	id->bscale_center = gtk_range_get_value (range);
	update_pic_colorbar(ptr);
	id->update = 1;
}

static void bscale_slope_moved (GtkRange *range, gpointer ptr)
{
	ImageData *id = (ImageData*) ptr;
	
	id->bscale_slope = gtk_range_get_value (range);
	update_pic_colorbar(ptr);
	id->update = 1;
}




gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{

    switch (event->keyval)
    {
    case GDK_KEY_p:
       // printf("key pressed: %s\n", "p");
        break;
    case GDK_KEY_S:
      //  printf("key pressed: %s\n", "S");
        break;

    default:
        return FALSE;
    }

    return FALSE;
}





void window_configure_callback(GtkWindow *window, 
      GdkEvent *event, gpointer data) {
          
   int x, y;
   GString *buf;


	ImageData *id = (ImageData*) data;
   
   id->mainwindow_x = event->configure.x;
   id->mainwindow_y = event->configure.y;
   
   id->mainwindow_width = event->configure.width;
   id->mainwindow_height = event->configure.height;
   
   
   buf = g_string_new(NULL);   
   g_string_printf(buf, "%d %d    %d, %d", id->mainwindow_x, id->mainwindow_y, id->mainwindow_width, id->mainwindow_height);
//   g_string_printf(buf, "%d, %d", id->mainwindow_width, id->mainwindow_height);
   
   gtk_window_set_title(window, buf->str);
   
   g_string_free(buf, TRUE);
}












int main(int argc, char **argv) {

	// Read image from shared memory
	IMAGE *imarray;    // pointer to array of images
	char filename[64];
	char tmpstring[100];
	
	float ZOOMVIEW = 2.0;
	
	int NBIMAGES = 1;
	imarray = (IMAGE*) malloc(sizeof(IMAGE)*NBIMAGES);
	

	
	ImageStreamIO_filename(filename, 64, argv[1]);
	printf("Filename = %s\n", filename);
	if(ImageStreamIO_openIm(&imarray[0], argv[1]) == -1)
	{
		printf("ERROR: cannot load stream \"%s\"\n", argv[1]);
		return -1;
	}

	int imXsize = imarray[0].md[0].size[0];
	int imYsize = imarray[0].md[0].size[1];

	printf("image size = %d x %d\n", imXsize, imYsize);

	if(argc>2)
	{
		ZOOMVIEW = atof(argv[2]);
	}
	

    // Initialize GTK
    gtk_init(&argc, &argv);
    



    ImageData id;
    id.ysize = imYsize;
    id.xsize = imXsize;
    id.streamimage = &imarray[0];

	id.x0view = 0;
	id.x1view = imXsize;
	id.y0view = 0;
	id.y1view = imYsize;	
	id.viewXsize = (int) (1.0*imXsize*ZOOMVIEW);
	id.viewYsize = (int) (1.0*imYsize*ZOOMVIEW);
    id.vmin = 0.0;
    id.vmax = 1.0;
    id.showsaturated_min = 0;
    id.showsaturated_max = 0;
    id.zoomFact = ZOOMVIEW;
    id.update = 1;
    
    id.button1pressed = 0;
	id.button3pressed = 0;
    
    id.stride = id.viewXsize * BYTES_PER_PIXEL;
    id.stride += (4 - id.stride % 4) % 4; // ensure multiple of 4
	
	printf("stride = %d\n", id.stride);

    guchar *pixels = calloc(id.viewYsize * id.stride, 1);

    GdkPixbuf *pb = gdk_pixbuf_new_from_data(
                        pixels,
                        GDK_COLORSPACE_RGB,     // colorspace
                        0,                      // has_alpha
                        8,                      // bits-per-sample
                        id.viewXsize, id.viewYsize,       // cols, rows
                        id.stride,              // rowstride
                        free_pixels,            // destroy_fn
                        NULL                    // destroy_fn_data
                    );
	id.gtkimage = GTK_IMAGE(gtk_image_new_from_pixbuf(pb));



	// COLORBAR
	id.colorbar_height = 20;
	id.stride_colorbar = id.viewXsize * BYTES_PER_PIXEL;
	id.stride_colorbar += (4 - id.stride_colorbar % 4) % 4; // ensure multiple of 4
	guchar *colorbarpixels = calloc(id.colorbar_height * id.stride_colorbar, 1);
	
	GdkPixbuf *colorbarpb = gdk_pixbuf_new_from_data(
                        colorbarpixels,
                        GDK_COLORSPACE_RGB,                 // colorspace
                        0,                                  // has_alpha
                        8,                                  // bits-per-sample
                        id.viewXsize, id.colorbar_height,       // cols, rows
                        id.stride_colorbar,                     // rowstride
                        free_pixels,                        // destroy_fn
                        NULL                                // destroy_fn_data
                    );
	
   id.gtkimage_colorbar = GTK_IMAGE(gtk_image_new_from_pixbuf(colorbarpb));





	GtkCssProvider *cssProvider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(cssProvider, "theme.css", NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                               GTK_STYLE_PROVIDER(cssProvider),
                               GTK_STYLE_PROVIDER_PRIORITY_USER);

	/*GtkStyleContext *context;
    context = gtk_widget_get_style_context(la);
	gtk_style_context_add_provider (context,
                                    GTK_STYLE_PROVIDER(provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_USER);
*/

    // OBJECTS




    // window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Milk Stream Image Viewer");
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
  //  gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE); // detect window resizing

    //    gtk_window_set_default_size(GTK_WINDOW(window), COLS, ROWS);


    // box
    GtkWidget *mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5); // pack items vertically
    GtkWidget *box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1); // pack items vertically


	// frame and grid for pixel info
	GtkWidget *frame_pixelinfo = gtk_frame_new("Pixel info");
	GtkWidget *grid_pixelinfo = gtk_grid_new();

	// frame and grid for brightness scale
	GtkWidget *frame_brightnessscale = gtk_frame_new("Brightness Scale");
	GtkWidget *grid_brightnessscale = gtk_grid_new();




    // box2 content
    GtkWidget *buttonUpdateLevels = gtk_button_new_with_label ( "Update levels" );
	
	id.GTKlabelxcoord = gtk_label_new ("x = ");
	gtk_label_set_xalign(GTK_LABEL(id.GTKlabelxcoord), 0.0);
	gtk_label_set_justify(GTK_LABEL(id.GTKlabelxcoord), GTK_JUSTIFY_LEFT);
	
	id.GTKlabelycoord = gtk_label_new ("y = ");
	gtk_label_set_xalign(GTK_LABEL(id.GTKlabelycoord), 0.0);
	gtk_label_set_justify(GTK_LABEL(id.GTKlabelycoord), GTK_JUSTIFY_LEFT);

	id.GTKlabelpixval = gtk_label_new ("val = ");
	gtk_label_set_xalign(GTK_LABEL(id.GTKlabelpixval), 0.0);
	gtk_label_set_justify(GTK_LABEL(id.GTKlabelpixval), GTK_JUSTIFY_LEFT);
	
	id.GTKlabelzoom = gtk_label_new ("zoom = 1");
	gtk_label_set_xalign(GTK_LABEL(id.GTKlabelzoom), 0.0);
	gtk_label_set_justify(GTK_LABEL(id.GTKlabelzoom), GTK_JUSTIFY_LEFT);	
	
	id.GTKlabelxview = gtk_label_new ("x view = ");
	gtk_label_set_xalign(GTK_LABEL(id.GTKlabelxview), 0.0);
	gtk_label_set_justify(GTK_LABEL(id.GTKlabelxview), GTK_JUSTIFY_LEFT);	
	
	id.GTKlabelyview = gtk_label_new ("y view = ");
	gtk_label_set_xalign(GTK_LABEL(id.GTKlabelyview), 0.0);
	gtk_label_set_justify(GTK_LABEL(id.GTKlabelyview), GTK_JUSTIFY_LEFT);



	GtkWidget *GTKlabel_scale_min = gtk_label_new ("Min");
	GtkWidget *GTKlabel_scale_max = gtk_label_new ("Max");
	GtkWidget *GTKlabel_scale_input = gtk_label_new ("Input");
	GtkWidget *GTKlabel_scale_current = gtk_label_new ("Current");
	id.GTKlabel_scale_vmin = gtk_label_new ("-");
	id.GTKlabel_scale_vmax = gtk_label_new ("-");

	id.GTKentry_vmin = gtk_entry_new();
	sprintf(tmpstring, "%.2f", id.vmin);
	gtk_entry_set_text(GTK_ENTRY(id.GTKentry_vmin), tmpstring); 
	gtk_entry_set_max_length(GTK_ENTRY(id.GTKentry_vmin), 8);
	gtk_entry_set_width_chars(GTK_ENTRY(id.GTKentry_vmin), 8);

	id.GTKentry_vmax = gtk_entry_new();
	sprintf(tmpstring, "%.2f", id.vmax);
	gtk_entry_set_text(GTK_ENTRY(id.GTKentry_vmax), tmpstring); 
	gtk_entry_set_max_length(GTK_ENTRY(id.GTKentry_vmax), 8);
	gtk_entry_set_width_chars(GTK_ENTRY(id.GTKentry_vmax), 8);

	GtkWidget *GTKlabel_scale_saturation = gtk_label_new ("Saturation");
	id.GTKcheck_saturation_min = gtk_check_button_new_with_label ("min");
	id.GTKcheck_saturation_max = gtk_check_button_new_with_label ("max");

	id.GTKscale_bscale_slope = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.2, 3.0, 0.2);
	id.GTKscale_bscale_center = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05);
	id.bscale_slope = 1.0;
	id.bscale_center = 0.5;
	gtk_range_set_value (GTK_RANGE(id.GTKscale_bscale_slope), id.bscale_slope);
	gtk_range_set_value (GTK_RANGE(id.GTKscale_bscale_center), id.bscale_center);


	// window for image
	GtkWidget *image = gtk_image_new();
	



    // LAYOUT

    // add boxes to window
    gtk_container_add (GTK_CONTAINER (window), mainbox);
    gtk_box_pack_start (GTK_BOX (mainbox), box1, FALSE, FALSE, 0); // image display 
    gtk_box_pack_start (GTK_BOX (mainbox), box2, FALSE, FALSE, 0); // info panel



	// insert image
    gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(id.gtkimage), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(id.gtkimage_colorbar), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(gtk_label_new ("Colorbar")), FALSE, FALSE, 0);

	gtk_widget_set_hexpand (GTK_WIDGET(id.gtkimage), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET(id.gtkimage), TRUE);
	//gtk_widget_set_valign (GTK_WIDGET(id.gtkimage), GTK_ALIGN_START);
	gtk_widget_set_hexpand (GTK_WIDGET(id.gtkimage_colorbar), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET(id.gtkimage_colorbar), TRUE);
//	gtk_widget_set_valign (GTK_WIDGET(id.gtkimage_colorbar), GTK_ALIGN_START);


	gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET(frame_pixelinfo), FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame_pixelinfo), GTK_WIDGET (grid_pixelinfo));
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("Pixel :")),   0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(id.GTKlabelxcoord),           1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(id.GTKlabelycoord),           2, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(id.GTKlabelpixval),           3, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("Zoom :")),    0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(id.GTKlabelzoom),             1, 1, 2, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("X range :")), 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(id.GTKlabelxview),            1, 2, 3, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("Y range :")), 0, 3, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(id.GTKlabelyview),            1, 3, 3, 1);

	

	gtk_box_pack_start (GTK_BOX (box2), GTK_WIDGET(frame_brightnessscale), FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame_brightnessscale), GTK_WIDGET (grid_brightnessscale));
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_min),           1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_max),           2, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_input),         0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKentry_vmin),             1, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKentry_vmax),             2, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(buttonUpdateLevels),           1, 2, 2, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_current),       0, 3, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKlabel_scale_vmin),       1, 3, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKlabel_scale_vmax),       2, 3, 1, 1);	

	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_saturation),    0, 4, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKcheck_saturation_min),   1, 4, 1, 1);
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKcheck_saturation_max),   2, 4, 1, 1);

	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKscale_bscale_slope),  0, 5, 3, 1); 
	gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(id.GTKscale_bscale_center), 0, 6, 3, 1);



	// SOME INITIALIZATION
	update_pic_colorbar( &id);
	UpdateLevelscallback( buttonUpdateLevels, NULL, (void*) &id );
	

    // EVENTS


    // Start window at center of screen
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);



    // Exit program
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(window_configure_callback), &id);


    // mouse moves
    gtk_widget_set_events (window, GDK_POINTER_MOTION_MASK);
    g_signal_connect (window, "motion-notify-event", G_CALLBACK (mouse_moved), &id);


    gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK );
    g_signal_connect (window, "button-press-event", G_CALLBACK (buttonpresscallback), &id );

    gtk_widget_add_events (window, GDK_BUTTON_RELEASE_MASK );
    g_signal_connect (window, "button-release-event", G_CALLBACK (buttonreleasecallback), &id );


	gtk_widget_add_events (window, GDK_SCROLL_MASK );
	g_signal_connect (window, "scroll-event", G_CALLBACK (buttonscrollcallback), &id );




	g_signal_connect (buttonUpdateLevels, "button-press-event", G_CALLBACK (UpdateLevelscallback), &id );
	g_signal_connect (GTK_TOGGLE_BUTTON (id.GTKcheck_saturation_min), "toggled", G_CALLBACK (saturation_min_toggled_callback), &id);
	g_signal_connect (GTK_TOGGLE_BUTTON (id.GTKcheck_saturation_max), "toggled", G_CALLBACK (saturation_max_toggled_callback), &id);

	g_signal_connect (id.GTKscale_bscale_slope, "value-changed", G_CALLBACK (bscale_slope_moved), &id);
	g_signal_connect (id.GTKscale_bscale_center, "value-changed", G_CALLBACK (bscale_center_moved), &id);


    // keyboard
    g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (on_key_press), &id);
    // note : "key-release-event"

    // timeout
    g_timeout_add(100,         // milliseconds
                  update_pic,  // handler function
                  &id);        // data



    gtk_widget_show_all(window);




    gtk_main();

	free(imarray);


    return 0;
}
