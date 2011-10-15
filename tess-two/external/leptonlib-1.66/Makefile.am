AUTOMAKE_OPTIONS=foreign no-dependencies
SUBDIRS=src #include

# We can't use WORDS_BIGENDIAN from config_auto.h because this cannot be
# included in arrayaccess.h but it must be included in many files that
# require arrayaccess.h.  Therefore, we must use our own methods to
# get the L_LITTLE_ENDIAN or L_BIG_ENDIAN defines into the complation.
MAKE = $(CC) -o endiantest endiantest.c; export ENDIANNESS=-D`./endiantest`; make -f Makefile
AM_CFLAGS = -D_BSD_SOURCE -DANSI -fPIC

EXTRA_DIST = config README.html leptonica-license.txt moller52.jpg \
      version-notes.html

dist-hook:
# Also remove extra files not needed in a distribution
	rm -rf `find $(distdir) -name configure.ac`
	rm -rf `find $(distdir) -name acinclude.m4`
	rm -rf `find $(distdir) -name aclocal.m4`
