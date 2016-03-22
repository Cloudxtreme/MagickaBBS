#!/bin/sh

libtoolize --copy --force && aclocal && autoheader && automake --copy --add-missing --force-missing && autoconf --force
