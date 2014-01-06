AM_CXXFLAGS += -Werror

# Count lines of code
.PHONY: cloc cloc_jellyfish
cloc:
	cloc --force-lang="Ruby,yaggo" --force-lang="make,am" --force-lang="make,mk" \
	  --exclude-dir="gtest" --ignored=cloc_ignored_src_files \
	  $(top_srcdir)/jellyfish $(top_srcdir)/include $(top_srcdir)/lib $(top_srcdir)/sub_commands $(top_srcdir)/tests $(top_srcdir)/unit_tests \
	  $(top_srcdir)/Makefile.am $(top_srcdir)/*.mk

cloc_jellyfish:
	cloc $(top_srcdir)/jellyfish $(top_srcdir)/include $(top_srcdir)/lib $(top_srcdir)/sub_commands

cloc_library:
	cloc $(top_srcdir)/include $(top_srcdir)/lib

# Make a dependency on yaggo the software
$(YAGGO_SOURCES): $(YAGGO)

# Print the value of a variable
print-%:
	@echo -n $($*)

# Launch unit tests
unittests:
	@$(MAKE) check TESTS=unit_tests/unit_tests.sh
