L2M = latex2man -t transfile
MI = makeinfo
PDFLATEX = pdflatex
M4 = m4

TARGETS = jellyfish.man jellyfish.html jellyfish.pdf
all: $(TARGETS)

man: jellyfish.man
	man ./$<

options.tex: generate_options_tex.sh
	sh $< > $@

%-full.tex: %.tex options.tex
	$(M4) $< > $@

%.man: %-full.tex
	$(L2M) $< $@

%.html: %-full.tex
	$(L2M) -H $< $@

%.texi: %-full.tex
	$(L2M) -T $< $@

%.info: %.texi
	$(MI) --force $< $@

%.latex: %-full.tex
	$(L2M) -L $< $@

%.pdf: %.latex
	$(PDFLATEX) $<

clean:
	rm -f $(TARGETS) *-full.tex options.tex
