BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = splitscreen.c
assets_ttf = $(wildcard assets/*.ttf)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_ttf:%.ttf=%.font64)))

MKFONT_FLAGS ?=

all: splitscreen.z64

filesystem/%.font64: assets/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	@$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem "$<"

filesystem/BakbakOne-Regular.font64: MKFONT_FLAGS+=--size 12

$(BUILD_DIR)/splitscreen.dfs: $(assets_conv)
$(BUILD_DIR)/splitscreen.elf: $(src:%.c=$(BUILD_DIR)/%.o)

splitscreen.z64: N64_ROM_TITLE="N64 SplitScreen Demo"
splitscreen.z64: $(BUILD_DIR)/splitscreen.dfs 

clean:
	rm -rf $(BUILD_DIR) filesystem splitscreen.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
