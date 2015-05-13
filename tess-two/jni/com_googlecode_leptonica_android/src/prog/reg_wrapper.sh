#!/bin/sh
#
#  This testing wrapper was written by James Le Cuirot.
#
#  It runs all the programs in AUTO_REG_PROGS in Makefile.am
#  when the command 'make check' is invoked.  This same set can
#  be run by doing:
#      alltests_reg generate
#      alltests_reg compare
#
#  Ten of the tests require gnuplot.  These tests are skipped if
#  gnuplot is not available.
#
#  The wrapper receives several parameters in this form:
#      path/to/source/config/test-driver <TEST DRIVER ARGS> -- ./foo_reg
#
#  Shell trickery is used to strip off the final parameter and
#  transform the invocation into this.
#      path/to/source/config/test-driver <TEST DRIVER ARGS>
#      -- /bin/sh -c "cd \"path/to/source/prog\" &&
#      \"path/to/build/prog/\"./foo_reg generate &&
#      \"path/to/build/prog/\"./foo_reg compare"
#
#  This also allows testing when you build in a different directory
#  from the install directory, and the logs still get written to
#  the build directory.

eval TEST=\${${#}}

TEST_NAME="${TEST##*/}"
TEST_NAME="${TEST_NAME%_reg*}"

case "${TEST_NAME}" in
    colormask|colorspace|dna|enhance|fpix1|kernel|nearline|projection|rankbin|rankhisto)
        which gnuplot > /dev/null || which wgnuplot > /dev/null || exec ${@%${TEST}} /bin/sh -c "exit 77" ;;
esac

exec ${@%${TEST}} /bin/sh -c "cd \"${srcdir}\" && \"${PWD}/\"${TEST} generate && \"${PWD}/\"${TEST} compare"
