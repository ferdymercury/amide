/* amitk_line_profile.h
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2003-2014 Andy Loening
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

#ifndef __AMITK_LINE_PROFILE_H__
#define __AMITK_LINE_PROFILE_H__

/* header files that are always needed with this file */
#include <glib-object.h>
#include "amitk_object.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_LINE_PROFILE		      (amitk_line_profile_get_type ())
#define AMITK_LINE_PROFILE(object)	      (G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_LINE_PROFILE, AmitkLineProfile))
#define AMITK_LINE_PROFILE_CLASS(klass)	      (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_LINE_PROFILE, AmitkLineProfileClass))
#define AMITK_IS_LINE_PROFILE(object)	      (G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_LINE_PROFILE))
#define AMITK_IS_LINE_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_LINE_PROFILE))
#define	AMITK_LINE_PROFILE_GET_CLASS(object)  (G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_LINE_PROFILE, AmitkLineProfileClass))


#define AMITK_LINE_PROFILE_VIEW(linep)        (AMITK_LINE_PROFILE(linep)->view)
#define AMITK_LINE_PROFILE_ANGLE(linep)       (AMITK_LINE_PROFILE(linep)->angle)
#define AMITK_LINE_PROFILE_VISIBLE(linep)     (AMITK_LINE_PROFILE(linep)->visible)

#define AMITK_LINE_PROFILE_START_POINT(linep) (AMITK_LINE_PROFILE(linep)->start_point)
#define AMITK_LINE_PROFILE_END_POINT(linep)   (AMITK_LINE_PROFILE(linep)->end_point)


typedef struct _AmitkLineProfileClass AmitkLineProfileClass;
typedef struct _AmitkLineProfile      AmitkLineProfile;


struct _AmitkLineProfile {

  GObject parent;

  AmitkView view;
  amide_real_t angle; /* in radians */
  gboolean visible;

  /* in base coordinate frame.  These are updated by the canvas */
  AmitkPoint start_point;
  AmitkPoint end_point;

};

struct _AmitkLineProfileClass
{
  GObjectClass parent_class;
  void (* line_profile_changed) (AmitkLineProfile * line_profile);

};

typedef struct _AmitkLineProfileDataElement AmitkLineProfileDataElement;

struct _AmitkLineProfileDataElement {
  amide_real_t value;
  amide_real_t location;
};


/* ------------ external functions ---------- */

GType	            amitk_line_profile_get_type	      (void);
AmitkLineProfile*   amitk_line_profile_new            (void);
void                amitk_line_profile_copy_in_place  (AmitkLineProfile * dest_profile,
						       const AmitkLineProfile * src_profile);
void                amitk_line_profile_set_view       (AmitkLineProfile * line_profile,
						       const AmitkView view);
void                amitk_line_profile_set_angle      (AmitkLineProfile * line_profile,
						       const amide_real_t angle);
void                amitk_line_profile_set_visible    (AmitkLineProfile * line_profile,
						       const gboolean visible);
void                amitk_line_profile_set_start_point(AmitkLineProfile * line_profile,
						       const AmitkPoint start_point);
void                amitk_line_profile_set_end_point  (AmitkLineProfile * line_profile,
						       const AmitkPoint end_point);

G_END_DECLS
#endif /* __AMITK_LINE_PROFILE_H__ */

