#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include <gtk/gtk.h>


#include "ImageStruct.h"
#include "ImageStreamIO.h"

#include "Config.h"

#include "shmimview.h"
#include "shmimview-view.h"
#include "shmimview-scale.h"
#include "shmimview-process.h"


#define BYTES_PER_PIXEL 3


// GTK widget pointers
app_widgets *widgets;


// Image viewing data
IMAGEDATAVIEW *imdataview;

// Images
IMAGE *imarray;














int main(int argc, char *argv[])
{
    GtkBuilder      *builder;

	printf("SHMIM viewer version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);


    widgets = g_slice_new(app_widgets);

    imdataview = (IMAGEDATAVIEW*) malloc(sizeof(IMAGEDATAVIEW)*NB_IMDATAVIEW_MAX);
    imarray = (IMAGE*) malloc(sizeof(IMAGE)*NB_IMAGES_MAX);

    for(int i=0; i<NB_IMDATAVIEW_MAX; i++) {
        imdataview[i].imindex  = -1;
        imdataview[i].computearrayinit = 0;
        imdataview[i].allocated_viewpixels = 0;

        imdataview[i].scale_range_perc = 0.0; // min/max
        imdataview[i].scale_type = SCALE_LINEAR;
        imdataview[i].scalefunc = *scalefunction_linear;
        imdataview[i].scale_coeff = 0.0;

        imdataview[i].view_streaminfo = 0;
        imdataview[i].view_pixinfo    = 0;
        imdataview[i].view_imstat     = 0;
        imdataview[i].view_timing     = 0;
    }


    gtk_init(&argc, &argv);

    widgets->pressed_status = 0;

	
	char uifilename[200];
	sprintf(uifilename, "%s/glade/shmimview.glade", SOURCE_DIR);
    builder = gtk_builder_new_from_file(uifilename);

    //widgets->filechooser     = GTK_WIDGET(gtk_builder_get_object(builder, "filechooser"));

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
	widgets->scale_log1    = GTK_WIDGET(gtk_builder_get_object(builder, "scale_log 1"));
	widgets->scale_log2    = GTK_WIDGET(gtk_builder_get_object(builder, "scale_log 2"));
	widgets->scale_log3    = GTK_WIDGET(gtk_builder_get_object(builder, "scale_log 3"));
	widgets->scale_log4    = GTK_WIDGET(gtk_builder_get_object(builder, "scale_log 4"));
	widgets->scale_power01 = GTK_WIDGET(gtk_builder_get_object(builder, "scale_power01"));
	widgets->scale_power02 = GTK_WIDGET(gtk_builder_get_object(builder, "scale_power02"));
	widgets->scale_power05 = GTK_WIDGET(gtk_builder_get_object(builder, "scale_power05"));
	widgets->scale_linear  = GTK_WIDGET(gtk_builder_get_object(builder, "scale_linear"));
	widgets->scale_power20 = GTK_WIDGET(gtk_builder_get_object(builder, "scale_power20"));
	widgets->scale_power40 = GTK_WIDGET(gtk_builder_get_object(builder, "scale_power40"));
	widgets->scale_power80 = GTK_WIDGET(gtk_builder_get_object(builder, "scale_power80"));

	widgets->scale_rangeminmax = GTK_WIDGET(gtk_builder_get_object(builder, "scale_rangeminmax"));
	widgets->scale_range005    = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range005"));
	widgets->scale_range01     = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range01"));
	widgets->scale_range02     = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range02"));
	widgets->scale_range03     = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range03"));
	widgets->scale_range05     = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range05"));
	widgets->scale_range10     = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range10"));
	widgets->scale_range20     = GTK_WIDGET(gtk_builder_get_object(builder, "scale_range20"));

	widgets->colormap_grey     = GTK_WIDGET(gtk_builder_get_object(builder, "colormap_grey"));
	widgets->colormap_heat     = GTK_WIDGET(gtk_builder_get_object(builder, "colormap_heat"));
	widgets->colormap_cool     = GTK_WIDGET(gtk_builder_get_object(builder, "colormap_cool"));


	gtk_widget_add_events( widgets->mainwindow, GDK_KEY_PRESS_MASK);



    //	gtk_box_pack_start (GTK_BOX (widgets->imbox), GTK_WIDGET(imdataview[0].gtkimage), FALSE, FALSE, 0);
    //	gtk_box_pack_start (GTK_BOX (widgets->imbox), GTK_WIDGET(gtk_label_new ("Colorbar")), FALSE, FALSE, 0);





    // STREAM FILE OPEN


    widgets->button_file_choose = GTK_WIDGET(gtk_builder_get_object(builder, "button_file_choose"));

    GtkFileFilter *filefilter;

    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(widgets->button_file_choose), "/milk/shm/");

    filefilter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filefilter,"*.im.shm");
    gtk_file_filter_set_name(filefilter,"milk stream");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgets->button_file_choose), filefilter);



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
    
    printf("QUIT APPLICATION\n");
    fflush(stdout);
    
    
    g_slice_free(app_widgets, widgets);
    free(imdataview);
    free(imarray);

    return 0;
}









void free_pixels(guchar *pixels, __attribute__((unused)) gpointer data) {
    free(pixels);
}



int open_shm_image(
    char *streamname,
    int index)
{
    char filename[64];

	int viewindex = 0;

    ImageStreamIO_filename(filename, 64, streamname);
    printf("Filename = %s\n", filename);

    if(ImageStreamIO_openIm(&imarray[index], streamname) == -1)
    {
        printf("ERROR: cannot load stream \"%s\"\n", streamname);
        return -1;
    }
    else
    {
        printf("--------------------------- LOADED %s %d %d\n",
               imarray[index].md[0].name,
               imarray[index].md[0].size[0],
               imarray[index].md[0].size[1]);
        
        sprintf(imdataview[viewindex].imname, "%s", streamname);
        imdataview[viewindex].naxis = imarray[index].md[0].naxis;
        
		if(imdataview[viewindex].imindex != -1) {
			close_shm_image(viewindex);
		}

		imdataview[viewindex].imindex = 0;

		// image size
        imdataview[viewindex].xsize = imarray[index].md[0].size[0];
        imdataview[viewindex].ysize = imarray[index].md[0].size[1];
        

	/*		for(int ii=0;ii<256;ii++)
				for(int jj=0;jj<256;jj++)
					imarray[index].array.F[jj*256+ii] = 1.0*ii;*/

		// start with zoom 1
		resize_PixelBufferView(imdataview[viewindex].xsize, imdataview[viewindex].ysize);


		// set pix active area
		imdataview[viewindex].iimin = 0;
		imdataview[viewindex].iimax = imarray[index].md[0].size[0];
		imdataview[viewindex].jjmin = 0;
		imdataview[viewindex].jjmax = imarray[index].md[0].size[1];


		// we initially set zoom factor = 1
		
//		imdataview[viewindex].viewXsize = imarray[index].md[0].size[0];
//		imdataview[viewindex].viewYsize = imarray[index].md[0].size[1];
//		printf("VIEW SIZE %d %d\n", imdataview[viewindex].viewXsize, imdataview[viewindex].viewYsize);
//		gtk_widget_set_size_request();

		// set view active area
        imdataview[viewindex].xviewmin = 0;
        imdataview[viewindex].xviewmax = imdataview[viewindex].viewXsize; 
        imdataview[viewindex].yviewmin = 0;
        imdataview[viewindex].yviewmax = imdataview[viewindex].viewYsize;


		
		


        imdataview[viewindex].vmin = 0.0;
        imdataview[viewindex].vmax = 1.0;
        imdataview[viewindex].bscale_slope = 1.0;
        imdataview[viewindex].bscale_center = 0.5;

        imdataview[viewindex].showsaturated_min = 0;
        imdataview[viewindex].showsaturated_max = 0;
        imdataview[viewindex].zoomFact = 1;
        imdataview[viewindex].update = 1;

        imdataview[viewindex].autominmax = 1;

        imdataview[viewindex].button1pressed = 0;
        imdataview[viewindex].button3pressed = 0;
    }

    return 0;
}








int close_shm_image(int viewindex)
{
	printf("  imindex = %d\n", imdataview[viewindex].imindex);
    if(imdataview[viewindex].imindex != -1)
    {
        g_object_unref (imdataview[viewindex].pbview);
        gtk_image_clear (imdataview[viewindex].gtkimage);
        imdataview[viewindex].imindex = -1;

        imdataview[viewindex].computearrayinit = 0;
        free(imdataview[viewindex].computearray);

		printf("Set to empty.png\n");
		fflush(stdout);
		
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
    printf("FILE OPEN\n");


    widgets->filechooserdialog = gtk_file_chooser_dialog_new ("Open File",
                                 GTK_WINDOW(widgets->mainwindow),
                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                 "_Cancel",
                                 GTK_RESPONSE_CANCEL,
                                 "_Open",
                                 GTK_RESPONSE_ACCEPT,
                                 NULL);



    GtkFileFilter *filefilter;

    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(widgets->filechooserdialog), "/milk/shm/");

    filefilter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filefilter,"*.im.shm");
    gtk_file_filter_set_name(filefilter,"milk stream");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgets->filechooserdialog), filefilter);


    if (gtk_dialog_run (GTK_DIALOG (widgets->filechooserdialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widgets->filechooserdialog));
        printf("FILE : %s\n", filename);


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
            printf("streamname = \"%s\"\n", streamname);
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


void on_spinbtn_zoom_value_changed()
{
	printf("zoom changed\n");
}
