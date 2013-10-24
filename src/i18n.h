/* i18n stuff */

#undef _
#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(string) gettext(string)
#else
#  define _(string) (string)
#endif
#define N_(string) (string)
