/* This is the GTK 2 GtkGammaCurve widget, ported to GTK 3.
   Incorporated in AMIDE by Yavor Doganov <yavor@gnu.org>, 2024.  */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "amitk_curve.h"
#include "amitk_gamma.h"
#include "gamma_icons.c"

/* forward declarations: */
static void amitk_gamma_curve_destroy   (GtkWidget *object);
static void curve_type_changed_callback (AmitkCurve *curve, gpointer data);
static void button_realize_callback     (GtkWidget *w);
static void button_toggled_callback     (GtkToggleButton *b, gpointer data);
static void button_clicked_callback     (GtkButton *b, gpointer data);

G_DEFINE_TYPE (AmitkGammaCurve, amitk_gamma_curve, GTK_TYPE_BOX)

static void
amitk_gamma_curve_class_init (AmitkGammaCurveClass *class)
{
  GtkWidgetClass *object_class;

  object_class = (GtkWidgetClass *) class;
  object_class->destroy = amitk_gamma_curve_destroy;
}

static void
amitk_gamma_curve_init (AmitkGammaCurve *curve)
{
  GtkWidget *vbox;
  gint i;
  static gboolean icons_initialized = FALSE;

  if (!icons_initialized)
    {
      gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                        "/com/github/ferdymercury/amide/gamma");
      icons_initialized = TRUE;
    }

  gtk_orientable_set_orientation (GTK_ORIENTABLE (curve),
                                  GTK_ORIENTATION_VERTICAL);
  curve->grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (curve->grid), 3);
  gtk_container_add (GTK_CONTAINER (curve), curve->grid);

  curve->curve = amitk_curve_new ();
  g_signal_connect (curve->curve, "curve-type-changed",
                    G_CALLBACK (curve_type_changed_callback), curve);
  gtk_grid_attach (GTK_GRID (curve->grid), curve->curve, 0, 0, 1, 1);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
  gtk_grid_attach (GTK_GRID (curve->grid), vbox, 1, 0, 1, 1);

  /* toggle buttons: */
  for (i = 0; i < 2; ++i)
    {
      curve->button[i] = gtk_toggle_button_new ();
      g_object_set_data (G_OBJECT (curve->button[i]), "_AmitkGammaCurveIndex",
                         GINT_TO_POINTER (i));
      gtk_container_add (GTK_CONTAINER (vbox), curve->button[i]);
      g_signal_connect (curve->button[i], "realize",
                        G_CALLBACK (button_realize_callback), NULL);
      g_signal_connect (curve->button[i], "toggled",
                        G_CALLBACK (button_toggled_callback), curve);
      gtk_widget_show (curve->button[i]);
    }

  /* push button: */
  curve->button[2] = gtk_button_new ();
  g_object_set_data (G_OBJECT (curve->button[2]), "_AmitkGammaCurveIndex",
                     GINT_TO_POINTER (2));
  gtk_container_add (GTK_CONTAINER (vbox), curve->button[2]);
  g_signal_connect (curve->button[2], "realize",
                    G_CALLBACK (button_realize_callback), NULL);
  g_signal_connect (curve->button[2], "clicked",
                    G_CALLBACK (button_clicked_callback), curve);
  gtk_widget_show (curve->button[2]);

  gtk_widget_show (vbox);
  gtk_widget_show (curve->grid);
  gtk_widget_show (curve->curve);
}

static void
button_realize_callback (GtkWidget *w)
{
  GtkWidget *image;
  gchar *name;
  gint i;

  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w),
                                           "_AmitkGammaCurveIndex"));
  name = g_strdup_printf ("amide_gamma_icon%d", i);
  image = gtk_image_new_from_icon_name (name, GTK_ICON_SIZE_BUTTON);
  g_free (name);
  gtk_container_add (GTK_CONTAINER (w), image);
  gtk_widget_show (image);
}

static void
button_toggled_callback (GtkToggleButton *b, gpointer data)
{
  AmitkGammaCurve *c = data;
  AmitkCurveType type;
  int active, i;

  if (!gtk_toggle_button_get_active(b))
    return;

  active = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b),
                                               "_AmitkGammaCurveIndex"));

  for (i = 0; i < 2; ++i)
    if ((i != active)
        && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->button[i])))
      break;

  if (i < 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->button[i]), FALSE);

  if (active)
    type = AMITK_CURVE_TYPE_LINEAR;
  else
    type = AMITK_CURVE_TYPE_SPLINE;

  amitk_curve_set_curve_type (AMITK_CURVE (c->curve), type);
}

static void
button_clicked_callback (GtkButton *b, gpointer data)
{
  AmitkGammaCurve *c = data;

  /* reset */
  amitk_curve_reset (AMITK_CURVE (c->curve));
}

static void
curve_type_changed_callback (AmitkCurve *curve, gpointer data)
{
  AmitkGammaCurve *c = data;
  int active;

  if (curve->curve_type == AMITK_CURVE_TYPE_SPLINE)
    active = 0;
  else
    active = 1;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->button[active])))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->button[active]), TRUE);
}

GtkWidget *
amitk_gamma_curve_new (void)
{
  return g_object_new (AMITK_TYPE_GAMMA_CURVE, NULL);
}

static void
amitk_gamma_curve_destroy (GtkWidget *object)
{
  GTK_WIDGET_CLASS (amitk_gamma_curve_parent_class)->destroy (object);
}
