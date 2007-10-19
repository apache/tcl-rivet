#!/bin/sh

# Be sure to use a recent autotools set.  1.9 works for me.

VERSION="1.10"

if [ "$VERSION" != "" ]
    then
    # Add the dash
    VERSION="-$VERSION"
fi

libtoolize -f -c && aclocal${VERSION} && autoheader && automake${VERSION} --add-missing && autoconf
