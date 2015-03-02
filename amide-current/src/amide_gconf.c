/* amide_gconf.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2007-2011 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
 * win32 gconf code is derived directly from gnumeric
 *
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
#include <errno.h>



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

gboolean amide_gconf_get_bool_with_default(const gchar *key, gboolean default_val) {
  guchar *val = go_conf_get (root, key, REG_BINARY);
  gboolean res;

  if (val) {
    res = (gboolean) *val;
    g_free (val);
    return res;
  } else {
    return default_val;
  }
}

gboolean amide_gconf_get_bool(const gchar * key) {
  return amide_gconf_get_bool_with_default(key,FALSE);
}



gint amide_gconf_get_int_with_default(const gchar *key, gint default_val) {
  guchar *val = go_conf_get (root, key, REG_DWORD);
  gint res;

  if (val) {
    res = *(gint *) val;
    g_free (val);
    return res;
  } else {
    return default_val;
  }
}

gint amide_gconf_get_int(const gchar * key) {
  return amide_gconf_get_int_with_default(key,0);
}



gdouble amide_gconf_get_float_with_default(const gchar *key,gdouble default_val) {
  gdouble res = -1;
  gchar *val = (gchar *) go_conf_get (root, key, REG_SZ);
  gboolean valid=FALSE;

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

gdouble amide_gconf_get_float(const gchar * key) {
  return amide_gconf_get_float_with_default(key, 0.0);
}



/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string_with_default(const gchar * key, const gchar * default_str) {

  gchar *val = (gchar *) go_conf_get(root, key, REG_SZ);
  if (val)
    return val;
  else
    return g_strdup(default_str);
}

/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string (const gchar *key) {
  return amide_gconf_get_string_with_default(key,NULL);
}



gboolean amide_gconf_set_int(const gchar *key, gint val) {
  return go_conf_win32_set (root, key, REG_DWORD, (guchar *) &val, sizeof (DWORD));
}

gboolean amide_gconf_set_float(const char *key, gdouble val) {
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (str, sizeof (str), val);
  return go_conf_win32_set(root, key, REG_SZ, (guchar *) str, strlen (str) + 1);
}

gboolean amide_gconf_set_bool(const gchar *key, gboolean val) {
  guchar bool = val ? 1 : 0;
  return go_conf_win32_set (root, key, REG_BINARY, (guchar *) &bool, sizeof (bool));
}

gboolean amide_gconf_set_string (const char *key, const gchar *str) {
  return go_conf_win32_set(root, key, REG_SZ, (guchar *) str, strlen (str) + 1);
}






#else

/* ------------------- unix version ---------------------- */

#include <gconf/gconf-client.h>

/* internal variables */
static GConfClient * client=NULL;


void amide_gconf_init(void) {
  client = gconf_client_get_default();
  return;
}

void amide_gconf_shutdown(void) {
  g_object_unref(client); 
  return;
}

gint amide_gconf_get_int(const char * key) {
  gchar * temp_key;
  gint return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_get_int(client,temp_key,NULL);
  g_free(temp_key);
  return return_val;
}

gdouble amide_gconf_get_float(const char * key) {
  gchar * temp_key;
  gdouble return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_get_float(client,temp_key,NULL);
  g_free(temp_key);
  return return_val;
}

gboolean amide_gconf_get_bool(const char * key) {
  gchar * temp_key;
  gboolean return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_get_bool(client,temp_key,NULL);
  g_free(temp_key);
  return return_val;
}

gchar * amide_gconf_get_string(const char * key) {
  gchar * temp_key;
  gchar * return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_get_string(client,temp_key,NULL);
  g_free(temp_key);
  return return_val;
}

gboolean amide_gconf_set_int(const char * key, gint val) {
  gchar * temp_key;
  gboolean return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_set_int(client,temp_key,val,NULL);
  g_free(temp_key);
  return return_val;
}

gboolean amide_gconf_set_float(const char * key, gdouble val) {
  gchar * temp_key;
  gboolean return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_set_float(client,temp_key,val,NULL);
  g_free(temp_key);
  return return_val;
}

gboolean amide_gconf_set_bool(const char * key, gboolean val) {
  gchar * temp_key;
  gboolean return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_set_bool(client,temp_key,val,NULL);
  g_free(temp_key);
  return return_val;
}

gboolean amide_gconf_set_string(const char * key, const gchar * val) {
  gchar * temp_key;
  gboolean return_val;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  return_val = gconf_client_set_string(client,temp_key,val,NULL);
  g_free(temp_key);
  return return_val;
}

/* it's pretty retarded that gconf doesn't have a function that can do this more easily */
gboolean amide_gconf_has_value(const gchar *key) {
  GConfValue * temp_val;
  gchar * temp_key;

  temp_key = g_strdup_printf("/apps/%s/%s",PACKAGE,key);
  temp_val = gconf_client_get(client, temp_key, NULL);
  g_free(temp_key);

  if (temp_val == NULL) return FALSE;
  gconf_value_free(temp_val);
  return TRUE;
}


/* some helper functions */
gint amide_gconf_get_int_with_default(const gchar * key, const gint default_int) {

  if (amide_gconf_has_value(key)) 
    return amide_gconf_get_int(key);
  else
    return default_int;
}

gdouble amide_gconf_get_float_with_default(const gchar * key, const gdouble default_float) {
  if (amide_gconf_has_value(key)) 
    return amide_gconf_get_float(key);
  else
    return default_float;
}

gboolean amide_gconf_get_bool_with_default(const gchar * key, const gboolean default_bool) {
  if (amide_gconf_has_value(key)) 
    return amide_gconf_get_bool(key);
  else
    return default_bool;
}

/* returns an allocated string that'll need to be free'd */
gchar * amide_gconf_get_string_with_default(const gchar * key, const gchar * default_str) {
  if (amide_gconf_has_value(key)) 
    return amide_gconf_get_string(key);
  else
    return g_strdup(default_str);
}

#endif











