#!/bin/sh

while [ "$1" != "" ]; do
    PARAM=$1
    case $PARAM in
        -C)
           make -f Makefile.ca clean
           exit 0
           ;;
        -R)
           make -f Makefile.ca clean
           ;;
        -X)
           make -f Makefile.ca clean
           ;;
        *)
           echo "usage: build_project.sh [-C | -R | -X]"
           exit 1
           ;;
    esac
    shift
done

make -f Makefile.ca
