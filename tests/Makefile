CC = cc
CFLAGS = -Wall -Wextra -DHOST_BUILD -I../src
SRCDIR = ../src

.PHONY: test clean

test: test_printers test_json
	@echo "--- Running printer model tests ---"
	@./test_printers
	@echo ""
	@echo "--- Running JSON parser tests ---"
	@./test_json

test_printers: test_printers.c $(SRCDIR)/printers.c $(SRCDIR)/printers.h
	$(CC) $(CFLAGS) -o $@ test_printers.c $(SRCDIR)/printers.c

test_json: test_json.c $(SRCDIR)/json.c $(SRCDIR)/json.h $(SRCDIR)/printers.c $(SRCDIR)/printers.h $(SRCDIR)/jsmn.h
	$(CC) $(CFLAGS) -o $@ test_json.c $(SRCDIR)/json.c $(SRCDIR)/printers.c

clean:
	rm -f test_printers test_json
