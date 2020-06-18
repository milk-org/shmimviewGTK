#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>


#include "ImageStreamIO/ImageStruct.h"
#include "ImageStreamIO/ImageStreamIO.h"


#include "shmimview.h"


#define BYTES_PER_PIXEL 3


// GTK widget pointers
extern app_widgets *widgets;


extern IMAGEDATAVIEW *imdataview;

extern COLORMAPDATA cmapdata;


// Images
extern IMAGE *imarray;

extern gboolean verbose;





/*
 * Compate function for qsort
 *
 */
int floatcompare (const void * a, const void * b)
{
    float fa = *(const float*) a;
    float fb = *(const float*) b;
    return (fa > fb) - (fa < fb);
}








// colormap functions
/*
static void colormap_RGBval (
    float pixval,
    unsigned char *Rval,
    unsigned char *Gval,
    unsigned char *Bval)
{
    int viewindex = 0;
    float x, x2;
    float xlim;

    switch ( imdataview[viewindex].colormap ) {

    case COLORMAP_HEAT :
        *Rval = (unsigned char) (sqrt(pixval)*255);
        *Gval = (unsigned char) (pixval*sqrt(pixval)*255);
        *Bval = (unsigned char) (pixval*pixval*255);
        break;

    case COLORMAP_COOL :
        *Rval = (unsigned char) (pixval*pixval*255);
        *Gval = (unsigned char) (pixval*sqrt(pixval)*255);
        *Bval = (unsigned char) (sqrt(pixval)*255);
        break;

    case COLORMAP_RGB :
		if(pixval < 0.5) {
			*Rval = (unsigned char) (255.0*(1.0-pixval*2.0));
			*Gval = (unsigned char) (pixval*2.0*255);
			*Bval = (unsigned char) (0);
		}
		else
		{
			*Rval = (unsigned char) (0);
			*Gval = (unsigned char) (255.0*(2.0-pixval*2.0));
			*Bval = (unsigned char) ((pixval-0.5)*2.0*255);
		}
        break;

    case COLORMAP_BRY :
        xlim = 0.25;
        if(pixval<xlim) {
            *Rval = (unsigned char) 0;
            *Gval = (unsigned char) 0;
        }
        else {
            x = (pixval-xlim)/(1.0-xlim);
            x2 = x*x;
            *Rval = (unsigned char) ( sqrt(x) * 255);
            *Gval = (unsigned char) ( x2 * 255);
        }
        *Bval = (unsigned char) ( (0.5-0.5*cos(pixval*M_PI*3)) * 255);
        break;


    default :
        *Rval = (unsigned char) (pixval*255);
        *Gval = (unsigned char) (pixval*255);
        *Bval = (unsigned char) (pixval*255);
        break;
    }

}



*/

/*
void PixVal_to_RGB(float pixval, guchar *rval, guchar *gval, guchar *bval)
{
    float pixval1;
    int pixsat = 0;
    int viewindex = 0;

    pixval1 = 0.5 + (pixval-imdataview[viewindex].bscale_center)* imdataview[viewindex].bscale_slope;
             // (pixval-imdataview[viewindex].bscale_center) * imdataview[viewindex].bscale_slope;


    if(pixval1 < 0.0)
    {
        pixval1 = 0.0;
        if(imdataview[viewindex].showsaturated_min == 1)
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
        if(imdataview[viewindex].showsaturated_max == 1)
        {
            pixsat = 1;
            *rval = 255;
            *gval = 0;
            *bval = 0;
        }
    }

    if(pixsat==0)
    {
        float pixval2;

        pixval2 = imdataview[viewindex].scalefunc( pixval1, imdataview[viewindex].scale_coeff);

        colormap_RGBval(pixval2, rval, gval, bval);
    }

}
*/








void free_viewpixels(
    guchar *pixels,
    __attribute__((unused)) gpointer data
) {
    int viewindex = 0;
    imdataview[viewindex].allocated_viewpixels = 0;
    free(pixels);
}





int resize_PixelBufferView(
    int xsize,
    int ysize
)
{
    int viewindex = 0;

    static int finit = 0;

    imdataview[viewindex].xviewsize = xsize;
    imdataview[viewindex].yviewsize = ysize;

    // (re-)allocate pixel buffer

    imdataview[viewindex].stride = imdataview[viewindex].xviewsize * BYTES_PER_PIXEL;
    imdataview[viewindex].stride += (4 - imdataview[viewindex].stride % 4) % 4; // ensure multiple of 4

    if(verbose) {
        printf("stride = %d\n", imdataview[viewindex].stride);
        printf("ALLOCATING viewpixels %d %d\n", imdataview[viewindex].xviewsize, imdataview[viewindex].yviewsize);
        fflush(stdout);
    }

    if(finit==1) // only do this on first call
    {
        if(verbose) {
            printf("[%d]\n", __LINE__);
            fflush(stdout);
        }
        if(imdataview[viewindex].allocated_viewpixels == 1) {
            if(verbose) {
                printf("[%d]\n", __LINE__);
                fflush(stdout);
            }
            free(imdataview[viewindex].viewpixels);
            imdataview[viewindex].allocated_viewpixels = 0;
        }
    }

    if(verbose) {
        printf("[%d]\n", __LINE__);
        fflush(stdout);
    }

    imdataview[viewindex].viewpixels = calloc(imdataview[viewindex].yviewsize * imdataview[viewindex].stride, 1);
    imdataview[viewindex].allocated_viewpixels = 1;



    if(verbose) {
        printf("DONE\n");
        fflush(stdout);
    }


    imdataview[viewindex].pbview = gdk_pixbuf_new_from_data(
                                       imdataview[viewindex].viewpixels,
                                       GDK_COLORSPACE_RGB,     // colorspace
                                       0,                      // has_alpha
                                       8,                      // bits-per-sample
                                       imdataview[viewindex].xviewsize, imdataview[viewindex].yviewsize,       // cols, rows
                                       imdataview[viewindex].stride,              // rowstride
                                       free_viewpixels,            // destroy_fn
                                       NULL                    // destroy_fn_data
                                   );


    imdataview[viewindex].gtkimage = GTK_IMAGE(gtk_image_new_from_pixbuf(imdataview[viewindex].pbview));

    imdataview[viewindex].update = 1;

    if(verbose) {
        printf("EVENTBOX SIZE DONE\n");
        fflush(stdout);
    }
    finit = 1;

    return 0;
}






int update_pic() {
    static unsigned long long imcnt0 = 0;

    static guchar *Rarray = NULL;
    static guchar *Garray = NULL;
    static guchar *Barray = NULL;

    int ii, jj;

    static int xviewsize_save = -1;
    static int yviewsize_save = -1;

    static int iimin_save = -1;
    static int iimax_save = -1;
    static int jjmin_save = -1;
    static int jjmax_save = -1;

    static int xviewmin_save = -1;
    static int xviewmax_save = -1;
    static int yviewmin_save = -1;
    static int yviewmax_save = -1;


    int viewindex = 0;
    int imindex;
    //int imindexdisp;

    int xviewsize, yviewsize; // local copies

    static unsigned long viewcnt = 0;

    static int alloc_cnt = 0;




    if(imdataview[viewindex].dispmap == 1) {
        //imindexdisp = imdataview[viewindex].dispmap_imindex;
        imindex     = imdataview[viewindex].imindex;
    }
    else
    {
        //imindexdisp = imdataview[viewindex].imindex;
        imindex     = imdataview[viewindex].imindex;
    }

    if(viewcnt == 0)
        imdataview[viewindex].update = 1;




    if(imindex != -1)
    {

        if(imcnt0 != imarray[imindex].md[0].cnt0) {
            imdataview[viewindex].update = 1;
        }

        if(imdataview[viewindex].update == 1)
        {
            // get view area size
            GtkAllocation alloc;
            gtk_widget_get_allocation(widgets->imviewport, &alloc);

            GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(widgets->imviewport));
            double hval = gtk_adjustment_get_value (hadj);

            GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widgets->imviewport));
            double vval = gtk_adjustment_get_value (vadj);

            if(verbose) {
                printf("----- widget size is currently %d x %d  start at %7.2f x %7.2f\n", alloc.width, alloc.height, hval, vval);
            }

            imdataview[viewindex].xviewmin = hval;
            imdataview[viewindex].xviewmax = hval + alloc.width;

            imdataview[viewindex].yviewmin = vval;
            imdataview[viewindex].yviewmax = vval + alloc.height;


            imdataview[viewindex].iimin = (int) ( imdataview[viewindex].xviewmin / imdataview[viewindex].zoomFact );
            imdataview[viewindex].iimax = (int) ( imdataview[viewindex].xviewmax / imdataview[viewindex].zoomFact );
            imdataview[viewindex].jjmin = (int) ( imdataview[viewindex].yviewmin / imdataview[viewindex].zoomFact );
            imdataview[viewindex].jjmax = (int) ( imdataview[viewindex].yviewmax / imdataview[viewindex].zoomFact );


            //	GdkPixbuf *pb = gtk_image_get_pixbuf(imdataview[viewindex].gtkimage);



            char string_streaminfo[300];
            char string_datatype[8];

            switch ( imarray[imindex].md[0].datatype )
            {
            case _DATATYPE_FLOAT:
                strcpy(string_datatype, "FLOAT");
                break;

            case _DATATYPE_DOUBLE:
                strcpy(string_datatype, "DOUBLE");
                break;

            case _DATATYPE_UINT8:
                strcpy(string_datatype, "UINT8");
                break;

            case _DATATYPE_UINT16:
                strcpy(string_datatype, "UINT16");
                break;

            case _DATATYPE_UINT32:
                strcpy(string_datatype, "UINT32");
                break;

            case _DATATYPE_UINT64:
                strcpy(string_datatype, "UINT64");
                break;


            case _DATATYPE_INT8:
                strcpy(string_datatype, "INT8");
                break;

            case _DATATYPE_INT16:
                strcpy(string_datatype, "INT16");
                break;

            case _DATATYPE_INT32:
                strcpy(string_datatype, "INT32");
                break;

            case _DATATYPE_INT64:
                strcpy(string_datatype, "INT64");
                break;
            }












            // optional information display
            if(imdataview[viewindex].view_streaminfo == 1) {
                sprintf(string_streaminfo, "[%d] %s %s %d x %d\nmin/max: %.2g - %.2g\nzoom: %.2f\nscale: %.3f %3f",
                        viewindex,
                        imdataview[viewindex].imname,
                        string_datatype,
                        imdataview[viewindex].xsize,
                        imdataview[viewindex].ysize,
                        imdataview[viewindex].vmin,
                        imdataview[viewindex].vmax,
                        imdataview[viewindex].zoomFact,
                        imdataview[viewindex].bscale_center,
                        imdataview[viewindex].bscale_slope
                       );
                gtk_label_set_text ( GTK_LABEL(widgets->label_streaminfo), string_streaminfo);
            }
            else {
                gtk_label_set_text ( GTK_LABEL(widgets->label_streaminfo), "");
            }


            if(imdataview[viewindex].view_imstat == 1) {
                char stringinfo[200];
                sprintf(stringinfo, "TOTAL: %.2g",
                        1.23
                       );
                gtk_label_set_text ( GTK_LABEL(widgets->label_imstat), stringinfo);
            }
            else {
                gtk_label_set_text ( GTK_LABEL(widgets->label_imstat), "");
            }


            if(imdataview[viewindex].view_timing == 1) {
                char stringinfo[200];
                sprintf(stringinfo, "cnt0: %ld",
                        imarray[imindex].md[0].cnt0
                       );
                gtk_label_set_text ( GTK_LABEL(widgets->label_timing), stringinfo);
            }
            else {
                gtk_label_set_text ( GTK_LABEL(widgets->label_timing), "");
            }







            imcnt0 = imarray[imindex].md[0].cnt0;



            guchar *pbviewarray = gdk_pixbuf_get_pixels(imdataview[viewindex].pbview);

            //imdataview[0].zoomFact = 1.0 * imdataview[0].xsize / (imdataview[0].x1view - imdataview[0].x0view);



			// For convenience, copy to local variables
			
            xviewsize = imdataview[viewindex].xviewsize;
            yviewsize = imdataview[viewindex].yviewsize;

			int xsize = imdataview[viewindex].xsize;
			int ysize = imdataview[viewindex].ysize;
			long xysize = xsize*ysize;

			int imid = 0;

            if(verbose) {
                printf("============================================\n");
                printf(" ----- %10lu ----------------\n",    viewcnt);
                printf("dispmap                     : %d\n", imdataview[viewindex].dispmap);
                printf("\n");
                printf("STREAM:\n");
                printf("Image stream size           = %4d %4d\n", xsize, ysize);
                printf("\n");
                printf("VIEW:\n");
                printf("xviewsize                   = %d\n", xviewsize);
                printf("yviewsize                   = %d\n", yviewsize);
                printf("\n");
                //printf("xsizedisp                   = %d\n", imdataview[viewindex].xsizedisp);
                //printf("ysizedisp                   = %d\n", imdataview[viewindex].ysizedisp);
                printf("\n");
                printf("ACTIVE VIEW AREA X xview    = %4d - %4d (size = %4d)\n", imdataview[viewindex].xviewmin, imdataview[viewindex].xviewmax, alloc.width);
                printf("ACTIVE VIEW AREA Y yview    = %4d - %4d (size = %4d)\n", imdataview[viewindex].yviewmin, imdataview[viewindex].yviewmax, alloc.height);
                printf("\n");
                printf("VIEW  zoomFact              = %f\n", imdataview[viewindex].zoomFact);
                printf("ACTIVE PIX AREA ii          = %4ld - %4ld\n", imdataview[viewindex].iimin, imdataview[viewindex].iimax);
                printf("ACTIVE PIX AREA jj          = %4ld - %4ld\n", imdataview[viewindex].jjmin, imdataview[viewindex].jjmax);
                printf("============================================\n");
                fflush(stdout);
            }

            viewcnt++;













            if(imdataview[viewindex].computearrayinit == 0)
            {
                if(verbose) {
                    printf("INIT ALLOCATE\n");
                }

                imdataview[viewindex].computearray   = (float*) malloc(sizeof(float) * xysize);
                imdataview[viewindex].computearray16 = (uint16_t*) malloc(sizeof(uint16_t) * xysize);

                if(Rarray != NULL) {
                    free(Rarray);
                    Rarray = NULL;
                }
                Rarray = (guchar*) malloc(sizeof(guchar) * xysize);

                if(Garray != NULL) {
                    free(Garray);
                    Garray = NULL;
                }
                Garray = (guchar*) malloc(sizeof(guchar) * xysize);

                if(Barray != NULL) {
                    free(Barray);
                    Barray = NULL;
                }
                Barray = (guchar*) malloc(sizeof(guchar) * xysize);

                alloc_cnt += 1;

                if(imdataview[viewindex].PixelIndexRaw_array != NULL) {
                    free(imdataview[viewindex].PixelIndexRaw_array);
                    imdataview[viewindex].PixelIndexRaw_array = NULL;
                }
                imdataview[viewindex].PixelIndexRaw_array = (long*) malloc(sizeof(long) * xviewsize * yviewsize);

                if(imdataview[viewindex].PixelIndexViewBuff_array != NULL) {
                    free(imdataview[viewindex].PixelIndexViewBuff_array);
                    imdataview[viewindex].PixelIndexViewBuff_array = NULL;
                }
                imdataview[viewindex].PixelIndexViewBuff_array = (int*) malloc(sizeof(int) * xviewsize * yviewsize);

                imdataview[viewindex].computearrayinit = 1;
            }
            else
            {
                if(verbose) {
                    printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                    fflush(stdout);
                }
                // has view size changed ?
                if( (xviewsize != xviewsize_save)
                        || (yviewsize != yviewsize_save))
                {

                    if(imdataview[viewindex].PixelIndexRaw_array != NULL) {
                        free(imdataview[viewindex].PixelIndexRaw_array);
                    }
                    imdataview[viewindex].PixelIndexRaw_array = (long*) malloc(sizeof(long) * xviewsize * yviewsize);

                    if(verbose) {
                        printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                        fflush(stdout);
                    }

                    if(imdataview[viewindex].PixelIndexViewBuff_array != NULL) {
                        free(imdataview[viewindex].PixelIndexViewBuff_array);
                    }
                    imdataview[viewindex].PixelIndexViewBuff_array = (int*) malloc(sizeof(int) * xviewsize * yviewsize);

                    alloc_cnt -= 1;
                    if(verbose) {
                        printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                        fflush(stdout);
                    }
                }
                if(verbose) {
                    printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                    fflush(stdout);
                }
            }


            //
            // If view window changed, recompute mapping from original image to view
            //
            if( (xviewsize != xviewsize_save)
                    || (xviewsize != xviewsize_save)
                    || (imdataview[viewindex].iimin != iimin_save)
                    || (imdataview[viewindex].iimax != iimax_save)
                    || (imdataview[viewindex].jjmin != jjmin_save)
                    || (imdataview[viewindex].jjmax != jjmax_save)
                    || (imdataview[viewindex].xviewmin != xviewmin_save)
                    || (imdataview[viewindex].xviewmax != xviewmax_save)
                    || (imdataview[viewindex].yviewmin != yviewmin_save)
                    || (imdataview[viewindex].yviewmax != yviewmax_save))
            {
                if(verbose) {
                    printf("------------ RECOMPUTING ARRAY TRANSF\n");
                    fflush(stdout);
                }

                // if window changed, recompute mapping between screen pixel and image pixel
                int xviewrange = imdataview[viewindex].xviewmax - imdataview[viewindex].xviewmin;
                int yviewrange = imdataview[viewindex].yviewmax - imdataview[viewindex].yviewmin;

                if(verbose) {
                    printf("xviewrange = %d\n", xviewrange);
                    printf("yviewrange = %d\n", yviewrange);
                    fflush(stdout);
                }

                //int iirange = imdataview[viewindex].iimax - imdataview[viewindex].iimin;
                //int jjrange = imdataview[viewindex].jjmax - imdataview[viewindex].jjmin;


                if(verbose) {
                    printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                    fflush(stdout);
                }


                // initialize mapping from view to view buffer
                for(int yview = 0; yview < yviewsize; yview++)
                    for (int xview = 0; xview < xviewsize; xview++)
                    {
                        int pixindexView = yview * xviewsize + xview;

                        int pixindexBuff = yview * imdataview[viewindex].stride + xview * BYTES_PER_PIXEL;

                        imdataview[viewindex].PixelIndexRaw_array[pixindexView]      = -1;
                        imdataview[viewindex].PixelIndexViewBuff_array[pixindexView] = pixindexBuff;
                    }

                if(verbose) {
                    printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                    printf("             xview %d - %d\n", imdataview[viewindex].xviewmin, imdataview[viewindex].xviewmax);
                    printf("             yview %d - %d\n", imdataview[viewindex].yviewmin, imdataview[viewindex].yviewmax);
                    fflush(stdout);
                }


                for (int yview = imdataview[viewindex].yviewmin; yview < imdataview[viewindex].yviewmax; yview++) {
                    if((yview >= 0) && (yview < yviewsize)) {
                        for (int xview = imdataview[viewindex].xviewmin; xview < imdataview[viewindex].xviewmax; xview++) {
                            if((xview >= 0)&&(xview < xviewsize)) {
                                long pixindexRaw;
                                long pixindexView;

                                pixindexView = yview * xviewsize + xview;



                                ii = (int) ( xview / imdataview[viewindex].zoomFact );
                                jj = (int) ( yview / imdataview[viewindex].zoomFact );
                                pixindexRaw = jj*imdataview[viewindex].xsize + ii;

                                if(ii<0) {
                                    ii = 0;
                                    pixindexRaw = -1;
                                }
                                else if(jj<0) {
                                    jj = 0;
                                    pixindexRaw = -1;
                                }
                                else if(ii > imdataview[viewindex].xsize-1) {
                                    ii = imdataview[viewindex].xsize-1;
                                    pixindexRaw = -1;
                                }
                                else if(jj > imdataview[viewindex].ysize-1) {
                                    jj = imdataview[viewindex].ysize-1;
                                    pixindexRaw = -1;
                                }


                                if(imdataview[viewindex].dispmap == 1) {
                                    pixindexRaw = imarray[imdataview[viewindex].dispmap_imindex].array.SI32[jj*imdataview[viewindex].xsize+ii];
                                }
                                imdataview[viewindex].PixelIndexRaw_array[pixindexView] = pixindexRaw;
                            }
                        }
                    }
                }

                if(verbose) {
                    printf("------------ DONE RECOMPUTING ARRAY TRANSF\n");
                    fflush(stdout);
                }
            }


            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                fflush(stdout);
            }

            xviewsize_save = xviewsize;
            yviewsize_save = yviewsize;
            iimin_save = imdataview[viewindex].iimin;
            iimax_save = imdataview[viewindex].iimax;
            jjmin_save = imdataview[viewindex].jjmin;
            jjmax_save = imdataview[viewindex].jjmax;
            xviewmin_save = imdataview[viewindex].xviewmin;
            xviewmax_save = imdataview[viewindex].xviewmax;
            yviewmin_save = imdataview[viewindex].yviewmin;
            yviewmax_save = imdataview[viewindex].yviewmax;

            //            printf("------------ [%5d]\n", __LINE__);
            //            fflush(stdout);



            //printf("[%4d]  %d %d    %d %d\n", __LINE__, iimin, iimax, jjmin, jjmax);

            // Fill in imdataview[viewindex].computearray
            // this is the image area over which things will be computed

            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                printf("             ii   %5ld - %5ld\n", imdataview[viewindex].iimin, imdataview[viewindex].iimax);
                printf("             jj   %5ld - %5ld\n", imdataview[viewindex].jjmin, imdataview[viewindex].jjmax);
                fflush(stdout);
            }

			
			
			// compute valid ii and jj range
			
			int iimin = 0;
			if(imdataview[viewindex].iimin > iimin) {
				iimin = imdataview[viewindex].iimin;
			}
			int iimax = imdataview[viewindex].xsize;
			if(imdataview[viewindex].iimax < iimax) {
				iimax = imdataview[viewindex].iimax;
			}
			int jjmin = 0;
			if(imdataview[viewindex].jjmin > jjmin) {
				jjmin = imdataview[viewindex].jjmin;
			}
			int jjmax = imdataview[viewindex].ysize;
			if(imdataview[viewindex].jjmax < jjmax) {
				jjmax = imdataview[viewindex].jjmax;
			}



            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                printf("             ii   %5d - %5d\n", iimin, iimax);
                printf("             jj   %5d - %5d\n", jjmin, jjmax);
                fflush(stdout);
            }


            
            switch ( imarray[imindex].md[0].datatype )
            {
            case _DATATYPE_FLOAT:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = imarray[imindex].array.F[jj*xsize+ii];
                break;

            case _DATATYPE_DOUBLE:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = imarray[imindex].array.D[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_UINT8:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.UI8[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_UINT16:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.UI16[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_UINT32:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.UI32[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_UINT64:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.UI64[imid*xysize+jj*xsize+ii];
                break;


            case _DATATYPE_INT8:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.SI8[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_INT16:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.SI16[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_INT32:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.SI32[imid*xysize+jj*xsize+ii];
                break;

            case _DATATYPE_INT64:
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                        imdataview[viewindex].computearray[jj*xsize+ii] = (float) imarray[imindex].array.SI64[imid*xysize+jj*xsize+ii];
                break;
            }

            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                fflush(stdout);
            }

            // Recompute min and max scale if needed
            if(imdataview[viewindex].update_minmax == 1)
            {
                float *varray = NULL;
                int nbpix;

                nbpix = (iimax - iimin) * (jjmax - jjmin);

                if(verbose) {
                    printf("------------ [%5d %s] nbpix = %d\n", __LINE__, __FILE__, nbpix);
                    fflush(stdout);
                }

                varray = (float*) malloc(sizeof(float) * nbpix);


                if(verbose) {
                    printf("------------ [%5d %s] nbpix = %d\n", __LINE__, __FILE__, nbpix);
                    fflush(stdout);
                }

                int pixindex = 0;
                for(ii=iimin; ii<iimax; ii++)
                    for(jj=jjmin; jj<jjmax; jj++)
                    {
                        varray[pixindex] = imdataview[viewindex].computearray[jj*xsize+ii];
                        pixindex ++;
                    }

                qsort(varray, nbpix, sizeof(float), floatcompare);

                int iipixmin = (int) (nbpix * imdataview[viewindex].scale_range_perc);
                if(iipixmin < 0) {
                    iipixmin = 0;
                }

                int iipixmax = (int) (nbpix * (1.0-imdataview[viewindex].scale_range_perc));
                if(iipixmax > nbpix-1) {
                    iipixmax = nbpix -1;
                }

                imdataview[viewindex].vmin = varray[iipixmin];
                imdataview[viewindex].vmax = varray[iipixmax];

                free(varray);

                imdataview[viewindex].update_minmax = 0;
            }


            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                fflush(stdout);
            }


            // compute R, G, B values
            for (ii=0; ii<xysize; ii++)
            {
                Rarray[ii] = 0;
                Garray[ii] = 0;
                Barray[ii] = 0;
            }
            for(jj=jjmin; jj<jjmax; jj++)
                for(ii=iimin; ii<iimax; ii++)
                {
                    float pixval;
                    int pixindex;

                    pixindex = jj*xsize+ii;
                    pixval = imdataview[viewindex].computearray[pixindex];
                    pixval = (pixval - imdataview[viewindex].vmin) / (imdataview[viewindex].vmax - imdataview[viewindex].vmin);
                    imdataview[viewindex].computearray[pixindex] = pixval;
                    imdataview[viewindex].computearray16[pixindex] = (uint16_t) (pixval*65535);
                    if(pixval<0.0) {
                        imdataview[viewindex].computearray16[pixindex] = 0;
                    }
                    if(pixval >= 1.0) {
                        imdataview[viewindex].computearray16[pixindex] = 65535;
                    }
                }



            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                fflush(stdout);
            }

            // apply colormap

            for(jj=jjmin; jj<jjmax; jj++)
                for(ii=iimin; ii<iimax; ii++)
                {
                    int pixindex;

                    pixindex = jj*xsize+ii;
                    uint16_t pixval16 = imdataview[viewindex].computearray16[pixindex];

                    Rarray[pixindex] = imdataview[viewindex].COLORMAP_RVAL[pixval16];
                    Garray[pixindex] = imdataview[viewindex].COLORMAP_GVAL[pixval16];
                    Barray[pixindex] = imdataview[viewindex].COLORMAP_BVAL[pixval16];

                    //PixVal_to_RGB( pixval, &Rarray[pixindex], &Garray[pixindex], &Barray[pixindex]);
                }


            if(verbose) {
                printf("------------ [%5d %s]  xviewsize = %d  yviewsize = %d\n", __LINE__, __FILE__, xviewsize, yviewsize);
                fflush(stdout);
            }



            // push to view - this is where mapping occurs

			// compute valid xview and yview range
			
			int xviewmin = 0;
			if(imdataview[viewindex].xviewmin > xviewmin) {
				iimin = imdataview[viewindex].xviewmin;
			}
			int xviewmax = imdataview[viewindex].xviewsize;
			if(imdataview[viewindex].xviewmax < xviewmax) {
				xviewmax = imdataview[viewindex].xviewmax;
			}
			int yviewmin = 0;
			if(imdataview[viewindex].yviewmin > yviewmin) {
				yviewmin = imdataview[viewindex].yviewmin;
			}
			int yviewmax = imdataview[viewindex].yviewsize;
			if(imdataview[viewindex].yviewmax < yviewmax) {
				yviewmax = imdataview[viewindex].yviewmax;
			}



            for(int xview = xviewmin; xview < xviewmax; xview++)
                for(int yview = yviewmin; yview < yviewmax; yview++)
                {
                    int pixindexView = yview * xviewsize + xview;

                    long pixindexRaw = imdataview[viewindex].PixelIndexRaw_array[pixindexView];
                    int pixindexPb = imdataview[viewindex].PixelIndexViewBuff_array[pixindexView];

                    // if(pixindex != -1) {

                    if(pixindexRaw == -2) {
                        pbviewarray[pixindexPb] = 70;
                        pbviewarray[pixindexPb+1] = 0;
                        pbviewarray[pixindexPb+2] = 0;
                    }
                    else if(pixindexRaw == -1) {
                        pbviewarray[pixindexPb] = 0;
                        pbviewarray[pixindexPb+1] = 70;
                        pbviewarray[pixindexPb+2] = 0;
                    }
                    else
                    {
                        pbviewarray[pixindexPb] = Rarray[pixindexRaw];
                        pbviewarray[pixindexPb+1] = Garray[pixindexRaw];
                        pbviewarray[pixindexPb+2] = Barray[pixindexRaw];
                    }
                    //}

                }

            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                fflush(stdout);
            }

            gtk_image_set_from_pixbuf(GTK_IMAGE(widgets->w_img_main), imdataview[viewindex].pbview);

            if(verbose) {
                printf("------------ [%5d %s]\n", __LINE__, __FILE__);
                fflush(stdout);
            }
        }
        imdataview[viewindex].update = 0;
    }




    return TRUE;
}



