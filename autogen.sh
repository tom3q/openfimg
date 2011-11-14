#!/bin/bash

#
# usage:
#
# banner <target name>
#
banner() {
	echo
	TG=`echo $1 | sed -e "s,/.*/,,g"`
	LINE=`echo $TG |sed -e "s/./-/g"`
	echo $LINE
	echo $TG
	echo $LINE
	echo
}

if [ ! -d "m4" ]; then
	mkdir m4
fi

banner "autoreconf"

autoreconf --force --install -Wall || exit $?

banner "Finished"
