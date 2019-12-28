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
	int autominmax;

	int update;

	//brightness scale
	GtkWidget *scaleLin;
	GtkWidget *scaleLog;
	GtkWidget *scalePower;
	GtkWidget *scaleSQRT;
	GtkWidget *scaleSquared;

	GtkWidget *scaleMinMax;


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
	GtkWidget *GTKcheck_autominmax;
	GtkWidget *GTKcheck_saturation_min;
	GtkWidget *GTKcheck_saturation_max;

	GtkWidget *GTKscale_bscale_slope;
	GtkWidget *GTKscale_bscale_center;





} IMAGEDATAVIEW;



IMAGEDATAVIEW imdataview[10];









void free_pixels(guchar *pixels, gpointer data) {
    free(pixels);
}









void PixVal_to_RGB(float pixval, guchar *rval, guchar *gval, guchar *bval)
{
    float pixval1;
	int pixsat = 0;



    pixval1 = imdataview[0].bscale_center + (pixval-imdataview[0].bscale_center) * imdataview[0].bscale_slope;



    if(pixval1 < 0.0)
    {
        pixval1 = 0.0;
       if(imdataview[0].showsaturated_min == 1)
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
        if(imdataview[0].showsaturated_max == 1)
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








int update_pic_colorbar()
{
    GdkPixbuf *pb = gtk_image_get_pixbuf(imdataview[0].gtkimage_colorbar);
    guchar rval, gval, bval;

    guchar *array = gdk_pixbuf_get_pixels(pb);

	printf("---- colorbar\n");
	fflush(stdout);
	

    int ii, jj;
    for(ii=0; ii < imdataview[0].viewXsize; ii++)
    {
        for(jj=0; jj < imdataview[0].colorbar_height; jj++)
        {
            float pixval;

            pixval = 1.0*ii/imdataview[0].viewXsize;
            PixVal_to_RGB(pixval, &rval, &gval, &bval);

            int pixindex = jj * imdataview[0].stride_colorbar + ii * BYTES_PER_PIXEL;

#ifdef VERBOSE
            printf(" %3d / %3d   %3d / %3d  (%d) %d   %f ->  %d %d %d\n",
                   ii,
                   imdataview[0].viewXsize,
                   jj,
                   imdataview[0].colorbar_height,
                   imdataview[0].stride_colorbar,
                   pixindex,
                   pixval,
                   rval,
                   gval,
                   bval);
            fflush(stdout);
#endif

            array[pixindex] = rval;
            array[pixindex+1] = gval;
            array[pixindex+2] = bval;
        }
    }
    gtk_image_set_from_pixbuf(GTK_IMAGE(imdataview[0].gtkimage_colorbar), pb);

	printf("---- colorbar  DONE\n");
	fflush(stdout);

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


    GdkPixbuf *pb = gtk_image_get_pixbuf(imdataview[0].gtkimage);

    guchar rval, gval, bval;

    if(imcnt0 != imdataview[0].streamimage->md[0].cnt0)
        imdataview[0].update = 1;

    if(imdataview[0].update==1)
    {
#ifdef VERBOSE
        printf("----- updating image\n");
        fflush(stdout);
#endif






        imcnt0 = imdataview[0].streamimage->md[0].cnt0;
        int imXsize = 0;
        int imYsize = 0;
        int imid = 0;

        if( imdataview[0].streamimage->md[0].naxis == 2) {
          imXsize = imdataview[0].streamimage->md[0].size[0];
        	imYsize = imdataview[0].streamimage->md[0].size[1];
          imid = 0;
        } else if( imdataview[0].streamimage->md[0].naxis == 3) {
          imXsize = imdataview[0].streamimage->md[0].size[1];
        	imYsize = imdataview[0].streamimage->md[0].size[2];
          imid = imcnt0 % imdataview[0].streamimage->md[0].size[0];
        }
        int imSize = imXsize*imYsize;

        guchar *array = gdk_pixbuf_get_pixels(pb);

        //imdataview[0].zoomFact = 1.0 * imdataview[0].xsize / (imdataview[0].x1view - imdataview[0].x0view);

        // compute min and max window coord in raw image

        int iimin = (long) (0.5 + imdataview[0].x0view);
        int iimax = (long) (0.5 + imdataview[0].x0view + imdataview[0].viewXsize / imdataview[0].zoomFact) + 1;
        if (iimin < 0)
            iimin = 0;
        if (iimax > imXsize-1 )
            iimax = imXsize;

        int jjmin = (long) (0.5 + imdataview[0].y0view);
        int jjmax = (long) (0.5 + imdataview[0].y0view + imdataview[0].viewYsize / imdataview[0].zoomFact) + 1;
        if (jjmin < 0)
            jjmin = 0;
        if (jjmax > imYsize-1 )
            jjmax = imYsize;



        if(comparrayinit==0)
        {
            comparray = (float*) malloc(sizeof(float) * imSize);
            Rarray = (guchar*) malloc(sizeof(guchar) * imSize);
            Garray = (guchar*) malloc(sizeof(guchar) * imSize);
            Barray = (guchar*) malloc(sizeof(guchar) * imSize);
            PixelRaw_array = (long*) malloc(sizeof(long) * imdataview[0].viewXsize * imdataview[0].viewYsize);
            PixelBuff_array = (int*) malloc(sizeof(int) * imdataview[0].viewXsize * imdataview[0].viewYsize);
            comparrayinit = 1;
        }

        /*		printf("active image area :  %4d x %4d   at  %4d %4d    [%4d %4d]\n", iimax-iimin, jjmax-jjmin, iimin, jjmin, imdataview[0].viewXsize, imdataview[0].viewYsize);*/


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
            for (yview = 0; yview < imdataview[0].viewYsize; yview++)
                for (xview = 0; xview < imdataview[0].viewXsize; xview++)
                {
                    long pixindexRaw;
                    long pixindexView;
                    int pixindexBuff;

                    ii = (int) (0.5 + imdataview[0].x0view + xview / imdataview[0].zoomFact );
                    jj = (int) (0.5 + imdataview[0].y0view + yview / imdataview[0].zoomFact );

                    pixindexView = yview * imdataview[0].viewXsize + xview;
                    pixindexRaw = jj*imXsize+ii;
                    pixindexBuff = yview * imdataview[0].stride + xview * BYTES_PER_PIXEL;
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
        switch ( imdataview[0].streamimage->md[0].datatype )
        {
			case _DATATYPE_FLOAT:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = imdataview[0].streamimage->array.F[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_DOUBLE:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = imdataview[0].streamimage->array.D[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_UINT8:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.UI8[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_UINT16:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.UI16[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_UINT32:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.UI32[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_UINT64:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.UI64[imid*imSize+jj*imXsize+ii];
            break;


			case _DATATYPE_INT8:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.SI8[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_INT16:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.SI16[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_INT32:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.SI32[imid*imSize+jj*imXsize+ii];
            break;

			case _DATATYPE_INT64:
            for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                    comparray[jj*imXsize+ii] = (float) imdataview[0].streamimage->array.SI64[imid*imSize+jj*imXsize+ii];
            break;


        }

		// Recompute min and max scale if needed
		if(imdataview[0].autominmax == 1)
		{
			float minval, maxval;
			char tmpstring[200];

			minval = comparray[jjmin*imXsize+iimin];
			maxval = minval;

			for(ii=iimin; ii<iimax; ii++)
                for(jj=jjmin; jj<jjmax; jj++)
                {
					float tmpval = comparray[jj*imXsize+ii];
					if(tmpval < minval)
						minval = tmpval;
					if(tmpval > maxval)
						maxval = tmpval;
				}
			imdataview[0].vmin = minval;
			imdataview[0].vmax = maxval;

/* TBD
			sprintf(tmpstring, "%.2f", imdataview[0].vmin);
			gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabel_scale_vmin), tmpstring);

			sprintf(tmpstring, "%.2f", imdataview[0].vmax);
			gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabel_scale_vmax), tmpstring);
			*/
		}



        // compute R, G, B values
        for (ii=0; ii<imYsize*imXsize; ii++)
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

                pixindex = jj*imXsize+ii;
                pixval = comparray[pixindex];
                pixval = (pixval - imdataview[0].vmin) / (imdataview[0].vmax - imdataview[0].vmin);

                PixVal_to_RGB( pixval, &Rarray[pixindex], &Garray[pixindex], &Barray[pixindex]);
            }


        /*   if( (ii==imdataview[0].iisel) && (jj==imdataview[0].jjsel) )
           {
               rval = 0;
               gval = 255;
               bval = 0;
           }
        */

        long pixindexView;
        for(pixindexView = 0; pixindexView<imdataview[0].viewXsize*imdataview[0].viewYsize; pixindexView++)
        {
            long pixindexRaw = PixelRaw_array[pixindexView];
            int pixindex = PixelBuff_array[pixindexView];

            array[pixindex] = Rarray[pixindexRaw];
            array[pixindex+1] = Garray[pixindexRaw];
            array[pixindex+2] = Barray[pixindexRaw];

        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(imdataview[0].gtkimage), pb);
    }
    imdataview[0].update = 0;

    return TRUE;
}










int viewWindow_check_values()
{
    //   ImageData *id = (ImageData*) ptr;
    char labeltext[100];



    // Translate image along X

    if(imdataview[0].x0view < 0)
    {
        float offset = -imdataview[0].x0view;

#ifdef VERBOSE
        printf("[[ X + offset %8.2f ]]", offset);
#endif

        imdataview[0].x0view += offset;
        imdataview[0].x1view += offset;
    }
    if(imdataview[0].x1view > imdataview[0].xsize)
    {
        float offset = imdataview[0].x1view - imdataview[0].xsize;

#ifdef VERBOSE
        printf("[[ X - offset %8.2f ]]", offset);
#endif

        imdataview[0].x0view -= offset;
        imdataview[0].x1view -= offset;
    }


    // Translate image along Y

    if(imdataview[0].y0view < 0)
    {
        float offset = -imdataview[0].y0view;

#ifdef VERBOSE
        printf(" [[ Y + offset %8.2f ]]", offset);
#endif

        imdataview[0].y0view += offset;
        imdataview[0].y1view += offset;

#ifdef VERBOSE
        printf("->[ %8.2f %8.2f ]", imdataview[0].y0view, imdataview[0].y1view);
#endif
    }
    if(imdataview[0].y1view > imdataview[0].ysize)
    {
        float offset = imdataview[0].y1view - imdataview[0].ysize;

#ifdef VERBOSE
        printf("[[ Y - offset %8.2f ]]", offset);
#endif

        imdataview[0].y0view -= offset;
        imdataview[0].y1view -= offset;

#ifdef VERBOSE
        printf("->[ %8.2f %8.2f ] ", imdataview[0].y0view, imdataview[0].y1view);
#endif
    }




    // Change bounds as needed

    if(imdataview[0].x0view < 0)
        imdataview[0].x0view = 0;
    if(imdataview[0].y0view < 0)
        imdataview[0].y0view = 0;






    // Recompute zoom

    float zfactX, zfactY;

    zfactX = 1.0*imdataview[0].viewXsize / (imdataview[0].x1view - imdataview[0].x0view);
    zfactY = 1.0*imdataview[0].viewYsize / (imdataview[0].y1view - imdataview[0].y0view);

    if(zfactX>zfactY)
        imdataview[0].zoomFact = zfactX;
    else
        imdataview[0].zoomFact = zfactY;



#ifdef VERBOSE
    printf(" -> %8.4f ===\n", imdataview[0].zoomFact);
#endif


/* TBD
    sprintf(labeltext, "%8.2f", imdataview[0].zoomFact);
    gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelzoom), labeltext);
    sprintf(labeltext, "%8.2f - %8.2f", imdataview[0].x0view, imdataview[0].x1view);
    gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelxview), labeltext);
    sprintf(labeltext, "%8.2f - %8.2f", imdataview[0].y0view, imdataview[0].y1view);
    gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelyview), labeltext);
*/
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






gboolean scale_set_Lin_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin),     TRUE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scalePower),   FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT),    FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), FALSE);	

    return TRUE;
}


gboolean scale_set_Log_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog),     TRUE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scalePower),   FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT),    FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), FALSE);
	

    return TRUE;
}

gboolean scale_set_Power_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scalePower),   TRUE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT),    FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), FALSE);
	
    return TRUE;
}

gboolean scale_set_SQRT_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{

	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scalePower),   FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT),    TRUE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), FALSE);
	
    return TRUE;
}

gboolean scale_set_Squared_callback (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog),     FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scalePower),   FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT),    FALSE);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), TRUE);
	
    return TRUE;
}



//	GtkWidget *scaleMinMax;



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

/* TBD                sprintf(labeltext, "%5d", ii);
                gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelxcoord), labeltext);
                sprintf(labeltext, "%5d", jj);
                gtk_label_set_text(GTK_LABEL(imdataview[0].GTKlabelycoord), labeltext);
*/
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

            /*case GDK_SCROLL_SMOOTH :
            printf("SCROLL SMOOTH");
            printf("  delta_x = %f", (float) event->delta_x );
            printf("  delta_y = %f", (float) event->delta_y );
            break;*/

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





int show_popup(GtkWidget *widget, GdkEvent *event) {

    const gint RIGHT_CLICK = 3;

    if (event->type == GDK_BUTTON_PRESS) {

        GdkEventButton *bevent = (GdkEventButton *) event;

        if (bevent->button == RIGHT_CLICK) {

            gtk_menu_popup_at_widget(GTK_MENU(widget),
                                     GTK_WIDGET (event),
                                     GDK_GRAVITY_NORTH_WEST,
                                     GDK_GRAVITY_SOUTH_WEST,
                                     NULL);
        }

        return TRUE;
    }

    return FALSE;
}










GtkWidget * make_menubar( GtkAccelGroup *accel_group )
{
	GtkWidget * menubar = gtk_menu_bar_new ();


    // top level entries
    GtkWidget *fileML0  = gtk_menu_item_new_with_label("File");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileML0);

    GtkWidget *scaleML0 = gtk_menu_item_new_with_label("Scale");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), scaleML0);



    // File -> fileMenu

    GtkWidget *fileMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileML0), fileMenu); // attach to fileML0

    GtkWidget *imprMi = gtk_menu_item_new_with_label("Import");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), imprMi);

    GtkWidget *quitML1 = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitML1);
    gtk_widget_add_accelerator(quitML1, "activate", accel_group,
                               GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


	// Scale -> scaleMenu
	GtkWidget *scaleMenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(scaleML0), scaleMenu); // attach to scaleML0


	imdataview[0].scaleLin = gtk_check_menu_item_new_with_label ("Linear");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleLin);
	g_signal_connect(G_OBJECT(imdataview[0].scaleLin), "toggled", G_CALLBACK(scale_set_Lin_callback), NULL);
	

	imdataview[0].scaleLog = gtk_check_menu_item_new_with_label ("Log");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleLog);
	g_signal_connect(G_OBJECT(imdataview[0].scaleLog), "toggled", G_CALLBACK(scale_set_Log_callback), NULL);
	
/*	imdataview[0].scalePower = gtk_check_menu_item_new_with_label ("Power");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scalePower), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scalePower);
	g_signal_connect(G_OBJECT(imdataview[0].scalePower), "toggled", G_CALLBACK(scale_set_Power_callback), NULL);
	
	imdataview[0].scaleSQRT = gtk_check_menu_item_new_with_label ("Square root");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleSQRT);
	g_signal_connect(G_OBJECT(imdataview[0].scaleSQRT), "toggled", G_CALLBACK(scale_set_SQRT_callback), NULL);

	imdataview[0].scaleSquared = gtk_check_menu_item_new_with_label ("Squared");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleSquared);
	g_signal_connect(G_OBJECT(imdataview[0].scaleSquared), "toggled", G_CALLBACK(scale_set_Squared_callback), NULL);

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), sep);

	imdataview[0].scaleMinMax = gtk_check_menu_item_new_with_label ("Min Max");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleMinMax), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleMinMax);
	g_signal_connect(G_OBJECT(imdataview[0].scaleMinMax), "toggled", G_CALLBACK(scale_set_MinMax_callback), NULL);
*/



    // under fileMenu-Import

    GtkWidget *imprMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(imprMi), imprMenu);

    GtkWidget *feedMi = gtk_menu_item_new_with_label("Import news feed...");
    gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), feedMi);

    GtkWidget *bookMi = gtk_menu_item_new_with_label("Import bookmarks...");
    gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), bookMi);

   // GtkWidget *sep = gtk_separator_menu_item_new();
   // gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), sep);

    GtkWidget *mailMi = gtk_menu_item_new_with_label("Import mail...");
    gtk_menu_shell_append(GTK_MENU_SHELL(imprMenu), mailMi);


    g_signal_connect(G_OBJECT(quitML1), "activate",
                     G_CALLBACK(gtk_main_quit), NULL);

	return(menubar);

}





GtkWidget * make_iminfobox()
{
	char tmpstring[200];
	
	GtkWidget *iminfobox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1); // pack items vertically
	


    // frame and grid for pixel info
    GtkWidget *frame_pixelinfo = gtk_frame_new("Pixel info");
    GtkWidget *grid_pixelinfo = gtk_grid_new();

    // frame and grid for brightness scale
    GtkWidget *frame_brightnessscale = gtk_frame_new("Brightness Scale");
    GtkWidget *grid_brightnessscale = gtk_grid_new();




    // box2 content
    GtkWidget *buttonUpdateLevels = gtk_button_new_with_label ( "Update levels" );

    imdataview[0].GTKlabelxcoord = gtk_label_new ("x = ");
    //gtk_label_set_xalign(GTK_LABEL(imdataview[0].GTKlabelxcoord), 0.0);
    gtk_label_set_justify(GTK_LABEL(imdataview[0].GTKlabelxcoord), GTK_JUSTIFY_LEFT);

    imdataview[0].GTKlabelycoord = gtk_label_new ("y = ");
    //gtk_label_set_xalign(GTK_LABEL(imdataview[0].GTKlabelycoord), 0.0);
    gtk_label_set_justify(GTK_LABEL(imdataview[0].GTKlabelycoord), GTK_JUSTIFY_LEFT);

    imdataview[0].GTKlabelpixval = gtk_label_new ("val = ");
    //gtk_label_set_xalign(GTK_LABEL(imdataview[0].GTKlabelpixval), 0.0);
    gtk_label_set_justify(GTK_LABEL(imdataview[0].GTKlabelpixval), GTK_JUSTIFY_LEFT);

    imdataview[0].GTKlabelzoom = gtk_label_new ("zoom = 1");
    //gtk_label_set_xalign(GTK_LABEL(imdataview[0].GTKlabelzoom), 0.0);
    gtk_label_set_justify(GTK_LABEL(imdataview[0].GTKlabelzoom), GTK_JUSTIFY_LEFT);

    imdataview[0].GTKlabelxview = gtk_label_new ("x view = ");
    //gtk_label_set_xalign(GTK_LABEL(imdataview[0].GTKlabelxview), 0.0);
    gtk_label_set_justify(GTK_LABEL(imdataview[0].GTKlabelxview), GTK_JUSTIFY_LEFT);

    imdataview[0].GTKlabelyview = gtk_label_new ("y view = ");
    //gtk_label_set_xalign(GTK_LABEL(imdataview[0].GTKlabelyview), 0.0);
    gtk_label_set_justify(GTK_LABEL(imdataview[0].GTKlabelyview), GTK_JUSTIFY_LEFT);


	printf("LINE %d\n", __LINE__);
	fflush(stdout);


    GtkWidget *GTKlabel_scale_min = gtk_label_new ("Min");
    GtkWidget *GTKlabel_scale_max = gtk_label_new ("Max");
    GtkWidget *GTKlabel_scale_input = gtk_label_new ("Input");
    GtkWidget *GTKlabel_scale_current = gtk_label_new ("Current");
    imdataview[0].GTKlabel_scale_vmin = gtk_label_new ("-");
    imdataview[0].GTKlabel_scale_vmax = gtk_label_new ("-");

    imdataview[0].GTKentry_vmin = gtk_entry_new();
    sprintf(tmpstring, "%.2f", imdataview[0].vmin);
    gtk_entry_set_text(GTK_ENTRY(imdataview[0].GTKentry_vmin), tmpstring);
    gtk_entry_set_max_length(GTK_ENTRY(imdataview[0].GTKentry_vmin), 8);
    gtk_entry_set_width_chars(GTK_ENTRY(imdataview[0].GTKentry_vmin), 8);

    imdataview[0].GTKentry_vmax = gtk_entry_new();
    sprintf(tmpstring, "%.2f", imdataview[0].vmax);
    gtk_entry_set_text(GTK_ENTRY(imdataview[0].GTKentry_vmax), tmpstring);
    gtk_entry_set_max_length(GTK_ENTRY(imdataview[0].GTKentry_vmax), 8);
    gtk_entry_set_width_chars(GTK_ENTRY(imdataview[0].GTKentry_vmax), 8);

    GtkWidget *GTKlabel_scale_saturation = gtk_label_new ("Saturation");
    imdataview[0].GTKcheck_saturation_min = gtk_check_button_new_with_label ("min");
    imdataview[0].GTKcheck_saturation_max = gtk_check_button_new_with_label ("max");
    imdataview[0].GTKcheck_autominmax = gtk_check_button_new_with_label ("autominmax");

    imdataview[0].GTKscale_bscale_slope = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.2, 3.0, 0.2);
    imdataview[0].GTKscale_bscale_center = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05);

    gtk_range_set_value (GTK_RANGE(imdataview[0].GTKscale_bscale_slope), imdataview[0].bscale_slope);
    gtk_range_set_value (GTK_RANGE(imdataview[0].GTKscale_bscale_center), imdataview[0].bscale_center);
	
	printf("LINE %d\n", __LINE__);
	fflush(stdout);

    gtk_box_pack_start (GTK_BOX (iminfobox), GTK_WIDGET(frame_pixelinfo), FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frame_pixelinfo), GTK_WIDGET (grid_pixelinfo));
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("Pixel :")),   0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(imdataview[0].GTKlabelxcoord),           1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(imdataview[0].GTKlabelycoord),           2, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(imdataview[0].GTKlabelpixval),           3, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("Zoom :")),    0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(imdataview[0].GTKlabelzoom),             1, 1, 2, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("X range :")), 0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(imdataview[0].GTKlabelxview),            1, 2, 3, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(gtk_label_new ("Y range :")), 0, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_pixelinfo), GTK_WIDGET(imdataview[0].GTKlabelyview),            1, 3, 3, 1);



    gtk_box_pack_start (GTK_BOX (iminfobox), GTK_WIDGET(frame_brightnessscale), FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (frame_brightnessscale), GTK_WIDGET (grid_brightnessscale));
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_min),           1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_max),           2, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_input),         0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKentry_vmin),             1, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKentry_vmax),             2, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(buttonUpdateLevels),           1, 2, 2, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_current),       0, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKlabel_scale_vmin),       1, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKlabel_scale_vmax),       2, 3, 1, 1);

    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKcheck_autominmax),       0, 4, 1, 1);

    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(GTKlabel_scale_saturation),    0, 5, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKcheck_saturation_min),   1, 5, 1, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKcheck_saturation_max),   2, 5, 1, 1);

    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKscale_bscale_slope),  0, 6, 3, 1);
    gtk_grid_attach (GTK_GRID (grid_brightnessscale), GTK_WIDGET(imdataview[0].GTKscale_bscale_center), 0, 7, 3, 1);


	printf("LINE %d\n", __LINE__);
	fflush(stdout);


	UpdateLevelscallback( buttonUpdateLevels, NULL, NULL );
	
	
	printf("LINE %d\n", __LINE__);
	fflush(stdout);	
	
	    g_signal_connect (
        buttonUpdateLevels,
        "button-press-event",
        G_CALLBACK (UpdateLevelscallback),
        NULL );
	


    g_signal_connect (
        GTK_TOGGLE_BUTTON (
            imdataview[0].GTKcheck_autominmax),
        "toggled",
        G_CALLBACK (autominmax_toggled_callback),
        NULL );




    g_signal_connect (
        imdataview[0].GTKscale_bscale_slope,
        "value-changed",
        G_CALLBACK (bscale_slope_moved),
        NULL );
    g_signal_connect (
        imdataview[0].GTKscale_bscale_center,
        "value-changed",
        G_CALLBACK (bscale_center_moved), 
        NULL );

	printf("LINE %d\n", __LINE__);
	fflush(stdout);
	
	
    g_signal_connect (
        GTK_TOGGLE_BUTTON (imdataview[0].GTKcheck_saturation_min),
        "toggled",
        G_CALLBACK (saturation_min_toggled_callback),
        NULL );

    g_signal_connect (
        GTK_TOGGLE_BUTTON (imdataview[0].GTKcheck_saturation_max),
        "toggled",
        G_CALLBACK (saturation_max_toggled_callback),
        NULL );

	printf("LINE %d\n", __LINE__);
	fflush(stdout);	
	
	
	return(iminfobox);
}








int main(int argc, char **argv) {

    // Read image from shared memory
    IMAGE *imarray;    // pointer to array of images
    char filename[64];
    char tmpstring[100];

    float ZOOMVIEW = 1.0;

    int NBIMAGES = 1;
    imarray = (IMAGE*) malloc(sizeof(IMAGE)*NBIMAGES);



    ImageStreamIO_filename(filename, 64, argv[1]);
    printf("Filename = %s\n", filename);
    if(ImageStreamIO_openIm(&imarray[0], argv[1]) == -1)
    {
        printf("ERROR: cannot load stream \"%s\"\n", argv[1]);
        return -1;
    }

    int imXsize = 0;
    int imYsize = 0;
    if( imarray->md[0].naxis == 2) {
        imXsize = imarray->md[0].size[0];
        imYsize = imarray->md[0].size[1];
    } else if( imarray->md[0].naxis == 3) {
        imXsize = imarray->md[0].size[1];
        imYsize = imarray->md[0].size[2];
    }

    printf("image size = %d x %d\n", imXsize, imYsize);

    if(argc>2)    {
        ZOOMVIEW = atof(argv[2]);
    }





    // Initialize GTK
    gtk_init(&argc, &argv);




    imdataview[0].ysize = imYsize;
    imdataview[0].xsize = imXsize;
    imdataview[0].streamimage = &imarray[0];

    imdataview[0].x0view = 0;
    imdataview[0].x1view = imXsize;
    imdataview[0].y0view = 0;
    imdataview[0].y1view = imYsize;
    imdataview[0].viewXsize = (int) (1.0*imXsize*ZOOMVIEW);
    imdataview[0].viewYsize = (int) (1.0*imYsize*ZOOMVIEW);
    imdataview[0].vmin = 0.0;
    imdataview[0].vmax = 1.0;
    imdataview[0].bscale_slope = 1.0;
    imdataview[0].bscale_center = 0.5;

    imdataview[0].showsaturated_min = 0;
    imdataview[0].showsaturated_max = 0;
    imdataview[0].zoomFact = ZOOMVIEW;
    imdataview[0].update = 1;

    imdataview[0].autominmax = 1;

    imdataview[0].button1pressed = 0;
    imdataview[0].button3pressed = 0;

    imdataview[0].stride = imdataview[0].viewXsize * BYTES_PER_PIXEL;
    imdataview[0].stride += (4 - imdataview[0].stride % 4) % 4; // ensure multiple of 4

    printf("stride = %d\n", imdataview[0].stride);

    guchar *pixels = calloc(imdataview[0].viewYsize * imdataview[0].stride, 1);

    GdkPixbuf *pb = gdk_pixbuf_new_from_data(
                        pixels,
                        GDK_COLORSPACE_RGB,     // colorspace
                        0,                      // has_alpha
                        8,                      // bits-per-sample
                        imdataview[0].viewXsize, imdataview[0].viewYsize,       // cols, rows
                        imdataview[0].stride,              // rowstride
                        free_pixels,            // destroy_fn
                        NULL                    // destroy_fn_data
                    );
    imdataview[0].gtkimage = GTK_IMAGE(gtk_image_new_from_pixbuf(pb));



    // COLORBAR
    imdataview[0].colorbar_height = 20;
    imdataview[0].stride_colorbar = imdataview[0].viewXsize * BYTES_PER_PIXEL;
    imdataview[0].stride_colorbar += (4 - imdataview[0].stride_colorbar % 4) % 4; // ensure multiple of 4
    guchar *colorbarpixels = calloc(imdataview[0].colorbar_height * imdataview[0].stride_colorbar, 1);

    GdkPixbuf *colorbarpb = gdk_pixbuf_new_from_data(
                                colorbarpixels,
                                GDK_COLORSPACE_RGB,                 // colorspace
                                0,                                  // has_alpha
                                8,                                  // bits-per-sample
                                imdataview[0].viewXsize, imdataview[0].colorbar_height,       // cols, rows
                                imdataview[0].stride_colorbar,                     // rowstride
                                free_pixels,                        // destroy_fn
                                NULL                                // destroy_fn_data
                            );

    imdataview[0].gtkimage_colorbar = GTK_IMAGE(gtk_image_new_from_pixbuf(colorbarpb));





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



	// menubar
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	
	
	//GtkWidget *menubar = make_menubar(accel_group);


	GtkWidget * menubar = gtk_menu_bar_new ();


    // top level entries
    GtkWidget *fileML0  = gtk_menu_item_new_with_label("File");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileML0);

    GtkWidget *scaleML0 = gtk_menu_item_new_with_label("Scale");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), scaleML0);



    // File -> fileMenu

    GtkWidget *fileMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileML0), fileMenu); // attach to fileML0

    GtkWidget *imprMi = gtk_menu_item_new_with_label("Import");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), imprMi);

    GtkWidget *quitML1 = gtk_menu_item_new_with_label("Quit");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitML1);
    gtk_widget_add_accelerator(quitML1, "activate", accel_group,
                               GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


	// Scale -> scaleMenu
	GtkWidget *scaleMenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(scaleML0), scaleMenu); // attach to scaleML0


	imdataview[0].scaleLin = gtk_check_menu_item_new_with_label ("Linear");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleLin);
	g_signal_connect(G_OBJECT(imdataview[0].scaleLin), "toggled", G_CALLBACK(scale_set_Lin_callback), NULL);
	

	imdataview[0].scaleLog = gtk_check_menu_item_new_with_label ("Log");
	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog), TRUE);
	gtk_menu_shell_append(GTK_MENU_SHELL(scaleMenu), imdataview[0].scaleLog);
	g_signal_connect(G_OBJECT(imdataview[0].scaleLog), "toggled", G_CALLBACK(scale_set_Log_callback), NULL);
	




//	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLin),     TRUE);
//	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleLog),     TRUE);
//	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scalePower),   TRUE);
//	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSQRT),    FALSE);
//	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(imdataview[0].scaleSquared), TRUE);	















    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);






    // box
    GtkWidget *mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5); // pack items vertically
    
    GtkWidget *box2 = make_iminfobox();


    // window for image
    GtkWidget *image = gtk_image_new();




    // LAYOUT

    // add boxes to window
    gtk_container_add (GTK_CONTAINER (window), mainbox);

    gtk_box_pack_start (GTK_BOX (mainbox), box1, FALSE, FALSE, 0); // image display
    gtk_box_pack_start (GTK_BOX (mainbox), box2, FALSE, FALSE, 0); // info panel
    gtk_box_pack_start (GTK_BOX(box1), menubar, FALSE, FALSE, 0);





    // insert image
    gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(imdataview[0].gtkimage), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(imdataview[0].gtkimage_colorbar), FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(gtk_label_new ("Colorbar")), FALSE, FALSE, 0);

    gtk_widget_set_hexpand (GTK_WIDGET(imdataview[0].gtkimage), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET(imdataview[0].gtkimage), TRUE);
    //gtk_widget_set_valign (GTK_WIDGET(imdataview[0].gtkimage), GTK_ALIGN_START);
    gtk_widget_set_hexpand (GTK_WIDGET(imdataview[0].gtkimage_colorbar), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET(imdataview[0].gtkimage_colorbar), TRUE);
    //	gtk_widget_set_valign (GTK_WIDGET(imdataview[0].gtkimage_colorbar), GTK_ALIGN_START);





    // SOME INITIALIZATION
    update_pic_colorbar();    
    // Start window at center of screen
    // gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);




	// EVENTS

    // Exit program
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(
        G_OBJECT(window),
        "configure-event",
        G_CALLBACK(window_configure_callback),
        NULL );


    // mouse moves
    gtk_widget_set_events (window, GDK_POINTER_MOTION_MASK);
    g_signal_connect (
        window,
        "motion-notify-event",
        G_CALLBACK (mouse_moved),
        NULL );


    gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK );
    g_signal_connect (
        window,
        "button-press-event", 
        G_CALLBACK (buttonpresscallback), 
        NULL );



    gtk_widget_add_events (window, GDK_BUTTON_RELEASE_MASK );
    g_signal_connect (
        window,
        "button-release-event",
        G_CALLBACK (buttonreleasecallback),
        NULL );


    gtk_widget_add_events (window, GDK_SCROLL_MASK );
    g_signal_connect (
        window,
        "scroll-event",
        G_CALLBACK (buttonscrollcallback),
        NULL );



    // keyboard
    g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (on_key_press), NULL);
    
    // note : "key-release-event"

    // timeout
    g_timeout_add(100,         // milliseconds
                  update_pic,  // handler function
                  NULL );        // data




    gtk_widget_show_all(window);



    gtk_main();

    free(imarray);


    return 0;
}
