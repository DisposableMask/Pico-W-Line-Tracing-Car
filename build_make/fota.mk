# RP2040 FOTA linker, dependencies, and image packaging.
FOTA_LINKER_DIR := ../etc/linker/pico_rp2040
FOTA_LAYOUT := $(FOTA_LINKER_DIR)/fota_layout.ld
LNKFILE := $(FOTA_LINKER_DIR)/tkernel_fota_map.ld
LFLAGS += -L"$(FOTA_LINKER_DIR)"

CFLAGS += -DAPP_FOTA_TCP=1

FOTA_SDK_RP2 := $(PICO_SDK_PATH)/src/rp2_common
FOTA_MBEDTLS := $(PICO_SDK_PATH)/lib/mbedtls
FOTA_SDK_MODULES := hardware_flash hardware_watchdog hardware_ticks \
                   hardware_xip_cache hardware_boot_lock pico_bootrom \
                   pico_flash boot_bootrom_headers

CFLAGS += $(foreach module,$(FOTA_SDK_MODULES),-I"$(FOTA_SDK_RP2)/$(module)/include") \
          -I"$(FOTA_MBEDTLS)/include" \
          -DMBEDTLS_CONFIG_FILE='"fota_mbedtls_config.h"'

FOTA_OBJS := ./mtkernel_3/fota/flash.o ./mtkernel_3/fota/watchdog.o \
             ./mtkernel_3/fota/sha256.o ./mtkernel_3/fota/platform_util.o
OBJS += $(FOTA_OBJS)
C_DEPS += $(FOTA_OBJS:.o=.d)

mtkernel_3/fota/flash.o: $(FOTA_SDK_RP2)/hardware_flash/flash.c
mtkernel_3/fota/watchdog.o: $(FOTA_SDK_RP2)/hardware_watchdog/watchdog.c
mtkernel_3/fota/sha256.o: $(FOTA_MBEDTLS)/library/sha256.c
mtkernel_3/fota/platform_util.o: $(FOTA_MBEDTLS)/library/platform_util.c

$(FOTA_OBJS):
	@mkdir -p "$(@D)"
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) -MF"$(@:.o=.d)" -MT"$@" -c -o "$@" "$<"

FOTA_BOOT_UF2 := ../tools/pico_fota/pico_fota_bootloader.uf2
FOTA_UF2_PAD := ../tools/pico_fota/pad_rp2040_uf2.py
FOTA_SHA256 := ../tools/pico_fota/sha256_append.py
PICOTOOL ?= picotool

all: $(EXE_FILE)_image.bin

$(EXE_FILE)_image.bin: $(EXE_FILE).elf $(FOTA_SHA256)
	arm-none-eabi-objcopy -O binary "$<" "$@.tmp.bin"
	python3 -c 'import sys; from pathlib import Path; p = Path(sys.argv[1]); image = p.read_bytes(); p.write_bytes(image + bytes(-len(image) % 256))' "$@.tmp.bin"
	python3 "$(FOTA_SHA256)" --target-file "$@.tmp.bin"
	mv "$@.tmp.bin" "$@"

$(EXE_FILE)_app.uf2: $(EXE_FILE).elf | $(E2U)
	$(E2U) "$<" "$@"

$(EXE_FILE).uf2: $(FOTA_BOOT_UF2) $(EXE_FILE)_app.uf2 $(FOTA_UF2_PAD)
	"$(PICOTOOL)" uf2 combine "$<" "$(EXE_FILE)_app.uf2" "$@.unpadded.uf2" --family rp2040
	python3 "$(FOTA_UF2_PAD)" "$@.unpadded.uf2" "$@"
	rm -f "$@.unpadded.uf2"
