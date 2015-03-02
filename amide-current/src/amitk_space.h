/* amitk_space.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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

#ifndef __AMITK_SPACE_H__
#define __AMITK_SPACE_H__

#include "amitk_point.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_SPACE		(amitk_space_get_type ())
#define AMITK_SPACE(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_SPACE, AmitkSpace))
#define AMITK_SPACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_SPACE, AmitkSpaceClass))
#define AMITK_IS_SPACE(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_SPACE))
#define AMITK_IS_SPACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_SPACE))
#define	AMITK_SPACE_GET_CLASS(object)	(G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_SPACE, AmitkSpaceClass))

#define AMITK_SPACE_AXES(space)         (AMITK_SPACE(space)->axes)
#define AMITK_SPACE_OFFSET(space)       (AMITK_SPACE(space)->offset)


typedef struct _AmitkSpaceClass	AmitkSpaceClass;
typedef struct _AmitkSpace AmitkSpace;

#define AMITK_UNDO_LEVEL 10

/* The AmitkSpace structure is the base of the Amitk objects hierarchy.
 * It describes an euclidean reference frame with respect to the 
 * base coordinate space.  Offset is in the base coordinate frame.
 */
struct _AmitkSpace
{
  GObject parent;


  /* private info */
  AmitkPoint offset; /* with respect to the base coordinate frame */
  AmitkAxes axes;

};

struct _AmitkSpaceClass
{
  GObjectClass parent_class;

  void (* space_shift)          (AmitkSpace * space,
				 AmitkPoint * shift);
  void (* space_rotate)         (AmitkSpace * space,
				 AmitkPoint * vector,
				 amide_real_t theta,
				 AmitkPoint * center_of_rotation);
  void (* space_invert)         (AmitkSpace * space,
			         AmitkAxis which_axis,
				 AmitkPoint * center_of_inversion);
  void (* space_transform)      (AmitkSpace * space,
				 AmitkSpace * transform_space);
  void (* space_transform_axes) (AmitkSpace * space,
				 AmitkAxes    transform_axes,
				 AmitkPoint * center_of_rotation);
  void (* space_scale)          (AmitkSpace * space,
				 AmitkPoint * ref_point,
				 AmitkPoint * scaling);
  void (* space_changed)        (AmitkSpace * space);

};



/* Application-level methods */

GType	        amitk_space_get_type	            (void);
AmitkSpace*     amitk_space_new                     (void);
void            amitk_space_write_xml               (xmlNodePtr node, 
						     gchar * descriptor, 
						     AmitkSpace * space);
AmitkSpace *    amitk_space_read_xml                (xmlNodePtr nodes, 
						     gchar * descriptor,
						     gchar ** perror_buf);
void            amitk_space_set_offset              (AmitkSpace * space, 
						     const AmitkPoint new_offset);
void            amitk_space_shift_offset            (AmitkSpace * space, 
						     const AmitkPoint shift);
void            amitk_space_set_axes                (AmitkSpace * space, 
						     const AmitkAxes new_axes,
						     const AmitkPoint center_of_rotation);
AmitkSpace *    amitk_space_calculate_transform    (const AmitkSpace * dest_space,
						    const AmitkSpace * src_space);
void            amitk_space_transform               (AmitkSpace * space, 
						     const AmitkSpace * transform_space);
void            amitk_space_transform_axes          (AmitkSpace * space, 
						     const AmitkAxes transform_axes,
						     AmitkPoint center_of_rotation);
void            amitk_space_scale                   (AmitkSpace * space, 
						     AmitkPoint ref_point, 
						     AmitkPoint scaling);
AmitkPoint      amitk_space_get_axis                (const AmitkSpace * space, 
						     const AmitkAxis which_axis);
void            amitk_space_get_enclosing_corners   (const AmitkSpace * in_space, 
						     const AmitkCorners in_corners, 
						     const AmitkSpace * out_space, 
						     AmitkCorners out_corners);
AmitkSpace *    amitk_space_copy                    (const AmitkSpace * space);
void            amitk_space_copy_in_place           (AmitkSpace * dest_space,
						     const AmitkSpace * src_space);
gboolean        amitk_space_axes_equal              (const AmitkSpace * space1,
						     const AmitkSpace * space2);
gboolean        amitk_space_axes_close              (const AmitkSpace * space1,
						     const AmitkSpace * space2);
gboolean        amitk_space_equal                   (const AmitkSpace * space1,
						     const AmitkSpace * space2);
void            amitk_space_invert_axis             (AmitkSpace * space, 
						     const AmitkAxis which_axis,
						     const AmitkPoint center_of_inversion);
void            amitk_space_rotate_on_vector        (AmitkSpace * space, 
						     const AmitkPoint vector, 
						     const amide_real_t theta,
						     const AmitkPoint center_of_rotation);
AmitkSpace *    amitk_space_get_view_space          (const AmitkView view,
						     const AmitkLayout layout);
void            amitk_space_set_view_space          (AmitkSpace * set_space, 
						     const AmitkView view, 
						     const AmitkLayout layout);

  
/* functions that need to be fast, they do no error checking */
AmitkPoint     amitk_space_b2s           (const AmitkSpace * space, AmitkPoint in_rp);
AmitkPoint     amitk_space_s2b           (const AmitkSpace * space, const AmitkPoint in_rp);
#define amitk_space_s2s(in_space, out_space, in) (amitk_space_b2s((out_space), amitk_space_s2b((in_space), (in))))

AmitkPoint     amitk_space_s2b_dim       (const AmitkSpace * space, const AmitkPoint in_rp);
AmitkPoint     amitk_space_b2s_dim       (const AmitkSpace * space, const AmitkPoint in_rp);
#define amitk_space_s2s_dim(in_space, out_space, in) (amitk_space_b2s_dim((out_space), amitk_space_s2b_dim((in_space), (in))))


/* debugging functions */
void amitk_space_print(AmitkSpace * space, gchar * message);




G_END_DECLS

#endif /* __AMITK_SPACE_H__ */
