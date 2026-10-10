# Build the book: two LuaLaTeX passes, the second for the contents and the references.
#
#   make                   build zephyr-rtos.pdf, dated today
#   make VERSION=v1.2.3    the same, with the release's version on the title page
#   make png               redraw the lectures' copies of the figures from figures/
#   make clean             remove everything the build writes
#
# The book is typeset from the lectures' markdown, which is the source of truth; README.md says
# how to keep the two in step.

ROOT    := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
BOOK    := $(ROOT)/book
BUILD   := $(BOOK)/build
VERSION ?=
LATEX   := lualatex -interaction=nonstopmode -halt-on-error -file-line-error \
           -output-directory=$(BUILD)
# The book, with its version defined before anything is read; -jobname keeps the output book.pdf.
BOOKTEX := $(LATEX) -jobname=book '\def\bookversion{$(VERSION)}\input{book.tex}'

SOURCES := book.tex zephyrbook.sty zephyrbook.lua \
           $(wildcard front/*.tex chapters/*/*.tex figures/*.tex)

# The figures a lecture shows as a PNG: figures/<name>.tex -> lectures/<lecture>/appendix/images.
PNGS := build_pipeline:L01 priority_inversion:L04

.PHONY: all png clean FORCE

all: zephyr-rtos.pdf

# The version the last build was made for. The file is rewritten only when VERSION changes, so a
# new version, or none after one, rebuilds the title page and nothing else does.
$(BUILD)/version: FORCE
	@mkdir -p $(BUILD)
	@echo '$(VERSION)' | cmp -s - $@ || echo '$(VERSION)' > $@

zephyr-rtos.pdf: $(SOURCES) $(BUILD)/version
	@mkdir -p $(BUILD)
	cd $(BOOK) && $(BOOKTEX) >/dev/null || (tail -40 $(BUILD)/book.log; exit 1)
	cd $(BOOK) && $(BOOKTEX) >/dev/null || (tail -40 $(BUILD)/book.log; exit 1)
	@grep -E "^(Overfull|Underfull)|Warning" $(BUILD)/book.log | sort | uniq -c | sort -rn | head -20 || true
	@! grep -q "There were undefined references" $(BUILD)/book.log || \
	    (echo "error: undefined references; see $(BUILD)/book.log"; exit 1)
	cp $(BUILD)/book.pdf $@

# A lecture shows the same drawing as the book; this is the one command that makes it.
png:
	@mkdir -p $(BUILD)
	@for entry in $(PNGS); do \
	    name=$${entry%%:*}; lecture=$${entry##*:}; \
	    cd $(BOOK) && $(LATEX) -jobname=png \
	        "\\def\\pngfigure{$$name}\\input{figures/png.tex}" >/dev/null \
	        || (tail -40 $(BUILD)/png.log; exit 1); \
	    mkdir -p $(ROOT)/lectures/$$lecture/appendix/images; \
	    pdftoppm -r 200 -png -singlefile $(BUILD)/png.pdf \
	        $(ROOT)/lectures/$$lecture/appendix/images/$$name; \
	    echo "lectures/$$lecture/appendix/images/$$name.png"; \
	done

clean:
	rm -rf $(BUILD) zephyr-rtos.pdf
