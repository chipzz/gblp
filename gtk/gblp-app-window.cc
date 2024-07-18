#include "gblp-app-window.h"
#include "MipMappedSize.h"
#include "openglblp.h"
#include <math.h>
#include <algorithm>
/*
#include <optional>
*/

struct _GBLPAppWindow
{
	GtkApplicationWindow parent_instance;

	GtkAdjustment *mipmap_adjustment;
	GtkGLArea *glarea;

	OpenGlResources opengl_resources;
/*
	std::optional<MipMappedSize> size;
*/
	MipMappedSize *size;
};

struct _GBLPAppWindowClass
{
	GtkApplicationWindowClass parent_class;
};

G_DEFINE_TYPE(GBLPAppWindow, gblp_app_window, GTK_TYPE_APPLICATION_WINDOW)

static gboolean gblp_app_window_glarea_make_current(GtkGLArea *const area)
{
	if (gtk_gl_area_get_error(area) != NULL)
		return FALSE;

	gtk_gl_area_make_current(area);
	return TRUE;
}

static int get_level(const gint width, const gint height)
{
	return int(std::min(log2(width), log2(height)));
}

float adjustment = 1.0;

static MipMappedSize *gblp_app_window_glarea_get_size(GtkGLArea *const area)
{
	return GBLP_APP_WINDOW(gtk_widget_get_root(GTK_WIDGET(area)))->size;
}

static OpenGlResources &gblp_app_window_glarea_get_glresources(GtkGLArea *const area)
{
	return GBLP_APP_WINDOW(gtk_widget_get_root(GTK_WIDGET(area)))->opengl_resources;
}

#include <glm/gtc/type_ptr.hpp>
#if 0
#include <glm/gtc/matrix_transform.hpp>
#endif
static void gblp_app_window_glarea_resize(GtkGLArea *const area, gint viewport_width, gint viewport_height, gpointer user_data)
{
	MipMappedSize *const size = gblp_app_window_glarea_get_size(area);

	if (!size)
		return;

	if (!gblp_app_window_glarea_make_current(area))
		return;

	const unsigned int max_level = get_level(viewport_width * adjustment, viewport_height * adjustment);

	float width, height;
	size->get_size_for_level(max_level, width, height);

/*
	float mvp[16];
*/
	glm::mat4 mvp(1.0f);
	mvp = glm::scale(mvp, glm::vec3(width / viewport_width, -height / viewport_height, 1.0f));
	mvp = glm::translate(mvp, glm::vec3(-0.5f, -0.5f, 0.0f));
	gblp_app_window_glarea_get_glresources(area).set_mvp(glm::value_ptr(mvp));
}

static void gblp_app_window_glarea_realize(GtkGLArea *const area)
{
	if (!gblp_app_window_glarea_make_current(area))
		return;

	gblp_app_window_glarea_get_glresources(area).init();
}

static gboolean gblp_app_window_glarea_render(GtkGLArea *const area, GdkGLContext *const context)
{
	MipMappedSize *const size = gblp_app_window_glarea_get_size(area);

	if (!size)
		return TRUE;

	if (!gblp_app_window_glarea_make_current(area))
		return TRUE;

	gblp_app_window_glarea_get_glresources(area).render();

//	return TRUE;
	return FALSE;
}

static void gblp_app_window_glarea_unrealize(GtkGLArea *const area)
{
	if (!gblp_app_window_glarea_make_current(area))
		return;

	gblp_app_window_glarea_get_glresources(area).exit();
}

static void adjustment_changed(GBLPAppWindow *window, GtkAdjustment *adj)
{
	adjustment = gtk_adjustment_get_value(adj) / 100;

	gtk_widget_queue_draw(GTK_WIDGET(window->glarea));
}

/*
static void animate_toggled(GBLPAppWindow *window)
{
	gboolean toggled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(window->animate_button));

	if (toggled)
	{
		g_assert(window->tick_id == 0);

		window->first_frame_time = 0;
		window->tick_id =
			gtk_widget_add_tick_callback(window->glarea, animate_rotation, window, NULL);
	}
	else
	{
		g_assert(window->tick_id != 0);

		gtk_widget_remove_tick_callback(window->glarea, window->tick_id);
		window->tick_id = 0;
	}
}
*/

static void gblp_app_window_class_init(GBLPAppWindowClass *klass)
{
	GtkWidgetClass *const widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(widget_class, "/gblp/gblp-app-window.ui");

	gtk_widget_class_bind_template_child(widget_class, GBLPAppWindow, glarea);
	gtk_widget_class_bind_template_child(widget_class, GBLPAppWindow, mipmap_adjustment);

	gtk_widget_class_bind_template_callback(widget_class, adjustment_changed);
	gtk_widget_class_bind_template_callback(widget_class, gblp_app_window_glarea_realize);
	gtk_widget_class_bind_template_callback(widget_class, gblp_app_window_glarea_render);
	gtk_widget_class_bind_template_callback(widget_class, gblp_app_window_glarea_resize);
	gtk_widget_class_bind_template_callback(widget_class, gblp_app_window_glarea_unrealize);
}

static void gblp_error(GError *const error, const gchar *const format)
{
	if (!error)
		return;

	g_print(format, error->message);
	g_error_free(error);
}

static void gblp_app_window_load_blp_cb(GObject *source_object, GAsyncResult *res, gpointer data)
{
	GFile *const file = G_FILE(source_object);
	GError *error = NULL;
	char *contents;

	if (!g_file_load_contents_finish(file, res, &contents, NULL, NULL, &error))
	{
		gblp_error(error, "Error loading file contents: %s\n");
		return;
	}

	GBLPAppWindow *const window = GBLP_APP_WINDOW(data);

	if (!gblp_app_window_glarea_make_current(window->glarea))
		return;

	window->size = new MipMappedSize();
	window->opengl_resources.load_blp_data(contents, window->size->width, window->size->height);
	g_free(contents);

	gtk_widget_queue_draw(GTK_WIDGET(window->glarea));
}

void gblp_app_window_load_blp(GBLPAppWindow *const window, GFile *const blpfile)
{
	g_file_load_contents_async(blpfile, NULL, gblp_app_window_load_blp_cb, window);

	/*
	window->opengl_resources.load_blp(g_file_get_path(blpfile));
	*/

	/*
	char *contents;
	GError *error = NULL;

	if (!g_file_load_contents(blpfile, NULL, &contents, NULL, NULL, &error))
		return;

	window->opengl_resources.load_blp_data(contents);
	g_free(contents);

	gtk_widget_queue_draw(GTK_WIDGET(window->glarea));
	*/
}

// Hack for gettext
#define _(a) (a)

#if GTK_CHECK_VERSION(4, 10, 0)
static void gblp_app_window_on_open_response(GObject *const o, GAsyncResult *const result, gpointer window)
{
	GError *error = NULL;

	if (GFile *const file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(o), result, &error))
	{
		gblp_app_window_load_blp(GBLP_APP_WINDOW(window), file);
		g_object_unref(file);
	}
	else
		if (error && !g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
			gblp_error(error, "Error loading file contents: %s\n");
}

void gblp_app_window_load_dialog(GBLPAppWindow *const window, const char *const title_text, const char *const button_text)
{
	GtkFileDialog *const dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, title_text);
	gtk_file_dialog_set_accept_label(dialog, _(button_text));

	GtkFileFilter *const blp_filter = gtk_file_filter_new();
	gtk_file_filter_add_suffix(blp_filter, "blp");
	gtk_file_filter_set_name(blp_filter, "BLP Files");

	GListStore *const filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
	g_list_store_append(filters, blp_filter);

	gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
	gtk_file_dialog_set_default_filter(dialog, blp_filter);

	gtk_file_dialog_open(dialog, GTK_WINDOW(window), NULL, gblp_app_window_on_open_response, window);
}

static void size_prepared(GdkPixbufLoader *self, gint w, gint h, gpointer user_data)
{
	gdk_pixbuf_loader_set_size(self, 512, 512);
};

static void gblp_import_pixbuf(GdkPixbuf *const pixbuf)
{
	guchar *const pixels = gdk_pixbuf_get_pixels(pixbuf);
	// TODO
	int i = 0;
	i++;
};

static void gblp_import_gfile_cb(GObject *source_object, GAsyncResult *res, gpointer data)
{
	GFile *const file = G_FILE(source_object);
	GError *error = NULL;

	char *contents;
	gsize length;

	if (!g_file_load_contents_finish(file, res, &contents, &length, NULL, &error))
	{
		gblp_error(error, "Error loading file contents: %s\n");
		return;
	}

	GdkPixbufLoader *const loader = GDK_PIXBUF_LOADER(data);
	if (!gdk_pixbuf_loader_write(loader, (const guchar *) contents, length, &error))
	{
		gblp_error(error, "Error writing to loader: %s\n");
		return;
	}

	if (!gdk_pixbuf_loader_close(loader, &error))
	{
		gblp_error(error, "Error closing loader: %s\n");
		return;
	}

	gblp_import_pixbuf(gdk_pixbuf_loader_get_pixbuf(loader));
	g_free(contents);
}

static void gblp_import_gfile(GFile *const file)
{
	GError *error = NULL;
	printf("gblp_import_gfile file=\"%s\"\n", g_file_get_path(file));

	if (GFileInfo *const fileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, &error))
	{
		if (GdkPixbufLoader *const loader = gdk_pixbuf_loader_new_with_mime_type(
			g_content_type_get_mime_type(g_file_info_get_attribute_as_string(fileinfo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)), &error))
		{
			g_signal_connect(loader, "size-prepared", G_CALLBACK(size_prepared), NULL); // FIXME: pass size
			g_file_load_contents_async(file, NULL, gblp_import_gfile_cb, loader);
		}
		else
		{
			gblp_error(error, "Error creating loader: %s\n");
			return;
		}
	}
	else
	{
		gblp_error(error, "Error querying file: %s\n");
		return;
	}
};

static void gblp_app_window_on_import_response(GObject *const o, GAsyncResult *const result, gpointer window)
{
	GError *error = NULL;

	//if (GFile *const file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(o), result, &error))
	if (GListModel *const files_list_model = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(o), result, &error))
	{
		for (size_t i = 0; i < g_list_model_get_n_items(files_list_model); i++)
			gblp_import_gfile(G_FILE(g_list_model_get_item(files_list_model, i)));
		g_object_unref(files_list_model);
	}
	else
		if (error && !g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
			gblp_error(error, "Error loading file contents: %s\n");
}

void gblp_app_window_import_dialog(GBLPAppWindow *const window, const char *const title_text, const char *const button_text)
{
	GtkFileDialog *const dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, title_text);
	gtk_file_dialog_set_accept_label(dialog, _(button_text));

	GtkFileFilter *const pixbuf_filter = gtk_file_filter_new();
	gtk_file_filter_add_pixbuf_formats(pixbuf_filter);
	gtk_file_filter_set_name(pixbuf_filter, "Images");

	GListStore *const filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
	g_list_store_append(filters, pixbuf_filter);

	gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
	gtk_file_dialog_set_default_filter(dialog, pixbuf_filter);

	gtk_file_dialog_open_multiple(dialog, GTK_WINDOW(window), NULL, gblp_app_window_on_import_response, window);
}
#else
static void gblp_app_window_on_open_response(GtkDialog *const dialog, gint response, GBLPAppWindow *const window)
{
	if (response == GTK_RESPONSE_ACCEPT)
	{
		GFile *const file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
		gblp_app_window_load_blp(window, file);
		g_object_unref(file);
	}

	gtk_window_destroy(GTK_WINDOW(dialog));
}

void gblp_app_window_load_dialog(GBLPAppWindow *const window, const char *const title_text, const char *const button_text)
{
	GtkWidget *const dialog = gtk_file_chooser_dialog_new
	(
		title_text, GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_(button_text), GTK_RESPONSE_ACCEPT,
		NULL
	);
	g_signal_connect(dialog, "response", G_CALLBACK(gblp_app_window_on_open_response), window);

	gtk_window_present(GTK_WINDOW(dialog));
}
#endif

static void gblp_app_window_action_load(GSimpleAction *action, GVariant *parameter, gpointer w)
{
	gblp_app_window_load_dialog(GBLP_APP_WINDOW(w), "Load File", "_Load");
}

static void gblp_app_window_action_import(GSimpleAction *action, GVariant *parameter, gpointer w)
{
	gblp_app_window_import_dialog(GBLP_APP_WINDOW(w), "Add File", "_Add");
}
static void gblp_app_window_action_toggle(GSimpleAction *a, GVariant *parameter, gpointer w)
{
	GAction *const action = G_ACTION(a);
	GVariant *const variant = g_action_get_state(action);
	g_action_change_state(action, g_variant_new_boolean(!g_variant_get_boolean(variant)));
	g_variant_unref(variant);
}

static void gblp_app_window_action_texture_alpha_state(GSimpleAction *action, GVariant *value, gpointer w)
{
	g_simple_action_set_state(action, value);
	GBLPAppWindow *const window = GBLP_APP_WINDOW(w);
	window->opengl_resources.set_texture_alpha(g_variant_get_boolean(value));
	gtk_widget_queue_draw(GTK_WIDGET(window->glarea));
}

static const GActionEntry gblp_app_window_action_entries[] =
{
	{ "load", gblp_app_window_action_load },
	{ "import", gblp_app_window_action_import },
	{ "texture_alpha", gblp_app_window_action_toggle, NULL, "true", gblp_app_window_action_texture_alpha_state }
};

static void gblp_app_window_init(GBLPAppWindow *window)
{
	gtk_widget_init_template(GTK_WIDGET(window));

//	gtk_window_set_icon_name(GTK_WINDOW(window), "glarea");
	g_action_map_add_action_entries(G_ACTION_MAP(window), gblp_app_window_action_entries, G_N_ELEMENTS(gblp_app_window_action_entries), window);
}

GBLPAppWindow *gblp_app_window_new(GtkApplication *app)
{
	GBLPAppWindow *const app_window = GBLP_APP_WINDOW(g_object_new(GBLP_TYPE_APP_WINDOW, "application", app, NULL));
	gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(app_window), TRUE);
	return app_window;
}
