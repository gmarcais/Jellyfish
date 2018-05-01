##############################
# Gtest build.
##############################
# Build rules for libraries.
# check_LTLIBRARIES = libgtest.la libgtest_main.la
check_LIBRARIES = libgtest.a libgtest_main.a

libgtest_a_SOURCES = unit_tests/gtest/src/gtest.cc		\
                     unit_tests/gtest/src/gtest-internal-inl.h	\
                     unit_tests/gtest/src/gtest-printers.cc	\
                     unit_tests/gtest/src/gtest-death-test.cc	\
                     unit_tests/gtest/src/gtest-test-part.cc	\
                     unit_tests/gtest/src/gtest-filepath.cc	\
                     unit_tests/gtest/src/gtest-port.cc		\
                     unit_tests/gtest/src/gtest-typed-test.cc
libgtest_a_SOURCES += unit_tests/gtest/include/gtest/gtest.h					\
                      unit_tests/gtest/include/gtest/gtest-test-part.h				\
                      unit_tests/gtest/include/gtest/gtest_prod.h				\
                      unit_tests/gtest/include/gtest/gtest-param-test.h				\
                      unit_tests/gtest/include/gtest/internal/gtest-param-util.h		\
                      unit_tests/gtest/include/gtest/internal/gtest-linked_ptr.h		\
                      unit_tests/gtest/include/gtest/internal/gtest-internal.h			\
                      unit_tests/gtest/include/gtest/internal/gtest-filepath.h			\
                      unit_tests/gtest/include/gtest/internal/gtest-port.h			\
                      unit_tests/gtest/include/gtest/internal/gtest-type-util.h			\
                      unit_tests/gtest/include/gtest/internal/custom/gtest.h			\
                      unit_tests/gtest/include/gtest/internal/custom/gtest-port.h		\
                      unit_tests/gtest/include/gtest/internal/custom/gtest-printers.h		\
                      unit_tests/gtest/include/gtest/internal/gtest-port-arch.h			\
                      unit_tests/gtest/include/gtest/internal/gtest-string.h			\
                      unit_tests/gtest/include/gtest/internal/gtest-tuple.h			\
                      unit_tests/gtest/include/gtest/internal/gtest-death-test-internal.h	\
                      unit_tests/gtest/include/gtest/internal/gtest-param-util-generated.h	\
                      unit_tests/gtest/include/gtest/gtest-death-test.h				\
                      unit_tests/gtest/include/gtest/gtest-spi.h				\
                      unit_tests/gtest/include/gtest/gtest-typed-test.h				\
                      unit_tests/gtest/include/gtest/gtest-message.h				\
                      unit_tests/gtest/include/gtest/gtest-printers.h				\
                      unit_tests/gtest/include/gtest/gtest_pred_impl.h

libgtest_main_a_SOURCES = unit_tests/gtest/src/gtest_main.cc
libgtest_a_CXXFLAGS = -I$(srcdir)/unit_tests/gtest/include	\
                      -I$(srcdir)/unit_tests/gtest

libgtest_main_a_LIBADD = libgtest.a
libgtest_main_a_CXXFLAGS = -I$(srcdir)/unit_tests/gtest/include

EXTRA_DIST += unit_tests/gtest/COPYING
