# Standalone fastnat ubus backend.
# CC / CFLAGS / LDFLAGS are supplied by the OpenWrt package Makefile.

OBJS := fastnatd.o fwx_fastnat.o fwx_uci.o fwx_common.o
LIBS := -lubox -lubus -lblobmsg_json -luci -ljson-c

CFLAGS += -I.

all: fastnatd

fastnatd: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

fastnatd.o: fastnatd.c fwx.h fwx_fastnat.h fwx_uci.h
fwx_fastnat.o: fwx_fastnat.c fwx.h fwx_fastnat.h fwx_uci.h
fwx_uci.o: fwx_uci.c fwx_uci.h
fwx_common.o: fwx_common.c fwx.h fwx_uci.h

clean:
	rm -f fastnatd $(OBJS)

.PHONY: all clean