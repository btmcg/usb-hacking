kernel_build := /usr/lib/modules/$(shell uname --kernel-release)/build
gcc_path := $(shell dirname $$(gcc -print-prog-name=cc1))

MODULE_CPPFLAGS := \
  -include $(kernel_build)/include/linux/compiler_types.h \
  -include $(kernel_build)/include/linux/kconfig.h \
  -isystem$(gcc_path)/include \
  -isystem$(kernel_build)/arch/x86/include \
  -isystem$(kernel_build)/arch/x86/include/generated \
  -isystem$(kernel_build)/arch/x86/include/generated/uapi \
  -isystem$(kernel_build)/arch/x86/include/uapi \
  -isystem$(kernel_build)/include \
  -isystem$(kernel_build)/include/uapi \
  -std=gnu89 \
  -nostdinc \
  -falign-jumps=1 \
  -falign-loops=1 \
  -fcf-protection=none \
  -fconserve-stack \
  -fmacro-prefix-map=./= \
  -fmerge-constants \
  -fno-allow-store-data-races \
  -fno-asynchronous-unwind-tables \
  -fno-common \
  -fno-delete-null-pointer-checks \
  -fno-jump-tables \
  -fno-merge-all-constants \
  -fno-PIE \
  -fno-stack-check \
  -fno-strict-aliasing \
  -fno-strict-overflow \
  -fno-var-tracking-assignments \
  -fplugin=$(kernel_build)/scripts/gcc-plugins/structleak_plugin.so \
  -fplugin-arg-structleak_plugin-byref-all \
  -fshort-wchar \
  -fstack-protector-strong \
  -gdwarf-4 \
  -m64 \
  -mcmodel=kernel \
  -mfentry \
  -mindirect-branch-register \
  -mindirect-branch=thunk-extern \
  -mno-3dnow \
  -mno-80387 \
  -mno-avx \
  -mno-fp-ret-in-387 \
  -mno-mmx \
  -mno-red-zone \
  -mno-sse \
  -mno-sse2 \
  -mpreferred-stack-boundary=3 \
  -mrecord-mcount \
  -mskip-rax-setup \
  -mtune=generic \
  -pg \
  -Wdeclaration-after-statement \
  -Werror=date-time \
  -Werror=designated-init \
  -Werror=implicit-function-declaration \
  -Werror=implicit-int \
  -Werror=incompatible-pointer-types \
  -Werror=strict-prototypes \
  -Wframe-larger-than=2048 \
  -Wimplicit-fallthrough \
  -Wno-address-of-packed-member \
  -Wno-array-bounds \
  -Wno-format-overflow \
  -Wno-format-security \
  -Wno-format-truncation \
  -Wno-frame-address \
  -Wno-maybe-uninitialized \
  -Wno-packed-not-aligned \
  -Wno-pedantic \
  -Wno-pointer-sign \
  -Wno-restrict \
  -Wno-sign-compare \
  -Wno-stringop-overflow \
  -Wno-stringop-truncation \
  -Wno-trigraphs \
  -Wno-unused-but-set-variable \
  -Wno-unused-const-variable \
  -Wno-zero-length-bounds \
  -Wvla \
  -D__KERNEL__ \
  -DSTRUCTLEAK_PLUGIN \
  -DCC_USING_FENTRY \
  -DMODULE \
  -DKBUILD_BASENAME='"delcom"' \
  -DKBUILD_MODNAME='"delcom"'

MODULE_CFLAGS := -Wno-unused-parameter

# Step 1
# make -f /usr/lib/modules/$(uname -r)/build/scripts/Makefile.build obj=/home/brian/src/kernel-module single-build= need-builtin=1 need-modorder=1
#  { echo /home/brian/src/kernel-module/delcom.ko; :; } | awk '!x[$0]++' - > /home/brian/src/kernel-module/modules.order
#  { echo /home/brian/src/kernel-module/delcom.o; echo; } > /home/brian/src/kernel-module/delcom.mod
#
#
# Step 2 - create delcom.mod.c
# /usr/lib/modules/$(uname -r)/build/scripts/mod/modpost -a src/delcom_vi_hid/delcom_vi_hid.o
#
#
# Step 3 - convert .o into .ko
# ld -r -m elf_x86_64 -z max-page-size=0x200000 --build-id  -T /usr/lib/modules/$(uname -r)/build/scripts/module-common.lds -o src/delcom_vi_hid/delcom_vi_hid.ko src/delcom_vi_hid/delcom_vi_hid.o src/delcom_vi_hid/delcom_vi_hid.mod.o

$(call add-static-library-module,$(call get-path))
