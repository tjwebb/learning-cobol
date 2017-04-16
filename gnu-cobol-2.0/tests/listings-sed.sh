#! /bin/sh
# Necessary sed replacements for unifying a listing

# Note: We cater for a maximum version string of 14:
#       Mayor (2) '.' Minor (2) '.' Patchlevel (8 - as some people place a date here)
# Note: We replace the date two times, as not all systems have %e modifier in C
#       and use %E in this case ("Mon Feb 04" instead of "Mon Feb  4").


sed \
-e 's/GnuCOBOL [0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*  */GnuCOBOL V.R.P          /g' \
-e 's/[0-2][0-9]:[0-6][0-9]:[0-9][0-9] [0-9][0-9][0-9][0-9]/HH:MM:SS YYYY/g' \
-e 's/'"$(date +"%a %b %e")"'/DDD MMM dd/g' \
-e 's/'"$(date +"%a %b %d")"'/DDD MMM dd/g' \
<"$1" >"$2"
