# Makefile for nswrapper
# Copyright (C) 2014-2017 Tagged Inc., All Rights Reserved
# Author: Misha Nasledov <misha@nasledov.com>

CC := gcc
LIB := libnswrapper.so
SRCS := nswrapper.c
OBJS := $(SRCS:.c=.o)
CFLAGS := -fPIC -Wall
LIBS := -ldl
LDFLAGS := $(LIBS) -fPIC -shared
VERSION := 0.1

all: $(LIB)

$(LIB): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

install: all
	cp $(LIB) /usr/local/lib/

rpm: all
	rm -rf rpmbuild
	mkdir -p rpmbuild/usr/local/lib
	cp $(LIB) rpmbuild/usr/local/lib/
	fpm -n nswrapper -v ${VERSION} --iteration 1 -s dir -t rpm -C rpmbuild .
