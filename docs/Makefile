# $Id: Makefile,v 1.1.1.1 2002-12-04 13:46:46 ahu Exp $ 

all: powermail.txt powermail.pdf html/index.html html.tar.gz


clean:
	rm -rf *.dvi *.pdf *.tex *.toc *.aux *.txt *.ps *.bak *.tmp *~ *.log html

html/index.html: powermail.db
	db2html  -V %use-id-as-filename%  -o html powermail.db

html.tar.gz: html/index.html
	tar czf html.tar.gz html/

%.txt: %.db
	docbook2txt $<

%.pdf: %.db
	docbook2pdf $<

publish:
	rsync --copy-links --delete -avrze ssh ./html powermail.txt powermail.pdf \
	spoon.powerdns.com:/opt/websites/downloads.powerdns.com/www/documentation/powermail