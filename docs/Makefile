# $Id: Makefile,v 1.4 2002-12-29 16:10:58 ahu Exp $ 

all: powermail.txt powermail.pdf html/index.html html.tar.gz


clean distclean:
	rm -rf *.dvi *.pdf *.tex *.toc *.aux *.txt *.ps *.bak *.tmp *~ *.log html

html/index.html: powermail.sgml
	db2html -o html $<

html.tar.gz: html/index.html
	tar czf html.tar.gz html/

%.txt: %.sgml
	docbook2txt $<

%.pdf: %.sgml
	docbook2pdf $<

publish:
	rsync --copy-links --delete -avrze ssh ./html powermail.txt powermail.pdf \
	spoon.powerdns.com:/opt/websites/downloads.powerdns.com/www/documentation/powermail