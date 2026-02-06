/* amide_gconf.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 *
 * Copyright (C) 2007-2017 Andy Loening except as follows
 * win32 registery code is derived directly from gnumeric
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


#include "amide_gconf.h"
#include "amide.h"



#if defined (G_PLATFORM_WIN32)
/* --------------------------- win32 version --------------------------- */

/* Note, this stuff has been adapted from gnumeric, 
   specifically the file gnm-conf-win32.c
 */

#include <windows.h>
#include <errno.h>

#ifndef ERANGE
/* mingw has not defined ERANGE (yet), MSVC has it though */
# define ERANGE 34
#endif


typedef struct _GOConfNode GOConfNode;
struct _GOConfNode {
  HKEY hKey;
  gchar *path;
};

static GOConfNode *root = NULL;

static void go_conf_win32_clone (HKEY hSrcKey, gchar *key, HKEY hDstKey, gchar *buf1, gchar *buf2, gchar *buf3) {
#define WIN32_MAX_REG_KEYNAME_LEN 256
#define WIN32_MAX_REG_VALUENAME_LEN 32767
#define WIN32_INIT_VALUE_DATA_LEN 2048
  gint i;
  gchar *subkey, *value_name, *data;
  DWORD name_size, type, data_size;
  HKEY hSrcSK, hDstSK;
  FILETIME ft;
  LONG ret;

  if (RegOpenKeyEx (hSrcKey, key, 0, KEY_READ, &hSrcSK) != ERROR_SUCCESS)
    return;

  if (!buf1) {
    subkey = g_malloc (WIN32_MAX_REG_KEYNAME_LEN);
    value_name = g_malloc (WIN32_MAX_REG_VALUENAME_LEN);
    data = g_malloc (WIN32_INIT_VALUE_DATA_LEN);
  }
  else {
    subkey = buf1;
    value_name = buf2;
    data = buf3;
  }

  ret = ERROR_SUCCESS;
  for (i = 0; ret == ERROR_SUCCESS; ++i) {
    name_size = WIN32_MAX_REG_KEYNAME_LEN;
    ret = RegEnumKeyEx (hSrcSK, i, subkey, &name_size, NULL, NULL, NULL, &ft);
    if (ret != ERROR_SUCCESS)
      continue;

    if (RegCreateKeyEx (hDstKey, subkey, 0, NULL, 0, KEY_WRITE,
			NULL, &hDstSK, NULL) == ERROR_SUCCESS) {
      go_conf_win32_clone (hSrcSK, subkey, hDstSK, subkey, value_name, data);
      RegCloseKey (hDstSK);
    }
  }

  ret = ERROR_SUCCESS;
  for (i = 0; ret == ERROR_SUCCESS; ++i) {
    name_size = WIN32_MAX_REG_KEYNAME_LEN;
    data_size = WIN32_MAX_REG_VALUENAME_LEN;
    while ((ret = RegEnumValue (hSrcSK, i, value_name, &name_size,
				NULL, &type, data, &data_size)) ==
	   ERROR_MORE_DATA)
      data = g_realloc (data, data_size);
    if (ret != ERROR_SUCCESS)
      continue;
    
    RegSetValueEx (hDstKey, value_name, 0, type, data, data_size);
  }
  
  RegCloseKey (hSrcSK);
  if (!buf1) {
    g_free (subkey);
    g_free (value_name);
    g_free (data);
  }
}

static gboolean go_conf_win32_get_node (GOConfNode *node, HKEY *phKey, const gchar *key, gboolean *is_new) {
  gchar *path, *c;
  LONG ret;
  DWORD disposition;
  
  path = g_strconcat (node ? "" : "Software\\", key, NULL);
  for (c = path; *c; ++c) {
    if (*c == '/')
      *c = '\\';
  }
  ret = RegCreateKeyEx (node ? node->hKey : HKEY_CURRENT_USER, path,
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, phKey, &disposition);
  g_free (path);
  
  if (is_new) 
    *is_new = disposition == REG_CREATED_NEW_KEY;
  
#ifdef AMIDE_DEBUG
  if (ret != ERROR_SUCCESS) {
    LPTSTR msg_buf;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
		   FORMAT_MESSAGE_FROM_SYSTEM |
		   FORMAT_MESSAGE_IGNORE_INSERTS,
		   NULL,
		   ret,
		   MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPTSTR) &msg_buf,
		   0,
		   NULL);
    g_warning("Unable to save key '%s' : because %s", key, msg_buf); 
  }
#endif

  return ret == ERROR_SUCCESS;
}

GOConfNode * go_conf_get_node (GOConfNode *parent, const gchar *key) {
  HKEY hKey;
  GOConfNode *node = NULL;
  gboolean is_new;
  
  if (go_conf_win32_get_node (parent, &hKey, key, &is_new)) {
    if (!parent && is_new) {
      gchar *path;
      
      path = g_strconcat (".DEFAULT\\Software\\", key, NULL);
      go_conf_win32_clone (HKEY_USERS, path, hKey, NULL, NULL, NULL);
      g_free (path);
    }
    node = g_malloc (sizeof (GOConfNode));
    node->hKey = hKey;
    node->path = g_strdup (key);
  }
  
  return node;
}


static gboolean go_conf_win32_set (GOConfNode *node, const gchar *key, gint type, guchar *data, gint size) {
  gchar *last_sep, *path = NULL;
  HKEY hKey;
  gboolean ok;
  gboolean newpath=FALSE;

  if ((last_sep = strrchr (key, '/')) != NULL) {
    path = g_strndup (key, last_sep - key);
    ok = go_conf_win32_get_node (node, &hKey, path, NULL);
    g_free (path);
    newpath=TRUE;
    if (!ok) {
      return FALSE;
    }
    key = last_sep + 1;
  } else
    hKey = node->hKey;

  ok = RegSetValueEx (hKey, key, 0, type, data, size);
  if (ok != ERROR_SUCCESS) {
#ifdef AMIDE_DEBUG
    LPTSTR msg_buf;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
		   FORMAT_MESSAGE_FROM_SYSTEM |
		   FORMAT_MESSAGE_IGNORE_INSERTS,
		   NULL,
		   ok,
		   MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPTSTR) &msg_buf,
		   0,
		   NULL);
    g_warning("Unable to save key '%s' : because %s", key, msg_buf); 
#endif
    return FALSE;
  }

  if (newpath)
    RegCloseKey (hKey);
  
  return TRUE;
}

static gboolean go_conf_win32_get (GOConfNode *node, const gchar *key,
		   gulong *type, guchar **data, gulong *size,
		   gboolean realloc, gint *ret_code) {
  gchar *last_sep, *path = NULL;
  HKEY hKey;
  LONG ret;
  gboolean ok;
  
  if ((last_sep = strrchr (key, '/')) != NULL) {
    path = g_strndup (key, last_sep - key);
    ok = go_conf_win32_get_node (node, &hKey, path, NULL);
    g_free (path);
    if (!ok) {
      return FALSE;
    } 
    key = last_sep + 1;
  }
  else
    hKey = node->hKey;
  if (!*data && realloc) {
    RegQueryValueEx (hKey, key, NULL, type, NULL, size);
    *data = g_new (guchar, *size);
  }
  while ((ret = RegQueryValueEx (hKey, key, NULL,
				 type, *data, size)) == ERROR_MORE_DATA &&
	 realloc)
    *data = g_realloc (*data, *size);
  if (path)
    RegCloseKey (hKey);
  if (ret_code)
    *ret_code = ret;
  
  return ret == ERROR_SUCCESS;
}

static guchar * go_conf_get (GOConfNode *node, const gchar *key, gulong expected) {
  gulong type, size = 0;
  guchar *ptr = NULL;
  gint ret_code;

  if (!go_conf_win32_get (node, key, &type, &ptr, &size, TRUE, &ret_code)) {
#ifdef AMIDE_DEBUG
    LPTSTR msg_buf;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
		   FORMAT_MESSAGE_FROM_SYSTEM |
		   FORMAT_MESSAGE_IGNORE_INSERTS,
		   NULL,
		   ret_code,
		   MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPTSTR) &msg_buf,
		   0,
		   NULL);
    g_warning("Unable to load key '%s' : because %s", key, msg_buf);
    LocalFree (msg_buf);
#endif
    g_free (ptr);
    return NULL;
  }
  
  if (type != expected) {
#ifdef AMIDE_DEBUG
    g_warning("Expected `%lu' got `%lu' for key %s of node %s", expected, type, key, node->path);
#endif
    g_free (ptr);
    return NULL;
  }
  
  return ptr;
}


void amide_gconf_init(void) {
  root = go_conf_get_node (NULL, PACKAGE);
  return;
}

void amide_gconf_shutdown(void) {

  if (root) {
    RegCloseKey (root->hKey);
    g_free (root->path);
    g_free (root);
  }

  return;
}

gboolean amide_gconf_get_bool_with_default(const gchar * group, const gchar *key, gboolean default_val) {
  guchar * val;
  gboolean res;
  gchar * real_key;

  real_key = g_strdup_printf("%s/%s",group,key);
  val = go_conf_get (root, real_key, REG_BINARY);
  g_free(real_key);

  if (val) {
    res = (gboolean) *val;
    g_free (val);
    return res;
  } else {
    return default_val;
  }
}

gboolean amide_gconf_get_bool(const gchar * group, const gchar * key) {
  return amide_gconf_get_bool_with_default(group, key,FALSE);
}



gint amide_gconf_get_int_with_default(const gchar * group, const gchar *key, gint default_val) {
  guchar * val;
  gint res;
  gchar * real_key;

  real_key = g_strdup_printf("%s/%s",group,key);
  val = go_conf_get (root, real_key, REG_DWORD);
  g_free(real_key);

  if (val) {
    res = *(gint *) val;
    g_free (val);
    return res;
  } else {
    return default_val;
  }
}

gint amide_gconf_get_int(const gchar * group, const gchar * key) {
  return amide_gconf_get_int_with_default(group, key,0);
}



gdouble amide_gconf_get_float_with_default(const gchar * group, const gchar *key, gdouble default_val) {
  gdouble res = -1;
  gchar *val;
  gboolean valid=FALSE;
  gchar * real_key;

  real_key = g_strdup_printf("%s/%s",group,key);
  val = (gchar *) go_conf_get (root, real_key, REG_SZ);  
  g_free(real_key);

  if (val) {
    res = g_ascii_strtod (val, NULL);
    if (errno != ERANGE) valid = TRUE;
    g_free(val);
  }

  if (!valid) 
    return default_val;
  else
    return res;
}

gdouble amide_gconf_get_float(const gchar * group, const gchar * key) {
  return amide_gconf_get_float_with_default(group, key, 0.0);
}



/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string_with_default(const gchar * group, const gchar * key, const gchar * default_str) {

  gchar * val;
  gchar * real_key;

  real_key = g_strdup_printf("%s/%s",group,key);
  val = (gchar *) go_conf_get(root, real_key, REG_SZ);
  g_free(real_key);

  if (val)
    return val;
  else
    return g_strdup(default_str);
}

/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string (const gchar * group, const gchar *key) {
  return amide_gconf_get_string_with_default(group, key,NULL);
}



gboolean amide_gconf_set_int(const gchar * group, const gchar *key, gint val) {
  gchar * real_key;
  gboolean return_val;

  real_key = g_strdup_printf("%s/%s",group,key);
  return_val = go_conf_win32_set (root, real_key, REG_DWORD, (guchar *) &val, sizeof (DWORD));
  g_free(real_key);

  return return_val;
}

gboolean amide_gconf_set_float(const gchar * group, const char *key, gdouble val) {
  gchar * real_key;
  gboolean return_val;
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (str, sizeof (str), val);

  real_key = g_strdup_printf("%s/%s",group,key);
  return_val = go_conf_win32_set(root, real_key, REG_SZ, (guchar *) str, strlen (str) + 1);
  g_free(real_key);

  return return_val;
}

gboolean amide_gconf_set_bool(const gchar * group, const gchar *key, gboolean val) {
  gchar * real_key;
  gboolean return_val;
  guchar bool = val ? 1 : 0;

  real_key = g_strdup_printf("%s/%s",group,key);
  return_val = go_conf_win32_set (root, real_key, REG_BINARY, (guchar *) &bool, sizeof (bool));
  g_free(real_key);

  return return_val;
}

gboolean amide_gconf_set_string (const gchar * group, const char *key, const gchar *str) {
  gchar * real_key;
  gboolean return_val;

  real_key = g_strdup_printf("%s/%s",group,key);
  return_val = go_conf_win32_set(root, real_key, REG_SZ, (guchar *) str, strlen (str) + 1);
  g_free(real_key);

  return return_val;
}

#elif defined(AMIDE_NATIVE_GTK_OSX)

/* --------------------- flatfile version ----------------- */

#include <glib/gstdio.h>
#include <errno.h>

static GKeyFile *key_file = NULL;
static gchar * rcfilename = NULL;



void amide_gconf_init(void) {

  const gchar *home;

  home = g_get_home_dir();
  g_return_if_fail(home != NULL);

  rcfilename = g_build_filename (home, ".amiderc", NULL);
  g_return_if_fail(rcfilename != NULL);

  /* generate the key file, and load it in if a prior .ammiderc file exists */
  key_file = g_key_file_new();
  g_key_file_load_from_file(key_file, rcfilename, G_KEY_FILE_NONE, NULL); 

  return;
}

void amide_gconf_shutdown(void) {
  FILE *fp = NULL;
  gchar *key_data;

  g_return_if_fail(rcfilename != NULL);
  g_return_if_fail(key_file != NULL);
  
  fp = g_fopen (rcfilename, "w");
  if (fp == NULL) {
    g_warning ("Couldn't write configuration info to %s", rcfilename);
    return;
  }
  
  key_data = g_key_file_to_data (key_file, NULL, NULL);

  if (key_data != NULL) {
    fputs (key_data, fp);
    g_free (key_data);
  }
  
  fclose (fp);
  return;
}

gboolean amide_gconf_get_bool_with_default(const gchar * group, const gchar *key, gboolean default_val) {

  gboolean val;
  GError *error=NULL;

  g_return_val_if_fail(key_file != NULL, default_val);

  val = g_key_file_get_boolean (key_file, group, key, &error);

  if (error != NULL) {
    g_error_free(error);
    return default_val;
  } else
    return val;
}

gboolean amide_gconf_get_bool(const gchar * group, const gchar * key) {
  return amide_gconf_get_bool_with_default(group,key,FALSE);
}



gint amide_gconf_get_int_with_default(const gchar * group, const gchar *key, gint default_val) {
  gint val;
  GError *error=NULL;

  g_return_val_if_fail(key_file != NULL, default_val);

  val = g_key_file_get_integer(key_file, group, key, &error);

  if (error != NULL) {
    g_error_free(error);
    return default_val;
  } else
    return val;
}

gint amide_gconf_get_int(const gchar * group, const gchar * key) {
  return amide_gconf_get_int_with_default(group, key,0);
}



gdouble amide_gconf_get_float_with_default(const gchar * group, const gchar *key,gdouble default_val) {
  gchar *ptr;
  gdouble val;
  GError *error=NULL;
  gboolean valid = FALSE;

  g_return_val_if_fail(key_file != NULL, default_val);

  ptr = g_key_file_get_value (key_file, group, key, &error);

  if ((ptr != NULL) && (error == NULL)) {
    val = g_ascii_strtod (ptr, NULL);
    if (errno != ERANGE) valid = TRUE;
  }

  if (ptr != NULL)
    g_free (ptr);

  if (!valid)
    val = default_val;

  return val;
}

gdouble amide_gconf_get_float(const gchar * group, const gchar * key) {
  return amide_gconf_get_float_with_default(group, key, 0.0);
}



/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string_with_default(const gchar * group, const gchar * key, const gchar * default_val) {
  gchar *val;
  GError *error=NULL;

  g_return_val_if_fail(key_file != NULL, NULL);

  val = g_key_file_get_string(key_file, group, key, &error);

  if(error == NULL) 
    return val;
  else
    return g_strdup(default_val);
}

/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string (const gchar * group, const gchar *key) {
  return amide_gconf_get_string_with_default(group, key,NULL);
}


gboolean amide_gconf_set_bool(const gchar * group, const gchar *key, gboolean val) {
  g_return_val_if_fail(key_file != NULL, FALSE);
  g_key_file_set_boolean(key_file, group, key, val);
  return TRUE;
}

gboolean amide_gconf_set_int(const gchar * group, const gchar *key, gint val) {
  g_return_val_if_fail(key_file != NULL, FALSE);
  g_key_file_set_integer(key_file, group, key, val);
  return TRUE;
}

gboolean amide_gconf_set_float(const gchar * group, const char *key, gdouble val) {
  g_return_val_if_fail(key_file != NULL, FALSE);
  g_key_file_set_double(key_file, group, key, val);
  return TRUE;
}


gboolean amide_gconf_set_string (const gchar * group, const char *key, const gchar *val) {
  g_return_val_if_fail(key_file != NULL, FALSE);
  g_key_file_set_string (key_file, group, key, val);
  return TRUE;
}




#else

/* ------------------- gconf version ---------------------- */

#include <gio/gio.h>

/* internal variables */
static GSettings * settings=NULL;

const gchar * enum_keys[] = {
  "layout",
  "panel-layout",
  "which-default-directory",
  "threshold-style",
  "last-modality",
  "last-raw-format",
  "method",
  "calculation-type",
  "type",
  "view",
  NULL
};

void amide_gconf_init(void) {
  settings = g_settings_new("com.github.ferdymercury.amide");
  return;
}

void amide_gconf_shutdown(void) {
  g_object_unref(settings);
  return;
}

gint amide_gconf_get_int(const gchar * group, const gchar * key) {
  GSettings * child;
  gint return_val;

  child = g_settings_get_child(settings,group);
  if (g_strv_contains(enum_keys,key))
    return_val = g_settings_get_enum(child,key);
  else
    return_val = g_settings_get_int(child,key);
  g_object_unref(child);
  return return_val;
}

gdouble amide_gconf_get_float(const gchar * group, const gchar * key) {
  GSettings * child;
  gdouble return_val;

  child = g_settings_get_child(settings,group);
  return_val = g_settings_get_double(child,key);
  g_object_unref(child);
  return return_val;
}

gboolean amide_gconf_get_bool(const gchar * group, const gchar * key) {
  GSettings * child;
  gboolean return_val;

  child = g_settings_get_child(settings,group);
  return_val = g_settings_get_boolean(child,key);
  g_object_unref(child);
  return return_val;
}

gchar * amide_gconf_get_string(const gchar * group, const gchar * key) {
  GSettings * child;
  gchar * return_val;

  child = g_settings_get_child(settings,group);
  return_val = g_settings_get_string(child,key);
  g_object_unref(child);
  return return_val;
}

gboolean amide_gconf_set_int(const gchar * group, const gchar * key, gint val) {
  GSettings * child;
  gboolean return_val;

  child = g_settings_get_child(settings,group);
  if (g_strv_contains(enum_keys,key))
    return_val = g_settings_set_enum(child,key,val);
  else
    return_val = g_settings_set_int(child,key,val);
  g_object_unref(child);
  return return_val;
}

gboolean amide_gconf_set_float(const gchar * group, const gchar * key, gdouble val) {
  GSettings * child;
  gboolean return_val;

  child = g_settings_get_child(settings,group);
  return_val = g_settings_set_double(child,key,val);
  g_object_unref(child);
  return return_val;
}

gboolean amide_gconf_set_bool(const gchar * group, const gchar * key, gboolean val) {
  GSettings * child;
  gboolean return_val;

  child = g_settings_get_child(settings,group);
  return_val = g_settings_set_boolean(child,key,val);
  g_object_unref(child);
  return return_val;
}

gboolean amide_gconf_set_string(const gchar * group, const gchar * key, const gchar * val) {
  GSettings * child;
  gboolean return_val;

  child = g_settings_get_child(settings,group);
  return_val = g_settings_set_string(child,key,val);
  g_object_unref(child);
  return return_val;
}

/* some helper functions */
gint amide_gconf_get_int_with_default(const gchar * group, const gchar * key, const gint default_int) {
  return amide_gconf_get_int(group, key);
}

gdouble amide_gconf_get_float_with_default(const gchar * group, const gchar * key, const gdouble default_float) {
  return amide_gconf_get_float(group, key);
}

gboolean amide_gconf_get_bool_with_default(const gchar * group, const gchar * key, const gboolean default_bool) {
  return amide_gconf_get_bool(group, key);
}

/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string_with_default(const gchar * group, const gchar * key, const gchar * default_str) {
  return amide_gconf_get_string(group, key);
}

#endif











