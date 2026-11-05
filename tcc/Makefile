# Usage (from tcc/): make
FONTE  := trab_final_metpesq.md
SAIDA  := trab_final_metpesq_abnt
PANDOC := pandoc $(FONTE) -f abnt/leitor.lua \
	--metadata-file=abnt/metadados.yaml \
	--template=abnt/abntex2.latex \
	--citeproc --top-level-division=chapter --syntax-highlighting=none \
	--pdf-engine=pdflatex

$(SAIDA).pdf: $(FONTE) $(wildcard abnt/*.*) $(wildcard images/*.png tests/plots/*.png)
	$(PANDOC) -o $@

tex:
	$(PANDOC) -s -o $(SAIDA).tex

.PHONY: tex
