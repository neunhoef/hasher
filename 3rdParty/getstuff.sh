#!/bin/sh
if test ! -d velocypack ; then
    git clone https://github.com/arangodb/velocypack
fi
if test ! -d xxhash ; then
    git clone https://github.com/stbrumme/xxhash
fi
