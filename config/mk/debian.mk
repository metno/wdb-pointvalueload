###
# Debian package related stuff
###

DEBIAN_SOURCE=$(top_srcdir)/$(DEBIAN_INPUT_BASE)/debian

PKG_DIR        = $(PACKAGE)-$(VERSION)
DEBIAN_DIR     = $(PKG_DIR)/debian
ARCH = `dpkg-architecture -qDEB_HOST_ARCH_CPU`
DEBIAN_PACKAGE_NAME_BASE = `head -n1 $(DEBIAN_SOURCE)/changelog | sed "s/ (/_/" | sed "s/).*//"`
DEBIAN_PACKAGE = $(DEBIAN_PACKAGE_NAME_BASE)
DEBIAN_PACKAGE_NAME= $(DEBIAN_PACKAGE_NAME_BASE)_$(ARCH).deb
DEBIAN_SOURCE_PACKAGE_NAME = $(DEBIAN_PACKAGE_NAME_BASE).dsc

debian-name:
	@echo $(DEBIAN_PACKAGE_NAME)



debian:	dist clean-debian 
	tar xvzf $(PKG_DIR).tar.gz
	mkdir -p $(DEBIAN_DIR)
	rm -rf $(DEBIAN_DIR)/*
	cp -r $(DEBIAN_SOURCE)/* $(DEBIAN_DIR)
	@( if test -d $(DEBIAN_DIR) ; then \
         cd $(DEBIAN_DIR); \
         if test -f /etc/debian_version ; then \
	 	debversion=`cat /etc/debian_version`; \
		echo "----debversion: $$debversion"; \
		if [ "$$debversion" = "4.0" ]; then \
			if test -f control.etch ; then cp control.etch control; fi;  \
			if test -f rules.etch ; then cp rules.etch rules;  fi; \
		fi; \
		if [ "$$debversion" = "squeeze/sid" ]; then \
			if test -f control.lucid ; then cp control.lucid control; fi;  \
			if test -f rules.lucid ; then cp rules.lucid rules;  fi; \
		fi; \
         fi \
      fi \
    )
	chmod 744 $(DEBIAN_DIR)/rules
	cp $(PKG_DIR).tar.gz $(DEBIAN_PACKAGE).orig.tar.gz
	cd $(PKG_DIR) && dpkg-buildpackage -rfakeroot -us -uc -sa
	lintian $(DEBIAN_PACKAGE_NAME) 

update-debian:
	rm -rf $(DEBIAN_DIR)/*
	cp -r $(DEBIAN_SOURCE)/* $(DEBIAN_DIR)
	chmod 744 $(DEBIAN_DIR)/rules
	cd $(PKG_DIR) && dpkg-buildpackage -rfakeroot -us -uc -nc
	lintian $(DEBIAN_PACKAGE_NAME) 


clean-debian:
	debclean
	rm -rf $(PKG_DIR) $(DEBIAN_PACKAGE)*
