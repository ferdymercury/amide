/* amitk_progress_dialog.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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

/* adapated from gtkcolorsel.h */

#ifndef __AMITK_PROGRESS_DIALOG_H__
#define __AMITK_PROGRESS_DIALOG_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* ------------- Progress_Dialog---------- */

#define AMITK_TYPE_PROGRESS_DIALOG            (amitk_progress_dialog_get_type ())
#define AMITK_PROGRESS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_PROGRESS_DIALOG, AmitkProgressDialog))
#define AMITK_PROGRESS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkProgressDialogClass))
#define AMITK_IS_PROGRESS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_PROGRESS_DIALOG))
#define AMITK_IS_PROGRESS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_PROGRESS_DIALOG))

#define AMITK_PROGRESS_DIALOG_CAN_CONTINUE(obj)   (AMITK_PROGRESS_DIALOG(obj)->can_continue);

typedef struct _AmitkProgressDialog             AmitkProgressDialog;
typedef struct _AmitkProgressDialogClass        AmitkProgressDialogClass;


struct _AmitkProgressDialog
{
  GtkDialog dialog;

  GtkWidget * message_label;
  GtkWidget * progress_bar;
  gboolean can_continue;

};

struct _AmitkProgressDialogClass
{
  GtkDialogClass parent_class;
};  

GType      amitk_progress_dialog_get_type          (void);
gboolean   amitk_progress_dialog_update            (gpointer dialog, 
						    char * message, 
						    gdouble fraction);
void       amitk_progress_dialog_set_text          (AmitkProgressDialog * dialog, 
						    gchar * message);
gboolean   amitk_progress_dialog_set_fraction      (AmitkProgressDialog * dialog, 
						    gdouble fraction);
GtkWidget* amitk_progress_dialog_new               (GtkWindow * parent);

G_END_DECLS

#endif /* __AMITK_PROGRESS_DIALOG_H__ */

