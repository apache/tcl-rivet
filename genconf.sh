#!/bin/sh

# Be sure to use a recent autotools set.  1.8 works for me.
aclocal
autoconf
automake --add-missing
