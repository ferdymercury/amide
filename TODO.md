# Misc list of things todo:

for next release?
-----------------
* gnome_canvas font looks bad on windows, switched to Tahoma looks a little better but not true type'd. Figure out what font gtk's using.
* calling help on win32 creates coredump
* exporting gated studies via DICOM and reloading doesn't work. Maybe an error in exporting, or an error in importing gated DICOM
* change wait cursor to be whole dialog, not just canvas
* revert on dialogs doesn't work correctly... children get readded
* freehand/isocontour roi's - draw them at high mag - will see they're shifted just slightly off voxel lines [2]
 [x] noticed by Mark Huisman

* Gate picker -> multiple selection still screwed up


Mac OS X 
--------
* clicking on a .xif study loads up AMIDE, but without the file.
[x] noticed by Eric Wolsztynski


Win32
---------------- 
* XIF files appar to be limited to 2 GB even on 64bit windows 7 [noticed by Michael Braden]
* g_win32_get_package_installation_subdirectoy in amide_gnome is
  deprecated, currently commented out. Need to fix starting up help
* help works in cygwin setup, but not in installed package. Seems to know the
  right place to look, just isn't doing something correctly....
* Copy to clipboard function in roi analysis widget doesn't work on Windows 7,
  although it does work on Windows XP, likely a win32 bug in gtk_clipboard on win32... 
  (noticed by Arutselvan Natarajan)
* export pixel size text boxes get ignored - likely using wrong "update" event
  (noticed by Priti Madhav and Ian Miller)
* zdial on rendering doesn't work

big things:
-----------
* figuring out orientation from files loaded from xmedcon isn't strictly correct, and screws up with NIFTI format
  look at notes in libmdc_interface.c. Also would likely need fixes in libmdc itself (see
  0.10.3_nifti_unsubmitted changes)
* polygonal ROI's (requested by A.Mehranian)
* add drag-n-drop capabilities between study windows
	don't use treeview reorderable stuff, too limited,
* toolbox dialog for generating polar maps (requested by Jeff Arkles and others)
* should be able to specify units the image/display info is in, and request different output units.
	-allow changing of displayed units (mm, cm, m, s, min, uCi, Beq, etc.)
	-study would have an output units variable
	-each data set, can specify it's original units, and (if needed), a scaling factor to output unit
	-ui_preferences would change default units
	-need gobject based approach, so widgets can listen for change
	-> start with mm/cm/m
* add a data sets list item,
	-would allow reading in multi-bed data sets
	-raw_data type could have amitk_space made its parent, so it has its own space...
* calculating PV correction.  
	Correction factors could be calculated for each volume's 
	resolution and then cached.
* multi-step undo/redo, look into how other GTK apps do it
	-once this is done, drop the enact/cancel bit of shifting data sets and study's
	  as we can now just undo such a change
* create an AmitkAnalysis type, holding voxel statistics
	-when created, passed a study
	-use callbacks, so that stats are marked as current or obsolete,
	 stats also marked as obsolete if a AmitkAnalysis calculation type
         changed from "fast" to "accurate" or back
        -stats are only (re)calculated as needed (i.e. when stat values/raw values are requested)
        -has a callback "amitk_analysis_changed", so that the roi_analysis_widget
         knows to update it's table.
        -function to request stats as an array of structs
        -function to request raw values as array of structs
* ability to import and export AIR .air files
  -probably from rotate axis page
  -need object list widget: should update automatically when something is removed somewhere else

                       
small things:
-------------
* XIF directory format doesn't handle characters in study/data set names that are inconsistent with the file system [noticed by Michael Braden]
* boolean operators between ROI's, would allow subtracting one from another (requested by A. Mehranian)
* be able to export the color scale (with appropriate numerical values) to an image file (requested by Frezghi Habte)
* pop-up widget allowing value underneath cursor to be shown for all data sets (not just active). (requested by Axel Moeller)
* hot key to cycle through time frames (requested by Michael Braden)
* allow direct entry of transformation matrix in amitk_space_edit
* data set icons in amitk_tree could be generated from the actual respective data sets
* .xif files should try to compress data.
* amitk_preferences.c : default color table, panel layout, layout should be saved in gsettings as they 
  associated strings for these values, not as the enumerated numbers
* change tb_filter, tb_crop, tb_fads, so that an amitk_tree is used for the
	initial popup letting you change which data set will be used
* export ROI's to external file? and allow import?
* continue implementing progress bars for various functions:
	-roi analysis
	-loading in data sets
* could probably subclass isocontour's from AmitkRoi -> AimtkIsocontourRoi
	would allow simplification of amitk_canvas event cruft
	also probably get rid of amitk_roi_variable_types.c... not worth the performance improvement
* add sqrt and log abilities into color table lookup?
* should have option to use rigid body registration with scaling
* try to work in the max/min/distribution calculations into the loading loop
* fix zooming of off angle roi's on the canvas
* roi analysis dialog should probably be updated for any change of the roi's
* add key combination that sets current pixel value to max/min threshold
* rendered movies aren't smooth when looping in an external player, this happened when switching to ffmpeg

                                                                                


small short range things
--------------
* try to change all dialog box buttons to verbs
* add tooltips to everything....
* add more help buttons to the dialogs 


longer range things
----------------
* add report generation.  This would add colormaps, location, thickness
	study name, volume name, scan date, etc. to the export function.
	Would handle multiple images.
* rendering alterations -> 
	3D red/green imaging
	allow rotation of light source and view in addition to object
	allow changing of material properties
	transformation to rendering structure is inefficient when doing multiple volumes,
		each volume should be transfered using it's minimal voxel size, not the
		group's maximum minimal voxel size.  the rendered images should then be scaled
		and overlapped.  
	MIP rendering would be nice....
	More likely, will switch to using VTK at some point
* aligning of data using automated registration?


libgnomecanvas/pixbuf issues
---------------------

These problems will probably never be rectified, as libgnomecanvas is
now deprecated. More likely course of action will be likely to rewrite
the parts that use libgnomecanvas to use a new canvas system.  A new
canvas system has not yet been agreed on for gtk (as of 3.0 at
least). Goocanvas would have seemed to be the obvious choice, as it's
similar to libgnomecanvas but with cairo as the backend rather than
libart, but development of this seems to have stalled. May need to go
directly to cairo as the canvas.


* antialiased canvases have been unstable in the past, due to
	instabilities with ROI resizing starts spewing "*** attempt to
	put segment in horiz list twice" errors and eventually core
	dumps.  Either a libart or libgnomecanvas bug.  At the current
	time (amide 1.0.3) things seem to be working, so the appropriate
	lines in configure.in have been reenabled to allow building in
	support for antialiased canvases. To disable antialiasing, disable
	these lines again.

	last tested versions:
		libart_lgpl-2.3.21
		libgnomecanvas-2.30.3

	-make sure to check that lines in tb_crop are placed correctly
	-need to check that the export jpeg functions work, they have not been tested


* anti-aliased canvases doesn't support GDK line styles (as of
         libgnomecanvas 2.2.0) -ui_preferences_dialog, and
         amitk_object_dialog define out the appropriate code when
         antialiasing support is compiled in.  Add back in once (if
         ever) anti-aliased canvases support line styles.

* currently, alpha blending is done inside of image.c, a single pixbuf is generated,
	and this is handed to gnome_canvas.  It'd be nice to use separate pixbuf's but 
	each data set, and throwing them on the canvas.  The single biggest problem with this is 
	that the background buffer in gnome_canvas is RGB, not RGBA, so the buffer has an effective
	alpha of 0xFF, and you're just blending the image with whatever that background is, 
	screwing up the color levels.  may have to rewrite libgnomecanvas to use
	a 32bit buffer
	-would allow the use of cursor keys for moving data sets around each other


* using the gdk_pixbuf_scale functions
	Slicing from all data sets at zoom=1.0, and then scaling using
	the gdk_pixbuf_scale type functions doesn't work well, as the
	scale functions don't do a great job of interpolation (circa
	gdk-pixbuf-0.18.0).


things dependent on other things: 
-------------- 

* dependent on gtk fixing the GTK_FILE_CHOOSER_CREATE_FOLDER properties of gtkfilechooser.
  Currently, the gtkfilechooser widget puts out an error if you try to select
  a folder that's currently a filename (rather than allowing overwriting). This comes
  up really only when trying to overwrite a .xif file as a .xif directory using
  the same name.

* dependent on gtk > 2.14:
  -gtk (as of 2.14) doesn't have any version of gnome_help_display. As
  such, the relevant code from libgnome has been copied into
  amide_gnome.c. When gtk has this capability, the code in
  amide_gnome.c can be deleted.

* not currently implemented in gtk (at least as of 2.12):
	handle_box's don't get minimized with the main window...
	but no way to minimize them independently...

* amide_gconf.c is used to encapsulate configuration support (gconf
       for unix, registry entries based on code stolen from gnumeric
       for win32, and g_key_file (flat file) for mac os x). GConf is
       likely to be deprecated at some point in the future. GSettings
       doesn't seem appropriate, as it absolutely requires a schema to
       be in place (as of glib 2.30) or the program aborts... If
       GSettings ever becomes more user friendly will switch the GConf
       support to GSettings, otherwise will go with the g_key_file in
       place of gconf.


* setlocale is disabled on win32, this is because further setlocale
  commands seem to be ignored, and so reading in text hdr files
  (e.g. Concorde format ) gets screwed up for locales that use the
  comma for the radix. This is as of mingw 3.13 at least.

* in xml.c and amitk_point.c, have a bunch of G_PLATFORM_WIN32
	ifdef's, because g_strdup_printf
	wasn't obeying the setlocale command on windows as of glib 2.2
	This was causing errors in locales that use a comma as the
	radix sign.  Need to test if g_strdup_printf will work with later
	versions of glib.


* ui_time_dialog specifies AmitkDataSet as G_TYPE_POINTER instead of
	AMITK_TYPE_OBJECT when setting up gtk_list_store - this is due
	to a gtk bug, adds a reference with G_TYPE_OBJECT but doesn't
	remove it
	- also in amitk_tree.c, ui_alignment_dialog.c

* at some point, GtkGammaCurve will probably be deprecated from Gtk,
	can either copy the code into amide, or come up with something
	more fitting

* when bug-buddy supports sourceforge bug reporting,
	delete the following line from amide.c so we can have bug reporting
	"signal(SIGSEGV, SIG_DFL);"

* remove no-ops in raw_data.c
	I've needed this (at least) for gcc <= 3.0.3.  Left in for the moment as
	I haven't tested gcc 3.1 for this bug, and other people may be using
	old versions of gcc.

* Current implementation is cooperative multitasking, if that.  Should turn
	program into a true, multi-threaded design.  Note that gtk for windows
	(as of 2003.04.30) does not handle multi-threaded program.... so will
	have to wait for that.

* amitk_data_sets_export_to_file is very wasteful of memory.  It reslices the requisite
	data sets into a new data set, and then an additional copy data set is made
	inside of libmdc.  If medcon/libmdc ever supports slice-by-slice saving
	of data sets (instead of as an entire volume at once), we would only need
	to allocate one slice in amide, and one slice in libmdc, rather than
	one volume in amide, and one volume in libmdc.
	


factor analysis things:
-----------------------       
* factor analysis -> should be able to do from ROI's too.
	should be simple.  1 iteration to find the # of voxels,
	second iteration to fill in 
	probable use weight >=0.5 as cutoff
* need to add orthogonality component back into PLS
* principle component analysis should probably be demeaned
* need to extend 2 comp to 3 comp (or n-comp)
* should be able to combine main function for fads and comp models



