BUILD_DIR=build
BUILDTYPE?=MinSizeRel

all: build

build:
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR); cmake ../ -DCMAKE_BUILD_TYPE=$(BUILDTYPE)
	$(MAKE) -C $(BUILD_DIR) -j4

install:
	@$(MAKE) -C $(BUILD_DIR) install

clean:
	@$(MAKE) -C $(BUILD_DIR) clean

distclean:
	@rm -rf $(BUILD_DIR)

build/qjsc:
	$(MAKE) -C $(BUILD_DIR) qjsc -j4

gen: build/qjsc
	$(BUILD_DIR)/qjsc -c -m -o preact.c -N preact preact.js
	$(BUILD_DIR)/qjsc -c -m -o htm.c -N htm htm.js

.PHONY: all build install clean distclean gen