MODULE_NAME := test-runner
MODULE_CPPFLAGS := -I.
MODULE_LIBRARIES :=

$(call use-catch)
$(call use-fmt)

# $(call add-executable-module,$(call get-path))
