#include "../nfc_magic_gen4_i.h"

static byte * data = NULL;
static int nbytes = 0;
static char * hdr = NULL;
static bool have_result = false;


void nfc_magic_gen4_setup_hex_input(const char * header, int num_bytes, const byte* initial_value) {
    nbytes = num_bytes;
    data = malloc(num_bytes);
    if (initial_value != NULL) {
        memcpy(data, initial_value, num_bytes);
    } else {
        memset(data, 0, num_bytes);
    }
    hdr = malloc(strlen(header) + 1);
    strcpy(hdr, header);
    have_result = false;
}

void nfc_magic_gen4_destroy_hex_input() {
    free((void*)data);
    free((void*)hdr);
    data = NULL;
    hdr = NULL;
    have_result = false;
}

void nfc_magic_gen4_get_last_hex_input(byte* write_to, int num_bytes) {
    furi_assert(nbytes == num_bytes);
    memcpy(write_to, data, nbytes);
}

bool nfc_magic_gen4_hex_input_has_result() {
    return have_result;
}
bool nfc_magic_gen4_hex_input_is_setup() {
    return data != NULL;
}


static void on_save(void* context) {
    NfcMagic* app = context;
    have_result = true;
    view_dispatcher_send_custom_event(
        app->view_dispatcher, 0);
        
}

static void nfc_magic_gen4_scene_hex_input_setup_view(NfcMagic* nfc_magic) {
    ByteInput* input = nfc_magic->byte_input;
    byte_input_set_header_text(input, hdr);
    byte_input_set_result_callback(input, on_save, NULL, nfc_magic, data, nbytes);
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
}

void nfc_magic_gen4_scene_hex_input_on_enter(void* context) {
    NfcMagic* nfc_magic = context;

    nfc_magic_gen4_scene_hex_input_setup_view(nfc_magic);
}
bool nfc_magic_gen4_scene_hex_input_on_event(void* context, SceneManagerEvent event) {
    // UNUSED(event);
    if (event.type == SceneManagerEventTypeCustom) {
        NfcMagic* nfc_magic = context;
        scene_manager_previous_scene(nfc_magic->scene_manager);
        return true;
    }
    return false;
}

void nfc_magic_gen4_scene_hex_input_on_exit(void* context) {
    UNUSED(context);
}
