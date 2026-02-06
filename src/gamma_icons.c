/* This file contains the icons of the AmitkGammaCurve widget; it was
   generated semi-automatically by using the following GTK 2 program:

,----
| #include <gtk/gtk.h>
|
| int
| main (int argc, char **argv)
| {
|   GtkWidget *win;
|   GtkWidget *gamma;
|   GtkWidget *image;
|   GdkPixbuf *pixbuf;
|
|   gtk_init (&argc, &argv);
|
|   win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
|   g_signal_connect (win, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
|   gamma = gtk_gamma_curve_new ();
|   gtk_container_add (GTK_CONTAINER (win), gamma);
|   gtk_widget_show_all (win);
|
|   image = gtk_bin_get_child (GTK_BIN (GTK_GAMMA_CURVE (gamma)->button[0]));
|   pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (image));
|   gdk_pixbuf_save (pixbuf, "amide_gamma_icon0.png", "png", NULL);
|
|   image = gtk_bin_get_child (GTK_BIN (GTK_GAMMA_CURVE (gamma)->button[1]));
|   pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (image));
|   gdk_pixbuf_save (pixbuf, "amide_gamma_icon1.png", "png", NULL);
|
|   image = gtk_bin_get_child (GTK_BIN (GTK_GAMMA_CURVE (gamma)->button[4]));
|   pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (image));
|   gdk_pixbuf_save (pixbuf, "amide_gamma_icon2.png", "png", NULL);
|
|   gtk_main ();
|
|   return 0;
| }
`----

  Then the icons were put in the following GResource XML file:

,---- gamma_icons.xml
| <?xml version="1.0" encoding="UTF-8"?>
| <gresources>
|   <gresource prefix="/com/github/ferdymercury/amide/gamma">
|     <file>amide_gamma_icon0.png</file>
|     <file>amide_gamma_icon1.png</file>
|     <file>amide_gamma_icon2.png</file>
|   </gresource>
| </gresources>
`----

  This C file was obtained by running
    glib-compile-resources --generate-source gamma_icons.xml
*/

#include <gio/gio.h>

#if defined (__ELF__) && ( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
# define SECTION __attribute__ ((section (".gresource.gamma_icons"), aligned (8)))
#else
# define SECTION
#endif

static const SECTION union { const guint8 data[949]; const double alignment; void * const ptr;}  gamma_icons_resource_data = {
  "\107\126\141\162\151\141\156\164\000\000\000\000\000\000\000\000"
  "\030\000\000\000\034\001\000\000\000\000\000\050\011\000\000\000"
  "\000\000\000\000\000\000\000\000\001\000\000\000\001\000\000\000"
  "\004\000\000\000\004\000\000\000\005\000\000\000\010\000\000\000"
  "\011\000\000\000\302\257\211\013\004\000\000\000\034\001\000\000"
  "\004\000\114\000\040\001\000\000\044\001\000\000\364\213\151\075"
  "\007\000\000\000\044\001\000\000\025\000\166\000\100\001\000\000"
  "\306\001\000\000\362\132\105\075\007\000\000\000\306\001\000\000"
  "\025\000\166\000\340\001\000\000\252\002\000\000\163\163\127\075"
  "\007\000\000\000\252\002\000\000\025\000\166\000\300\002\000\000"
  "\157\003\000\000\324\265\002\000\377\377\377\377\157\003\000\000"
  "\001\000\114\000\160\003\000\000\164\003\000\000\363\023\273\152"
  "\010\000\000\000\164\003\000\000\006\000\114\000\174\003\000\000"
  "\200\003\000\000\224\135\334\227\000\000\000\000\200\003\000\000"
  "\007\000\114\000\210\003\000\000\214\003\000\000\205\101\141\346"
  "\005\000\000\000\214\003\000\000\006\000\114\000\224\003\000\000"
  "\240\003\000\000\244\212\170\021\006\000\000\000\240\003\000\000"
  "\015\000\114\000\260\003\000\000\264\003\000\000\143\157\155\057"
  "\006\000\000\000\141\155\151\144\145\137\147\141\155\155\141\137"
  "\151\143\157\156\062\056\160\156\147\000\000\000\000\000\000\000"
  "\166\000\000\000\000\000\000\000\211\120\116\107\015\012\032\012"
  "\000\000\000\015\111\110\104\122\000\000\000\020\000\000\000\020"
  "\010\006\000\000\000\037\363\377\141\000\000\000\004\163\102\111"
  "\124\010\010\010\010\174\010\144\210\000\000\000\055\111\104\101"
  "\124\070\215\143\144\040\037\374\147\140\140\140\140\042\127\167"
  "\223\243\043\105\126\377\037\325\074\252\171\030\152\376\337\344"
  "\350\110\276\315\020\313\311\267\035\000\000\350\036\357\200\371"
  "\024\076\000\000\000\000\111\105\116\104\256\102\140\202\000\000"
  "\050\165\165\141\171\051\141\155\151\144\145\137\147\141\155\155"
  "\141\137\151\143\157\156\060\056\160\156\147\000\000\000\000\000"
  "\272\000\000\000\000\000\000\000\211\120\116\107\015\012\032\012"
  "\000\000\000\015\111\110\104\122\000\000\000\020\000\000\000\020"
  "\010\006\000\000\000\037\363\377\141\000\000\000\004\163\102\111"
  "\124\010\010\010\010\174\010\144\210\000\000\000\161\111\104\101"
  "\124\070\215\255\121\133\016\300\040\014\102\157\326\223\065\361"
  "\342\354\143\232\350\046\353\346\044\061\215\017\204\322\204\165"
  "\020\000\362\052\273\230\375\320\076\345\271\205\274\334\202\204"
  "\273\023\000\153\175\124\127\140\267\302\017\156\055\264\164\147"
  "\051\023\140\002\122\044\117\145\065\264\177\175\320\357\025\171"
  "\337\024\224\102\075\147\061\373\146\177\274\322\123\311\215\254"
  "\322\165\367\241\116\025\102\173\001\244\275\067\070\000\162\046"
  "\070\352\371\102\163\071\000\000\000\000\111\105\116\104\256\102"
  "\140\202\000\000\050\165\165\141\171\051\141\155\151\144\145\137"
  "\147\141\155\155\141\137\151\143\157\156\061\056\160\156\147\000"
  "\237\000\000\000\000\000\000\000\211\120\116\107\015\012\032\012"
  "\000\000\000\015\111\110\104\122\000\000\000\020\000\000\000\020"
  "\010\006\000\000\000\037\363\377\141\000\000\000\004\163\102\111"
  "\124\010\010\010\010\174\010\144\210\000\000\000\126\111\104\101"
  "\124\070\215\275\222\061\016\000\040\010\003\253\377\341\377\213"
  "\276\255\216\106\305\100\300\330\021\070\050\204\202\270\010\000"
  "\305\131\111\055\156\302\004\170\203\115\266\211\270\100\315\005"
  "\215\374\242\272\007\232\210\033\126\025\335\071\014\037\053\174"
  "\235\376\316\101\346\160\151\007\117\032\260\317\347\011\211\110"
  "\334\140\000\170\126\030\101\135\240\266\230\000\000\000\000\111"
  "\105\116\104\256\102\140\202\000\000\050\165\165\141\171\051\057"
  "\000\000\000\000\141\155\151\144\145\057\000\000\007\000\000\000"
  "\147\151\164\150\165\142\057\000\010\000\000\000\147\141\155\155"
  "\141\057\000\000\002\000\000\000\003\000\000\000\001\000\000\000"
  "\146\145\162\144\171\155\145\162\143\165\162\171\057\000\000\000"
  "\005\000\000\000" };

static GStaticResource static_resource = { gamma_icons_resource_data.data, sizeof (gamma_icons_resource_data.data) - 1 /* nul terminator */, NULL, NULL, NULL };

G_MODULE_EXPORT
GResource *gamma_icons_get_resource (void);
GResource *gamma_icons_get_resource (void)
{
  return g_static_resource_get_resource (&static_resource);
}
/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_CONSTRUCTOR_H__
#define __G_CONSTRUCTOR_H__

/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a usable way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#ifndef __GTK_DOC_IGNORE__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
/* Visual studio 2008 and later has _Pragma */

/*
 * Only try to include gslist.h if not already included via glib.h,
 * so that items using gconstructor.h outside of GLib (such as
 * GResources) continue to build properly.
 */
#ifndef __G_LIB_H__
#include "gslist.h"
#endif

#include <stdlib.h>

#define G_HAS_CONSTRUCTORS 1

/* We do some weird things to avoid the constructors being optimized
 * away on VS2015 if WholeProgramOptimization is enabled. First we
 * make a reference to the array from the wrapper to make sure its
 * references. Then we use a pragma to make sure the wrapper function
 * symbol is always included at the link stage. Also, the symbols
 * need to be extern (but not dllexport), even though they are not
 * really used from another object file.
 */

/* We need to account for differences between the mangling of symbols
 * for x86 and x64/ARM/ARM64 programs, as symbols on x86 are prefixed
 * with an underscore but symbols on x64/ARM/ARM64 are not.
 */
#ifdef _M_IX86
#define G_MSVC_SYMBOL_PREFIX "_"
#else
#define G_MSVC_SYMBOL_PREFIX ""
#endif

#define G_DEFINE_CONSTRUCTOR(_func) G_MSVC_CTOR (_func, G_MSVC_SYMBOL_PREFIX)
#define G_DEFINE_DESTRUCTOR(_func) G_MSVC_DTOR (_func, G_MSVC_SYMBOL_PREFIX)

#define G_MSVC_CTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _wrapper(void);              \
  int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
  __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define G_MSVC_DTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _constructor(void);              \
  int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
   __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#elif defined (_MSC_VER)

#define G_HAS_CONSTRUCTORS 1

/* Pre Visual studio 2008 must use #pragma section */
#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func ## _wrapper;

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __GTK_DOC_IGNORE__ */
#endif /* __G_CONSTRUCTOR_H__ */

#ifdef G_HAS_CONSTRUCTORS

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(gamma_iconsresource_constructor)
#endif
G_DEFINE_CONSTRUCTOR(gamma_iconsresource_constructor)
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(gamma_iconsresource_destructor)
#endif
G_DEFINE_DESTRUCTOR(gamma_iconsresource_destructor)

#else
#warning "Constructor not supported on this compiler, linking in resources will not work"
#endif

static void gamma_iconsresource_constructor (void)
{
  g_static_resource_init (&static_resource);
}

static void gamma_iconsresource_destructor (void)
{
  g_static_resource_fini (&static_resource);
}
