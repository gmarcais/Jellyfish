# AM_CPPFLAGS += -Werror

# Count lines of code
.PHONY: cloc
cloc:
	cloc --force-lang="Ruby,yaggo" --force-lang="make,am" --force-lang="make,mk" \
	  --exclude-dir="gtest" --ignored=cloc_ignored_src_files \
	  $(top_srcdir)/jellyfish $(top_srcdir)/include $(top_srcdir)/lib $(top_srcdir)/sub_commands $(top_srcdir)/tests $(top_srcdir)/unittests \
	  $(top_srcdir)/Makefile.am $(top_srcdir)/*.mk


# Make a dependency on yaggo the software
$(YAGGO_SOURCES): $(YAGGO)
