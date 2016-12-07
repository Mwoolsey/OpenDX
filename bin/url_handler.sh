#!/bin/sh
#
# This script is an example of how to call the appropriate viewer based upon
# the URL method
#
# Michael Elkins <me@cs.hmc.edu>
#
# Created: March 10, 1997
# Last Edited: April 10, 2002
#
# Souped up 4/9/97 by Randall Hopper and Edited 4/10/02 by David Thompson
#

if [ -z "$1" ]; then
   echo No URL specified
   exit 1
fi

url="$1"
method=`echo "$url" | sed 's;^\([^:]*\):.*;\1;'`

case $method in
	ftp)	target=`echo "$url" | sed 's;^.*://\([^/]*\)/*\(.*\);\1:\2;'`
		ncftp -L -r -d 0 $target
		;;

	mailto)	mutt `echo "$url" | sed 's;^[^:]*:\(.*\);\1;'`
		;;

        *)      use_xbrowser=n
		case "$DISPLAY" in
                   :0)       use_xbrowser=y
                             ;;
                   :0.*)     use_xbrowser=y
                             ;;
                   unix:0.*) use_xbrowser=y
                             ;;
                esac
		if [ $use_xbrowser = n ]; then
			lynx "$url" 
		else
			netscape -remote "openURL($url)" 2> /dev/null || \
			(netscape "$url" &)
                fi
		;;
esac

