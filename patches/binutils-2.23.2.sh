#!/bin/bash
sed -i -e 's/@colophon/@@colophon/'\
       -e 's/doc@cygnus.com/doc@@cygnus.com/' $1/bfd/doc/bfd.texinfo
