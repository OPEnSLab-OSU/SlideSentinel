#Building Tex files.
LATEXINSTALL=/usr/local/apps/tex_live/current/bin/x86_64-linux/
# Latex stuff
LATEX	= $(LATEXINSTALL)latex -shell-escape
BIBTEX	= $(LATEXINSTALL)bibtex
DVIPS	= $(LATEXINSTALL)dvips
DVIPDF  = $(LATEXINSTALL)dvipdft
XDVI	= $(LATEXINSTALL)xdvi -gamma 4
GH	= $(LATEXINSTALL)gv

EXAMPLES = $(wildcard *.h)
SRC	:= $(shell egrep -l '^[^%]*\\begin\{document\}' *.tex)
TRG	= $(SRC:%.tex=%.dvi)
PSF	= $(SRC:%.tex=%.ps)
PDF	= $(SRC:%.tex=%.pdf)

CC:=gcc
CFLAGS:= -Wall -pthread

#wont build all without concurrency1 under pdf tag.
pdf: $(PDF)
#	$(CC) concurrency1.c $(CFLAGS) -o concurrency1

#concurrency1: concurrency1.c mt19937ar.h
#	$(CC) concurrency1.c $(CFLAGS) -o concurrency1

ps: $(PSF)

$(TRG): %.dvi: %.tex *.bib $(EXAMPLES)
#$(TRG): %.dvi: %.tex $(EXAMPLES)
	$(LATEX) $<
	$(BIBTEX) $(<:%.tex=%)
	$(LATEX) $<
	$(LATEX) $<

$(PSF):%.ps: %.dvi
	$(DVIPS) -R -Poutline -t letter $< -o $@

$(PDF): %.pdf: %.ps
#	$(DVIPDF) -o $@ $<
	ps2pdf $<

show: $(TRG)
	@for i in $(TRG) ; do $(XDVI) $$i & done

showps: $(PSF)
	@for i in $(PSF) ; do $(GH) $$i & done

all: pdf

clean:
	rm -f *.pdf *.ps *.dvi *.out *.log *.aux *.bbl *.blg *.pyg
#	rm concurrency1

.PHONY: all show clean ps pdf showps
