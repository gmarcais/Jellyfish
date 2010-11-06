L2M = latex2man -t transfile
MI = makeinfo
PDFLATEX = pdflatex

all: jellyfish.man jellyfish.html jellyfish.pdf

man: jellyfish.man
	man ./$<

%.man: %.tex
	$(L2M) $< $@

%.html: %.tex
	$(L2M) -H $< $@

%.texi: %.tex
	$(L2M) -T $< $@

%.info: %.texi
	$(MI) --force $< $@

%.latex: %.tex
	$(L2M) -L $< $@

%.pdf: %.latex
	$(PDFLATEX) $<
