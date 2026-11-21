#include "fota_stage.h"
#include "rover_flash.h"

#include <hardware/flash.h>
#include <hardware/regs/addressmap.h>
#include <hardware/watchdog.h>
#include <mbedtls/sha256.h>
#include <string.h>

enum { HASH_SIZE = 32 };

#define FOTA_SWAP_READY  UINT32_C(0xabcdef12)
#define FOTA_ROLLBACK_DUE UINT32_C(0x0deadead)

extern const uint8_t __FLASH_DOWNLOAD_SLOT_START[];
extern const uint8_t __FLASH_SWAP_SPACE_LENGTH[];
extern const uint8_t __FLASH_INFO_START[];
extern const uint32_t __FLASH_INFO_IS_DOWNLOAD_SLOT_VALID;
extern const uint32_t __FLASH_INFO_SHOULD_ROLLBACK;

struct flash_write {
    uint32_t offset;
    size_t length;
    const uint8_t *data;
};

static size_t expected;
static size_t written;

/* Keep the metadata sector buffer off the Wi-Fi task's stack. */
static uint8_t metadata[FLASH_SECTOR_SIZE];

static uint32_t flash_offset(const void *address)
{
    return (uint32_t)((uintptr_t)address - XIP_BASE);
}

static void erase_flash(void *arg)
{
    const struct flash_write *op = arg;
    flash_range_erase(op->offset, op->length);
}

static void program_flash(void *arg)
{
    const struct flash_write *op = arg;
    flash_range_program(op->offset, op->data, op->length);
}

static void rewrite_metadata(void *arg)
{
    erase_flash(arg);
    program_flash(arg);
}

static int set_metadata_flag(const uint32_t *flag, uint32_t value)
{
    const size_t flag_offset =
        (uintptr_t)flag -
        (uintptr_t)__FLASH_INFO_START;

    memcpy(metadata, __FLASH_INFO_START, sizeof(metadata));
    memcpy(metadata + flag_offset, &value, sizeof(value));

    struct flash_write op = {
        flash_offset(__FLASH_INFO_START), sizeof(metadata), metadata
    };
    return rover_flash_execute(rewrite_metadata, &op);
}

int fota_confirm_running_image(void)
{
    if (__FLASH_INFO_SHOULD_ROLLBACK != FOTA_ROLLBACK_DUE) {
        return 0;
    }
    return set_metadata_flag(&__FLASH_INFO_SHOULD_ROLLBACK, 0);
}

int fota_stage_begin(size_t image_size)
{
    const size_t capacity = (size_t)(uintptr_t)__FLASH_SWAP_SPACE_LENGTH;

    fota_stage_abort();
    if (image_size < 2 * FLASH_PAGE_SIZE ||
        image_size % FLASH_PAGE_SIZE != 0 || image_size > capacity ||
        __FLASH_INFO_SHOULD_ROLLBACK == FOTA_ROLLBACK_DUE) {
        return -1;
    }

    if (__FLASH_INFO_IS_DOWNLOAD_SLOT_VALID == FOTA_SWAP_READY &&
        set_metadata_flag(&__FLASH_INFO_IS_DOWNLOAD_SLOT_VALID, 0) != 0) {
        return -1;
    }

    struct flash_write op = {
        flash_offset(__FLASH_DOWNLOAD_SLOT_START), capacity, NULL
    };
    if (rover_flash_execute(erase_flash, &op) != 0) {
        return -1;
    }

    expected = image_size;
    return 0;
}

int fota_stage_write_page(size_t offset, const uint8_t page[256])
{
    if (expected == 0 || offset != written ||
        expected - written < FLASH_PAGE_SIZE) {
        return -1;
    }

    struct flash_write op = {
        flash_offset(__FLASH_DOWNLOAD_SLOT_START) + (uint32_t)offset,
        FLASH_PAGE_SIZE,
        page
    };
    if (rover_flash_execute(program_flash, &op) != 0) {
        return -1;
    }

    written += FLASH_PAGE_SIZE;
    return 0;
}

void fota_stage_verify_and_reboot(size_t image_size)
{
    if (expected == 0 || image_size != expected || written != expected) {
        return;
    }

    uint8_t digest[HASH_SIZE];
    const uint8_t *image = __FLASH_DOWNLOAD_SLOT_START;

    /* The packager appends one page with the digest in its final 32 bytes. */
    if (mbedtls_sha256(image, image_size - FLASH_PAGE_SIZE, digest, 0) != 0 ||
        memcmp(digest, image + image_size - HASH_SIZE, sizeof(digest)) != 0) {
        return;
    }

    if (set_metadata_flag(&__FLASH_INFO_IS_DOWNLOAD_SLOT_VALID,
                          FOTA_SWAP_READY) == 0) {
        watchdog_reboot(0, 0, 0);
        for (;;) {
        }
    }
}

void fota_stage_abort(void)
{
    expected = 0;
    written = 0;
}
