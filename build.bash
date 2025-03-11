#!/bin/bash

# mariebuild build script.
# Use this only if you have no version of mariebuild which can build itself.

SRCDIR="src/"
BUILD_ROOT="build/"
DEBUG_DIR="$BUILD_ROOT""debug/"
RELEASE_DIR="$BUILD_ROOT""release/"
OBJ_DIR="obj/"

CC="clang"
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
		OUTNAME="$OBJ$i.o"
		INNAME="$SRCDIR$i.c"

		if ! [ -d $(dirname "$OUTNAME") ]; then
			echo "	MKDIR -p $(dirname $OUTNAME)"
			mkdir -p "$(dirname $OUTNAME)"
		fi

		echo "	CC $INNAME"

		$CC $CFLAGS $EXTRA_CFLAGS -c -o $OUTNAME $INNAME || exit

		COMPILED_OBJECTS+=("${OUTNAME}")
	done
}

function build() {
	OBJECTS=("stringutil cptrlist signals logging types executor c_rule target build main")

	echo "==> Compiling Sources for \"$BIN_DEST\""
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
	echo "==> Linking \"$BIN_DEST\""
	echo "	LD -o $BIN_DEST ${COMPILED_OBJECTS[@]} $LDFLAGS $EXTRA_LDFLAGS"

	$CC $CFLAGS -o $BIN_DEST ${COMPILED_OBJECTS[@]} $LDFLAGS $EXTRA_LDFLAGS

	if [ $RELEASE ]; then
		echo "	STRIP $BIN_DEST"

		STRIPFLAGS="--strip-all"
		unameOut="\$(uname -s)"
		case "${unameOut}" in
		  Darwin*)    STRIPFLAGS="-S -X";;
		esac

		strip $STRIPFLAGS $BIN_DEST
	fi
}

function setup() {
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

if [ "$1" = "--release" ]; then
	echo "==> Building in release mode"
	CFLAGS="$BASE_CFLAGS $RELEASE_CFLAGS"
	OBJ="$RELEASE_DIR""$OBJ_DIR"
	BIN_DEST="$RELEASE_DIR""$BIN_NAME"
	RELEASE=1
else
	CFLAGS="$BASE_CFLAGS $DEBUG_CFLAGS"
	OBJ="$DEBUG_DIR""$OBJ_DIR"
	BIN_DEST="$DEBUG_DIR""$BIN_NAME"
fi

if [ "$1" = "--compile-flags" ]; then
	sed --posix 's/ /\n/g' <<<"${CFLAGS[@]}" > compile_flags.txt

	echo "==> Generated compile_flags.txt"
	exit
fi

if [ "$1" = "--manpage" ]; then
	COMMAND="asciidoctor -b manpage doc/mariebuild.adoc"
	echo "	$COMMAND"
	$COMMAND
	exit $?
fi

setup
build
echo "==> Finished build!"
