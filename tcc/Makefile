# Usage (from tcc/): make
FONTE  := trab_final_metpesq.md
SAIDA  := trab_final_metpesq_abnt
PANDOC := pandoc $(FONTE) -f abnt/leitor.lua \
	--metadata-file=abnt/metadados.yaml \
	--template=abnt/abntex2.latex \
	--citeproc --top-level-division=chapter --syntax-highlighting=none \
	--pdf-engine=pdflatex

$(SAIDA).pdf: $(FONTE) $(wildcard abnt/*.*) abnt/img/vtable.png
	$(PANDOC) -o $@

# images/vtable.png is a WebP with a .png name; pdfTeX needs a real PNG.
abnt/img/vtable.png: images/vtable.png
	mkdir -p $(@D)
	magick webp:$< png:$@

tex: abnt/img/vtable.png
	$(PANDOC) -s -o $(SAIDA).tex

.PHONY: tex
