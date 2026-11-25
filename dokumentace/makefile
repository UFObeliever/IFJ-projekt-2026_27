# Překlad dokumentace:    make          -> dokumentace.pdf
# Přegenerování tabulek:  make tabulky  (gen_tabulky.py)
all: dokumentace.pdf

TABLES = gramatika.tex ll_tabulka.tex first_follow.tex psa_gramatika.tex \
         prec_tabulka.tex prec_priority.tex

$(TABLES): gen_tabulky.py
	python3 gen_tabulky.py

tabulky: $(TABLES)

dokumentace.pdf: dokumentace.tex $(TABLES)
	latexmk -pdf -interaction=nonstopmode dokumentace.tex

clean:
	latexmk -C
