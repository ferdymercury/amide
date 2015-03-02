/* amitk_progress_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
 */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/

/* adapted from gtkcolorsel.c */


#include "amide_config.h"
#include "amide_intl.h"
#include "amitk_marshal.h"
#include "amitk_progress_dialog.h"

static void dialog_class_init (AmitkProgressDialogClass *klass);
static void dialog_init (AmitkProgressDialog *progress_dialog);
static void dialog_response(GtkDialog * dialog, gint response_id);

static GtkDialogClass *progress_dialog_parent_class;


GType amitk_progress_dialog_get_type (void)
{
  static GType progress_dialog_type = 0;

  if (!progress_dialog_type)
    {
      GTypeInfo progress_dialog_info =
      {
	sizeof (AmitkProgressDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) dialog_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkProgressDialog),
	0,   /* # preallocs */
	(GInstanceInitFunc) dialog_init,
	NULL   /* value table */
      };

      progress_dialog_type = 
	g_type_register_static(GTK_TYPE_DIALOG, "AmitkProgressDialog", &progress_dialog_info, 0);
    }

  return progress_dialog_type;
}

static void dialog_class_init (AmitkProgressDialogClass *klass)
{
  /*  GtkObjectClass *gtkobject_class; */
  GtkDialogClass *gtkdialog_class = GTK_DIALOG_CLASS(klass);

  /* gtkobject_class = (GtkObjectClass*) klass; */

  progress_dialog_parent_class = g_type_class_peek_parent(klass);

  gtkdialog_class->response = dialog_response;

}

static void dialog_init (AmitkProgressDialog * dialog)
{

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

  dialog->can_continue = TRUE;

  dialog->message_label = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), dialog->message_label, FALSE, FALSE, 0);

  dialog->progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(dialog->progress_bar), 0.01);
  gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(dialog->progress_bar), 
				   GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), dialog->progress_bar, FALSE, FALSE, 0);

  return;
}

static void dialog_response(GtkDialog * dialog, gint response_id) {

  g_return_if_fail(AMITK_IS_PROGRESS_DIALOG(dialog));

  AMITK_PROGRESS_DIALOG(dialog)->can_continue = FALSE;

  if (progress_dialog_parent_class->response)
    (* progress_dialog_parent_class->response) (dialog, response_id);

  return;
}

gboolean amitk_progress_dialog_update(gpointer dialog_pointer, char * message, gdouble fraction) {

  AmitkProgressDialog * dialog = AMITK_PROGRESS_DIALOG(dialog_pointer);

  if (message != NULL)
    amitk_progress_dialog_set_text(dialog, message);

  if ((fraction >= 0.0) || (fraction < -0.5))
    return amitk_progress_dialog_set_fraction(dialog, fraction);
  else
    return AMITK_PROGRESS_DIALOG_CAN_CONTINUE(dialog);
}

void amitk_progress_dialog_set_text(AmitkProgressDialog * dialog, gchar * message) {

  g_return_if_fail(AMITK_IS_PROGRESS_DIALOG(dialog));

  gtk_label_set_text(GTK_LABEL(dialog->message_label), message);
}

gboolean amitk_progress_dialog_set_fraction(AmitkProgressDialog * dialog, gdouble fraction) {

  if (fraction > 1.0) {
    if (GTK_WIDGET_VISIBLE(dialog))
      gtk_widget_hide_all(GTK_WIDGET(dialog));

  } else if (fraction >= 0.0) {
    if (!GTK_WIDGET_VISIBLE(dialog)) {
      gtk_widget_show_all(GTK_WIDGET(dialog));
      dialog->can_continue = TRUE;
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dialog->progress_bar), fraction);

  } else if (fraction < -0.5) {
    if (!GTK_WIDGET_VISIBLE(dialog)) {
      gtk_widget_show_all(GTK_WIDGET(dialog));
      dialog->can_continue = TRUE;
    }
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(dialog->progress_bar));

  }

  /* let spin while events are pending, this allows cancel to happen */
  while (gtk_events_pending()) gtk_main_iteration();

  return AMITK_PROGRESS_DIALOG_CAN_CONTINUE(dialog);
}

GtkWidget* amitk_progress_dialog_new (GtkWindow * parent)
{
  AmitkProgressDialog *dialog;

  dialog = g_object_new(AMITK_TYPE_PROGRESS_DIALOG, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Progress Dialog"));
  gtk_window_set_transient_for(GTK_WINDOW (dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog), TRUE);
  gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

  return GTK_WIDGET (dialog);
}


