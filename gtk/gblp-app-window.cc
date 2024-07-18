#include "gblp-app-window.h"
#include "openglblp.h"
#include <math.h>
#include <algorithm>

struct _GBLPAppWindow
{
	GtkApplicationWindow parent_instance;

	GtkAdjustment *mipmap_adjustment;
	GtkGLArea *glarea;

	OpenGlResources opengl_resources;
};

struct _GBLPAppWindowClass
{
	GtkApplicationWindowClass parent_class;
};

G_DEFINE_TYPE(GBLPAppWindow, gblp_app_window, GTK_TYPE_APPLICATION_WINDOW)

static gboolean gblp_app_window_glarea_make_current(GtkGLArea *area)
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

#include <glm/gtc/matrix_transform.hpp>

//float mvp[16];
extern glm::mat4 mvp;
float adjustment = 1.0;

static void gblp_app_window_glarea_resize(GtkGLArea *area, gint viewport_width, gint viewport_height, gpointer user_data)
{
	if (!gblp_app_window_glarea_make_current(area))
		return;

	const int max_dim = 1 << get_level(viewport_width * adjustment, viewport_height * adjustment);

	mvp = glm::mat4(1.0f);
	mvp = glm::scale(mvp, glm::vec3(2*float(max_dim) / viewport_width, -2*float(max_dim) / viewport_height, 1.0f));
	mvp = glm::translate(mvp, glm::vec3(-0.5f, -0.5f, 0.0f));
}

static OpenGlResources &gblp_app_window_glarea_get_glresources(GtkGLArea *area)
{
	return GBLP_APP_WINDOW(gtk_widget_get_root(GTK_WIDGET(area)))->opengl_resources;
}

static void gblp_app_window_glarea_realize(GtkGLArea *area)
{
	if (!gblp_app_window_glarea_make_current(area))
		return;

	gblp_app_window_glarea_get_glresources(area).init();
}

static gboolean gblp_app_window_glarea_render(GtkGLArea *area, GdkGLContext *context)
{
	if (!gblp_app_window_glarea_make_current(area))
		return FALSE;

	gblp_app_window_glarea_get_glresources(area).render();

//	return TRUE;
	return FALSE;
}

static void gblp_app_window_glarea_unrealize(GtkGLArea *area)
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

void gblp_app_window_load_blp_cb(GObject *source_object, GAsyncResult *res, gpointer data)
{
	GFile *file = G_FILE(source_object);
	GError *error = NULL;
	char *contents;

	if (!g_file_load_contents_finish(file, res, &contents, NULL, NULL, &error))
	{
		if (error)
		{
			g_print("Error loading file contents: %s\n", error->message);
			g_error_free(error);
		}
		return;
	}

	GBLPAppWindow *const window = GBLP_APP_WINDOW(data);

	if (!gblp_app_window_glarea_make_current(window->glarea))
		return;

	window->opengl_resources.load_blp_data(contents);
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
		{
			g_print("Error loading file contents: %s\n", error->message);
			g_error_free(error);
		}
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

static void gblp_app_window_on_import_response(GObject *const o, GAsyncResult *const result, gpointer window)
{
	GError *error = NULL;

	//if (GFile *const file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(o), result, &error))
	if (GListModel *const files_list_model = gtk_file_dialog_open_multiple_finish(GTK_FILE_DIALOG(o), result, &error))
	{
			for (size_t i = 0; i < g_list_model_get_n_items(files_list_model); i++) {
				GFile *const file = G_FILE(g_list_model_get_item(files_list_model, i));
				printf("gblp_application_on_open open file=\"%s\" type=\"%ul\"\n", g_file_get_path(file), g_file_get_type());

				//gblp_app_window_load_blp(GBLP_APP_WINDOW(window), file);
			}
			g_object_unref(files_list_model);
	}
	else
		if (error && !g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED))
		{
			g_print("Error loading file contents: %s\n", error->message);
			g_error_free(error);
		}
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
