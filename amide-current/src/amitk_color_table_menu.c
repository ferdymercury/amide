/* amitk_color_table_menu.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2007 Andy Loening
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


#include "amide_config.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "amitk_color_table_menu.h"
#include "image.h"

#define MENU_COLOR_SCALE_HEIGHT 8
#define MENU_COLOR_SCALE_WIDTH 30


static void color_table_menu_class_init (AmitkColorTableMenuClass *klass);
static void color_table_menu_init (AmitkColorTableMenu *menu);

//static GtkComboBoxClass *parent_class;
static GtkOptionMenuClass *parent_class;

GType amitk_color_table_menu_get_type (void) {

  static GType color_table_menu_type = 0;

  if (!color_table_menu_type)
    {
      static const GTypeInfo color_table_menu_info =
      {
	sizeof (AmitkColorTableMenuClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) color_table_menu_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkColorTableMenu),
	0, /* # preallocs */
	(GInstanceInitFunc) color_table_menu_init,
	NULL /* value table */
      };

      //      color_table_menu_type = g_type_register_static(GTK_TYPE_COMBO_BOX,
      color_table_menu_type = g_type_register_static(GTK_TYPE_OPTION_MENU,
						     "AmitkColorTableMenu", 
						     &color_table_menu_info, 0);
    }

  return color_table_menu_type;
}

static void color_table_menu_class_init (AmitkColorTableMenuClass *klass) {
  parent_class = g_type_class_peek_parent(klass);
}



#if 1
/* this gets called when the hbox inside of a menu item is realized */
static void pix_box_realize_cb(GtkWidget * pix_box, gpointer data) {

  AmitkColorTable color_table = GPOINTER_TO_INT(data);
  GdkPixbuf * pixbuf;
  GtkWidget * image;

  g_return_if_fail(pix_box != NULL);

  /* make sure we don't get called again */
  if (gtk_container_get_children(GTK_CONTAINER(pix_box)) != NULL) return;

  pixbuf = image_from_colortable(color_table, MENU_COLOR_SCALE_WIDTH, MENU_COLOR_SCALE_HEIGHT,
				 0, MENU_COLOR_SCALE_WIDTH-1, 0, MENU_COLOR_SCALE_WIDTH-1, TRUE);
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(pixbuf);

  gtk_box_pack_start(GTK_BOX(pix_box), image, FALSE, FALSE, 0);
  gtk_widget_show(image);

  return;
}

static GtkWidget * item_new(AmitkColorTable color_table) {

  GtkWidget * label;
  GtkWidget * hbox;
  GtkWidget * pix_box;
  GtkWidget * item;

  item = gtk_menu_item_new();

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(item), hbox);
  gtk_widget_show(hbox);

  pix_box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), pix_box, FALSE, FALSE, 3);
  gtk_widget_show(pix_box);
  g_signal_connect(G_OBJECT(pix_box), "realize",  G_CALLBACK(pix_box_realize_cb), 
		   GINT_TO_POINTER(color_table));

  label = gtk_label_new(color_table_menu_names[color_table]);
  gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 3);
  gtk_widget_show(label);

  return item;
}


 static void color_table_menu_init (AmitkColorTableMenu * ct_menu) {
 
  GtkWidget * menu;
  GtkWidget * menuitem;
   AmitkColorTable i_color_table;

   menu = gtk_menu_new();
                                                                                
   for(i_color_table=0; i_color_table<AMITK_COLOR_TABLE_NUM; i_color_table++) {
    menuitem = item_new(i_color_table);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_show(menuitem);
   }
                                                                                
  gtk_option_menu_set_menu(GTK_OPTION_MENU(ct_menu), menu);
  gtk_widget_show(menu);

  gtk_widget_set_size_request(GTK_WIDGET(ct_menu), 185, -1);

  return;
}
 
#endif




#if 0


static void color_table_menu_init (AmitkColorTableMenu * ct_menu) {

  GtkCellRenderer *renderer;
  GdkPixbuf *pixbuf;
  GtkTreeIter iter;
  GtkListStore *store;
  AmitkColorTable i_color_table;
  
  /* create the store of data */
  store = gtk_list_store_new(2, /* NUM_COLUMNS */
			     GDK_TYPE_PIXBUF, 
			     G_TYPE_STRING);

  for(i_color_table=0; i_color_table<AMITK_COLOR_TABLE_NUM; i_color_table++) {
    pixbuf = image_from_colortable(i_color_table, MENU_COLOR_SCALE_WIDTH, MENU_COLOR_SCALE_HEIGHT,
				   0, MENU_COLOR_SCALE_WIDTH-1, 0, MENU_COLOR_SCALE_WIDTH-1, TRUE);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			0, pixbuf,
			1, color_table_menu_names[i_color_table],
			-1);
    g_object_unref(pixbuf);
  }


  /* add it to the combo box */
  gtk_combo_box_set_model(GTK_COMBO_BOX(ct_menu), GTK_TREE_MODEL(store));

  /* setup the cell renderers */
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ct_menu),
			      renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ct_menu), 
				  renderer,"pixbuf", 0, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ct_menu),
			      renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (ct_menu), 
				  renderer, "text", 1, NULL);

  return;
}

#endif



GtkWidget * amitk_color_table_menu_new(AmitkColorTable color_table) {

  AmitkColorTableMenu * color_table_menu;
  color_table_menu = g_object_new(amitk_color_table_menu_get_type (), NULL);

  return GTK_WIDGET (color_table_menu);
}



