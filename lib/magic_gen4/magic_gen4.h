#pragma once

#include <lib/nfc/protocols/mifare_classic.h>
typedef unsigned char byte;

bool magic_wupa();
bool execute_magic_command(
    const byte* password,
    byte command,
    const byte* payload,
    int payload_size,
    byte* buffer,
    int buffer_size);
bool execute_magic_command_timeout(
    const byte* password,
    byte command,
    const byte* payload,
    int payload_size,
    byte* buffer,
    int buffer_size,
    int timeout);    

bool magic_read_block(uint8_t block_num, MfClassicBlock* data);

bool magic_data_access_cmd();

bool magic_write_blk(uint8_t block_num, MfClassicBlock* data);

bool magic_wipe();

void magic_deactivate();

void magic_test();
