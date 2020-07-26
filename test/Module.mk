MODULE_NAME := test-runner
MODULE_CPPFLAGS := -I.
MODULE_LIBRARIES :=

$(use-catch)
$(use-fmt)

# $(call add-executable-module,$(get-path))
