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

#
# a140808 amazon aws iot client application
#

include $(TOPDIR)/rules.mk

PKG_NAME:=a140808
PKG_VERSION:=1.0
PKG_RELEASE:=1
AWS_IOT_SDK_VER:=v2.1.1

# PKG_SOURCE:=a140808-src-$(PKG_VERSION).tar.gz
# PKG_SOURCE_URL:=https://github.com/rest-switch/device_a140808
# PKG_MD5SUM:=41b0a9bca945b4ae2608421afd8f606e

include $(INCLUDE_DIR)/version.mk
include $(INCLUDE_DIR)/package.mk

define Download/aws-iot-sdk
	URL:=https://github.com/aws/aws-iot-device-sdk-embedded-C/archive/
	FILE:=$(AWS_IOT_SDK_VER).tar.gz
	HASH:=74d434b3258654cea048b20eb52d4fc627f5c87e8727ce180a1d529e3285a97e
#	SUBDIR:=aws-iot-sdk-$(AWS_IOT_SDK_VER)
endef
$(eval $(call Download,aws-iot-sdk))

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/external/aws-iot-sdk
	$(TAR) -xzvf $(DL_DIR)/$(AWS_IOT_SDK_VER).tar.gz --strip-components=1 -C $(PKG_BUILD_DIR)/external/aws-iot-sdk
	$(Build/Prepare/Default)
endef

define Package/a140808
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=a140808 amazon aws iot client application
	URL:=http://www.rest-switch.com
	DEPENDS:=+libmbedtls
endef

TARGET_LDFLAGS := -lmbedtls -lmbedx509 -lmbedcrypto

define Package/a140808/description
	a140808 is an amazon aws iot client for the hlk-rm04 based device
endef


define Package/a140808/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/a140808 $(1)/usr/bin/
	$(INSTALL_BIN) ./files/thingname $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/uci-defaults
	sed -e "s/host='.*'/host='$(shell cat ./files/host.cfg || echo ' '    | awk -F: '{print $$1}')'/" \
	    -e "s/port='.*'/port='$(shell cat ./files/host.cfg || echo '8883' | awk -F: '{print ($$2==0+$$2)?$$2:8883}')'/" \
	    ./files/08_init_aws-iot_cfg > $(1)/etc/uci-defaults/08_init_aws-iot_cfg

	$(INSTALL_DIR) $(1)/etc/aws-iot-certs
	$(INSTALL_CONF) ./files/certs/*.pem $(1)/etc/aws-iot-certs/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/a140808.init $(1)/etc/init.d/a140808
endef

$(eval $(call BuildPackage,a140808))
