CC ?= gcc
CFLAGS += -Wall -Wextra -pedantic -O2 -g

all: pavlisp

minilisp: pavlisp.c

clean:
	@rm -fv pavlisp

.PHONY: all clean
