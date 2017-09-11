#! /bin/bash
set -exo nounset

if [ $# -ne 0 ] ; then
	git pull origin master
	git submodule foreach git fetch
	git submodule foreach git checkout master
	git submodule foreach git merge origin/master
	if [ -f Makefile ] ; then
		make distclean
	fi
fi

autoreconf --install

if [ $# -eq 0 ] ; then exit 0
else
	test "x$1" == xmake
	DIR="$(readlink -f `pwd`)"
	rm -rf ../C-Thread-Pool-build
	mkdir ../C-Thread-Pool-build
	cd ../C-Thread-Pool-build
	"$DIR/configure" "--prefix=$HOME"
	make
	make install
fi
