

#include "lib/FatFs_SPI/sd_driver/hw_config.h"
#include "hardware/rtc.h"
#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"
#include "string.h"

sd_card_t *sd_get_by_name(const char *const name);

FATFS *sd_get_fs_by_name(const char *name);

bool run_setrtc(const char *datetime_str);

void run_format();

void run_mount();

void run_unmount();

void run_getfree();

void run_ls();

void run_cat();
