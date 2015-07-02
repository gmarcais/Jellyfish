AM_CXXFLAGS += -Werror

# Count lines of code
.PHONY: cloc cloc_jellyfish
cloc:
	cloc --force-lang="Ruby,yaggo" --force-lang="make,am" --force-lang="make,mk" \
	  --exclude-dir="gtest" --ignored=cloc_ignored_src_files \
	  $(srcdir)/jellyfish $(srcdir)/include $(srcdir)/lib $(srcdir)/sub_commands $(srcdir)/tests $(srcdir)/unit_tests \
	  $(srcdir)/Makefile.am $(srcdir)/*.mk

cloc_jellyfish:
	cloc $(srcdir)/jellyfish $(srcdir)/include $(srcdir)/lib $(srcdir)/sub_commands

cloc_library:
	cloc $(srcdir)/include $(srcdir)/lib

# Make a dependency on yaggo the software
$(YAGGO_SOURCES): $(YAGGO)

# Launch unit tests
unittests:
	@$(MAKE) check TESTS=unit_tests/unit_tests.sh
