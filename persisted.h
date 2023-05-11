#pragma once

#include <furi.h>
#include <stdlib.h>
#include <storage/storage.h>
#include <toolbox/saved_struct.h>

#define MAGIC4_SETTINGS_FILE_NAME "apps/Tools/nfcmagic4.save"

#define MAGIC4_SETTINGS_VER (1)
#define MAGIC4_SETTINGS_PATH EXT_PATH(MAGIC4_SETTINGS_FILE_NAME)
#define MAGIC4_SETTINGS_MAGIC (42)

#define SAVE_MAGIC4_SETTINGS(x) \
    saved_struct_save(           \
        MAGIC4_SETTINGS_PATH,   \
        (x),                     \
        sizeof(NfcMagic4State),    \
        MAGIC4_SETTINGS_MAGIC,  \
        MAGIC4_SETTINGS_VER)

#define LOAD_MAGIC4_SETTINGS(x) \
    saved_struct_load(           \
        MAGIC4_SETTINGS_PATH,   \
        (x),                     \
        sizeof(NfcMagic4State),    \
        MAGIC4_SETTINGS_MAGIC,  \
        MAGIC4_SETTINGS_VER)

typedef struct {
    uint8_t password[4];
} NfcMagic4State;