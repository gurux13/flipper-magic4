#pragma once
#include "common.h"
void nfc_magic_gen4_set_one_set_operation(byte operation);
enum {
    NfcMagicSetOneOpShadow = 0x32,
    NfcMagicSetOneOpUidLength = 0x68,
    NfcMagicSetOneOpUlEnable = 0x69,
    NfcMagicSetOneOpUlMode = 0x6A,
    NfcMagicSetOneOpDirectWrite = 0xCF
};