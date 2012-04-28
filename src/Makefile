
# wl-520gu attempt with trunk, hit same wifi bug as with precompiled 10.03.1
#TOPDIR=/home/garlick/work/openwrt/openwrt/trunk
#export STAGING_DIR=$(TOPDIR)/staging_dir
#TCDIR=$(STAGING_DIR)/toolchain-mipsel_gcc-4.6-linaro_uClibc-0.9.33
#TRX=$(TOPDIR)/bin/brcm47xx/openwrt-brcm47xx-squashfs.trx
#export PATH := $(TCDIR)/bin:$(PATH)

# build 10.03.1 tag with target sys BCM947XX/953XX [2.4]
# target profile ASUS WL-500g
TOPDIR=/home/garlick/work/openwrt/openwrt/backfire_10.03.1
export STAGING_DIR=$(TOPDIR)/staging_dir
TCDIR=$(STAGING_DIR)/toolchain-mipsel_gcc-3.4.6_uClibc-0.9.30.1
TRX=$(TOPDIR)/bin/brcm-2.4/openwrt-brcm-2.4-squashfs.trx
export PATH := $(TCDIR)/usr/bin:$(PATH)

ifneq ($(TOPDIR),)
CC = mipsel-openwrt-linux-gcc
LD = mipsel-openwrt-linux-ld
CFLAGS = -I$(TCDIR)/include
LDFLAGS = -L$(TCDIR)/lib
endif

CFLAGS += -Wall

all: netscope

openwrt: $(TRX)

$(TRX): netscope
	mkdir -m 0755 -p $(TOPDIR)/files/bin
	cp netscope $(TOPDIR)/files/bin
	chmod 755 $(TOPDIR)/files/bin/netscope
	make -C $(TOPDIR)

# hold down reset while powering up
# wl-520gu will be 192.168.1.1 awaiting tftp of boot image
upload: $(TRX)
	atftp --trace --option "timeout 1" --option "mode octet" --put \
		--local-file $(TRX) 192.168.1.1

clean:
	rm -f a.out core *.o netscope
