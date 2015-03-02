/* amide_gconf.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2007-2014 Andy Loening
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

#ifndef __AMIDE_GCONF_H__
#define __AMIDE_GCONF_H__

/* header files that are always needed with this file */
#include <glib.h>
#include "amide_config.h"

G_BEGIN_DECLS

void amide_gconf_init();
void amide_gconf_shutdown();

gint     amide_gconf_get_int   (const char * group, const char * key);
gdouble  amide_gconf_get_float (const char * group, const char * key);
gboolean amide_gconf_get_bool  (const char * group, const char * key);
gchar *  amide_gconf_get_string(const char * group, const char * key);

gboolean amide_gconf_set_int   (const char * group, const char * key, gint val);
gboolean amide_gconf_set_float (const char * group, const char * key, gdouble val);
gboolean amide_gconf_set_bool  (const char * group, const char * key, gboolean val);
gboolean amide_gconf_set_string(const char * group, const char * key, const gchar * val);

gint     amide_gconf_get_int_with_default   (const char * group, const gchar * key, const gint default_int);
gdouble  amide_gconf_get_float_with_default (const char * group, const gchar * key, const gdouble default_float);
gboolean amide_gconf_get_bool_with_default  (const char * group, const gchar * key, const gboolean default_bool);
gchar *  amide_gconf_get_string_with_default(const char * group, const gchar * key, const gchar * default_str);

G_END_DECLS

#endif /* __AMIDE_GCONF_H__ */
