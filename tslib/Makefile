SHELL = /bin/sh

CC = gcc
#CFLAGS = -std=c99 -O0 -g -Wall -Wno-unused-variable
CFLAGS = -std=c99 -O3 -ffast-math -g -pedantic -pipe -Wall -Wextra

LD = gcc
LDFLAGS += -g -static

AR = ar
ARFLAGS = rcls

RANLIB = ranlib
RM = rm -f

SRCS = $(wildcard *.c) 
OBJS = $(SRCS:%.c=%.o)

INCLUDES = -I . -I../common -I../libstructures/ -I../h264bitstream/ -I../logging/
LIBS = -L . -ltslib -L../h264bitstream/.libs -lh264bitstream -L../logging/ -llogging   -L../libstructures/ -ldatastruct -lm

CFLAGS  += $(INCLUDES)
LDFLAGS += $(LIBS)

all: libtslib.a

libtslib.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^ 
	$(RANLIB) $@

clean:
	rm -f $(OBJS) *.a core
