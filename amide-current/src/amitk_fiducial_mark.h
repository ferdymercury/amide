/* amitk_fiducial_mark.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2003 Andy Loening
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

#ifndef __AMITK_FIDUCIAL_MARK_H__
#define __AMITK_FIDUCIAL_MARK_H__


#include "amitk_object.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_FIDUCIAL_MARK		     (amitk_fiducial_mark_get_type ())
#define AMITK_FIDUCIAL_MARK(fiducial_mark)	     (G_TYPE_CHECK_INSTANCE_CAST ((fiducial_mark), AMITK_TYPE_FIDUCIAL_MARK, AmitkFiducialMark))
#define AMITK_FIDUCIAL_MARK_CLASS(klass)	     (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_FIDUCIAL_MARK, AmitkFiducialMarkClass))
#define AMITK_IS_FIDUCIAL_MARK(fiducial_mark)	     (G_TYPE_CHECK_INSTANCE_TYPE ((fiducial_mark), AMITK_TYPE_FIDUCIAL_MARK))
#define AMITK_IS_FIDUCIAL_MARK_CLASS(klass)	     (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_FIDUCIAL_MARK))
#define	AMITK_FIDUCIAL_MARK_GET_CLASS(fiducial_mark) (G_TYPE_CHECK_GET_CLASS ((fiducial_mark), AMITK_TYPE_FIDUCIAL_MARK, AmitkFiducialMarkClass))

#define AMITK_FIDUCIAL_MARK_GET(mark)                (AMITK_SPACE_OFFSET(mark))



typedef struct _AmitkFiducialMarkClass AmitkFiducialMarkClass;
typedef struct _AmitkFiducialMark AmitkFiducialMark;


struct _AmitkFiducialMark
{
  AmitkObject parent;

};

struct _AmitkFiducialMarkClass
{
  AmitkObjectClass parent_class;

};



/* Application-level methods */

GType	            amitk_fiducial_mark_get_type	     (void);
AmitkFiducialMark * amitk_fiducial_mark_new                  (void);
void                amitk_fiducial_mark_set                  (AmitkFiducialMark * fiducial_mark,
							      AmitkPoint new_point);


G_END_DECLS

#endif /* __AMITK_FIDUCIAL_MARK_H__ */
