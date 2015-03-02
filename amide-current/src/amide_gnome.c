/* amide_gnome.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2007-2014 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
 * Code directly copied from libgnome
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


/* this file contains gnome functions that I still need but will hopefully have replacements
at some point in the future */


#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "amitk_common.h"
#include "amide_gnome.h"

#if !defined(G_OS_WIN32) && !defined(AMIDE_NATIVE_GTK_OSX)
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-uri.h>
//#else
//#include <windows.h>
#endif

typedef enum {
  AMIDE_GNOME_URL_ERROR_PARSE,
  AMIDE_GNOME_URL_ERROR_LAUNCH,
  AMIDE_GNOME_URL_ERROR_URL,
  AMIDE_GNOME_URL_ERROR_NO_DEFAULT,
  AMIDE_GNOME_URL_ERROR_NOT_SUPPORTED,
  AMIDE_GNOME_URL_ERROR_VFS,
  AMIDE_GNOME_URL_ERROR_CANCELLED
} AmideGnomeURLError;

#define AMIDE_GNOME_URL_ERROR (amide_gnome_url_error_quark ())

GQuark amide_gnome_url_error_quark (void) {
  static GQuark error_quark = 0;

  if (error_quark == 0)
    error_quark = g_quark_from_static_string ("amide-gnome-url-error-quark");

  return error_quark;
}


static gboolean amide_gnome_url_show_with_env (const char  *url,  char       **envp, GError     **error) {
#if !defined(G_OS_WIN32) && !defined(AMIDE_NATIVE_GTK_OSX)
	GnomeVFSResult result;
	GnomeVFSURI *vfs_uri;

	g_return_val_if_fail (url != NULL, FALSE);

	result = gnome_vfs_url_show_with_env (url, envp);

	switch (result) {
	case GNOME_VFS_OK:
		return TRUE;

	case GNOME_VFS_ERROR_INTERNAL:
		g_set_error (error,
		             AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_VFS,
			     _("Unknown internal error while displaying this location."));
		break;

	case GNOME_VFS_ERROR_BAD_PARAMETERS:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_URL,
			     _("The specified location is invalid."));
		break;

	case GNOME_VFS_ERROR_PARSE:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_PARSE,
			     _("There was an error parsing the default action command associated "
			       "with this location."));
		break;

	case GNOME_VFS_ERROR_LAUNCH:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_LAUNCH,
			     _("There was an error launching the default action command associated "
			       "with this location."));
		break;

	case GNOME_VFS_ERROR_NO_DEFAULT:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_NO_DEFAULT,
			     _("There is no default action associated with this location."));
		break;

	case GNOME_VFS_ERROR_NOT_SUPPORTED:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_NOT_SUPPORTED,
			     _("The default action does not support this protocol."));
		break;

	case GNOME_VFS_ERROR_CANCELLED:
		g_set_error (error,
		             AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_CANCELLED,
			     _("The request was cancelled."));
		break;

	case GNOME_VFS_ERROR_HOST_NOT_FOUND:
		{
			vfs_uri = gnome_vfs_uri_new (url);
			if (gnome_vfs_uri_get_host_name (vfs_uri) != NULL) {
				g_set_error (error,
					     AMIDE_GNOME_URL_ERROR,
					     AMIDE_GNOME_URL_ERROR_VFS,
					     _("The host \"%s\" could not be found."),
					     gnome_vfs_uri_get_host_name (vfs_uri));
			} else {
				g_set_error (error,
					     AMIDE_GNOME_URL_ERROR,
					     AMIDE_GNOME_URL_ERROR_VFS,
					     _("The host could not be found."));
			}
			gnome_vfs_uri_unref (vfs_uri);
		}
		break;

	case GNOME_VFS_ERROR_INVALID_URI:
	case GNOME_VFS_ERROR_NOT_FOUND:
		g_set_error (error,
		             AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_VFS,
			     _("The location or file could not be found."));
		break;

	case GNOME_VFS_ERROR_LOGIN_FAILED:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_VFS,
			     _("The login has failed."));
		break;
	default:
		g_set_error (error,
			     AMIDE_GNOME_URL_ERROR,
			     AMIDE_GNOME_URL_ERROR_VFS,
			     "%s", gnome_vfs_result_to_string (result));
	}

	return FALSE;
#else
	char   *argv[] = { NULL, NULL, NULL };

	/* TODO : handle translations when we generate them */
	argv[0] = (char *)"hh";
	argv[1] = url;
	g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
		NULL, NULL, NULL, error);

	return TRUE;
#endif
}

static gboolean amide_gnome_help_display_uri_with_env (const char  *help_uri, char **envp, GError     **error) {
	GError *real_error;
	gboolean retval;

	real_error = NULL;
	retval = amide_gnome_url_show_with_env (help_uri, envp, &real_error);

	if (real_error != NULL) g_propagate_error (error, real_error);

	return retval;
}



static char * locate_help_file (const char *path, const char *doc_name) {
	int i, j;
	char *exts[] = {  "", ".xml", ".docbook", ".sgml", ".html", NULL };
	const char * const * lang_list = g_get_language_names ();

	for (j = 0;lang_list[j] != NULL; j++) {
		const char *lang = lang_list[j];

		/* This has to be a valid language AND a language with
		 * no encoding postfix.  The language will come up without
		 * encoding next */
		if (lang == NULL ||
		    strchr (lang, '.') != NULL)
			continue;

		for (i = 0; exts[i] != NULL; i++) {
			char *name;
			char *full;

			name = g_strconcat (doc_name, exts[i], NULL);
			full = g_build_filename (path, lang, name, NULL);
			g_free (name);
			
			if (g_file_test (full, G_FILE_TEST_EXISTS))
			  return full;

			g_free (full);
		}
	}

	return NULL;
}

/* returns an array of strings (strings can be potentially null) - array and strings need to be free'd */
static gchar ** amide_gnome_program_locate_help_file (const gchar *file_name, gboolean only_if_exists) {

  gchar *buf;
  gchar ** retvals;
  gchar ** dirs;
  const gchar * const * global_dirs;
  gint i;
  gint j;
  gint count=0;
  gint slots;

  g_return_val_if_fail (file_name != NULL, NULL);

  global_dirs = g_get_system_data_dirs();
  while (global_dirs[count] != NULL) count++; /* count */

  /* allocate the array of strings -  extra spot for AMIDE_DATADIR, 
     plus an extra spot if on win32,
     plus one at end for NULL termination */
#if defined (G_PLATFORM_WIN32)
  slots = count+3;
#else
  slots = count+2;
#endif

  dirs = g_try_new0(gchar *,slots); 
  retvals = g_try_new0(gchar *,slots);
  g_return_val_if_fail((dirs != NULL) && (retvals != NULL), NULL);

  j=0;
  /* copy over the directories */
  dirs[j] = g_strdup(AMIDE_DATADIR); /* first entry */
  j++;

  /* FIXME, below function is now deprecated 
#if defined (G_PLATFORM_WIN32)
  dirs[j] = g_win32_get_package_installation_subdirectory(NULL, NULL,"share");
  j++;
#endif
  */

  i=0;
  while (global_dirs[i] != NULL) { /* rest of the entries */
    dirs[j] = g_strdup(global_dirs[i]);
    i++;
    j++;
  }


  /* Potentially add an absolute path */
  if (g_path_is_absolute (file_name)) 
    if (!only_if_exists || g_file_test (file_name, G_FILE_TEST_EXISTS)) {
      retvals[0] = g_strdup(file_name);
      return retvals; /* we're already done */
    }

  /* use the prefix */
  if (AMIDE_DATADIR == NULL) {
    g_warning (G_STRLOC ": Directory properties not set correctly.  "
	       "Cannot locate application specific files.");
    return retvals;
  }

  i=0;
  j=0;
  while (dirs[i] != NULL) {
    buf = g_strdup_printf ("%s%s%s%s%s%s%s", dirs[i], G_DIR_SEPARATOR_S,"gnome",G_DIR_SEPARATOR_S, "help", G_DIR_SEPARATOR_S,file_name);
    
    if (!only_if_exists || g_file_test (buf, G_FILE_TEST_EXISTS)) {
      retvals[j] = buf;
      j++;
    } else
      g_free(buf);
    i++;
  }

  i=0;
  while (dirs[i] != NULL) {
    g_free(dirs[i]);
    i++;
  }
  g_free(dirs);

  return retvals;
}


typedef enum {
  AMIDE_GNOME_HELP_ERROR_INTERNAL,
  AMIDE_GNOME_HELP_ERROR_NOT_FOUND
} AmideGnomeHelpError;

#define AMIDE_GNOME_HELP_ERROR (amide_gnome_help_error_quark ())
GQuark amide_gnome_help_error_quark (void) {
  static GQuark error_quark = 0;
  
  if (error_quark == 0)
    error_quark = g_quark_from_static_string ("amide-gnome-help-error-quark");
  
  return error_quark;
}


gboolean amide_gnome_help_display (const char *file_name, const char *link_id, GError **error) {

  gchar ** help_paths;
  gchar *file=NULL;
  struct stat help_st;
  gchar *uri=NULL;
  gboolean retval;
  const char    *doc_id = PACKAGE;
  gint i;
  gchar * temp_str=NULL;
  
  g_return_val_if_fail (file_name != NULL, FALSE);
  
  retval = FALSE;
  
  help_paths = amide_gnome_program_locate_help_file (doc_id,FALSE);
  
  if (help_paths == NULL) {
    g_set_error (error, AMIDE_GNOME_HELP_ERROR, AMIDE_GNOME_HELP_ERROR_INTERNAL,
		 _("Unable to find the app or global HELP domain"));
    goto out;
  }
  
  /* Try to access the help paths, first the app-specific help path
   * and then falling back to the global help path if the first one fails.
   */
  
  i=0;
  while ((help_paths[i] != NULL) && (file == NULL)) {
    if (g_stat (help_paths[i], &help_st) == 0) {
      if (!S_ISDIR (help_st.st_mode)) 
	goto error;
      file = locate_help_file (help_paths[i], file_name);
    }
    i++;
  }
  if (file == NULL) goto error;

  /* Now that we have a file name, try to display it in the help browser */
  if (link_id)
    uri = g_strconcat ("ghelp://", file, "?", link_id, NULL);
  else
    uri = g_strconcat ("ghelp://", file, NULL);
  
  retval = amide_gnome_help_display_uri_with_env (uri, NULL, error);

  goto out;

 error:
  
  i=0;
  while (help_paths[i] != NULL) {
    amitk_append_str(&temp_str, "%s;", help_paths[i]);
    i++;
  }
  g_set_error (error, AMIDE_GNOME_HELP_ERROR, AMIDE_GNOME_HELP_ERROR_NOT_FOUND,
	       _("Unable to find the help files in any of the following directories, "
                 " or these are not valid directories: %s "
		 ":  Please check your installation"),
	       temp_str);
  g_free(temp_str);

 out:
  if (help_paths != NULL) {
    i=0;
    while (help_paths[i] != NULL) {
      g_free(help_paths[i]);
      i++;
    }
    g_free(help_paths);
  }
  if (file != NULL) g_free (file);
  if (uri != NULL) g_free (uri);

  return retval;
}


