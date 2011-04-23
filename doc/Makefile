L2M = latex2man -t transfile
MI = makeinfo
PDFLATEX = pdflatex
M4 = m4

all: jellyfish.man jellyfish.html jellyfish.pdf

man: jellyfish.man
	man ./$<

%.man: %.tex options.tex
	$(M4) $< | $(L2M) /dev/fd/0 $@

%.html: %.tex options.tex
	$(L2M) -H $< $@

%.texi: %.tex options.tex
	$(L2M) -T $< $@

%.info: %.texi
	$(MI) --force $< $@

%.latex: %.tex options.tex
	$(L2M) -L $< $@

%.pdf: %.latex
	$(PDFLATEX) $<
