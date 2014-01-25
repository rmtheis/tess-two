#!/bin/sh
#
#  This testing wrapper was written by James Le Cuirot.
#
#  It runs:
#      alltests_reg generate
#      alltests_reg compare
#  when the command 'make check' is invoked
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
exec ${@%${TEST}} /bin/sh -c "cd \"${srcdir}\" && \"${PWD}/\"${TEST} generate && \"${PWD}/\"${TEST} compare"
