#
# Makefile
#
# Copyright (C) 2010 Creytiv.com
#

PROJECT	  := retest
VERSION   := 0.5.3

LIBRE_MK  := $(shell [ -f ../re/mk/re.mk ] && \
	echo "../re/mk/re.mk")
ifeq ($(LIBRE_MK),)
LIBRE_MK  := $(shell [ -f /usr/share/re/re.mk ] && \
	echo "/usr/share/re/re.mk")
endif
ifeq ($(LIBRE_MK),)
LIBRE_MK  := $(shell [ -f /usr/local/share/re/re.mk ] && \
	echo "/usr/local/share/re/re.mk")
endif

include $(LIBRE_MK)

LIBREM_PATH	:= $(shell [ -d ../rem ] && echo "../rem")

INSTALL := install
ifeq ($(DESTDIR),)
PREFIX  := /usr/local
else
PREFIX  := /usr
endif
BINDIR	:= $(PREFIX)/bin
CFLAGS	+= -Isrc -I$(LIBRE_INC)
CFLAGS  += -I$(LIBREM_PATH)/include -I$(SYSROOT)/local/include/rem
CXXFLAGS	+= -Isrc -I$(LIBRE_INC)
CXXFLAGS  += -I$(LIBREM_PATH)/include -I$(SYSROOT)/local/include/rem

# XXX: common for C/C++
CPPFLAGS += -DHAVE_INTTYPES_H

CPPFLAGS += -I$(SYSROOT)/include


BIN	:= $(PROJECT)$(BIN_SUFFIX)

ifneq ($(LIBREM_PATH),)
LIBS	+= -L$(LIBREM_PATH)
endif

LIBS	+= -lrem -lm


include src/srcs.mk

OBJS      := $(patsubst %.c,$(BUILD)/src/%.o,$(filter %.c,$(SRCS)))
OBJS      += $(patsubst %.cpp,$(BUILD)/src/%.o,$(filter %.cpp,$(SRCS)))

all: $(BIN)

-include $(OBJS:.o=.d)

$(BIN): $(OBJS)
	@echo "  LD      $@"
	@$(CC) $(LFLAGS) $^ -L$(LIBRE_SO) -lre $(LIBS) -o $@

$(BUILD)/%.o: %.c $(BUILD) Makefile src/srcs.mk
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -o $@ -c $< $(DFLAGS)

$(BUILD)/%.o: %.cpp $(BUILD) Makefile src/srcs.mk
	@echo "  CXX     $@"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $< $(DFLAGS)

$(BUILD): Makefile
	@mkdir -p $(BUILD)/src/mock
	@mkdir -p $(BUILD)/src/combo
	@touch $@

clean:
	@rm -rf $(BIN) $(BUILD)

install: $(BIN)
	@mkdir -p $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(BIN) $(DESTDIR)$(BINDIR)

GCOV_SRCS := $(patsubst %,src/%,$(SRCS))
gcov:
	for n in $(GCOV_SRCS); do \
		echo -ne "$${n}:\t" ; \
		gcov -n -o $(BUILD)/src $${n} | grep Lines ; \
	done
