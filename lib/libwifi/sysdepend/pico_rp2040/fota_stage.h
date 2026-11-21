#ifndef FOTA_STAGE_H
#define FOTA_STAGE_H

#include <stddef.h>
#include <stdint.h>

int fota_confirm_running_image(void);
int fota_stage_begin(size_t image_size);
int fota_stage_write_page(size_t offset, const uint8_t page[256]);
void fota_stage_verify_and_reboot(size_t image_size);
void fota_stage_abort(void);

#endif
