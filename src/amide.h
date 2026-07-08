/* amide.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2017 Andy Loening
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

#ifndef __AMIDE_H__
#define __AMIDE_H__

#include <glib.h>
#include <pango/pango.h>
#include "amide_intl.h"

G_BEGIN_DECLS

/* some basic defines for packing tables */
#define X_PACKING_OPTIONS GTK_EXPAND
#define Y_PACKING_OPTIONS GTK_EXPAND
#define X_PADDING 5
#define Y_PADDING 5                 


typedef enum {
  AMIDE_EYE_LEFT, 
  AMIDE_EYE_RIGHT, 
  AMIDE_EYE_NUM
} AmideEye;

/* Ugly workaround for old GLib bug that is being exposed:
   https://gitlab.gnome.org/GNOME/glib/-/issues/299.  The program
   often aborts with the custom log handler with:
   (process:1197716): GdkPixbuf-DEBUG (recursed):
   gdk_pixbuf_from_pixdata() called on:  */
#undef g_warning
#undef g_message
#undef g_debug
#define g_warning(...) amide_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, \
                                 __VA_ARGS__)
#define g_message(...) amide_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, \
                                 __VA_ARGS__)
#define g_debug(...)   amide_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, \
                                 __VA_ARGS__)

void amide_log(const gchar *log_domain,
               GLogLevelFlags log_level,
               const gchar *format,
               ...);

/* external variables */
extern gchar * object_menu_names[];

G_END_DECLS

#endif /* __AMIDE_H__ */
