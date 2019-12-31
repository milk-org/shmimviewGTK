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









void free_viewpixels(guchar *pixels, __attribute__((unused)) gpointer data) {
    free(pixels);
}


int resize_PixelBufferView(int xsize, int ysize)
{
    int viewindex = 0;

    static int finit = 0;

    imdataview[viewindex].viewXsize = xsize;
    imdataview[viewindex].viewYsize = ysize;

    // (re-)allocate pixel buffer

    imdataview[viewindex].stride = imdataview[viewindex].viewXsize * BYTES_PER_PIXEL;
    imdataview[viewindex].stride += (4 - imdataview[viewindex].stride % 4) % 4; // ensure multiple of 4

    if(verbose) {
        printf("stride = %d\n", imdataview[viewindex].stride);
        printf("ALLOCATING viewpixels %d %d\n", imdataview[viewindex].viewXsize, imdataview[viewindex].viewYsize);
        fflush(stdout);
    }

    if(finit==1) // only do this on first call
    {
        if(imdataview[viewindex].allocated_viewpixels == 1) {
            free(imdataview[viewindex].viewpixels);
        }
    }

    imdataview[viewindex].viewpixels = calloc(imdataview[viewindex].viewYsize * imdataview[viewindex].stride, 1);
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
                                       imdataview[viewindex].viewXsize, imdataview[viewindex].viewYsize,       // cols, rows
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

    static guchar *Rarray;
    static guchar *Garray;
    static guchar *Barray;

    int ii, jj;

    static int viewXsize_save = -1;
    static int viewYsize_save = -1;

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
    int imindexdisp;

    int viewXsize, viewYsize; // local copies

    static unsigned long viewcnt = 0;

    static int alloc_cnt = 0;



	if(imdataview[viewindex].dispmap == 1) {
		imindexdisp = imdataview[viewindex].dispmap_imindex;
		imindex     = imdataview[viewindex].imindex;
	}
	else
	{
		imindexdisp = imdataview[viewindex].imindex;
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

            //	GdkPixbuf *pb = gtk_image_get_pixbuf(imdataview[viewindex].gtkimage);

            if(verbose) {
                printf("============================================\n");
                printf(" ----- %10lu ----------------\n", viewcnt);
                printf("\n");
                printf("MAX VIEW  viewXsize         = %d\n", imdataview[viewindex].viewXsize);
                printf("MAX VIEW  viewYsize         = %d\n", imdataview[viewindex].viewYsize);
                printf("\n");
                printf("ACTIVE VIEW AREA X xview    = %4d - %4d\n", imdataview[viewindex].xviewmin, imdataview[viewindex].xviewmax);
                printf("ACTIVE VIEW AREA Y yview    = %4d - %4d\n", imdataview[viewindex].yviewmin, imdataview[viewindex].yviewmax);
                printf("\n");
                printf("VIEW  zoomFact              = %f\n", imdataview[viewindex].zoomFact);
                printf("ACTIVE PIX AREA ii          = %4ld - %4ld\n", imdataview[viewindex].iimin, imdataview[viewindex].iimax);
                printf("ACTIVE PIX AREA jj          = %4ld - %4ld\n", imdataview[viewindex].jjmin, imdataview[viewindex].jjmax);
                printf("============================================\n");
                fflush(stdout);
            }

            viewcnt++;


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
            int imXsizedisp = 0;
            int imYsizedisp = 0;
            int imiddisp = 0;

            if( imarray[imindexdisp].md[0].naxis == 2) {				
                imXsizedisp = imarray[imindexdisp].md[0].size[0];
                imYsizedisp = imarray[imindexdisp].md[0].size[1];
                imiddisp = 0;
            } else if( imarray[imindexdisp].md[0].naxis == 3) {
                imXsizedisp = imarray[imindexdisp].md[0].size[1];
                imYsizedisp = imarray[imindexdisp].md[0].size[2];
                imiddisp = imcnt0 % imarray[imindexdisp].md[0].size[0];
            }
            int imSizedisp = imXsizedisp * imYsizedisp;
            

            int imXsize = 0;
            int imYsize = 0;
            int imid = 0;

            if( imarray[imindex].md[0].naxis == 2) {				
                imXsize = imarray[imindex].md[0].size[0];
                imYsize = imarray[imindex].md[0].size[1];
                imid = 0;
            } else if( imarray[imindex].md[0].naxis == 3) {
                imXsize = imarray[imindex].md[0].size[1];
                imYsize = imarray[imindex].md[0].size[2];
                imid = imcnt0 % imarray[imindex].md[0].size[0];
            }
            int imSize = imXsize * imYsize;


            guchar *array = gdk_pixbuf_get_pixels(imdataview[viewindex].pbview);

            //imdataview[0].zoomFact = 1.0 * imdataview[0].xsize / (imdataview[0].x1view - imdataview[0].x0view);

            // compute min and max window coord in raw image


            viewXsize = imdataview[viewindex].viewXsize;
            viewYsize = imdataview[viewindex].viewYsize;

            if(imdataview[viewindex].computearrayinit == 0)
            {
				if(verbose) {
					printf("INIT ALLOCATE\n");
				}

                imdataview[viewindex].computearray = (float*) malloc(sizeof(float) * imSize);
                Rarray = (guchar*) malloc(sizeof(guchar) * imSize);
                Garray = (guchar*) malloc(sizeof(guchar) * imSize);
                Barray = (guchar*) malloc(sizeof(guchar) * imSize);

                alloc_cnt += 1;
                imdataview[viewindex].PixelRaw_array = (long*) malloc(sizeof(long) * viewXsize * viewYsize);
                imdataview[viewindex].PixelBuff_array = (int*) malloc(sizeof(int) * viewXsize * viewYsize);
                imdataview[viewindex].computearrayinit = 1;
            } 
            else
            {
                if(verbose) {
                    printf("------------ [%5d]\n", __LINE__);
                    fflush(stdout);
                }
                // has view size changed ?
                if( (viewXsize != viewXsize_save)
                        || (viewYsize != viewYsize_save))
                {
                    if(verbose) {
                        printf("------------ [%5d] %d alloc_cnt = %d\n", __LINE__, viewindex, alloc_cnt);
                        fflush(stdout);
                    }

                    free(imdataview[viewindex].PixelRaw_array);
                    if(verbose) {
                        printf("------------ [%5d] %d alloc_cnt = %d\n", __LINE__, viewindex, alloc_cnt);
                        fflush(stdout);
                    }
                    imdataview[viewindex].PixelRaw_array = (long*) malloc(sizeof(long) * viewXsize * viewYsize);
                    if(verbose) {
                        printf("------------ [%5d]\n", __LINE__);
                        fflush(stdout);
                    }
                    free(imdataview[viewindex].PixelBuff_array);
                    imdataview[viewindex].PixelBuff_array = (int*) malloc(sizeof(int) * viewXsize * viewYsize);

                    alloc_cnt -= 1;
                    if(verbose) {
                        printf("------------ [%5d]\n", __LINE__);
                        fflush(stdout);
                    }
                }
                if(verbose) {
                    printf("------------ [%5d]\n", __LINE__);
                    fflush(stdout);
                }
            }


			//
            // If view window changed, recompute mapping from original image to view
            //
            if( (viewXsize != viewXsize_save)
                    || (viewYsize != viewYsize_save)
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

                int iirange = imdataview[viewindex].iimax - imdataview[viewindex].iimin;
                int jjrange = imdataview[viewindex].jjmax - imdataview[viewindex].jjmin;

                int iirangedisp = imdataview[viewindex].iimaxdisp - imdataview[viewindex].iimindisp;
                int jjrangedisp = imdataview[viewindex].jjmaxdisp - imdataview[viewindex].jjmindisp;

                if(verbose) {
                    printf("------------ [%5d]\n", __LINE__);
                    fflush(stdout);
                }

                for(int yview = 0; yview < viewYsize; yview++)
                    for (int xview = 0; xview < viewXsize; xview++)
                    {
                        int pixindexView = yview * viewXsize + xview;

                        int pixindexBuff = yview * imdataview[viewindex].stride + xview * BYTES_PER_PIXEL;

                        imdataview[viewindex].PixelRaw_array[pixindexView] = -1;
                        imdataview[viewindex].PixelBuff_array[pixindexView] = pixindexBuff;
                    }

                if(verbose) {
                    printf("------------ [%5d]\n", __LINE__);
                    fflush(stdout);
                }


                for (int yview = 0; yview < yviewrange; yview++)
                    for (int xview = 0; xview < xviewrange; xview++)
                    {
                        long pixindexRaw;
                        long pixindexView;

                        pixindexView = yview * viewXsize + xview;

                        ii = (int) ( 1.0*xview/xviewrange*iirangedisp +  imdataview[viewindex].iimindisp);
                        jj = (int) ( 1.0*yview/yviewrange*jjrangedisp +  imdataview[viewindex].jjmindisp);

                        pixindexRaw = jj*imXsizedisp + ii;
                        
                        if(imdataview[viewindex].dispmap == 1) {
							pixindexRaw = imarray[imdataview[viewindex].dispmap_imindex].array.SI32[jj*imXsizedisp+ii];
						} 


                        if(ii<0) {
                            ii = 0;
                            pixindexRaw = -1;
                        }

                        if(jj<0) {
                            jj = 0;
                            pixindexRaw = -1;
                        }

                        if(ii > imdataview[viewindex].xsizedisp-1) {
                            ii = imdataview[viewindex].xsizedisp-1;
                            pixindexRaw = -1;
                        }

                        if(jj > imdataview[viewindex].ysizedisp-1) {
                            jj = imdataview[viewindex].ysizedisp-1;
                            pixindexRaw = -1;
                        }

                        imdataview[viewindex].PixelRaw_array[pixindexView] = pixindexRaw;
                    }

                if(verbose) {
                    printf("------------ DONE RECOMPUTING ARRAY TRANSF\n");
                    fflush(stdout);
                }
            }


            viewXsize_save = viewXsize;
            viewYsize_save = viewYsize;
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



            switch ( imarray[imindex].md[0].datatype )
            {
            case _DATATYPE_FLOAT:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = imarray[imindex].array.F[jj*imXsize+ii];
                break;

            case _DATATYPE_DOUBLE:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = imarray[imindex].array.D[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_UINT8:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.UI8[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_UINT16:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.UI16[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_UINT32:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.UI32[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_UINT64:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.UI64[imid*imSize+jj*imXsize+ii];
                break;


            case _DATATYPE_INT8:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.SI8[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_INT16:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.SI16[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_INT32:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.SI32[imid*imSize+jj*imXsize+ii];
                break;

            case _DATATYPE_INT64:
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                        imdataview[viewindex].computearray[jj*imXsize+ii] = (float) imarray[imindex].array.SI64[imid*imSize+jj*imXsize+ii];
                break;
            }


            //           printf("------------ [%5d]\n", __LINE__);
            //           fflush(stdout);


            // Recompute min and max scale if needed
            if(imdataview[viewindex].autominmax == 1)
            {
                float *varray;
                int nbpix;

                nbpix = (imdataview[viewindex].iimax - imdataview[viewindex].iimin) * (imdataview[viewindex].jjmax - imdataview[viewindex].jjmin);

                varray = (float*) malloc(sizeof(float) * nbpix);

                int pixindex = 0;
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                    for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                    {
                        varray[pixindex] = imdataview[viewindex].computearray[jj*imXsize+ii];
                        pixindex ++;
                    }

                qsort(varray, nbpix, sizeof(float), floatcompare);

                int iimin = (int) (nbpix * imdataview[viewindex].scale_range_perc);
                if(iimin < 0) {
                    iimin = 0;
                }

                int iimax = (int) (nbpix * (1.0-imdataview[viewindex].scale_range_perc));
                if(iimax > nbpix-1) {
                    iimax = nbpix -1;
                }


                imdataview[viewindex].vmin = varray[iimin];
                imdataview[viewindex].vmax = varray[iimax];

                free(varray);
            }



            //            printf("------------ [%5d]\n", __LINE__);
            //            fflush(stdout);


            // compute R, G, B values
            for (ii=0; ii<imYsize*imXsize; ii++)
            {
                Rarray[ii] = 0;
                Garray[ii] = 0;
                Barray[ii] = 0;
            }
            for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                {
                    float pixval;
                    int pixindex;

                    pixindex = jj*imXsize+ii;
                    pixval = imdataview[viewindex].computearray[pixindex];
                    pixval = (pixval - imdataview[viewindex].vmin) / (imdataview[viewindex].vmax - imdataview[viewindex].vmin);
                    imdataview[viewindex].computearray[pixindex] = pixval;
                }








            for(jj=imdataview[viewindex].jjmin; jj<imdataview[viewindex].jjmax; jj++)
                for(ii=imdataview[viewindex].iimin; ii<imdataview[viewindex].iimax; ii++)
                {
                    float pixval;
                    int pixindex;

                    pixindex = jj*imXsize+ii;
                    pixval = imdataview[viewindex].computearray[pixindex];
                    PixVal_to_RGB( pixval, &Rarray[pixindex], &Garray[pixindex], &Barray[pixindex]);
                }


            if(verbose) {
                printf("------------ [%5d]\n", __LINE__);
                fflush(stdout);
            }


            for(int xview = 0; xview < viewXsize; xview++)
                for(int yview = 0; yview < viewYsize; yview++)
                {
                    int pixindexView = yview * viewXsize + xview;

                    long pixindexRaw = imdataview[viewindex].PixelRaw_array[pixindexView];
                    int pixindexPb = imdataview[viewindex].PixelBuff_array[pixindexView];

                    // if(pixindex != -1) {
                    if(pixindexRaw == -1) {
                        array[pixindexPb] = 0;
                        array[pixindexPb+1] = 70;
                        array[pixindexPb+2] = 0;
                    }
                    else
                    {
                        array[pixindexPb] = Rarray[pixindexRaw];
                        array[pixindexPb+1] = Garray[pixindexRaw];
                        array[pixindexPb+2] = Barray[pixindexRaw];
                    }
                    //}

                }

            if(verbose) {
                printf("------------ [%5d]\n", __LINE__);
                fflush(stdout);
            }

            //gtk_image_set_from_pixbuf(GTK_IMAGE(imdataview[viewindex].gtkimage), pb);
            gtk_image_set_from_pixbuf(GTK_IMAGE(widgets->w_img_main), imdataview[viewindex].pbview);

            if(verbose) {
                printf("------------ [%5d]\n", __LINE__);
                fflush(stdout);
            }
        }
        imdataview[viewindex].update = 0;
    }

    return TRUE;
}



