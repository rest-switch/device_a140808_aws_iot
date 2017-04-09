#
# Copyright 2015-2017 The REST Switch Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, Licensor provides the Work (and each Contributor provides its 
# Contributions) on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, 
# without limitation, any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A PARTICULAR 
# PURPOSE. You are solely responsible for determining the appropriateness of using or redistributing the Work and assume any 
# risks associated with Your exercise of permissions under this License.
#
# Author: John Clark (johnc@restswitch.com)
#

OUTBIN           = bin
TARGET           = $(OUTBIN)/lede-17.01-ramips-rt305x-hlk-rm04-squashfs-sysupgrade.bin
LEDE_ROOT        = source-lede-17.01
LEDE_COMMIT      = $(shell wget --header="accept:application/vnd.github.VERSION.sha" -qO- https://api.github.com/repos/lede-project/source/commits/lede-17.01 | cut -c1-7)
LEDE_SRC_URL     = https://github.com/lede-project/source/archive/lede-17.01.tar.gz
LEDE_FEEDS       = $(LEDE_ROOT)/feeds
LEDE_CFG         = $(LEDE_ROOT)/.config
LEDE_TGT         = $(LEDE_ROOT)/bin/targets/ramips/rt305x/lede-17.01-$(shell cat "$(LEDE_ROOT)/version")-ramips-rt305x-hlk-rm04-squashfs-sysupgrade.bin
PATCHES          = $(sort $(wildcard patches/*.patch))
CFG_DIFF         = patches/diff_config
UBOOT            = firmware/hlk-rm04_uboot-50000.bin
MAC              = $(shell echo $(mac) | sed 's/[- :]//g')
MAC2BIN          = python -c'import sys;sys.stdout.write(bytearray.fromhex(sys.argv[1]))'
MAC1A_ADDR       = $(shell echo -n $$((0x40004)))
MAC1B_ADDR       = $(shell echo -n $$((0x40028)))
MAC2_ADDR        = $(shell echo -n $$((0x4002e)))
SERIAL_ADDR      = $(shell echo -n $$((0x40400)))
SERIAL_GEN       = tools/scripts/write_bin.py -g | cut -sf2 -d:
SSH_TOOL         = tools/scripts/ssh_access.sh
FLASH_TOOL       = tools/scripts/program_flash.py
MINIPRO          = $(OUTBIN)/minipro
MINIPRO_SRC_URL  = https://github.com/inindev/minipro/archive/master.tar.gz

.DEFAULT all: target

target: | config certs ssh_check
	$(MAKE) -C "$(LEDE_ROOT)" SOURCE_DATE_EPOCH=$$(date +%s)
	@if [ ! -d "$(OUTBIN)" ]; then mkdir "$(OUTBIN)"; fi
	@cp "$(LEDE_TGT)" "$(TARGET)"

image: | $(TARGET)
    ifeq ("","$(MAC)")
	@echo "********************************************************************************"
	@echo " error: Image target requires a MAC address to be specified: mac=aabbccddeeff"
	@echo "********************************************************************************"
	@exit 10
    endif

    ifneq (0,$(shell echo "$(MAC)" | grep -iq '^[0-9a-f]\{12\}$$'; echo $$?))
	@echo "********************************************************************************"
	@echo " error: The MAC address "$(MAC)" is invalid."
	@echo "        A valid MAC address must be 12 hex chars: mac=aabbccddeeff"
	@echo "********************************************************************************"
	@exit 12
    endif

	$(eval MAC2 := $(shell printf "%x" $$((0x$(MAC) + 1))))
	$(eval DEVID := $(shell $(SERIAL_GEN)))
	$(eval IMGFILE := a140808_$(DEVID).bin)
	@echo
	@echo "  building image file..."
	@echo "    Image File: $(IMGFILE)"
	@echo "     Device ID: $(DEVID)"
	@echo "         MAC 1: $(MAC)"
	@echo "         MAC 2: $(MAC2)"
	@echo
	@echo

	cp "$(UBOOT)" "$(OUTBIN)/$(IMGFILE)"
	# mac addresses
	$(MAC2BIN) "$(MAC)"  | dd bs=1 of="$(OUTBIN)/$(IMGFILE)" count=6 seek=$(MAC1A_ADDR) conv=notrunc
	$(MAC2BIN) "$(MAC)"  | dd bs=1 of="$(OUTBIN)/$(IMGFILE)" count=6 seek=$(MAC1B_ADDR) conv=notrunc
	$(MAC2BIN) "$(MAC2)" | dd bs=1 of="$(OUTBIN)/$(IMGFILE)" count=6 seek=$(MAC2_ADDR) conv=notrunc
	# serial number
	@echo -n $(DEVID) | dd bs=1 of="$(OUTBIN)/$(IMGFILE)" count=9 seek=$(SERIAL_ADDR) conv=notrunc
	cat "$(TARGET)" >> "$(OUTBIN)/$(IMGFILE)"
	# padding
	@tr '\000' '\377' < /dev/zero | dd bs=1 count=$$((0x400000 - $$(stat -c '%s' "$(OUTBIN)/$(IMGFILE)"))) >> "$(OUTBIN)/$(IMGFILE)"

program: minipro
	@python "$(FLASH_TOOL)" -p "$(strip $(flash))" -f "$(strip $(image))" 2>/dev/null || ( rc=$$?; \
	if [ "$$rc" -eq "2" ] ; then \
	    echo "********************************************************************************"; \
	    echo " error: Detected a flash memory other than MX25L3206E, PM25LQ032C, or W25Q32BV "; \
	    echo "        manually specify the flash memory name using: flash=xxxxxx"; \
	    echo "        see: minipro/devices.h for a complete list of flash names"; \
	    echo "    *** please report the new memory name to: johnc@restswitch.com ***"; \
	    echo "********************************************************************************"; \
	    exit 2; \
	elif [ "$$rc" -eq "3" ] ; then \
	    echo "********************************************************************************"; \
	    echo " error: The flash device must be either MX25L3206E, PM25LQ032C, or W25Q32BV"; \
	    echo "             eg: flash=MX25L3206E or flash=W25Q32BV"; \
	    echo "            see: minipro/devices.h for the complete list"; \
	    echo "********************************************************************************"; \
	    exit 3; \
	elif [ "$$rc" -eq "4" ] ; then \
	    echo "********************************************************************************"; \
	    echo " error: Invalid flash name specified. Check flash= setting and try again."; \
	    echo "            see: minipro/devices.h for the complete list"; \
	    echo "********************************************************************************"; \
	    exit 4; \
	elif [ "$$rc" -eq "5" ] ; then \
	    echo "********************************************************************************"; \
	    echo " error: Image file not found or not specified - eg: image=a140808_a????????.bin"; \
	    echo "********************************************************************************"; \
	    exit 5; \
	elif [ "$$rc" -eq "6" ] ; then \
	    echo "********************************************************************************"; \
	    echo " error: Image file incorrect size: \"$(strip $(image))\""; \
	    echo "********************************************************************************"; \
	    exit 6; \
	elif [ "$$rc" -eq "8" ] ; then \
	    echo "********************************************************************************"; \
	    echo " io error: Check Minipro and device connections then try again."; \
	    echo "********************************************************************************"; \
	    exit 8; \
	else \
	    echo "********************************************************************************"; \
	    echo " error: Programming failed with python script error $$rc"; \
	    echo "        Check connections and try again."; \
	    echo "        Debug using the script directly: tools/scripts/program_flash.py"; \
	    echo "********************************************************************************"; \
	    exit 9; \
	fi )

$(LEDE_ROOT):
	@echo
	@echo fetching lede project...
	wget -O- "$(LEDE_SRC_URL)" | tar xz
	echo "$(LEDE_COMMIT)" > "$(LEDE_ROOT)/version"

patch: $(LEDE_ROOT)/package/utils/a140808
$(LEDE_ROOT)/package/utils/a140808: | $(LEDE_ROOT)
	@echo
	@echo applying a140808 patches to lede project...
	$(foreach patch,$(PATCHES),if patch --dry-run -N -p1 -d "$(LEDE_ROOT)" < "$(patch)" 2>&1 >/dev/null; then patch -p1 -d "$(LEDE_ROOT)" < "$(patch)"; fi;)
	ln -sf ../../../a140808 "$(LEDE_ROOT)/package/utils/a140808"

feeds: $(LEDE_FEEDS)
$(LEDE_FEEDS): | patch
	@echo
	@echo applying feeds to lede project...
	"$(LEDE_ROOT)/scripts/feeds" update -a
	"$(LEDE_ROOT)/scripts/feeds" install -a
	@rm -f "$(LEDE_CFG)" # remove .config file that the feeds gen

config: $(LEDE_CFG)
$(LEDE_CFG): | feeds
	@echo
	@echo generating lede project .config file...
	cat "$(CFG_DIFF)" > "$(LEDE_CFG)"
	$(MAKE) -C "$(LEDE_ROOT)" defconfig

certs: patch
    ifeq ("$(wildcard config/*.pem)","")
	@echo
	@echo " error: no pem certificates present in certs directory"
	@echo "        please include root_ca.pem, cert.pem, and private.pem"
	@echo
	exit 1
    endif
	@echo copying certificate files...
	cp config/*.pem "$(LEDE_ROOT)/package/utils/a140808/files/certs"

    ifeq ("$(wildcard config/host.cfg)","")
	@echo
	echo " error: config/host.cfg not found"
	@echo
	exit 2
    endif
	@echo copying host.cfg file...
	cp config/host.cfg "$(LEDE_ROOT)/package/utils/a140808/files"

minipro: $(MINIPRO)
$(MINIPRO):
	@if [ ! -d "tools/minipro" ]; then \
		echo; \
		echo "fetching minipro source...";\
		mkdir -p "tools/minipro"; \
		wget -O- $(MINIPRO_SRC_URL) | tar --strip-components=1 -xzvC "tools/minipro"; \
	fi
	$(MAKE) -C "tools/minipro"
	mv "tools/minipro/minipro" "$(OUTBIN)"

clean distclean:
	$(MAKE) -C "tools/minipro" clean
	@if [ -d "$(LEDE_ROOT)" ]; then $(MAKE) -C "$(LEDE_ROOT)" $@; fi
	rm -rf "$(OUTBIN)"

ifeq ("none","$(strip $(ssh))")
   ssh_check: ssh_none
else
   ssh_check: ssh_safe
endif

ssh_none:
	@"$(SSH_TOOL)" "-n"

ssh_cert:
   ifneq ("","$(strip $(sshcert))")
	@"$(SSH_TOOL)" "-c" "$(strip $(sshcert))"
   endif

ssh_pw:
   ifneq ("","$(strip $(sshpw))")
	@"$(SSH_TOOL)" "-p" "$(strip $(sshpw))"
   endif

ssh_safe: ssh_pw ssh_cert
	@"$(SSH_TOOL)" "-a" || ( \
	echo "********************************************************************************"; \
	echo " error: No root password nor cert file specified."; \
	echo "        ssh access will not be possible without a root password or"; \
	echo "        RSA certificate file. Specify "make ssh=none" if this is desired."; \
	echo ""; \
	echo "        options"; \
	echo "        ---------"; \
	echo "          no ssh access:"; \
	echo "            ssh=none"; \
	echo "          no ssh password access:"; \
	echo "            sshpw=none"; \
	echo "          ssh password access (prompt):"; \
	echo "            sshpw=pmt"; \
	echo "          ssh password access:"; \
	echo "            sshpw=<password>"; \
	echo "          no ssh certificate access:"; \
	echo "            sshcert=none"; \
	echo "          ssh certificate access, create new cert:"; \
	echo "            sshcert=gen"; \
	echo "          ssh certificate access, use existing cert:"; \
	echo "            sshcert=<filespec>"; \
	echo ""; \
	echo "********************************************************************************"; \
	exit 14 )

.PHONY: all target image program patch feeds config certs minipro clean distclean ssh_none ssh_cert ssh_pw ssh_safe
