#include "../nfc_magic_gen4_i.h"

void nfc_magic_gen4_setup_hex_input(const char * header, int num_bytes, const byte* initial_value, uint32_t scene_to_return);
void nfc_magic_gen4_destroy_hex_input();
void nfc_magic_gen4_get_last_hex_input(byte* write_to, int num_bytes);
bool nfc_magic_gen4_hex_input_has_result();
bool nfc_magic_gen4_hex_input_is_setup();