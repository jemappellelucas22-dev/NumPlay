# D-Side 1: standalone NumWorks app, built from NumPlay's SDK template.
Q ?= @
CC = arm-none-eabi-gcc
NWLINK ?= npx --yes -- nwlink@1.0.0
BUILD_DIR = output
SRC = src/main.c
HEADERS = ../common/epsilon_app.h ../common/epsilon_files.h ../common/np_text.h ../../launcher/src/gfx.h ../../launcher/src/np.h ../../launcher/src/font.h
CFLAGS = -std=c11 $(shell $(NWLINK) eadk-cflags-device)
CFLAGS += -Os -Wall -Wextra -Wno-unused-parameter -fno-math-errno -fsingle-precision-constant
CFLAGS += -fno-tree-loop-distribute-patterns -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nano.specs -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level -Wl,--gc-sections -flinker-output=nolto-rel
.PHONY: build check clean
build: $(BUILD_DIR)/DSide1.nwa
check: build
	$(Q) $(NWLINK) nwa-bin $(BUILD_DIR)/DSide1.nwa $(BUILD_DIR)/DSide1.bin
	@echo "BIN     $(BUILD_DIR)/DSide1.bin: $$(wc -c < $(BUILD_DIR)/DSide1.bin) bytes"
$(BUILD_DIR)/main.o: $(SRC) $(HEADERS) | $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/gfx.o: ../../launcher/src/gfx.c $(HEADERS) | $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/font_data.o: ../../launcher/src/font_data.c ../../launcher/src/font.h | $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/xfont.o: ../../launcher/src/xfont.c ../../launcher/src/np_xfont.h $(HEADERS) | $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) -c $< -o $@
$(BUILD_DIR)/icon.o: ../../sdk/template/src/icon.png | $(BUILD_DIR)
	$(Q) $(NWLINK) png-icon-o $< $@
$(BUILD_DIR)/DSide1.nwa: $(BUILD_DIR)/main.o $(BUILD_DIR)/gfx.o $(BUILD_DIR)/font_data.o $(BUILD_DIR)/xfont.o $(BUILD_DIR)/icon.o
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) $^ -lm -o $@
	$(Q) arm-none-eabi-strip --strip-unneeded $@
$(BUILD_DIR):
	$(Q) mkdir -p $@
clean:
	$(Q) rm -rf $(BUILD_DIR)
