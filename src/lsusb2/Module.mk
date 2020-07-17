MODULE_CPPFLAGS = -isystem/usr/include/libusb-1.0
MODULE_LDLIBS = -lusb-1.0
MODULE_LIBRARIES = util
$(call use-fmt)
$(call add-executable-module,$(call get-path))
