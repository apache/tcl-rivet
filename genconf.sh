#!/bin/sh

# Be sure to use a recent autotools set.  1.8 works for me.

VERSION="1.8"

if [ "$VERSION" != "" ]
    then
    # Add the dash
    VERSION="-$VERSION"
fi

aclocal${VERSION}
autoconf
automake${VERSION} --add-missing
