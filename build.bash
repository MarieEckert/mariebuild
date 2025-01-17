#!/bin/bash

# mariebuild build script.
# Use this only if you have no version of mariebuild which can build itself.

CC="clang"
SRCDIR="src/"
OBJDIR="obj/"

BASE_CFLAGS="-std=c17 -pedantic-errors -Wall -Wextra -Werror -Wno-gnu-statement-expression -Iinclude/ -Isrc/"
DEBUG_CFLAGS="-ggdb -DDEFAULT_LOG_LEVEL=LOG_DEBUG"
RELEASE_CFLAGS="-Oz"
LDFLAGS="-lm -Llib/ -lmcfg_2"

BIN_NAME="mb"

function build_objs() {
	COMPILED_OBJECTS=()

	unameOut="$(uname -s)"
	case "${unameOut}" in
		Darwin*)
			if [ -d /opt/homebrew/include/ ]; then
				EXTRA_CFLAGS="-I/opt/homebrew/include"
			else
				EXTRA_CFLAGS="-I/usr/local/include"
			fi
	esac

	for i in $1
	do
		OUTNAME="$OBJDIR$i.o"
		INNAME="$SRCDIR$i.c"

		echo "	CC $INNAME"

		$CC $CFLAGS $EXTRA_CFLAGS -c -o $OUTNAME $INNAME || exit

		COMPILED_OBJECTS+=("${OUTNAME}")
	done
}

function build() {
	OBJECTS=("strlist logging types executor c_rule target build main")

	echo "==> Compiling Sources for \"$BIN_NAME\""
	build_objs "${OBJECTS[@]}"

	unameOut="$(uname -s)"
	case "${unameOut}" in
		Darwin*)		LDFLAGS="$LDFLAGS -L/usr/local/lib -largp";;
	esac

	unameOut="$(uname -s)"
	case "${unameOut}" in
		Darwin*)
			if [ -d /opt/homebrew/lib/ ]; then
				EXTRA_LDFLAGS="-L/opt/homebrew/lib -largp"
			else
				EXTRA_LDFLAGS="-L/usr/local/lib -largp"
			fi
			printf "	-> Linking for Darwin, EXTRA_LDFLAGS=$EXTRA_LDFLAGS\\n";;
	esac
	echo "==> Linking \"$BIN_NAME\""
	echo "	LD -o $BIN_NAME ${COMPILED_OBJECTS[@]} $LDFLAGS $EXTRA_LDFLAGS"

	$CC $CFLAGS -o $BIN_NAME ${COMPILED_OBJECTS[@]} $LDFLAGS $EXTRA_LDFLAGS

	if [ $RELEASE ]; then
		echo "	STRIP $BIN_NAME"

		STRIPFLAGS="--strip-all"
		unameOut="\$(uname -s)"
		case "${unameOut}" in
		  Darwin*)    STRIPFLAGS="-S -X";;
		esac

		strip $STRIPFLAGS ./mb
	fi
}

function setup() {
	if [ ! -d "$OBJDIR" ]; then
		mkdir $OBJDIR
	fi

	if [ ! -f "lib/libmcfg_2.a" ]; then
		echo "==> libmcfg_2 is missing, running setup.bash"

		# set IFS to an emtpy string so that leading spaces are kept around
		SAVE_IFS=$IFS
		IFS=''

		(
			bash setup.bash 2>&1 |
				while read -r line; do
					echo "setup.bash: $line";
				done
		) || exit

		IFS=$SAVE_IFS
		echo "==> successfully built libmcfg_2"
	fi
}

echo "MB build script. "

if [ "$1" = "--compile-flags" ]; then
	sed --posix 's/ /\n/g' <<<"${CFLAGS[@]}" > compile_flags.txt

	echo "==> Generated compile_flags.txt"
	exit
fi

if [ "$1" = "--release" ]; then
	echo "==> Building in release mode"
	CFLAGS="$BASE_CFLAGS $RELEASE_CFLAGS"
	RELEASE=1
else
	CFLAGS="$BASE_CFLAGS $DEBUG_CFLAGS"
fi

setup
build
echo "==> Finished build!"
