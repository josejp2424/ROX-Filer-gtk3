/*
 * ROX-Filer, filer for the ROX desktop project
 * Copyright (C) 2006, Thomas Leonard and others (see changelog for details).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* pinboard.c - icons on the desktop background */

/*
 * Modificado por josejp2424 (2026):
 * port a GTK3 y adaptaciones específicas de esta versión.
 */

#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <math.h>
#include <libxml/parser.h>
#include <signal.h>

#include "global.h"

#include "pinboard.h"
#include "main.h"
#include "dnd.h"
#include "pixmaps.h"
#include "type.h"
#include "choices.h"
#include "support.h"
#include "gui_support.h"
#include "options.h"
#include "diritem.h"
#include "bind.h"
#include "icon.h"
#include "run.h"
#include "appinfo.h"
#include "menu.h"
#include "xml.h"
#include "tasklist.h"
#include "panel.h"		/* For panel_mark_used() */
#include "wrapped.h"
#include "dropbox.h"

static gboolean tmp_icon_selected = FALSE;		/* When dragging */

static GtkWidget *set_backdrop_dialog = NULL;

struct _Pinboard {
	guchar		*name;		/* Leaf name */
	GList		*icons;

	gchar		*backdrop;	/* Pathname */
	BackdropStyle	backdrop_style;
	gint		to_backdrop_app; /* pipe FD, or -1 */
	gint		from_backdrop_app; /* pipe FD, or -1 */
	gint		input_tag;
	GString		*input_buffer;

	GtkWidget	*window;	/* Screen-sized window */
	GtkWidget	*fixed;
	GdkPixbuf	*backdrop_pixbuf;
	GdkRGBA	background_rgba;
	GtkCssProvider	*css_provider;
};

#define IS_PIN_ICON(obj) G_TYPE_CHECK_INSTANCE_TYPE((obj), pin_icon_get_type())

typedef struct _PinIconClass PinIconClass;
typedef struct _PinIcon PinIcon;

struct _PinIconClass {
	IconClass parent;
};

struct _PinIcon {
	Icon		icon;

	int		x, y;
	GtkWidget	*win;
	GtkWidget	*widget;	/* GtkImage containing the icon */
	GtkWidget	*label;
};

/* The number of pixels between the bottom of the image and the top
 * of the text.
 */
#define GAP 4

/* The size of the border around the icon which is used when winking */
#define WINK_FRAME 2

/* Grid sizes */
#define GRID_STEP_FINE   2
#define GRID_STEP_MED    16
#define GRID_STEP_COARSE 32

/* Used in options */
#define CORNER_TOP_LEFT 0
#define CORNER_TOP_RIGHT 1
#define CORNER_BOTTOM_LEFT 2
#define CORNER_BOTTOM_RIGHT 3

#define DIR_HORZ 0
#define DIR_VERT 1

static PinIcon	*current_wink_icon = NULL;
static gint	wink_timeout;

/* Used for the text colours (only) in the icons */
GdkRGBA pin_text_fg_col, pin_text_bg_col;
PangoFontDescription *pinboard_font = NULL; /* NULL => Gtk default */

static GdkRGBA pin_text_shadow_col;

Pinboard	*current_pinboard = NULL;
static gint	loading_pinboard = 0;		/* Non-zero => loading */

/* The Icon that was used to start the current drag, if any */
Icon *pinboard_drag_in_progress = NULL;

/* For selecting groups of icons */
static gboolean lasso_in_progress = FALSE;
static int lasso_rect_x1, lasso_rect_x2;
static int lasso_rect_y1, lasso_rect_y2;

/* Tracking icon positions */
static GHashTable *placed_icons=NULL;

Option o_pinboard_tasklist_per_workspace;
static Option o_pinboard_clamp_icons, o_pinboard_grid_step;
static Option o_pinboard_fg_colour, o_pinboard_bg_colour;
static Option o_pinboard_tasklist, o_forward_buttons_13;
static Option o_iconify_start, o_iconify_dir;
static Option o_label_font, o_pinboard_shadow_colour;
static Option o_pinboard_shadow_labels;
static Option o_blackbox_hack;

static Option o_search_step_x, o_search_step_y;
static Option o_top_margin, o_bottom_margin, o_left_margin, o_right_margin;
static Option o_pinboard_image_scaling;

/* Static prototypes */
static GType pin_icon_get_type(void);
static void set_size_and_style(PinIcon *pi);
static gint end_wink(gpointer data);
static gboolean button_release_event(GtkWidget *widget,
			    	     GdkEventButton *event,
                            	     PinIcon *pi);
static gboolean enter_notify(GtkWidget *widget,
			     GdkEventCrossing *event,
			     PinIcon *pi);
static gint leave_notify(GtkWidget *widget,
			      GdkEventCrossing *event,
			      PinIcon *pi);
static gboolean button_press_event(GtkWidget *widget,
			    GdkEventButton *event,
                            PinIcon *pi);
static gboolean scroll_event(GtkWidget *widget,
                             GdkEventScroll *event);
static gint icon_motion_notify(GtkWidget *widget,
			       GdkEventMotion *event,
			       PinIcon *pi);
static const char *pin_from_file(gchar *line);
static void snap_to_grid(int *x, int *y);
static void offset_from_centre(PinIcon *pi, int *x, int *y);
static gboolean drag_motion(GtkWidget		*widget,
                            GdkDragContext	*context,
                            gint		x,
                            gint		y,
                            guint		time,
			    PinIcon		*pi);
static void drag_set_pinicon_dest(PinIcon *pi);
static void drag_leave(GtkWidget	*widget,
                       GdkDragContext	*context,
		       guint32		time,
		       PinIcon		*pi);
static gboolean bg_drag_motion(GtkWidget	*widget,
                               GdkDragContext	*context,
                               gint		x,
                               gint		y,
                               guint		time,
			       gpointer		data);
static gboolean bg_drag_leave(GtkWidget		*widget,
			      GdkDragContext	*context,
			      guint32		time,
			      gpointer		data);
static gboolean bg_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static void drag_end(GtkWidget *widget,
			GdkDragContext *context,
			PinIcon *pi);
static void reshape_all(void);
static void pinboard_check_options(void);
static void pinboard_load_from_xml(xmlDocPtr doc);
static void pinboard_clear(void);
static void pinboard_save(void);
static PinIcon *pin_icon_new(const char *pathname, const char *name);
static void pin_icon_destroyed(PinIcon *pi);
static void pin_icon_set_tip(PinIcon *pi);
static void pinboard_show_menu(GdkEventButton *event, PinIcon *pi);
static void create_pinboard_window(Pinboard *pinboard);
static void reload_backdrop(Pinboard *pinboard,
			    const gchar *backdrop,
			    BackdropStyle backdrop_style);
static void pinboard_reshape_icon(Icon *icon);
static void abandon_backdrop_app(Pinboard *pinboard);
static void drag_backdrop_dropped(GtkWidget	*drop_box,
				  const guchar	*path,
				  GtkWidget	*dialog);
static void backdrop_response(GtkWidget *dialog, gint response, gpointer data);
static void find_free_rect(Pinboard *pinboard, GdkRectangle *rect,
			   gboolean old, int start, int direction, int start_coord);
static void update_pinboard_font(void);
static void draw_lasso(void);
static gint lasso_motion(GtkWidget *widget, GdkEventMotion *event, gpointer d);
static void clear_backdrop(GtkWidget *drop_box, gpointer data);
static void radios_changed(Radios *radios, gpointer data);
static void update_radios(GtkWidget *dialog);
static void pinboard_set_backdrop_box(void);

/****************************************************************
 *			EXTERNAL INTERFACE			*
 ****************************************************************/

void pinboard_init(void)
{
	option_add_string(&o_pinboard_fg_colour, "pinboard_fg_colour", "#fff");
	option_add_string(&o_pinboard_bg_colour, "pinboard_bg_colour", "#888");
	option_add_string(&o_pinboard_shadow_colour, "pinboard_shadow_colour",
			"#000");
	option_add_string(&o_label_font, "label_font", "");
	option_add_int(&o_pinboard_shadow_labels, "pinboard_shadow_labels", 1);

	option_add_int(&o_pinboard_clamp_icons, "pinboard_clamp_icons", 1);
	option_add_int(&o_pinboard_grid_step, "pinboard_grid_step",
							GRID_STEP_COARSE);
	option_add_int(&o_pinboard_tasklist, "pinboard_tasklist", TRUE);
	option_add_int(&o_pinboard_tasklist_per_workspace, "pinboard_tasklist_per_workspace", FALSE);
	option_add_int(&o_forward_buttons_13, "pinboard_forward_buttons_13",
			FALSE);

	option_add_int(&o_iconify_start, "iconify_start", CORNER_TOP_RIGHT);
	option_add_int(&o_iconify_dir, "iconify_dir", DIR_VERT);

	option_add_int(&o_blackbox_hack, "blackbox_hack", FALSE);

	option_add_int(&o_search_step_x, "pinboard_search_step_x", 32);
	option_add_int(&o_search_step_y, "pinboard_search_step_y", 32);

	option_add_int(&o_top_margin, "pinboard_top_margin", 7);
	option_add_int(&o_bottom_margin, "pinboard_bottom_margin", 7);
	option_add_int(&o_left_margin, "pinboard_left_margin", 6);
	option_add_int(&o_right_margin, "pinboard_right_margin", 6);

	option_add_int(&o_pinboard_image_scaling, "pinboard_image_scaling", 0);

	option_add_notify(pinboard_check_options);

	gdk_rgba_parse(&pin_text_fg_col, (const gchar *) o_pinboard_fg_colour.value);
	gdk_rgba_parse(&pin_text_bg_col, (const gchar *) o_pinboard_bg_colour.value);
	gdk_rgba_parse(&pin_text_shadow_col, (const gchar *) o_pinboard_shadow_colour.value);
	update_pinboard_font();

	placed_icons=g_hash_table_new(g_str_hash, g_str_equal);
}

/* Load 'pb_<pinboard>' config file from Choices (if it exists)
 * and make it the current pinboard.
 * Any existing pinned items are removed. You must call this
 * at least once before using the pinboard. NULL disables the
 * pinboard.
 */
void pinboard_activate(const gchar *name)
{
	Pinboard	*old_board = current_pinboard;
	guchar		*path, *slash;

	/* Treat an empty name the same as NULL */
	if (name && !*name)
		name = NULL;

	if (old_board)
		pinboard_clear();

	if (!name)
	{
		if (number_of_windows < 1 && gtk_main_level() > 0)
			gtk_main_quit();

		gdk_property_delete(gdk_get_default_root_window(),
				gdk_atom_intern("_XROOTPMAP_ID", FALSE));
		return;
	}

	number_of_windows++;

	slash = strchr(name, '/');
	if (slash)
	{
		if (access(name, F_OK))
			path = NULL;	/* File does not (yet) exist */
		else
			path = g_strdup(name);
	}
	else
	{
		guchar	*leaf;

		leaf = g_strconcat("pb_", name, NULL);
		path = choices_find_xdg_path_load(leaf, PROJECT, SITE);
		g_free(leaf);
	}

	current_pinboard = g_new(Pinboard, 1);
	current_pinboard->name = g_strdup(name);
	current_pinboard->icons = NULL;
	current_pinboard->window = NULL;
	current_pinboard->backdrop = NULL;
	current_pinboard->backdrop_style = BACKDROP_NONE;
	current_pinboard->to_backdrop_app = -1;
	current_pinboard->from_backdrop_app = -1;
	current_pinboard->input_tag = -1;
	current_pinboard->input_buffer = NULL;

	create_pinboard_window(current_pinboard);

	loading_pinboard++;
	if (path)
	{
		xmlDocPtr doc;
		doc = xmlParseFile(path);
		if (doc)
		{
			pinboard_load_from_xml(doc);
			xmlFreeDoc(doc);
			reload_backdrop(current_pinboard,
					current_pinboard->backdrop,
					current_pinboard->backdrop_style);
		}
		else
		{
			parse_file(path, pin_from_file);
			info_message(_("Your old pinboard file has been "
					"converted to the new XML format."));
			pinboard_save();
		}
		g_free(path);
	}
	else
		pinboard_pin(home_dir, "Home",
				4 + ICON_WIDTH / 2,
				4 + ICON_HEIGHT / 2,
				NULL);
	loading_pinboard--;

	if (o_pinboard_tasklist.int_value)
		tasklist_set_active(TRUE);
}

/* Return the window of the current pinboard, or NULL.
 * Used to make sure lowering the panels doesn't lose them...
 */
GdkWindow *pinboard_get_window(void)
{
	if (current_pinboard)
		return gtk_widget_get_window(current_pinboard->window);
	return NULL;
}

const char *pinboard_get_name(void)
{
	g_return_val_if_fail(current_pinboard != NULL, NULL);

	return current_pinboard->name;
}

/* Add widget to the pinboard. Caller is responsible for coping with pinboard
 * being cleared.
 */
void pinboard_add_widget(GtkWidget *widget, const gchar *name)
{
	GtkRequisition req;
	GdkRectangle *rect=NULL;
	gboolean found=FALSE;

	g_return_if_fail(current_pinboard != NULL);

	gtk_fixed_put(GTK_FIXED(current_pinboard->fixed), widget, 0, 0);

	gtk_widget_get_preferred_size(widget, &req, NULL);

	if(name) {
		rect=g_hash_table_lookup(placed_icons, name);
		if(rect)
			found=TRUE;
	}
	/*printf("%s at %p %d\n", name? name: "(nil)", rect, found);*/

	if(rect) {
		if(rect->width<req.width || rect->height<req.height) {
			found=FALSE;
		}
	} else {
		rect=g_new(GdkRectangle, 1);
		rect->width = req.width;
		rect->height = req.height;
	}
	/*printf("%s at %d,%d %d\n", name? name: "(nil)", rect->x,
	  rect->y, found);*/
	find_free_rect(current_pinboard, rect, found,
			o_iconify_start.int_value, o_iconify_dir.int_value, 0);
	/*printf("%s at %d,%d %d\n", name? name: "(nil)", rect->x,
	  rect->y, found);*/

	gtk_fixed_move(GTK_FIXED(current_pinboard->fixed),
			widget, rect->x, rect->y);

	/* Store the new position (key and value are never freed) */
	if(name)
		g_hash_table_insert(placed_icons, g_strdup(name),
				    rect);
}

void pinboard_moved_widget(GtkWidget *widget, const gchar *name,
			   int x, int y)
{
	GdkRectangle *rect;

	if(!name)
		return;
	rect=g_hash_table_lookup(placed_icons, name);
	if(!rect)
		return;

	rect->x=x;
	rect->y=y;
}

/* Add a new icon to the background.
 * 'path' should be an absolute pathname.
 * 'x' and 'y' are the coordinates of the point in the middle of the text.
 *   If they are negative, the icon is placed automatically.
 *   The values then indicate where they should be added.
 *   x: -1 means left (vertical), -2 means right (vertical),
 *      -3 means left (horizontal), -4 means right (horizontal)
 *   y: -1 means top, -2 means bottom
 *
 *   Half-automatic placement
 *   x or y: -5 means placement starting at CORNER_TOP_LEFT
 *              of box bounded by non-zero x or y coordinate.
 *
 *   x or y: -6 means placement starting at CORNER_TOP_RIGHT
 *              of box bounded by non-zero x or y coordinate.
 *
 *   x or y: -7 means placement starting at CORNER_BOTTOM_LEFT
 *              of box bounded by non-zero x or y coordinate.
 *
 *   x or y: -8 means placement starting at CORNER_BOTTOM_RIGHT
 *              of box bounded by non-zero x or y coordinate.
 *
 * 'name' is the name to use. If NULL then the leafname of path is used.
 * If update is TRUE and there's already an icon for that path, it is updated.
 */
void pinboard_pin_with_args(const gchar *path, const gchar *name,
				   int x, int y, const gchar *shortcut,
				   const gchar *args, gboolean locked, gboolean update)
{
	GtkWidget	*vbox;
	PinIcon		*pi;
	Icon		*icon;

	g_return_if_fail(path != NULL);
	g_return_if_fail(current_pinboard != NULL);

	pi = NULL;

	if (update)
	{
		GList		*iter;

		for (iter = current_pinboard->icons; iter; iter = iter->next)
		{
			icon = (Icon *) iter->data;
			if (strcmp(icon->path, path) == 0)
			{
				pi = (PinIcon *) icon;
				break;
			}
		}
		if (pi)
		{
			if (x < 0)
				x = pi->x;
			if (y < 0)
				y = pi->y;
			icon_set_path(icon, path, name);
			goto out; /* The icon is already on the pinboard. */
		}
	}

	pi = pin_icon_new(path, name);
	icon = (Icon *) pi;

	/* A pinboard item is a normal GTK3 button containing GtkImage and
	 * WrappedLabel; drawing and input stay inside public widget APIs. */
	pi->win = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(pi->win), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus(pi->win, FALSE);
	gtk_style_context_add_class(gtk_widget_get_style_context(pi->win),
		"rox-pin-icon");

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, GAP);
	gtk_container_add(GTK_CONTAINER(pi->win), vbox);

	pi->widget = gtk_image_new();
	gtk_widget_set_halign(pi->widget, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(pi->widget, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(vbox), pi->widget, FALSE, FALSE, 0);

	pi->label = wrapped_label_new(icon->item->leafname, 180);
	gtk_style_context_add_class(gtk_widget_get_style_context(pi->label),
		"rox-pin-label");
	gtk_box_pack_start(GTK_BOX(vbox), pi->label, FALSE, FALSE, 0);

	drag_set_pinicon_dest(pi);
	g_signal_connect(pi->win, "drag_data_get",
				G_CALLBACK(drag_data_get), NULL);

	gtk_fixed_put(GTK_FIXED(current_pinboard->fixed), pi->win, 0, 0);

	/* find free space for item if position is not given */
	if (x < 0 || y < 0)
	{
		GtkRequisition req;
		GdkRectangle rect;
		int start_coord = 0;
		int placement = CORNER_TOP_LEFT;
		int direction = DIR_VERT;

		if (x == -3 || x == -4)
		{
			direction = DIR_HORZ;
		}

		switch (x)
		{
			case -1:
			case -3:
				if (y == -2)
					placement = CORNER_BOTTOM_LEFT;
				break;
			case -2:
			case -4:
				switch (y)
				{
					case -1:
						placement = CORNER_TOP_RIGHT;
						break;
					case -2:
						placement = CORNER_BOTTOM_RIGHT;
						break;
				}
				break;
		}

		if (x == -5 || y == -5)
			placement = CORNER_TOP_LEFT;
		if (x == -6 || y == -6)
			placement = CORNER_TOP_RIGHT;
		if (x == -7 || y == -7)
			placement = CORNER_BOTTOM_LEFT;
		if (x == -8 || y == -8)
			placement = CORNER_BOTTOM_RIGHT;

		if (y > 0)
		{
			direction = DIR_HORZ;
			start_coord = y;
		}
		else if (x > 0)
			start_coord = x;

		pinboard_reshape_icon((Icon *) pi);
		gtk_widget_get_preferred_size(pi->widget, &req, NULL);
		rect.width = req.width;
		rect.height = req.height;
		gtk_widget_get_preferred_size(pi->label, &req, NULL);
		rect.width = MAX(rect.width, req.width);
		rect.height += req.height + GAP;
		find_free_rect(current_pinboard, &rect, FALSE,
			placement, direction, start_coord);
		x = rect.x + rect.width/2;
		y = rect.y + rect.height/2;
	}

	gtk_widget_show_all(pi->win);

	gtk_widget_add_events(pi->win,
			GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK |
			GDK_BUTTON3_MOTION_MASK);
	g_signal_connect(pi->win, "enter-notify-event",
			G_CALLBACK(enter_notify), pi);
	g_signal_connect(pi->win, "leave-notify-event",
			G_CALLBACK(leave_notify), pi);
	g_signal_connect(pi->win, "button-press-event",
			G_CALLBACK(button_press_event), pi);
	g_signal_connect(pi->win, "button-release-event",
			G_CALLBACK(button_release_event), pi);
	g_signal_connect(pi->win, "motion-notify-event",
			G_CALLBACK(icon_motion_notify), pi);
	g_signal_connect_swapped(pi->win, "destroy",
			G_CALLBACK(pin_icon_destroyed), pi);

	current_pinboard->icons = g_list_prepend(current_pinboard->icons, pi);

out:
	snap_to_grid(&x, &y);

	pi->x = x;
	pi->y = y;

	pinboard_reshape_icon((Icon *) pi);

	gtk_widget_realize(pi->win);

	pin_icon_set_tip(pi);

	icon_set_shortcut(icon, shortcut);
	icon_set_arguments(icon, args);
	icon->locked = locked;

	if (!loading_pinboard)
		pinboard_save();
}

void pinboard_pin(const gchar *path, const gchar *name, int x, int y,
		  const gchar *shortcut)
{
	pinboard_pin_with_args(path, name, x, y, shortcut, NULL, FALSE, FALSE);
}

/*
 * Remove an icon from the background.  The first icon with a matching
 * path is removed.  If name is not NULL then that also must match
 */
gboolean pinboard_remove(const gchar *path, const gchar *name)
{
	GList *item;
	PinIcon		*pi;
	Icon		*icon;

	g_return_val_if_fail(current_pinboard != NULL, FALSE);

	for(item=current_pinboard->icons; item; item=g_list_next(item)) {
		pi=(PinIcon *) item->data;
		icon=(Icon *) pi;

		if(strcmp(icon->path, path)!=0)
			continue;

		if(name && strcmp(name, icon->item->leafname)!=0)
			continue;

		icon->locked = FALSE;
		icon_destroy(icon);

		pinboard_save();

		return TRUE;
	}

	return FALSE;
}

/* Put a border around the icon, briefly.
 * If icon is NULL then cancel any existing wink.
 * The icon will automatically unhighlight unless timeout is FALSE,
 * in which case you must call this function again (with NULL or another
 * icon) to remove the highlight.
 */
static void pinboard_wink_item(PinIcon *pi, gboolean timeout)
{
	PinIcon *old = current_wink_icon;

	if (old == pi)
		return;

	current_wink_icon = pi;

	if (old)
	{
		gtk_style_context_remove_class(
			gtk_widget_get_style_context(old->win), "rox-wink");
		gtk_widget_queue_draw(old->win);
	}

	if (wink_timeout != -1)
	{
		g_source_remove(wink_timeout);
		wink_timeout = -1;
	}

	if (pi)
	{
		gtk_style_context_add_class(
			gtk_widget_get_style_context(pi->win), "rox-wink");
		gtk_widget_queue_draw(pi->win);

		if (timeout)
			wink_timeout = g_timeout_add(300, end_wink, NULL);
	}
}

/* 'app' is saved as the new application to set the backdrop. It will then be
 * run, and should communicate with the filer as described in the manual.
 */
void pinboard_set_backdrop_app(const gchar *app)
{
	XMLwrapper *ai;
	DirItem *item;
	gboolean can_set;

	item = diritem_new("");
	diritem_restat(app, item, NULL);
	if (!(item->flags & ITEM_FLAG_APPDIR))
	{
		delayed_error(_("The backdrop handler must be an application "
				"directory. Drag an application directory "
				"into the Set Backdrop dialog box, or (for "
				"programmers) pass it to the SOAP "
				"SetBackdropApp method."));
		diritem_free(item);
		return;
	}

	ai = appinfo_get(app, item);
	diritem_free(item);

	can_set = ai && xml_get_section(ai, ROX_NS, "CanSetBackdrop") != NULL;
	if (ai)
		g_object_unref(ai);

	if (can_set)
		pinboard_set_backdrop(app, BACKDROP_PROGRAM);
	else
		delayed_error(_("You can only set the backdrop to an image "
				"or to a program which knows how to "
				"manage ROX-Filer's backdrop.\n\n"
				"Programmers: the application's AppInfo.xml "
				"must contain the CanSetBackdrop element, as "
				"described in ROX-Filer's manual."));
}

/* Open a dialog box allowing the user to set the backdrop */
static void pinboard_set_backdrop_box(void)
{
	GtkWidget *dialog, *frame, *label, *hbox;
	GtkBox *vbox;
	Radios *radios;

	g_return_if_fail(current_pinboard != NULL);

	if (set_backdrop_dialog)
		gtk_widget_destroy(set_backdrop_dialog);

	/* GTK_DIALOG_NO_SEPARATOR was removed in GTK3. */
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Set backdrop"));
	dialog_add_icon_button(GTK_DIALOG(dialog), ROX_ICON_CLOSE,
			_("_Close"), GTK_RESPONSE_OK);
	set_backdrop_dialog = dialog;
	g_signal_connect(dialog, "destroy",
			G_CALLBACK(gtk_widget_destroyed), &set_backdrop_dialog);
	vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	label = gtk_label_new(_("Choose a style and drag an image in:"));
	rox_widget_set_padding(GTK_WIDGET(label), 4, 0);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.0);
	gtk_box_pack_start(vbox, label, TRUE, TRUE, 4);

	/* The Centred, Scaled, Fit, Tiled radios... */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_box_pack_start(vbox, hbox, TRUE, TRUE, 4);

	radios = radios_new(radios_changed, dialog);
	g_object_set_data(G_OBJECT(dialog), "rox-radios", radios);
	g_object_set_data(G_OBJECT(dialog), "rox-radios-hbox", hbox);

	radios_add(radios, _("Centre the image without scaling it"),
			BACKDROP_CENTRE, _("Centre"));
	radios_add(radios, _("Scale the image to fit the backdrop area, "
			     "without distorting it"),
			BACKDROP_SCALE, _("Scale"));
	radios_add(radios, _("Scale the image to fit the backdrop area, "
			     "regardless of image dimensions - overscale"),
			BACKDROP_FIT, _("Fit"));
	radios_add(radios, _("Stretch the image to fill the backdrop area"),
			BACKDROP_STRETCH, _("Stretch"));
	radios_add(radios, _("Tile the image over the backdrop area"),
			BACKDROP_TILE, _("Tile"));

	update_radios(dialog);

	/* The drop area... */
	frame = drop_box_new(_("Drop an image here"));
	g_object_set_data(G_OBJECT(dialog), "rox-dropbox", frame);

	radios_pack(radios, GTK_BOX(hbox));
	gtk_box_pack_start(vbox, frame, TRUE, TRUE, 4);

	drop_box_set_path(DROP_BOX(frame), current_pinboard->backdrop);

	g_signal_connect(frame, "path_dropped",
			G_CALLBACK(drag_backdrop_dropped), dialog);
	g_signal_connect(frame, "clear",
			G_CALLBACK(clear_backdrop), dialog);

	g_signal_connect(dialog, "response",
			 G_CALLBACK(backdrop_response), NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gtk_widget_show_all(dialog);
}

/* GTK3-native appearance for pinboard icons.  Keep the ROX layout while
 * GtkImage, WrappedLabel and CSS perform all rendering through public APIs. */
static void pinboard_update_css(Pinboard *pinboard)
{
	gchar *css;
	gchar fg[8], bg[8], shadow[8];
	const gchar *shadow_rule;

	g_return_if_fail(pinboard != NULL);

	g_snprintf(fg, sizeof(fg), "#%02x%02x%02x",
		(guint) CLAMP(pin_text_fg_col.red * 255.0 + 0.5, 0.0, 255.0),
		(guint) CLAMP(pin_text_fg_col.green * 255.0 + 0.5, 0.0, 255.0),
		(guint) CLAMP(pin_text_fg_col.blue * 255.0 + 0.5, 0.0, 255.0));
	g_snprintf(bg, sizeof(bg), "#%02x%02x%02x",
		(guint) CLAMP(pin_text_bg_col.red * 255.0 + 0.5, 0.0, 255.0),
		(guint) CLAMP(pin_text_bg_col.green * 255.0 + 0.5, 0.0, 255.0),
		(guint) CLAMP(pin_text_bg_col.blue * 255.0 + 0.5, 0.0, 255.0));
	g_snprintf(shadow, sizeof(shadow), "#%02x%02x%02x",
		(guint) CLAMP(pin_text_shadow_col.red * 255.0 + 0.5, 0.0, 255.0),
		(guint) CLAMP(pin_text_shadow_col.green * 255.0 + 0.5, 0.0, 255.0),
		(guint) CLAMP(pin_text_shadow_col.blue * 255.0 + 0.5, 0.0, 255.0));

	shadow_rule = o_pinboard_shadow_labels.int_value
		? "text-shadow: 1px 1px 1px %s;" : "text-shadow: none;";

	if (!pinboard->css_provider)
	{
		pinboard->css_provider = gtk_css_provider_new();
		gtk_style_context_add_provider_for_screen(
			gtk_widget_get_screen(pinboard->window),
			GTK_STYLE_PROVIDER(pinboard->css_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}

	if (o_pinboard_shadow_labels.int_value)
	{
		css = g_strdup_printf(
			"#rox-pinboard button.rox-pin-icon {"
			" background: transparent; border: 2px solid transparent;"
			" box-shadow: none; padding: 0; }"
			"#rox-pinboard button.rox-pin-icon:hover { background: transparent; }"
			"#rox-pinboard button.rox-pin-icon.rox-wink {"
			" border-color: #ffffff; box-shadow: inset 0 0 0 1px #000000; }"
			"#rox-pinboard label.rox-pin-label {"
			" color: %s; background: transparent;"
			" text-shadow: 1px 1px 1px %s; padding: 1px 2px; }"
			"#rox-pinboard label.rox-pin-label.rox-selected {"
			" color: @theme_selected_fg_color;"
			" background-color: @theme_selected_bg_color;"
			" text-shadow: none; border-radius: 2px; }",
			fg, shadow);
	}
	else
	{
		css = g_strdup_printf(
			"#rox-pinboard button.rox-pin-icon {"
			" background: transparent; border: 2px solid transparent;"
			" box-shadow: none; padding: 0; }"
			"#rox-pinboard button.rox-pin-icon:hover { background: transparent; }"
			"#rox-pinboard button.rox-pin-icon.rox-wink {"
			" border-color: #ffffff; box-shadow: inset 0 0 0 1px #000000; }"
			"#rox-pinboard label.rox-pin-label {"
			" color: %s; background: transparent; text-shadow: none;"
			" padding: 1px 2px; }"
			"#rox-pinboard label.rox-pin-label.rox-selected {"
			" color: @theme_selected_fg_color;"
			" background-color: @theme_selected_bg_color;"
			" text-shadow: none; border-radius: 2px; }",
			fg);
	}

	gtk_css_provider_load_from_data(pinboard->css_provider, css, -1, NULL);
	g_free(css);

	/* The backdrop colour is painted by bg_draw(), not by deprecated
	 * gtk_widget_modify_bg()/GtkStyle pixmaps. */
	pinboard->background_rgba = pin_text_bg_col;
	pinboard->background_rgba.alpha = 1.0;
	(void) bg;
	(void) shadow_rule;
}

/* Set and save (path, style) as the new backdrop. */
void pinboard_set_backdrop(const gchar *path, BackdropStyle style)
{
	g_return_if_fail((path == NULL && style == BACKDROP_NONE) ||
			 (path != NULL && style != BACKDROP_NONE));

	if (!current_pinboard)
	{
		if (!path)
			return;
		pinboard_activate("Default");
		delayed_error(_("No pinboard was in use... "
			"the 'Default' pinboard has been selected. "
			"Use 'rox -p=Default' to turn it on in future."));
		g_return_if_fail(current_pinboard != NULL);
	}

	abandon_backdrop_app(current_pinboard);
	g_free(current_pinboard->backdrop);
	current_pinboard->backdrop = g_strdup(path);
	current_pinboard->backdrop_style = style;
	reload_backdrop(current_pinboard, current_pinboard->backdrop,
		current_pinboard->backdrop_style);
	pinboard_save();

	if (set_backdrop_dialog)
	{
		DropBox *box = g_object_get_data(G_OBJECT(set_backdrop_dialog),
			"rox-dropbox");
		g_return_if_fail(box != NULL);
		drop_box_set_path(box, current_pinboard->backdrop);
		update_radios(set_backdrop_dialog);
	}
}

/* Called on XRandR screen resizes. */
void pinboard_update_size(void)
{
	int width, height;

	if (!current_pinboard)
		return;

	gtk_window_get_size(GTK_WINDOW(current_pinboard->window), &width, &height);
	width = MAX(width, screen_width);
	height = MAX(height, screen_height);
	gtk_widget_set_size_request(current_pinboard->window, width, height);
}

static void backdrop_response(GtkWidget *dialog, gint response, gpointer data)
{
	gtk_widget_destroy(dialog);
}

static void clear_backdrop(GtkWidget *drop_box, gpointer data)
{
	pinboard_set_backdrop(NULL, BACKDROP_NONE);
}

static void drag_backdrop_dropped(GtkWidget *drop_box,
				  const guchar *path,
				  GtkWidget *dialog)
{
	struct stat info;
	Radios *radios;

	radios = g_object_get_data(G_OBJECT(dialog), "rox-radios");
	g_return_if_fail(radios != NULL);

	if (mc_stat(path, &info))
	{
		delayed_error(_("Can't access '%s':\n%s"), path,
			g_strerror(errno));
		return;
	}

	if (S_ISDIR(info.st_mode))
		pinboard_set_backdrop_app(path);
	else if (S_ISREG(info.st_mode))
		pinboard_set_backdrop((const gchar *) path,
			radios_get_value(radios));
	else
		delayed_error(_("Only files (and certain applications) can be "
			"used to set the background image."));
}

static gboolean recreate_pinboard(gchar *name)
{
	pinboard_activate(name);
	g_free(name);
	return G_SOURCE_REMOVE;
}

static void pinboard_check_options(void)
{
	GdkRGBA n_fg, n_bg, n_shadow;

	gdk_rgba_parse(&n_fg, (const gchar *) o_pinboard_fg_colour.value);
	gdk_rgba_parse(&n_bg, (const gchar *) o_pinboard_bg_colour.value);
	gdk_rgba_parse(&n_shadow, (const gchar *) o_pinboard_shadow_colour.value);

	if (o_override_redirect.has_changed && current_pinboard)
	{
		gchar *name = g_strdup((const gchar *) current_pinboard->name);
		pinboard_activate(NULL);
		g_idle_add((GSourceFunc) recreate_pinboard, name);
	}

	tasklist_set_active(o_pinboard_tasklist.int_value && current_pinboard);

	if (!gdk_rgba_equal(&n_fg, &pin_text_fg_col) ||
		!gdk_rgba_equal(&n_bg, &pin_text_bg_col) ||
		!gdk_rgba_equal(&n_shadow, &pin_text_shadow_col) ||
		o_pinboard_shadow_labels.has_changed || o_label_font.has_changed)
	{
		pin_text_fg_col = n_fg;
		pin_text_bg_col = n_bg;
		pin_text_shadow_col = n_shadow;
		update_pinboard_font();

		if (current_pinboard)
		{
			pinboard_update_css(current_pinboard);
			reload_backdrop(current_pinboard,
				current_pinboard->backdrop,
				current_pinboard->backdrop_style);
			reshape_all();
		}
		tasklist_style_changed();
	}
}

static gint end_wink(gpointer data)
{
	pinboard_wink_item(NULL, FALSE);
	return G_SOURCE_REMOVE;
}

static GdkPixbuf *pin_icon_emblem(DirItem *item)
{
	const gchar *name = NULL;
	GError *error = NULL;
	GdkPixbuf *pixbuf;

	if (item->flags & ITEM_FLAG_SYMLINK)
		name = "emblem-symbolic-link";
	else if (item->flags & ITEM_FLAG_MOUNT_POINT)
		name = (item->flags & ITEM_FLAG_MOUNTED)
			? "media-eject" : "drive-harddisk";

	if (!name)
		return NULL;

	pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name,
		16, GTK_ICON_LOOKUP_FORCE_SIZE, &error);
	if (error)
	{
		g_error_free(error);
		return NULL;
	}
	return pixbuf;
}

static GdkPixbuf *pin_icon_render_pixbuf(PinIcon *pi)
{
	Icon *icon = (Icon *) pi;
	MaskedPixmap *image = di_image(icon->item);
	GdkPixbuf *pixbuf;
	GdkPixbuf *emblem;

	if (icon->selected)
	{
		GdkRGBA colour = {0.25, 0.45, 0.75, 1.0};
		GtkStyleContext *context = gtk_widget_get_style_context(pi->label);

		rox_style_context_get_background(context,
			GTK_STATE_FLAG_SELECTED, &colour);
		colour.alpha = 1.0;
		pixbuf = create_spotlight_pixbuf(image->pixbuf, &colour);
	}
	else
		pixbuf = gdk_pixbuf_copy(image->pixbuf);

	emblem = pin_icon_emblem(icon->item);
	if (emblem)
	{
		gint width = MIN(16, gdk_pixbuf_get_width(pixbuf));
		gint height = MIN(16, gdk_pixbuf_get_height(pixbuf));
		gdk_pixbuf_composite(emblem, pixbuf,
			0, 0, width, height, 0, 0,
			width / (gdouble) gdk_pixbuf_get_width(emblem),
			height / (gdouble) gdk_pixbuf_get_height(emblem),
			GDK_INTERP_BILINEAR, 255);
		g_object_unref(emblem);
	}

	return pixbuf;
}

/* Sets the GTK3 widget appearance and updates its natural size. */
static void set_size_and_style(PinIcon *pi)
{
	Icon *icon = (Icon *) pi;
	GdkPixbuf *pixbuf = pin_icon_render_pixbuf(pi);
	GtkStyleContext *label_context;

	gtk_image_set_from_pixbuf(GTK_IMAGE(pi->widget), pixbuf);
	g_object_unref(pixbuf);

	widget_modify_font(pi->label, pinboard_font);
	wrapped_label_set_text(WRAPPED_LABEL(pi->label), icon->item->leafname);

	label_context = gtk_widget_get_style_context(pi->label);
	if (icon->selected)
		gtk_style_context_add_class(label_context, "rox-selected");
	else
		gtk_style_context_remove_class(label_context, "rox-selected");
}

static gboolean enter_notify(GtkWidget *widget,
			     GdkEventCrossing *event,
			     PinIcon *pi)
{
	icon_may_update((Icon *) pi);
	pin_icon_set_tip(pi);
	return TRUE;
}

static gint leave_notify(GtkWidget *widget,
			      GdkEventCrossing *event,
			      PinIcon *pi)
{
	return TRUE;
}

static void select_lasso(void)
{
	GList *next;
	int minx, miny, maxx, maxy;

	g_return_if_fail(lasso_in_progress == TRUE);

	minx = MIN(lasso_rect_x1, lasso_rect_x2);
	miny = MIN(lasso_rect_y1, lasso_rect_y2);
	maxx = MAX(lasso_rect_x1, lasso_rect_x2);
	maxy = MAX(lasso_rect_y1, lasso_rect_y2);

	for (next = current_pinboard->icons; next; next = next->next)
	{
		PinIcon *pi = (PinIcon *) next->data;
		GtkAllocation alloc;
		int cx, cy;

		gtk_widget_get_allocation(pi->win, &alloc);
		cx = alloc.x + alloc.width / 2;
		cy = alloc.y + alloc.height / 2;

		if (cx > minx && cx < maxx && cy > miny && cy < maxy)
			icon_set_selected((Icon *) pi, TRUE);
	}
}

static void cancel_lasso(void)
{
	draw_lasso();
	lasso_in_progress = FALSE;
}

static void pinboard_lasso_box(int start_x, int start_y)
{
	if (lasso_in_progress)
		cancel_lasso();
	lasso_in_progress = TRUE;
	lasso_rect_x1 = lasso_rect_x2 = start_x;
	lasso_rect_y1 = lasso_rect_y2 = start_y;

	draw_lasso();
}

static gint lasso_motion(GtkWidget *widget, GdkEventMotion *event, gpointer d)
{
	if (!lasso_in_progress)
		return FALSE;

	if (lasso_rect_x2 != event->x || lasso_rect_y2 != event->y)
	{
		draw_lasso();
		lasso_rect_x2 = event->x;
		lasso_rect_y2 = event->y;
		draw_lasso();
	}

	return FALSE;
}

/* Queue the GTK3 drawing layer.  Cairo repaints the old and new lasso
 * positions, so no XOR or manual GdkWindow invalidation is required. */
static void draw_lasso(void)
{
	if (current_pinboard)
		gtk_widget_queue_draw(current_pinboard->fixed);
}

static void perform_action(PinIcon *pi, GdkEventButton *event)
{
	BindAction	action;
	Icon		*icon = (Icon *) pi;

	action = bind_lookup_bev(pi ? BIND_PINBOARD_ICON : BIND_PINBOARD,
				 event);

	/* Actions that can happen with or without an icon */
	switch (action)
	{
		case ACT_LASSO_CLEAR:
			icon_select_only(NULL);
			/* (no break) */
		case ACT_LASSO_MODIFY:
			pinboard_lasso_box(event->x, event->y);
			return;
		case ACT_CLEAR_SELECTION:
			icon_select_only(NULL);
			return;
		case ACT_POPUP_MENU:
			dnd_motion_ungrab();
			pinboard_show_menu(event, pi);
			return;
		case ACT_IGNORE:
			return;
		default:
			break;
	}

	g_return_if_fail(pi != NULL);

	switch (action)
	{
		case ACT_OPEN_ITEM:
			dnd_motion_ungrab();
			pinboard_wink_item(pi, TRUE);
			if (event->type == GDK_2BUTTON_PRESS)
				icon_set_selected(icon, FALSE);
			icon_run(icon);
			break;
		case ACT_EDIT_ITEM:
			dnd_motion_ungrab();
			pinboard_wink_item(pi, TRUE);
			if (event->type == GDK_2BUTTON_PRESS)
				icon_set_selected(icon, FALSE);
			run_diritem(icon->path, icon->item, NULL, NULL, TRUE);
			break;
		case ACT_PRIME_AND_SELECT:
			if (!icon->selected)
				icon_select_only(icon);
			dnd_motion_start(MOTION_READY_FOR_DND);
			break;
		case ACT_PRIME_AND_TOGGLE:
			icon_set_selected(icon, !icon->selected);
			dnd_motion_start(MOTION_READY_FOR_DND);
			break;
		case ACT_PRIME_FOR_DND:
			dnd_motion_start(MOTION_READY_FOR_DND);
			break;
		case ACT_TOGGLE_SELECTED:
			icon_set_selected(icon, !icon->selected);
			break;
		case ACT_SELECT_EXCL:
			icon_select_only(icon);
			break;
		default:
			g_warning("Unsupported action : %d\n", action);
			break;
	}
}

static void forward_to_root(GdkEventButton *event)
{
	XButtonEvent xev;

	if (event->type == GDK_BUTTON_PRESS)
	{
		xev.type = ButtonPress;
		if (!o_blackbox_hack.int_value)
		{
			GdkDisplay *display = gdk_display_get_default();
			GdkSeat *seat = display ? gdk_display_get_default_seat(display) : NULL;
			if (seat)
				gdk_seat_ungrab(seat);
		}
	}
	else
		xev.type = ButtonRelease;

	xev.window = gdk_x11_get_default_root_xwindow();
	xev.root =  xev.window;
	xev.subwindow = None;
	xev.time = event->time;
	xev.x = event->x_root;	/* Needed for icewm */
	xev.y = event->y_root;
	xev.x_root = event->x_root;
	xev.y_root = event->y_root;
	xev.state = event->state;
	xev.button = event->button;
	xev.same_screen = True;

	XSendEvent(rox_x11_display(), xev.window, False,
		ButtonPressMask | ButtonReleaseMask, (XEvent *) &xev);
}

#define FORWARDED_BUTTON(pi, b) ((b) == 2 || \
	(((b) == 3 || (b) == 1) && o_forward_buttons_13.int_value && !pi))

/* pi is NULL if this is a root event */
static gboolean button_release_event(GtkWidget *widget,
			    	     GdkEventButton *event,
                            	     PinIcon *pi)
{
	if (FORWARDED_BUTTON(pi, event->button))
		forward_to_root(event);
	else if (dnd_motion_release(event))
	{
		if (motion_buttons_pressed == 0 && lasso_in_progress)
		{
			select_lasso();
			cancel_lasso();
		}
		return FALSE;
	}

	perform_action(pi, event);

	return TRUE;
}

/* pi is NULL if this is a root event */
static gboolean button_press_event(GtkWidget *widget,
			    	   GdkEventButton *event,
                            	   PinIcon *pi)
{
	/* Just in case we've jumped in front of everything... */
	gdk_window_lower(gtk_widget_get_window(current_pinboard->window));

	if (FORWARDED_BUTTON(pi, event->button))
		forward_to_root(event);
	else if (dnd_motion_press(widget, event))
		perform_action(pi, event);

	return TRUE;
}

/* Forward mouse scroll events as buttons 4 and 5 to the window manager
 * (for old window managers that don't catch the buttons themselves)
 */
static gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event)
{
	XButtonEvent xev;

	xev.type = ButtonPress;
	xev.window = gdk_x11_get_default_root_xwindow();
	xev.root =  xev.window;
	xev.subwindow = None;
	xev.time = event->time;
	xev.x = event->x_root;	/* Needed for icewm */
	xev.y = event->y_root;
	xev.x_root = event->x_root;
	xev.y_root = event->y_root;
	xev.state = event->state;
	xev.same_screen = True;

	if (event->direction == GDK_SCROLL_UP)
		xev.button = 4;
	else if (event->direction == GDK_SCROLL_DOWN)
		xev.button = 5;
	else
		return FALSE;

	XSendEvent(rox_x11_display(), xev.window, False,
			ButtonPressMask, (XEvent *) &xev);

	return TRUE;
}

static void start_drag(PinIcon *pi, GdkEventMotion *event)
{
	GtkWidget *widget = pi->win;
	Icon	  *icon = (Icon *) pi;

	if (!icon->selected)
	{
		tmp_icon_selected = TRUE;
		icon_select_only(icon);
	}

	g_return_if_fail(icon_selection != NULL);

	pinboard_drag_in_progress = icon;

	if (icon_selection->next == NULL)
		drag_one_item(widget, event, icon->path, icon->item, NULL);
	else
	{
		guchar	*uri_list;

		uri_list = icon_create_uri_list();
		drag_selection(widget, event, uri_list);
		g_free(uri_list);
	}
}

/* An icon is being dragged around... */
static gint icon_motion_notify(GtkWidget *widget,
			       GdkEventMotion *event,
			       PinIcon *pi)
{
	if (motion_state == MOTION_READY_FOR_DND)
	{
		if (dnd_motion_moved(event))
			start_drag(pi, event);
		return TRUE;
	}

	return FALSE;
}

static void backdrop_from_xml(xmlNode *node)
{
	gchar *style;

	g_free(current_pinboard->backdrop);
	current_pinboard->backdrop = xmlNodeGetContent(node);

	style = xmlGetProp(node, "style");

	if (style)
	{
		current_pinboard->backdrop_style =
		  g_ascii_strcasecmp(style, "Tiled") == 0 ? BACKDROP_TILE :
		  g_ascii_strcasecmp(style, "Scaled") == 0 ? BACKDROP_SCALE :
		  g_ascii_strcasecmp(style, "Fit") == 0 ? BACKDROP_FIT :
		  g_ascii_strcasecmp(style, "Stretched") == 0 ? BACKDROP_STRETCH :
		  g_ascii_strcasecmp(style, "Centred") == 0 ? BACKDROP_CENTRE :
		  g_ascii_strcasecmp(style, "Program") == 0 ? BACKDROP_PROGRAM :
							     BACKDROP_NONE;
		g_free(style);
	}
	else
		current_pinboard->backdrop_style = BACKDROP_TILE;
}

/* Create one pinboard icon for each icon in the doc */
static void pinboard_load_from_xml(xmlDocPtr doc)
{
	xmlNodePtr node, root;
	char	   *tmp, *label, *path, *shortcut, *args;
	int	   x, y;
	gboolean locked;

	root = xmlDocGetRootElement(doc);

	for (node = root->xmlChildrenNode; node; node = node->next)
	{
		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (strcmp(node->name, "backdrop") == 0)
		{
			backdrop_from_xml(node);
			continue;
		}
		if (strcmp(node->name, "icon") != 0)
			continue;

		tmp = xmlGetProp(node, "x");
		if (!tmp)
			continue;
		x = atoi(tmp);
		g_free(tmp);

		tmp = xmlGetProp(node, "y");
		if (!tmp)
			continue;
		y = atoi(tmp);
		g_free(tmp);

		label = xmlGetProp(node, "label");
		if (!label)
			label = g_strdup("<missing label>");
		path = xmlNodeGetContent(node);
		if (!path)
			path = g_strdup("<missing path>");
		shortcut = xmlGetProp(node, "shortcut");
		args = xmlGetProp(node, "args");

		tmp = xmlGetProp(node, "locked");
		if (tmp)
		{
			locked = text_to_boolean(tmp, FALSE);
			g_free(tmp);
		}
		else
			locked = FALSE;

		pinboard_pin_with_args(path, label, x, y, shortcut, args, locked, FALSE);

		g_free(path);
		g_free(label);
		g_free(shortcut);
		g_free(args);
	}
}

/* Called for each line in the pinboard file while loading a new board.
 * Only used for old-format files when converting to XML.
 */
static const char *pin_from_file(gchar *line)
{
	gchar	*leaf = NULL;
	int	x, y, n;

	if (*line == '<')
	{
		gchar	*end;

		end = strchr(line + 1, '>');
		if (!end)
			return _("Missing '>' in icon label");

		leaf = g_strndup(line + 1, end - line - 1);

		line = end + 1;

		while (g_ascii_isspace(*line))
			line++;
		if (*line != ',')
			return _("Missing ',' after icon label");
		line++;
	}

	if (sscanf(line, " %d , %d , %n", &x, &y, &n) < 2)
		return NULL;		/* Ignore format errors */

	pinboard_pin(line + n, leaf, x, y, NULL);

	g_free(leaf);

	return NULL;
}

/* Write the current state of the pinboard to the current pinboard file */
static void pinboard_save(void)
{
	guchar	*save = NULL;
	guchar	*save_new = NULL;
	GList	*next;
	xmlDocPtr doc = NULL;
	xmlNodePtr root;

	g_return_if_fail(current_pinboard != NULL);

	if (strchr(current_pinboard->name, '/'))
		save = g_strdup(current_pinboard->name);
	else
	{
		guchar	*leaf;

		leaf = g_strconcat("pb_", current_pinboard->name, NULL);
		save = choices_find_xdg_path_save(leaf, PROJECT, SITE, TRUE);
		g_free(leaf);
	}

	if (!save)
		return;

	doc = xmlNewDoc("1.0");
	xmlDocSetRootElement(doc, xmlNewDocNode(doc, NULL, "pinboard", NULL));

	root = xmlDocGetRootElement(doc);

	if (current_pinboard->backdrop)
	{
		BackdropStyle style = current_pinboard->backdrop_style;
		xmlNodePtr tree;

		tree = xmlNewTextChild(root, NULL, "backdrop",
				current_pinboard->backdrop);
		xmlSetProp(tree, "style",
			style == BACKDROP_TILE   ? "Tiled" :
			style == BACKDROP_CENTRE ? "Centred" :
			style == BACKDROP_SCALE  ? "Scaled" :
			style == BACKDROP_FIT    ? "Fit" :
			style == BACKDROP_STRETCH  ? "Stretched" :
						   "Program");
	}

	for (next = current_pinboard->icons; next; next = next->next)
	{
		xmlNodePtr tree;
		PinIcon *pi = (PinIcon *) next->data;
		Icon	*icon = (Icon *) pi;
		char *tmp;

		tree = xmlNewTextChild(root, NULL, "icon", icon->src_path);

		tmp = g_strdup_printf("%d", pi->x);
		xmlSetProp(tree, "x", tmp);
		g_free(tmp);

		tmp = g_strdup_printf("%d", pi->y);
		xmlSetProp(tree, "y", tmp);
		g_free(tmp);

		xmlSetProp(tree, "label", icon->item->leafname);
		if (icon->shortcut)
			xmlSetProp(tree, "shortcut", icon->shortcut);
		if (icon->args)
			xmlSetProp(tree, "args", icon->args);
		if (icon->locked)
			xmlSetProp(tree, "locked", "true");
	}

	save_new = g_strconcat(save, ".new", NULL);
	if (save_xml_file(doc, save_new) || rename(save_new, save))
		delayed_error(_("Error saving pinboard %s: %s"),
				save, g_strerror(errno));
	g_free(save_new);

	g_free(save);
	if (doc)
		xmlFreeDoc(doc);
}

static void snap_to_grid(int *x, int *y)
{
	int step = o_pinboard_grid_step.int_value;

	*x = ((*x + step / 2) / step) * step;
	*y = ((*y + step / 2) / step) * step;
}

/* Convert (x,y) from a centre point to a window position */
static void offset_from_centre(PinIcon *pi, int *x, int *y)
{
	gboolean clamp = o_pinboard_clamp_icons.int_value;
	GtkRequisition req;

	gtk_widget_get_preferred_size(pi->win, &req, NULL);

	*x -= req.width >> 1;
	*y -= req.height >> 1;
	*x = CLAMP(*x, 0, screen_width - (clamp ? req.width : 0));
	*y = CLAMP(*y, 0, screen_height - (clamp ? req.height : 0));
}

/* Same as drag_set_dest(), but for pinboard icons */
static void drag_set_pinicon_dest(PinIcon *pi)
{
	GtkWidget	*obj = pi->win;

	make_drop_target(pi->win, 0);

	g_signal_connect(obj, "drag_motion", G_CALLBACK(drag_motion), pi);
	g_signal_connect(obj, "drag_leave", G_CALLBACK(drag_leave), pi);
	g_signal_connect(obj, "drag_end", G_CALLBACK(drag_end), pi);
}

/* Called during the drag when the mouse is in a widget registered
 * as a drop target. Returns TRUE if we can accept the drop.
 */
static gboolean drag_motion(GtkWidget		*widget,
                            GdkDragContext	*context,
                            gint		x,
                            gint		y,
                            guint		time,
			    PinIcon		*pi)
{
	GdkDragAction	action = gdk_drag_context_get_suggested_action(context);
	const char	*type = NULL;
	Icon		*icon = (Icon *) pi;
	DirItem		*item = icon->item;

	if (gtk_drag_get_source_widget(context) == widget)
	{
		g_dataset_set_data(context, "drop_dest_type",
				   (gpointer) drop_dest_pass_through);
		return FALSE;	/* Can't drag something to itself! */
	}

	if (icon->selected)
		goto out;	/* Can't drag a selection to itself */

	type = dnd_motion_item(context, &item);

	if ((gdk_drag_context_get_actions(context) & GDK_ACTION_ASK) && o_dnd_left_menu.int_value
		&& type != drop_dest_prog)
	{
		GdkModifierType state = 0;
		GdkDisplay *display = gtk_widget_get_display(widget);
		GdkSeat *seat = gdk_display_get_default_seat(display);
		GdkDevice *pointer = gdk_seat_get_pointer(seat);
		GdkWindow *window = gtk_widget_get_window(widget);
		if (window)
			gdk_device_get_state(pointer, window, NULL, &state);
		if (state & GDK_BUTTON1_MASK)
			action = GDK_ACTION_ASK;
	}

	if (!item)
		type = NULL;
out:
	/* We actually must pretend to accept the drop, even if the
	 * directory isn't writeable, so that the spring-opening
	 * thing works.
	 */

	/* Don't allow drops to non-writeable directories */
	if (o_dnd_spring_open.int_value == FALSE &&
			type == drop_dest_dir &&
			access(icon->path, W_OK) != 0)
	{
		type = NULL;
	}

	g_dataset_set_data(context, "drop_dest_type", (gpointer) type);
	if (type)
	{
		gdk_drag_status(context, action, time);
		g_dataset_set_data_full(context, "drop_dest_path",
				g_strdup(icon->path), g_free);
		if (type == drop_dest_dir)
			dnd_spring_load(context, NULL);

		pinboard_wink_item(pi, FALSE);
	}
	else
		gdk_drag_status(context, 0, time);

	/* Always return TRUE to stop the pinboard getting the events */
	return TRUE;
}

static gboolean pinboard_shadow = FALSE;
static gint shadow_x, shadow_y;
#define SHADOW_SIZE (ICON_WIDTH)

static gboolean bg_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	Pinboard *pinboard = current_pinboard;

	if (!pinboard)
		return FALSE;

	gdk_cairo_set_source_rgba(cr, &pinboard->background_rgba);
	cairo_paint(cr);

	if (pinboard->backdrop_pixbuf)
	{
		gdk_cairo_set_source_pixbuf(cr, pinboard->backdrop_pixbuf, 0, 0);
		if (pinboard->backdrop_style == BACKDROP_TILE)
			cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_REPEAT);
		cairo_paint(cr);
	}

	cairo_set_line_width(cr, 1.0);
	if (pinboard_shadow)
	{
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_rectangle(cr, shadow_x + 0.5, shadow_y + 0.5,
			SHADOW_SIZE, SHADOW_SIZE);
		cairo_stroke(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_rectangle(cr, shadow_x + 1.5, shadow_y + 1.5,
			SHADOW_SIZE - 2, SHADOW_SIZE - 2);
		cairo_stroke(cr);
	}

	if (lasso_in_progress)
	{
		gint x = MIN(lasso_rect_x1, lasso_rect_x2);
		gint y = MIN(lasso_rect_y1, lasso_rect_y2);
		gint width = ABS(lasso_rect_x1 - lasso_rect_x2);
		gint height = ABS(lasso_rect_y1 - lasso_rect_y2);

		if (width > 4 && height > 4)
		{
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_rectangle(cr, x + 0.5, y + 0.5, width - 1, height - 1);
			cairo_stroke(cr);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_rectangle(cr, x + 1.5, y + 1.5, width - 3, height - 3);
			cairo_stroke(cr);
		}
	}

	return FALSE;
}

/* Draw a 'shadow' under an icon being dragged, showing where
 * it will land.
 */
static void pinboard_set_shadow(gboolean on)
{
	if (on)
	{
		GdkWindow *window = gtk_widget_get_window(current_pinboard->fixed);
		GdkDisplay *display = gtk_widget_get_display(current_pinboard->fixed);
		GdkSeat *seat = gdk_display_get_default_seat(display);
		GdkDevice *pointer = gdk_seat_get_pointer(seat);
		gint old_x = shadow_x, old_y = shadow_y;

		if (window)
			gdk_window_get_device_position(window, pointer,
				&shadow_x, &shadow_y, NULL);
		snap_to_grid(&shadow_x, &shadow_y);
		shadow_x -= SHADOW_SIZE / 2;
		shadow_y -= SHADOW_SIZE / 2;

		if (pinboard_shadow && shadow_x == old_x && shadow_y == old_y)
			return;
	}

	pinboard_shadow = on;
	gtk_widget_queue_draw(current_pinboard->fixed);
}

/* Called when dragging some pinboard icons finishes */
void pinboard_move_icons(void)
{
	int	x = shadow_x, y = shadow_y;
	PinIcon	*pi = (PinIcon *) pinboard_drag_in_progress;
	int	width, height;
	int	dx, dy;
	GList	*next;

	g_return_if_fail(pi != NULL);

	x += SHADOW_SIZE / 2;
	y += SHADOW_SIZE / 2;
	snap_to_grid(&x, &y);

	if (pi->x == x && pi->y == y)
		return;

	/* Find out how much the dragged icon moved (after snapping).
	 * Move all selected icons by the same amount.
	 */
	dx = x - pi->x;
	dy = y - pi->y;

	/* Move the other selected icons to keep the same relative
	 * position.
	 */
	for (next = icon_selection; next; next = next->next)
	{
		PinIcon *pi = (PinIcon *) next->data;
		int	nx, ny;

		g_return_if_fail(IS_PIN_ICON(pi));

		pi->x += dx;
		pi->y += dy;
		nx = pi->x;
		ny = pi->y;

		width = gtk_widget_get_allocated_width(pi->win);
		height = gtk_widget_get_allocated_height(pi->win);
		offset_from_centre(pi, &nx, &ny);

		fixed_move_fast(GTK_FIXED(current_pinboard->fixed),
				pi->win, nx, ny);
	}

	pinboard_save();
}

static void drag_leave(GtkWidget	*widget,
                       GdkDragContext	*context,
		       guint32		time,
		       PinIcon		*pi)
{
	pinboard_wink_item(NULL, FALSE);
	dnd_spring_abort();
}

static gboolean bg_drag_leave(GtkWidget		*widget,
			      GdkDragContext	*context,
			      guint32		time,
			      gpointer		data)
{
	pinboard_set_shadow(FALSE);
	return TRUE;
}

static gboolean bg_drag_motion(GtkWidget	*widget,
                               GdkDragContext	*context,
                               gint		x,
                               gint		y,
                               guint		time,
			       gpointer		data)
{
	/* Dragging from the pinboard to the pinboard is not allowed */

	if (!provides(context, text_uri_list))
		return FALSE;

	pinboard_set_shadow(TRUE);

	{
		GdkDragAction suggested = gdk_drag_context_get_suggested_action(context);
		gdk_drag_status(context,
			suggested == GDK_ACTION_ASK ? GDK_ACTION_LINK : suggested,
			time);
	}
	return TRUE;
}

static void drag_end(GtkWidget *widget,
		     GdkDragContext *context,
		     PinIcon *pi)
{
	pinboard_drag_in_progress = NULL;
	if (tmp_icon_selected)
	{
		icon_select_only(NULL);
		tmp_icon_selected = FALSE;
	}
}

/* Something which affects all the icons has changed - reshape
 * and redraw all of them.
 */
static void reshape_all(void)
{
	GList	*next;

	g_return_if_fail(current_pinboard != NULL);

	for (next = current_pinboard->icons; next; next = next->next)
	{
		Icon *icon = (Icon *) next->data;
		pinboard_reshape_icon(icon);
	}
}

/* Turns off the pinboard. Does not call gtk_main_quit. */
static void pinboard_clear(void)
{
	GList	*next;

	g_return_if_fail(current_pinboard != NULL);

	tasklist_set_active(FALSE);

	next = current_pinboard->icons;
	while (next)
	{
		PinIcon	*pi = (PinIcon *) next->data;

		next = next->next;

		gtk_widget_destroy(pi->win);
	}

	gtk_widget_destroy(current_pinboard->window);

	abandon_backdrop_app(current_pinboard);

	if (current_pinboard->backdrop_pixbuf)
		g_object_unref(current_pinboard->backdrop_pixbuf);
	if (current_pinboard->css_provider)
		g_object_unref(current_pinboard->css_provider);

	g_free(current_pinboard->name);
	null_g_free(&current_pinboard);

	number_of_windows--;
}

static gpointer parent_class;

static void pin_icon_destroy(Icon *icon)
{
	PinIcon *pi = (PinIcon *) icon;

	g_return_if_fail(pi->win != NULL);

	gtk_widget_hide(pi->win);	/* Stops flicker - stupid GtkFixed! */
	gtk_widget_destroy(pi->win);
}

static void pinboard_remove_items(void)
{
	g_return_if_fail(icon_selection != NULL);

	while (icon_selection)
		icon_destroy((Icon *) icon_selection->data);

	pinboard_save();
}

static void pin_icon_update(Icon *icon)
{
	pinboard_reshape_icon(icon);
	pinboard_save();
}

static gboolean pin_icon_same_group(Icon *icon, Icon *other)
{
	return IS_PIN_ICON(other);
}

static void pin_wink_icon(Icon *icon)
{
	pinboard_wink_item((PinIcon *) icon, TRUE);
}

static void pin_icon_class_init(gpointer gclass, gpointer data)
{
	IconClass *icon = (IconClass *) gclass;

	parent_class = g_type_class_peek_parent(gclass);

	icon->destroy = pin_icon_destroy;
	icon->redraw = pinboard_reshape_icon;
	icon->update = pin_icon_update;
	icon->wink = pin_wink_icon;
	icon->remove_items = pinboard_remove_items;
	icon->same_group = pin_icon_same_group;
}

static void pin_icon_init(GTypeInstance *object, gpointer gclass)
{
}

static GType pin_icon_get_type(void)
{
	static GType type = 0;

	if (!type)
	{
		static const GTypeInfo info =
		{
			sizeof (PinIconClass),
			NULL,			/* base_init */
			NULL,			/* base_finalise */
			pin_icon_class_init,
			NULL,			/* class_finalise */
			NULL,			/* class_data */
			sizeof(PinIcon),
			0,			/* n_preallocs */
			pin_icon_init
		};

		type = g_type_register_static(icon_get_type(),
						"PinIcon", &info, 0);
	}

	return type;
}

static PinIcon *pin_icon_new(const char *pathname, const char *name)
{
	PinIcon *pi;
	Icon	  *icon;

	pi = g_object_new(pin_icon_get_type(), NULL);
	icon = (Icon *) pi;

	icon_set_path(icon, pathname, name);

	return pi;
}

/* Called when the window widget is somehow destroyed */
static void pin_icon_destroyed(PinIcon *pi)
{
	g_return_if_fail(pi->win != NULL);

	pi->win = NULL;

	pinboard_wink_item(NULL, FALSE);

	if (pinboard_drag_in_progress == (Icon *) pi)
		pinboard_drag_in_progress = NULL;

	if (current_pinboard)
		current_pinboard->icons =
			g_list_remove(current_pinboard->icons, pi);

	g_object_unref(pi);
}

/* Set the tooltip */
static void pin_icon_set_tip(PinIcon *pi)
{
	XMLwrapper	*ai;
	xmlNode 	*node;
	Icon		*icon = (Icon *) pi;

	g_return_if_fail(pi != NULL);

	ai = appinfo_get(icon->path, icon->item);

	if (ai && ((node = xml_get_section(ai, NULL, "Summary"))))
	{
		guchar *str;
		str = xmlNodeListGetString(node->doc,
				node->xmlChildrenNode, 1);
		if (str)
		{
			gtk_widget_set_tooltip_text(pi->win, str);
			g_free(str);
		}
	}
	else
		gtk_widget_set_tooltip_text(pi->widget, NULL);

	if (ai)
		g_object_unref(ai);
}

static void pinboard_show_menu(GdkEventButton *event, PinIcon *pi)
{
	GtkWidget *option_item;
	GtkWidget *new_panel_item;

	option_item = gtk_menu_item_new_with_label(_("Backdrop..."));
	g_signal_connect(option_item, "activate",
			 G_CALLBACK(pinboard_set_backdrop_box), NULL);
	new_panel_item = gtk_menu_item_new_with_label(_("Add Panel"));
	menu_item_set_icon(new_panel_item, ROX_ICON_ADD);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(new_panel_item),
			panel_new_panel_submenu());
	icon_prepare_menu((Icon *) pi, option_item, new_panel_item, NULL);
	gtk_widget_show_all(icon_menu);
	gtk_menu_popup_at_pointer(GTK_MENU(icon_menu), (GdkEvent *) event);
}

static void create_pinboard_window(Pinboard *pinboard)
{
	GtkWidget *win;
	GdkWindow *gdk_window;

	g_return_if_fail(pinboard->window == NULL);

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_set_name(win, "rox-pinboard");
	pinboard->window = win;
	pinboard->fixed = gtk_fixed_new();
	gtk_widget_set_hexpand(pinboard->fixed, TRUE);
	gtk_widget_set_vexpand(pinboard->fixed, TRUE);
	gtk_container_add(GTK_CONTAINER(win), pinboard->fixed);

	/* This remains an X11 desktop window for Puppy/Essora, but all GTK
	 * interaction now goes through GTK3 public accessors. */
	rox_window_set_wmclass(GTK_WINDOW(win), "ROX-Pinboard", PROJECT);
	gtk_widget_set_size_request(win, screen_width, screen_height);
	gtk_widget_realize(win);
	gtk_window_move(GTK_WINDOW(win), 0, 0);
	make_panel_window(win);

	gdk_window = gtk_widget_get_window(win);
	if (gdk_window)
	{
		GdkAtom desktop_type = gdk_atom_intern(
			"_NET_WM_WINDOW_TYPE_DESKTOP", FALSE);
		gdk_property_change(gdk_window,
			gdk_atom_intern("_NET_WM_WINDOW_TYPE", FALSE),
			gdk_atom_intern("ATOM", FALSE), 32,
			GDK_PROP_MODE_REPLACE, (guchar *) &desktop_type, 1);
	}

	gtk_widget_add_events(win,
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK |
		GDK_BUTTON3_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(win, "button-press-event",
		G_CALLBACK(button_press_event), NULL);
	g_signal_connect(win, "button-release-event",
		G_CALLBACK(button_release_event), NULL);
	g_signal_connect(win, "motion-notify-event",
		G_CALLBACK(lasso_motion), NULL);
	g_signal_connect(pinboard->fixed, "draw", G_CALLBACK(bg_draw), NULL);
	g_signal_connect(win, "scroll-event", G_CALLBACK(scroll_event), NULL);

	drag_set_pinboard_dest(win);
	g_signal_connect(win, "drag-motion", G_CALLBACK(bg_drag_motion), NULL);
	g_signal_connect(win, "drag-leave", G_CALLBACK(bg_drag_leave), NULL);

	pinboard_update_css(pinboard);
	reload_backdrop(pinboard, NULL, BACKDROP_NONE);

	gtk_widget_show_all(win);
	gdk_window = gtk_widget_get_window(win);
	if (gdk_window)
		gdk_window_lower(gdk_window);
}

/* Load image 'path' and scale according to 'style'.  GTK3 keeps the
 * backdrop as a GdkPixbuf and bg_draw() paints it with Cairo. */
static GdkPixbuf *load_backdrop(const gchar *path, BackdropStyle style)
{
	GdkPixbuf *pixbuf;
	GError *error = NULL;

	pixbuf = gdk_pixbuf_new_from_file(path, &error);
	if (!pixbuf)
	{
		delayed_error(_("Error loading backdrop image:\n%s\n"
			"Backdrop removed."), error ? error->message : _("Unknown error"));
		if (error)
			g_error_free(error);
		pinboard_set_backdrop(NULL, BACKDROP_NONE);
		return NULL;
	}

	if (style == BACKDROP_STRETCH)
	{
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf,
			screen_width, screen_height,
			o_pinboard_image_scaling.int_value
				? GDK_INTERP_BILINEAR : GDK_INTERP_HYPER);
		g_object_unref(pixbuf);
		return scaled;
	}

	if (style == BACKDROP_CENTRE || style == BACKDROP_SCALE ||
		style == BACKDROP_FIT)
	{
		GdkPixbuf *canvas;
		gint width = gdk_pixbuf_get_width(pixbuf);
		gint height = gdk_pixbuf_get_height(pixbuf);
		gdouble scale = 1.0;
		gdouble scale_x = screen_width / (gdouble) width;
		gdouble scale_y = screen_height / (gdouble) height;
		gint dest_width, dest_height, x, y;
		guint32 fill;

		if (style == BACKDROP_SCALE)
			scale = MIN(scale_x, scale_y);
		else if (style == BACKDROP_FIT)
			scale = MAX(scale_x, scale_y);

		dest_width = MAX(1, (gint) floor(width * scale + 0.5));
		dest_height = MAX(1, (gint) floor(height * scale + 0.5));
		x = (screen_width - dest_width) / 2;
		y = (screen_height - dest_height) / 2;

		canvas = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
			screen_width, screen_height);
		fill = ((guint32) CLAMP(pin_text_bg_col.red * 255.0 + 0.5,
			0.0, 255.0) << 24) |
			((guint32) CLAMP(pin_text_bg_col.green * 255.0 + 0.5,
			0.0, 255.0) << 16) |
			((guint32) CLAMP(pin_text_bg_col.blue * 255.0 + 0.5,
			0.0, 255.0) << 8) | 0xff;
		gdk_pixbuf_fill(canvas, fill);

		gdk_pixbuf_composite(pixbuf, canvas,
			MAX(0, x), MAX(0, y),
			MIN(screen_width, dest_width),
			MIN(screen_height, dest_height),
			x, y, scale, scale,
			o_pinboard_image_scaling.int_value
				? GDK_INTERP_BILINEAR : GDK_INTERP_HYPER,
			255);
		g_object_unref(pixbuf);
		return canvas;
	}

	/* BACKDROP_TILE keeps the source at its natural size; Cairo repeats it. */
	return pixbuf;
}

static void abandon_backdrop_app(Pinboard *pinboard)
{
	g_return_if_fail(pinboard != NULL);

	if (pinboard->to_backdrop_app != -1)
	{
		close(pinboard->to_backdrop_app);
		close(pinboard->from_backdrop_app);
		g_source_remove(pinboard->input_tag);
		g_string_free(pinboard->input_buffer, TRUE);
		pinboard->to_backdrop_app = -1;
		pinboard->from_backdrop_app = -1;
		pinboard->input_tag = -1;
		pinboard->input_buffer = NULL;
	}

	g_return_if_fail(pinboard->to_backdrop_app == -1);
	g_return_if_fail(pinboard->from_backdrop_app == -1);
	g_return_if_fail(pinboard->input_tag == -1);
	g_return_if_fail(pinboard->input_buffer == NULL);
}

/* A single line has been read from the child.
 * Processes the command, and replies 'ok' (or abandons the child on error).
 */
static void command_from_backdrop_app(Pinboard *pinboard, const gchar *command)
{
	BackdropStyle style;
	const char *ok = "ok\n";

	if (strncmp(command, "tile ", 5) == 0)
	{
		style = BACKDROP_TILE;
		command += 5;
	}
	else if (strncmp(command, "scale ", 6) == 0)
	{
		style = BACKDROP_SCALE;
		command += 6;
	}
	else if (strncmp(command, "fit ", 4) == 0)
	{
		style = BACKDROP_FIT;
		command += 4;
	}
	else if (strncmp(command, "stretch ", 8) == 0)
	{
		style = BACKDROP_STRETCH;
		command += 8;
	}
	else if (strncmp(command, "centre ", 7) == 0)
	{
		style = BACKDROP_CENTRE;
		command += 7;
	}
	else
	{
		g_warning("Invalid command '%s' from backdrop app\n",
				command);
		abandon_backdrop_app(pinboard);
		return;
	}

	/* Load the backdrop. May abandon the program if loading fails. */
	reload_backdrop(pinboard, command, style);

	if (pinboard->to_backdrop_app == -1)
		return;

	while (*ok)
	{
		int sent;

		sent = write(pinboard->to_backdrop_app, ok, strlen(ok));
		if (sent <= 0)
		{
			/* Remote app quit? Not an error. */
			abandon_backdrop_app(pinboard);
			break;
		}
		ok += sent;
	}
}

static void backdrop_from_child(Pinboard *pinboard,
				int src, RoxInputCondition cond)
{
	char buf[256];
	int got;

	got = read(src, buf, sizeof(buf));

	if (got <= 0)
	{
		if (got < 0)
			g_warning("backdrop_from_child: %s\n",
					g_strerror(errno));
		abandon_backdrop_app(pinboard);
		return;
	}

	g_string_append_len(pinboard->input_buffer, buf, got);

	while (pinboard->from_backdrop_app != -1)
	{
		int len;
		char *nl, *command;

		nl = strchr(pinboard->input_buffer->str, '\n');
		if (!nl)
			return;		/* Haven't got a whole line yet */

		len = nl - pinboard->input_buffer->str;
		command = g_strndup(pinboard->input_buffer->str, len);
		g_string_erase(pinboard->input_buffer, 0, len + 1);

		command_from_backdrop_app(pinboard, command);

		g_free(command);
	}
}

static void reload_backdrop(Pinboard *pinboard,
			    const gchar *backdrop,
			    BackdropStyle backdrop_style)
{

	if (backdrop && backdrop_style == BACKDROP_PROGRAM)
	{
		const char *argv[] = {NULL, "--backdrop", NULL};
		GError	*error = NULL;

		g_return_if_fail(pinboard->to_backdrop_app == -1);
		g_return_if_fail(pinboard->from_backdrop_app == -1);
		g_return_if_fail(pinboard->input_tag == -1);
		g_return_if_fail(pinboard->input_buffer == NULL);

		argv[0] = make_path(backdrop, "AppRun");

		/* Run the program. It'll send us a SOAP message and we'll
		 * get back here with a different style and image.
		 */

		if (g_spawn_async_with_pipes(NULL, (gchar **) argv, NULL,
				G_SPAWN_DO_NOT_REAP_CHILD |
				G_SPAWN_SEARCH_PATH,
				NULL, NULL,		/* Child setup fn */
				NULL,			/* Child PID */
				&pinboard->to_backdrop_app,
				&pinboard->from_backdrop_app,
				NULL,			/* Standard error */
				&error))
		{
			pinboard->input_buffer = g_string_new(NULL);
			pinboard->input_tag = rox_input_add_full(
					pinboard->from_backdrop_app,
					ROX_INPUT_READ,
					(RoxInputFunction) backdrop_from_child,
					pinboard, NULL);
		}
		else
		{
			delayed_error("%s", error ? error->message : "(null)");
			g_error_free(error);
		}
		return;
	}

	if (pinboard->backdrop_pixbuf)
	{
		g_object_unref(pinboard->backdrop_pixbuf);
		pinboard->backdrop_pixbuf = NULL;
	}

	pinboard->backdrop_style = backdrop_style;
	if (backdrop)
		pinboard->backdrop_pixbuf = load_backdrop(backdrop, backdrop_style);

	if (!gdk_rgba_parse(&pinboard->background_rgba,
		(const gchar *) o_pinboard_bg_colour.value))
	{
		pinboard->background_rgba.red = 0.5;
		pinboard->background_rgba.green = 0.5;
		pinboard->background_rgba.blue = 0.5;
		pinboard->background_rgba.alpha = 1.0;
	}

	gtk_widget_queue_draw(pinboard->fixed);

	/* The backdrop is held as a pixbuf/Cairo source.  Remove stale X11
	 * root-pixmap properties rather than publishing an invalid resource ID. */
	gdk_property_delete(gdk_get_default_root_window(),
		gdk_atom_intern("_XROOTPMAP_ID", FALSE));
	gdk_property_delete(gdk_get_default_root_window(),
		gdk_atom_intern("ESETROOT_PMAP_ID", FALSE));
}

/* Search the area (omin, imin) to (omax, imax) for a free region the size of
 * 'rect' that doesn't overlap 'used'.  Which of inner and outer is the
 * vertical axis depends on the configuration.
 *
 * id and od give the direction of the search (step size).
 *
 * Returns the start of the found region in inner/outer, or -1 if there is no
 * free space.
 */
static void region_union_gdk_rect(cairo_region_t *region,
				 const GdkRectangle *rect)
{
	cairo_rectangle_int_t cairo_rect = {
		rect->x, rect->y, rect->width, rect->height
	};
	cairo_region_union_rectangle(region, &cairo_rect);
}

static cairo_region_overlap_t region_contains_gdk_rect(
		cairo_region_t *region, const GdkRectangle *rect)
{
	cairo_rectangle_int_t cairo_rect = {
		rect->x, rect->y, rect->width, rect->height
	};
	return cairo_region_contains_rectangle(region, &cairo_rect);
}

static void search_free(GdkRectangle *rect, cairo_region_t *used,
			int *outer, int od, int omin, int omax,
			int *inner, int id, int imin, int imax)
{
	*outer = od > 0 ? omin : omax;
	while (*outer >= omin && *outer <= omax)
	{
		*inner = id > 0 ? imin : imax;
		while (*inner >= imin && *inner <= imax)
		{
			if (region_contains_gdk_rect(used, rect) ==
					CAIRO_REGION_OVERLAP_OUT)
				return;
			*inner += id;
		}

		*outer += od;
	}

	rect->x = -1;
	rect->y = -1;
}

/* Search the width x height area from (x0, y0) for a free region of size
 * 'rect'. direction indicates whether to search rows or columns. dx, dy gives
 * the direction of the search.
 */
static void search_free_area(GdkRectangle *rect, cairo_region_t *used,
		int direction, int dx, int dy, int x0, int y0, int width, int height)
{
	if (direction == DIR_VERT)
	{
		search_free(rect, used,
			&rect->x, dx, x0, x0 + width,
			&rect->y, dy, y0, y0 + height);
	}
	else
	{
		search_free(rect, used,
			&rect->y, dy, y0, y0 + height,
			&rect->x, dx, x0, x0 + width);
	}
}

static gboolean search_free_xinerama(GdkRectangle *rect, cairo_region_t *used,
		int direction, int dx, int dy, int rwidth, int rheight)
{
	GdkRectangle *geom = &monitor_geom[get_monitor_under_pointer()];

	search_free_area(rect, used, direction, dx, dy,
			geom->x, geom->y, geom->width - rwidth, geom->height - rheight);
	return rect->x != -1;
}

/* Finds a free area on the pinboard large enough for the width and height
 * of the given rectangle, by filling in the x and y fields of 'rect'.
 * If 'old' is true, 'rect' has a previous position and we first check
 * if it is viable.
 * The search order respects user preferences.
 * If no area is free, returns any old area.
 */
static void find_free_rect(Pinboard *pinboard, GdkRectangle *rect,
			   gboolean old, int start, int direction, int start_coord)
{
	cairo_region_t *used;
	GList *next;
	GdkRectangle used_rect;
	int dx = o_search_step_x.int_value, dy = o_search_step_y.int_value;
	int step = o_pinboard_grid_step.int_value;

	used = cairo_region_create();

	panel_mark_used(used);

	/* Subtract the no-go areas... */

	if (o_top_margin.int_value > 0)
	{
		used_rect.x = 0;
		used_rect.y = 0;
		used_rect.width = gdk_screen_width();
		used_rect.height = o_top_margin.int_value;
		region_union_gdk_rect(used, &used_rect);
	}

	if (o_bottom_margin.int_value > 0)
	{
		used_rect.x = 0;
		used_rect.y = gdk_screen_height() - o_bottom_margin.int_value;
		used_rect.width = gdk_screen_width();
		used_rect.height = o_bottom_margin.int_value;
		region_union_gdk_rect(used, &used_rect);
	}

	if (o_left_margin.int_value > 0)
	{
		used_rect.x = 0;
		used_rect.y = 0;
		used_rect.width = o_left_margin.int_value;
		used_rect.height = gdk_screen_height();
		region_union_gdk_rect(used, &used_rect);
	}

	if (o_right_margin.int_value > 0)
	{
		used_rect.x = gdk_screen_width() - o_right_margin.int_value;
		used_rect.y = 0;
		used_rect.width = o_right_margin.int_value;
		used_rect.height = gdk_screen_height();
		region_union_gdk_rect(used, &used_rect);
	}

	/* Subtract the used areas... */

	next = gtk_container_get_children(GTK_CONTAINER(pinboard->fixed));
	{
		GList *children = next;
		for (; next; next = next->next)
		{
			GtkWidget *child = GTK_WIDGET(next->data);
			GtkAllocation allocation;

			if (!gtk_widget_get_visible(child))
				continue;

			gtk_widget_get_allocation(child, &allocation);
			used_rect.x = allocation.x;
			used_rect.y = allocation.y;
			used_rect.width = allocation.width;
			used_rect.height = allocation.height;

			region_union_gdk_rect(used, &used_rect);
		}
		g_list_free(children);
	}

	/* Check the previous area */
	if(old) {
		if(region_contains_gdk_rect(used, rect)==CAIRO_REGION_OVERLAP_OUT) {
			cairo_region_destroy(used);
			return;
		}
	}

	/* Find the first free area (yes, this isn't exactly pretty, but
	 * it works). If you know a better (fast!) algorithm, let me know!
	 */

	if (start == CORNER_TOP_RIGHT || start == CORNER_BOTTOM_RIGHT)
		dx = -(((dx - 1) / step) + 1) * step;
	else
		dx = (((dx - 1) / step) + 1) * step;

	if (start == CORNER_BOTTOM_LEFT || start == CORNER_BOTTOM_RIGHT)
		dy = -(((dy - 1) / step) + 1) * step;
	else
		dy = (((dy - 1) / step) + 1) * step;

	/* If pinboard covers more than one monitor, try to find free space on
	 * monitor under pointer first (unless start_coord has been specified),
	 * then whole screen if that fails */
	if (n_monitors == 1 || (start_coord == 0 &&
			!search_free_xinerama(rect, used, direction,
				dx, dy, rect->width, rect->height)))
	{
		if (start_coord > 0)
		{
			GdkRectangle search_rect;
			search_rect.x = o_left_margin.int_value;
			search_rect.y = o_top_margin.int_value;
			search_rect.width = screen_width -
					(o_left_margin.int_value + o_right_margin.int_value);
			search_rect.height = screen_height -
					(o_top_margin.int_value + o_bottom_margin.int_value);

			if (direction == DIR_VERT)
			{
				if (dx > 0)
				{
					// start_coord is the left side of search_rect
					if (start_coord - (rect->width / 2) > search_rect.x &&
						search_rect.width - start_coord > 0)
					{
						search_rect.x = start_coord - (rect->width / 2);
						search_rect.width = search_rect.width - start_coord;
					}
				}
				else
				{
					// start_coord is the right side of search_rect
					if (start_coord + (rect->width / 2) > 0)
						search_rect.width = start_coord + (rect->width / 2);
				}
			}
			if (direction == DIR_HORZ)
			{
				if (dy > 0)
				{
					// start_coord is the top side of search_rect
					if (start_coord - (rect->height / 2) > search_rect.y &&
						search_rect.height - start_coord > 0)
					{
						search_rect.y = start_coord - (rect->height / 2);
						search_rect.height = search_rect.height - start_coord;
					}
				}
				else
				{
					// start_coord is the bottom side of search_rect
					if (start_coord + (rect->height / 2) > 0)
						search_rect.height = start_coord + (rect->height / 2);
				}
			}

			search_free_area(rect, used, direction, dx, dy,
					search_rect.x, search_rect.y,
					search_rect.width - rect->width,
					search_rect.height - rect->height);

			// If an empty space couldn't be found in search_rect,
			// search whole screen area.
			if (rect->x == -1)
			{
				search_free_area(rect, used, direction, dx, dy, 0, 0,
						screen_width - rect->width,
						screen_height - rect->height);
			}
		}
		else
		{
			search_free_area(rect, used, direction, dx, dy, 0, 0,
					screen_width - rect->width,
					screen_height - rect->height);
		}
	}

	cairo_region_destroy(used);

	if (rect->x == -1)
	{
		rect->x = 0;
		rect->y = 0;
	}
}

/* Icon's size, shape or appearance has changed - update the display */
static void pinboard_reshape_icon(Icon *icon)
{
	PinIcon	*pi = (PinIcon *) icon;
	int	x = pi->x, y = pi->y;

	set_size_and_style(pi);
	offset_from_centre(pi, &x, &y);

	{
		GtkAllocation allocation;
		gtk_widget_get_allocation(pi->win, &allocation);
		if (allocation.x != x || allocation.y != y)
			fixed_move_fast(GTK_FIXED(current_pinboard->fixed),
				pi->win, x, y);
	}

	/* Newer versions of GTK seem to need this, or the icon doesn't
	 * get redrawn.
	 */
	gtk_widget_queue_draw(pi->win);
}

/* Sets the pinboard_font global from the option. Doesn't do anything else. */
static void update_pinboard_font(void)
{
	if (pinboard_font)
		pango_font_description_free(pinboard_font);
	pinboard_font = o_label_font.value[0] != '\0'
			? pango_font_description_from_string(o_label_font.value)
			: NULL;
}

static void radios_changed(Radios *radios, gpointer data)
{
	GObject *dialog = G_OBJECT(data);
	DropBox *drop_box;
	const guchar *path;

	g_return_if_fail(dialog != NULL);

	drop_box = g_object_get_data(G_OBJECT(dialog), "rox-dropbox");

	g_return_if_fail(radios != NULL);
	g_return_if_fail(drop_box != NULL);
	g_return_if_fail(current_pinboard != NULL);

	if (current_pinboard->backdrop_style != BACKDROP_PROGRAM)
	{
		path = drop_box_get_path(drop_box);
		if (path)
			pinboard_set_backdrop(path, radios_get_value(radios));
	}
}

static void update_radios(GtkWidget *dialog)
{
	Radios *radios;
	GtkWidget *hbox;

	g_return_if_fail(dialog != NULL);

	radios = g_object_get_data(G_OBJECT(dialog), "rox-radios");
	hbox = g_object_get_data(G_OBJECT(dialog), "rox-radios-hbox");

	g_return_if_fail(current_pinboard != NULL);
	g_return_if_fail(radios != NULL);
	g_return_if_fail(hbox != NULL);

	switch (current_pinboard->backdrop_style)
	{
		case BACKDROP_TILE:
		case BACKDROP_STRETCH:
		case BACKDROP_SCALE:
		case BACKDROP_CENTRE:
		case BACKDROP_FIT:
			radios_set_value(radios,
					 current_pinboard->backdrop_style);
			gtk_widget_set_sensitive(hbox, TRUE);
			break;
		default:
			gtk_widget_set_sensitive(hbox, FALSE);
			radios_set_value(radios, BACKDROP_TILE);
			break;
	}
}
