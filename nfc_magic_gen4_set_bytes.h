#pragma once
#include "common.h"
void nfc_magic_gen4_set_bytes_set_operation(byte operation);
enum {
    NfcMagicSetBytesOpATQA = 0x35,
    NfcMagicSetBytesOpMaxSectors = 0x6B,
    NfcMagicSetBytesOpPassword = 0xFE,
};