include $(TOPDIR)/rules.mk

PKG_NAME:=diangx-oui-lookup
PKG_VERSION:=0.1.0
PKG_RELEASE:=1

TAB:=$(shell printf '\t')

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -DOUI_DEFAULT_DB=/usr/share/oui/manuf.gz

define Package/diangx-oui-lookup
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=OUI lookup tool
  DEPENDS:=+libstdcpp +zlib
endef

define Package/diangx-oui-lookup/description
 A small OUI/MAC vendor lookup utility using the Wireshark manuf database.
endef

define Build/Prepare
$(TAB)$(INSTALL_DIR) $(PKG_BUILD_DIR)
$(TAB)$(CP) ./CMakeLists.txt $(PKG_BUILD_DIR)/
$(TAB)$(CP) ./LICENSE $(PKG_BUILD_DIR)/
$(TAB)$(CP) ./README.md $(PKG_BUILD_DIR)/
$(TAB)$(CP) ./data $(PKG_BUILD_DIR)/
$(TAB)$(CP) ./src $(PKG_BUILD_DIR)/
endef

define Package/diangx-oui-lookup/install
$(TAB)$(INSTALL_DIR) $(1)/usr/bin
$(TAB)$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/oui $(1)/usr/bin/
$(TAB)$(INSTALL_DIR) $(1)/usr/share/oui
$(TAB)$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/oui/manuf.gz $(1)/usr/share/oui/
endef

$(eval $(call BuildPackage,diangx-oui-lookup))
