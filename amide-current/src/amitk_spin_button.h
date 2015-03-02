/* amitk_spin_button.h - cludge to get around bad gtk_spin_button behavior
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
 *
 * Author: Andy Loening <loening@ucla.edu>
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

#ifndef __AMITK_SPIN_BUTTON_H__
#define __AMITK_SPIN_BUTTON_H__


#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define AMITK_TYPE_SPIN_BUTTON                  (amitk_spin_button_get_type ())
#define AMITK_SPIN_BUTTON(obj)                  (AMITK_CHECK_CAST ((obj), AMITK_TYPE_SPIN_BUTTON, AmitkSpinButton))
#define AMITK_SPIN_BUTTON_CLASS(klass)          (AMITK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_SPIN_BUTTON, AmitkSpinButtonClass))
#define AMITK_IS_SPIN_BUTTON(obj)               (AMITK_CHECK_TYPE ((obj), AMITK_TYPE_SPIN_BUTTON))
#define AMITK_IS_SPIN_BUTTON_CLASS(klass)       (AMITK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_SPIN_BUTTON))


typedef struct _AmitkSpinButton	    AmitkSpinButton;
typedef struct _AmitkSpinButtonClass  AmitkSpinButtonClass;


struct _AmitkSpinButton
{
  GtkSpinButton spin_button;
  
};

struct _AmitkSpinButtonClass
{
  GtkSpinButtonClass parent_class;
};


GtkType		amitk_spin_button_get_type	   (void);
GtkWidget*	amitk_spin_button_new		   (GtkAdjustment  *adjustment,
						    gfloat	    climb_rate,
						    guint	    digits);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __AMITK_SPIN_BUTTON_H__ */
