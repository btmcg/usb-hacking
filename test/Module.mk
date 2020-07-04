LOCAL_MODULE := test-runner
LOCAL_CPPFLAGS := -I.
LOCAL_LIBRARIES :=

$(call use-catch)
$(call use-fmt)

# $(call add-executable-module,$(call get-path))
