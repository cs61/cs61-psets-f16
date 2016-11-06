# are we using clang?
ISCLANG := $(shell if $(CC) --version | grep LLVM >/dev/null; then echo 1; else echo 0; fi)

CFLAGS ?= -std=gnu11 -W -Wall -Wshadow -g $(DEFS)
O ?= -O3
ifeq ($(filter 0 1 2 3 s,$(O)$(NOOVERRIDEO)),$(strip $(O)))
override O := -O$(O)
endif
ifeq ($(SANITIZE),1)
ifeq ($(strip $(shell $(CC) -fsanitize=address -x c -E /dev/null 2>&1 | grep sanitize=)),)
CFLAGS += -fsanitize=address
else
$(info ** WARNING: Your C compiler does not support `-fsanitize=address`.)
endif
ifeq ($(strip $(shell $(CC) -fsanitize=undefined -x c -E /dev/null 2>&1 | grep sanitize=)),)
CFLAGS += -fsanitize=undefined
else
$(info ** WARNING: Your C compiler does not support `-fsanitize=undefined`.)
$(info ** You may want to install gcc-4.9 or greater.)
endif
ifeq ($(ISCLANG),0)
ifeq ($(wildcard /usr/bin/gold),/usr/bin/gold)
CFLAGS += -fuse-ld=gold
endif
endif
endif

# these rules ensure dependencies are created
DEPCFLAGS = -MD -MF $(DEPSDIR)/$*.d -MP
DEPSDIR := .deps
BUILDSTAMP := $(DEPSDIR)/rebuildstamp
DEPFILES := $(wildcard $(DEPSDIR)/*.d)
ifneq ($(DEPFILES),)
include $(DEPFILES)
endif


# when the C compiler or optimization flags change, rebuild all objects
ifneq ($(strip $(DEP_CC)),$(strip $(CC) $(CPPFLAGS) $(CFLAGS) $(O)))
DEP_CC := $(shell mkdir -p $(DEPSDIR); echo >$(BUILDSTAMP); echo "DEP_CC:=$(CC) $(CFLAGS) $(O)" >$(DEPSDIR)/_cc.d)
endif

V = 0
ifeq ($(V),1)
run = $(1) $(3)
xrun = /bin/echo "$(1) $(3)" && $(1) $(3)
else
run = @$(if $(2),/bin/echo "  $(2) $(3)" &&,) $(1) $(3)
xrun = $(if $(2),/bin/echo "  $(2) $(3)" &&,) $(1) $(3)
endif
runquiet = @$(1) $(3)

$(BUILDSTAMP):
	@mkdir -p $(@D)
	@echo >$@

always:
	@:

clean-hook:
	@:

.PHONY: always clean-hook
.PRECIOUS: %.o
