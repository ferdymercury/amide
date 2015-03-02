#ifndef __AMIDE_INTL_H__
#define __AMIDE_INTL_H__

#include "amide_config.h"


#ifdef ENABLE_NLS
#include<libintl.h>
#undef _
#define _(String) dgettext(GETTEXT_PACKAGE,String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#undef _
#define _(String) (String)
#undef N_
#define N_(String) (String)
#undef textdomain
#define textdomain(String) (String)
#undef gettext
#define gettext(String) (String)
#undef dgettext
#define dgettext(Domain,String) (String)
#undef dcgettext
#define dcgettext(Domain,String,Type) (String)
#undef bindtextdomain
#define bindtextdomain(Domain,Directory) (Domain) 
#endif

#endif
