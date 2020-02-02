#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <errno.h>

#include "ImageStruct.h"
#include "ImageStreamIO.h"

#include "Config.h"

#include "shmimview.h"
#include "shmimview-view.h"
#include "shmimview-scale.h"
#include "shmimview-process.h"


#define GETTEXT_PACKAGE "gtk20"


#define BYTES_PER_PIXEL 3


// GTK widget pointers
app_widgets *widgets;


// Image viewing data
IMAGEDATAVIEW *imdataview;

// color map data
COLORMAPDATA cmapdata;


// Images
char SHARED_MEMORY_DIRECTORY[200];
IMAGE *imarray;





static gint zoom = 2;
gboolean verbose = FALSE;





int precompute_cmap()
{
    int NBlevel = 65536;

    cmapdata.NBlevel = NBlevel; // 16 bit
    int viewindex = 0;


    // GREY (default)
    if ( cmapdata.colormapinit == 0 ) {
        cmapdata.COLORMAP_GREY_RVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_GREY_GVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_GREY_BVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);        
    }
    for(int clevel=0; clevel<NBlevel; clevel++) {
        float pixval = 1.0*clevel/NBlevel;

        pixval = imdataview[viewindex].scalefunc( pixval, imdataview[viewindex].scale_coeff);

        cmapdata.COLORMAP_GREY_RVAL[clevel] = (unsigned char) (pixval*255);
        cmapdata.COLORMAP_GREY_GVAL[clevel] = (unsigned char) (pixval*255);
        cmapdata.COLORMAP_GREY_BVAL[clevel] = (unsigned char) (pixval*255);
    }



    // COOL
    if ( cmapdata.colormapinit == 0 ) {
        cmapdata.COLORMAP_COOL_RVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_COOL_GVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_COOL_BVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);        
    }
    for(int clevel=0; clevel<NBlevel; clevel++) {
        float pixval = 1.0*clevel/NBlevel;

        pixval = imdataview[viewindex].scalefunc( pixval, imdataview[viewindex].scale_coeff);

        cmapdata.COLORMAP_COOL_RVAL[clevel] = (unsigned char) (pixval*pixval*255);
        cmapdata.COLORMAP_COOL_GVAL[clevel] = (unsigned char) (pixval*sqrt(pixval)*255);
        cmapdata.COLORMAP_COOL_BVAL[clevel] = (unsigned char) (sqrt(pixval)*255);
    }



    // HEAT
    if ( cmapdata.colormapinit == 0 ) {
        cmapdata.COLORMAP_HEAT_RVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_HEAT_GVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_HEAT_BVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);        
    }
    for(int clevel=0; clevel<NBlevel; clevel++) {
        float pixval = 1.0*clevel/NBlevel;

        pixval = imdataview[viewindex].scalefunc( pixval, imdataview[viewindex].scale_coeff);

        cmapdata.COLORMAP_HEAT_RVAL[clevel] = (unsigned char) (sqrt(pixval)*255);;
        cmapdata.COLORMAP_HEAT_GVAL[clevel] = (unsigned char) (pixval*sqrt(pixval)*255);
        cmapdata.COLORMAP_HEAT_BVAL[clevel] = (unsigned char) (pixval*pixval*255);
    }




    // BRY
    if ( cmapdata.colormapinit == 0 ) {
        cmapdata.COLORMAP_BRY_RVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_BRY_GVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_BRY_BVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);        
    }    
    float xlim = 0.25;
    for(int clevel=0; clevel<NBlevel; clevel++) {
        float pixval = 1.0*clevel/NBlevel;

        pixval = imdataview[viewindex].scalefunc( pixval, imdataview[viewindex].scale_coeff);

        if(pixval<xlim) {
            cmapdata.COLORMAP_BRY_RVAL[clevel] = (unsigned char) 0;
            cmapdata.COLORMAP_BRY_GVAL[clevel] = (unsigned char) 0;
        }
        else {
            float x = (pixval-xlim)/(1.0-xlim);
            float x2 = x*x;
            cmapdata.COLORMAP_BRY_RVAL[clevel] = (unsigned char) ( sqrt(x) * 255);
            cmapdata.COLORMAP_BRY_GVAL[clevel] = (unsigned char) ( x2 * 255);
        }
        cmapdata.COLORMAP_BRY_BVAL[clevel] = (unsigned char) ( (0.5-0.5*cos(pixval*M_PI*3)) * 255);
    }





    // RGB
    if ( cmapdata.colormapinit == 0 ) {
        cmapdata.COLORMAP_RGB_RVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_RGB_GVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);
        cmapdata.COLORMAP_RGB_BVAL = (unsigned char *) malloc(sizeof(unsigned char) * NBlevel);        
    }    
    for(int clevel=0; clevel<NBlevel; clevel++) {
        float pixval = 1.0*clevel/NBlevel;

        pixval = imdataview[viewindex].scalefunc( pixval, imdataview[viewindex].scale_coeff);

        if(pixval < 0.5) {
            cmapdata.COLORMAP_RGB_RVAL[clevel] = (unsigned char) ( 255.0 * (1.0-4.0*fabs(pixval-0.25)) );
            cmapdata.COLORMAP_RGB_GVAL[clevel] = (unsigned char) (pixval*2.0*255);
            cmapdata.COLORMAP_RGB_BVAL[clevel] = (unsigned char) (0);
        }
        else
        {
            cmapdata.COLORMAP_RGB_RVAL[clevel] = (unsigned char) (0);
            cmapdata.COLORMAP_RGB_GVAL[clevel] = (unsigned char) (255.0*(2.0-pixval*2.0));
            cmapdata.COLORMAP_RGB_BVAL[clevel] = (unsigned char) ((pixval-0.5)*2.0*255);
        }

    }


	cmapdata.colormapinit = 1;

    return 0;
}



int free_cmap()
{
	free(cmapdata.COLORMAP_GREY_RVAL);
	free(cmapdata.COLORMAP_GREY_GVAL);
	free(cmapdata.COLORMAP_GREY_BVAL);
	
	free(cmapdata.COLORMAP_COOL_RVAL);
	free(cmapdata.COLORMAP_COOL_GVAL);
	free(cmapdata.COLORMAP_COOL_BVAL);

	free(cmapdata.COLORMAP_HEAT_RVAL);
	free(cmapdata.COLORMAP_HEAT_GVAL);
	free(cmapdata.COLORMAP_HEAT_BVAL);	

	free(cmapdata.COLORMAP_BRY_RVAL);
	free(cmapdata.COLORMAP_BRY_GVAL);
	free(cmapdata.COLORMAP_BRY_BVAL);	

	free(cmapdata.COLORMAP_RGB_RVAL);
	free(cmapdata.COLORMAP_RGB_GVAL);
	free(cmapdata.COLORMAP_RGB_BVAL);	
	
	return 0;
}





int get_shmimdir(char *shmdirname)
{
    // SHM directory to store shared memory
    //
    // If MILK_SHM_DIR environment variable exists, use it
    // If fails, print warning, use SHAREDMEMDIR defined in ImageStruct.h
    // If fails -> use /tmp
    //
    int shmdirOK = 0; // toggles to 1 when directory is found
    DIR *tmpdir;

    // first, we try the env variable if it exists
    char *MILK_SHM_DIR = getenv("MILK_SHM_DIR");
    if(MILK_SHM_DIR != NULL) {
        if(verbose) {
            printf(" [ MILK_SHM_DIR ] '%s'\n", MILK_SHM_DIR);
        }

        sprintf(shmdirname, "%s", MILK_SHM_DIR);

        // does this direcory exist ?
        tmpdir = opendir(shmdirname);
        if(tmpdir) { // directory exits
            shmdirOK = 1;
            closedir(tmpdir);
            if(verbose) {
                printf("    Using SHM directory %s\n", shmdirname);
            }
        } else {
            printf("%c[%d;%dm", (char) 27, 1, 31); // set color red
            printf("    ERROR: Directory %s : %s\n", shmdirname, strerror(errno));
            printf("%c[%d;m", (char) 27, 0); // unset color red
            exit(EXIT_FAILURE);
        }
    } else {
        printf("%c[%d;%dm", (char) 27, 1, 31); // set color red
        printf("    WARNING: Environment variable MILK_SHM_DIR not specified -> falling back to default %s\n", SHAREDMEMDIR);
        printf("    BEWARE : Other milk users may be using the same SHM directory on this machine, and could see your milk session data and temporary files\n");
        printf("    BEWARE : Some scripts may rely on MILK_SHM_DIR to find/access shared memory and temporary files, and WILL not run.\n");
        printf("             Please set MILK_SHM_DIR and restart CLI to set up user-specific shared memory and temporary files\n");
        printf("             Example: Add \"export MILK_SHM_DIR=/milk/shm\" to .bashrc\n");
        printf("%c[%d;m", (char) 27, 0); // unset color red
    }

    // second, we try SHAREDMEMDIR default
    if(shmdirOK == 0) {
        tmpdir = opendir(SHAREDMEMDIR);
        if(tmpdir) { // directory exits
            sprintf(shmdirname, "%s", SHAREDMEMDIR);
            shmdirOK = 1;
            closedir(tmpdir);
            printf("    Using SHM directory %s\n", shmdirname);
        } else {
            printf("    Directory %s : %s\n", SHAREDMEMDIR, strerror(errno));
        }
    }

    // if all above fails, set to /tmp
    if(shmdirOK == 0) {
        tmpdir = opendir("/tmp");
        if(!tmpdir) {
            printf("    ERROR: Directory %s : %s\n", shmdirname, strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            sprintf(shmdirname, "/tmp");
            shmdirOK = 1;
            printf("    Using SHM directory %s\n", shmdirname);

            printf("    NOTE: Consider creating tmpfs directory and setting env var MILK_SHM_DIR for improved performance :\n");
            printf("        $ echo \"tmpfs %s tmpfs rw,nosuid,nodev\" | sudo tee -a /etc/fstab\n", SHAREDMEMDIR);
            printf("        $ sudo mkdir -p %s\n", SHAREDMEMDIR);
            printf("        $ sudo mount %s\n", SHAREDMEMDIR);
        }
    }

	return 0;
}





static GOptionEntry entries[] =
{
    { "zoom", 'z',    0, G_OPTION_ARG_INT,  &zoom,    "Zoom",       "Z" },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
    { NULL }
};




int main(int argc, char *argv[])
{
    GtkBuilder      *builder;
    GError          *error = NULL;
    GOptionContext *context;

    setlocale (LC_ALL, "");
    //  bindtextdomain (GETTEXT_PACKAGE, DT_DIR "/locale");
    //  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    //  textdomain (GETTEXT_PACKAGE);


    printf("SHMIM viewer version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    
    
    
    imarray = (IMAGE*) malloc(sizeof(IMAGE)*NB_IMAGES_MAX);
	for(int imindex = 0; imindex < NB_IMAGES_MAX; imindex++) {
		imarray[imindex].used = 0;
	}

    char shmdirname[200];
    get_shmimdir(shmdirname);
    strcpy(SHARED_MEMORY_DIRECTORY, shmdirname);

    context = g_option_context_new ("- view milk shared memory image stream");
            
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    if(verbose) {
        printf("zoom = %d\n", zoom);
    }

	g_option_context_free (context);




	imdataview = (IMAGEDATAVIEW*) malloc(sizeof(IMAGEDATAVIEW)*NB_IMDATAVIEW_MAX);
    for(int viewindex=0; viewindex<NB_IMDATAVIEW_MAX; viewindex++) {
        imdataview[viewindex].dispmap          = 0;

        imdataview[viewindex].imindex          = -1;
        imdataview[viewindex].computearrayinit = 0;
        imdataview[viewindex].allocated_viewpixels = 0;

        imdataview[viewindex].scale_range_perc = 0.0; // min/max
        imdataview[viewindex].scale_type       = SCALE_LINEAR;
        imdataview[viewindex].scalefunc        = *scalefunction_linear;
        imdataview[viewindex].scale_coeff      = 0.0;

        imdataview[viewindex].bscale_center    = 0.5;
        imdataview[viewindex].bscale_slope     = 1.0;

        imdataview[viewindex].view_streaminfo  = 0;
        imdataview[viewindex].view_pixinfo     = 0;
        imdataview[viewindex].view_imstat      = 0;
        imdataview[viewindex].view_timing      = 0;

		imdataview[viewindex].allocated_viewpixels = 0;
        imdataview[viewindex].viewpixels       = NULL;
        imdataview[viewindex].PixelIndexRaw_array   = NULL;
        imdataview[viewindex].PixelIndexViewBuff_array  = NULL;
    }


	// initialize color maps
	cmapdata.colormapinit = 0;
    precompute_cmap();

    for(int i=0; i<NB_IMDATAVIEW_MAX; i++) {
        // colormap
        imdataview[i].COLORMAP_RVAL = (unsigned char*) malloc(sizeof(unsigned char)*cmapdata.NBlevel);
        imdataview[i].COLORMAP_GVAL = (unsigned char*) malloc(sizeof(unsigned char)*cmapdata.NBlevel);
        imdataview[i].COLORMAP_BVAL = (unsigned char*) malloc(sizeof(unsigned char)*cmapdata.NBlevel);
        
        for(int l= 0; l< cmapdata.NBlevel; l++) {
        imdataview[i].COLORMAP_RVAL[l] = cmapdata.COLORMAP_GREY_RVAL[l];
        imdataview[i].COLORMAP_GVAL[l] = cmapdata.COLORMAP_GREY_GVAL[l];
        imdataview[i].COLORMAP_BVAL[l] = cmapdata.COLORMAP_GREY_BVAL[l];
		}
	}




    if(verbose) {
        printf("argc = %d\n", argc);
    }

    if(argc>1) {
        if(verbose) {
            char filename[200];
            sprintf(filename, "%s.im.shm", argv[1]);
            printf("Opening stream %s\n", filename);
        }
        open_shm_image(argv[1], 0);
    }




	widgets = g_slice_new(app_widgets);

	// Initialize the widget set
    gtk_init(&argc, &argv);



    widgets->pressed_button1_status = 0;
    widgets->pressed_button3_status = 0;


    char uifilename[200];
    sprintf(uifilename, "%s/glade/shmimview.glade", SOURCE_DIR);
    builder = gtk_builder_new_from_file(uifilename);


    widgets->mainwindow      = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
    widgets->imviewport      = GTK_WIDGET(gtk_builder_get_object(builder, "imviewport"));
    widgets->imscrollwindow  = GTK_WIDGET(gtk_builder_get_object(builder, "imscrollwindow"));
    widgets->imareageventbox = GTK_WIDGET(gtk_builder_get_object(builder, "imareageventbox"));
    widgets->w_img_main      = GTK_WIDGET(gtk_builder_get_object(builder, "img_main"));

    widgets->label_streaminfo = GTK_WIDGET(gtk_builder_get_object(builder, "label_streaminfo"));
    widgets->label_pixinfo    = GTK_WIDGET(gtk_builder_get_object(builder, "label_pixinfo"));
    widgets->label_imstat     = GTK_WIDGET(gtk_builder_get_object(builder, "label_imstat"));
    widgets->label_timing     = GTK_WIDGET(gtk_builder_get_object(builder, "label_timing"));


    // intensity scale


    gtk_widget_add_events( widgets->mainwindow, GDK_KEY_PRESS_MASK);



    // CONNECT TO SIGNALS

    gtk_builder_connect_signals(builder, widgets);
    //g_signal_connect(widgets->w_img_main, "size-allocate", G_CALLBACK(w_getsize), NULL);

    g_signal_connect (G_OBJECT (widgets->mainwindow), "key_press_event", G_CALLBACK (on_window_main_key_press_event), NULL);

    g_object_unref(builder);




    // empty image
    //GError **error;
    //GdkPixbuf *pbempty = gdk_pixbuf_new_from_file ("empty.png", error);
    //int viewindex = 0;
    //widgets->w_img_main = gtk_image_new_from_file ("empty.png");
    // imdataview[viewindex].gtkimage = GTK_IMAGE(gtk_image_new_from_file ("empty.png"));


    // timeout

    g_timeout_add(100,         // milliseconds
                  update_pic,  // handler function
                  NULL );        // data


    gtk_widget_show(widgets->mainwindow);
    gtk_main();

    if(verbose) {
        printf("QUIT APPLICATION\n");
        fflush(stdout);
    }




	//gtk_widget_destroy(
    g_slice_free(app_widgets, widgets);


	
	for(int imindex = 0; imindex < NB_IMAGES_MAX; imindex++)
	{
		//printf("imarray %d used = %d\n", imindex, imarray[imindex].used);
		if(imarray[imindex].used == 1) {
			//printf("Free image %d\n", imindex);
			ImageStreamIO_closeIm(&imarray[imindex]);
		}
	}


	

	
	for(int viewindex=0; viewindex<NB_IMDATAVIEW_MAX; viewindex++) {
		
		if(imdataview[viewindex].computearrayinit == 1) {
			free(imdataview[viewindex].computearray);
			free(imdataview[viewindex].computearray16);
			free(imdataview[viewindex].PixelIndexRaw_array);
			free(imdataview[viewindex].PixelIndexViewBuff_array);
			imdataview[viewindex].computearrayinit = 0;
		}
		if(imdataview[viewindex].allocated_viewpixels == 1) {
			free(imdataview[viewindex].viewpixels);
			imdataview[viewindex].allocated_viewpixels = 0;
		}

		free(imdataview[viewindex].COLORMAP_RVAL);
		free(imdataview[viewindex].COLORMAP_GVAL);
		free(imdataview[viewindex].COLORMAP_BVAL);
	}

    free(imdataview);
    free(imarray);
	free_cmap();
    
    return 0;
}









void free_pixels(guchar *pixels, __attribute__((unused)) gpointer data) {
    free(pixels);
}






int open_shm_image(
    char *streamname,
    int viewindex)
{
    char stream_filename[64];

    /*
     * An optional dispmap (display mapping) stream is used
     * to remap the input into a (usually larger) view map
     *
     * For example, the stream may consist of 10 pixel, and
     * the corresponding dispmap may be 100 x 100 pixel 2D
     * map. The pixel values in the dispmap (int) identify to
     * which zone the pixel belongs. In the display window,
     * the 100 x 100 array will be displayed, and each pixel
     * will take the value of the zone (0 to 9) to which is
     * belongs.
     * Zone values of -1 will be kept at zero.
     *
     *
     * standard name for dispmap corresponding to <streamname> is:
     * <streamname>_dispmap
     *
     */
    char dispmap_stream_filename[100];


	
    ImageStreamIO_filename(stream_filename, 64, streamname);

    char dispmap_streamname[100];
    sprintf(dispmap_streamname, "%s_dispmap", streamname);
    ImageStreamIO_filename(dispmap_stream_filename, 100, dispmap_streamname);

    if(verbose) {
        printf("Filename = %s\n", stream_filename);
        printf("dispname (optional) = %s\n", dispmap_stream_filename);
        fflush(stdout);
    }


    long imindex = 0;

    close_shm_image(viewindex);




    if(ImageStreamIO_openIm(&imarray[imindex], streamname) == -1)
    {
        printf("ERROR: cannot load stream \"%s\"\n", streamname);
        return -1;
    }
    else
    {
        if(verbose) {
            printf("--------------------------- LOADED %s %d %d into image %ld\n",
                   imarray[imindex].md[0].name,
                   imarray[imindex].md[0].size[0],
                   imarray[imindex].md[0].size[1],
                   imindex);
        }
        sprintf(imdataview[viewindex].imname, "%s", streamname);




        // look for display map

        int dispmap_index = imindex + 1;

        if( access( dispmap_stream_filename, F_OK ) != -1 ) {
            if(ImageStreamIO_openIm(&imarray[dispmap_index], dispmap_streamname) == IMAGESTREAMIO_SUCCESS)
            {
                imdataview[viewindex].dispmap = 1;
                imdataview[viewindex].dispmap_imindex = dispmap_index;
                printf("FOUND DISPLAY MAP\n");
            }
        }




        imdataview[viewindex].imindex = 0;
        imindex     = imdataview[viewindex].imindex;




        // image size
        imdataview[viewindex].naxis = imarray[imindex].md[0].naxis;
        imdataview[viewindex].xsize = imarray[imindex].md[0].size[0];
        imdataview[viewindex].ysize = imarray[imindex].md[0].size[1];



        if(verbose) {
            printf("map size = %d %d\n", imdataview[viewindex].xsize, imdataview[viewindex].ysize);
            fflush(stdout);
        }


        // start with zoom 1
		resize_PixelBufferView(imdataview[viewindex].xsize, imdataview[viewindex].ysize);


        // set pix active area in raw stream
        imdataview[viewindex].iimin = 0;
        imdataview[viewindex].iimax = imarray[imindex].md[0].size[0];
        imdataview[viewindex].jjmin = 0;
        imdataview[viewindex].jjmax = imarray[imindex].md[0].size[1];



        // we initially set zoom factor = 1

        //		imdataview[viewindex].xviewsize = imarray[index].md[0].size[0];
        //		imdataview[viewindex].vviewsize = imarray[index].md[0].size[1];
        //		printf("VIEW SIZE %d %d\n", imdataview[viewindex].xviewsize, imdataview[viewindex].yviewsize);
        //		gtk_widget_set_size_request();

        // set view active area
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].xviewsize;
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].yviewsize;


        imdataview[viewindex].vmin = 0.0;
        imdataview[viewindex].vmax = 1.0;
        imdataview[viewindex].bscale_slope = 1.0;
        imdataview[viewindex].bscale_center = 0.5;

        imdataview[viewindex].showsaturated_min = 0;
        imdataview[viewindex].showsaturated_max = 0;
        imdataview[viewindex].zoomFact = 1;

        imdataview[viewindex].update = 1;
        imdataview[viewindex].update_minmax = 1;

        imdataview[viewindex].button1pressed = 0;
        imdataview[viewindex].button3pressed = 0;		
    }

    return 0;
}








int close_shm_image(int viewindex)
{
    if(verbose) {
        printf("  imindex = %d\n", imdataview[viewindex].imindex);
    }


    if(imdataview[viewindex].imindex != -1)
    {

        if(verbose) {
            printf("CLEAR viewindex = %d\n", viewindex);
            fflush(stdout);
        }

        g_object_unref (imdataview[viewindex].pbview);


        if(verbose) {
            printf("[%d] %s\n", __LINE__, __FILE__);
            fflush(stdout);
        }

        // gtk_image_clear (imdataview[viewindex].gtkimage);

        imdataview[viewindex].computearrayinit = 0;

        if(imdataview[viewindex].computearray != NULL) {
            free(imdataview[viewindex].computearray);
            imdataview[viewindex].computearray = NULL;
        }
        if(imdataview[viewindex].computearray16 != NULL) {
            free(imdataview[viewindex].computearray16);
            imdataview[viewindex].computearray16 = NULL;
        }



        if(imdataview[viewindex].PixelIndexRaw_array != NULL) {
            free(imdataview[viewindex].PixelIndexRaw_array);
            imdataview[viewindex].PixelIndexRaw_array = NULL;
        }

        if(imdataview[viewindex].PixelIndexViewBuff_array != NULL) {
            free(imdataview[viewindex].PixelIndexViewBuff_array);
            imdataview[viewindex].PixelIndexViewBuff_array = NULL;
        }




        if(verbose) {
            printf("[%d] %s\n", __LINE__, __FILE__);
            fflush(stdout);
        }

        if(imdataview[viewindex].imindex != -1) {
            ImageStreamIO_closeIm( &imarray[imdataview[viewindex].imindex] );
            imdataview[viewindex].imindex = -1;
        }

        if(verbose) {
            printf("[%d] %s\n", __LINE__, __FILE__);
            fflush(stdout);
        }


        //imdataview[viewindex].gtkimage = GTK_IMAGE(gtk_image_new_from_file ("./empty.png"));
        //gtk_image_set_from_file (imdataview[viewindex].gtkimage, "empty.png");
    }

    return 0;
}







/*
gboolean on_button_file_choose_file_set(
    __attribute__((unused)) GtkWidget      *widget,
    __attribute__((unused)) gpointer        data)
{
    gchar *file_name = NULL;        // Name of file to open from dialog box

    g_print ("stream file open\n");

    file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgets->button_file_choose));
    if (file_name != NULL) {
        char streamname[100];

        g_print("Selected : %s\n", file_name);
        //   gtk_image_set_from_file(GTK_IMAGE(app_wdgts->w_img_main), file_name);
        char *tmpstring = basename(file_name);

        int i = 0;
        while(tmpstring[i] != '.') {
            streamname[i] = tmpstring[i];
            i++;
        }
        streamname[i] = '\0';
        printf("streamname = \"%s\"\n", streamname);
		open_shm_image(streamname, 0);


    }
    g_free(file_name);


    return TRUE;
}

*/






gboolean on_menuitem_open_activate(
    __attribute__((unused)) GtkWidget      *widget,
    __attribute__((unused)) gpointer        data)
{
    if(verbose) {
        printf("FILE OPEN\n");
    }


    widgets->filechooserdialog = gtk_file_chooser_dialog_new ("Open File",
                                 GTK_WINDOW(widgets->mainwindow),
                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                 "_Cancel",
                                 GTK_RESPONSE_CANCEL,
                                 "_Open",
                                 GTK_RESPONSE_ACCEPT,
                                 NULL);



    GtkFileFilter *filefilter;

    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(widgets->filechooserdialog), SHARED_MEMORY_DIRECTORY);

    filefilter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filefilter,"*.im.shm");
    gtk_file_filter_set_name(filefilter,"milk stream");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgets->filechooserdialog), filefilter);


    if (gtk_dialog_run (GTK_DIALOG (widgets->filechooserdialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widgets->filechooserdialog));
        if(verbose) {
            printf("FILE : %s\n", filename);
        }

        if (filename != NULL) {
            char streamname[100];

            g_print("Selected : %s\n", filename);
            //   gtk_image_set_from_file(GTK_IMAGE(app_wdgts->w_img_main), file_name);
            char *tmpstring = basename(filename);

            int i = 0;
            while(tmpstring[i] != '.') {
                streamname[i] = tmpstring[i];
                i++;
            }
            streamname[i] = '\0';
            if(verbose) {
                printf("streamname = \"%s\"\n", streamname);
            }
            open_shm_image(streamname, 0);


        }
        g_free(filename);

    }

    gtk_widget_destroy (widgets->filechooserdialog);


    return TRUE;
}






// File --> Quit
void on_menuitem_quit_activate(
    __attribute__((unused)) GtkMenuItem *menuitem,
    __attribute__((unused)) app_widgets *app_wdgts)
{
    gtk_main_quit();
}






// called when window is closed
void on_window_main_destroy()
{
    gtk_main_quit();
}

