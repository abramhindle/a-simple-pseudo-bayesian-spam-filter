all:
	rm -f .ham.db .spam.db || echo "lol"
	gcc -pg -lm -Wall filter.c -o filter
	./filter -init
	./filter -ham < filter.c
	./filter -spam < lookup3.c
	./filter -classify < filter.c || echo "SPAM"
	./filter -classify < lookup3.c || echo "SPAM"
	./filter -classify < Makefile || echo "SPAM"

run:
	rm -f .ham.db .spam.db || echo "lol"
	gcc -lm -O3 -Wall filter.c -o filter
	./filter -init
	cat test/read-nov-2005.SPLIT/new/* | ./filter -ham 
	cat test/SPAM.20071012.SPLIT/new/* | ./filter -spam
	echo HAM
	perl test.pl test/read-feb-2006.SPLIT/new/*
	echo SPAM
	perl test.pl test/SPAM.20071020.SPLIT/new/*
