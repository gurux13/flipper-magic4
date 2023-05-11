#include "../nfc_magic_gen4_i.h"
#include "./nfc_magic_gen4_scene_hex_input.h"
#include "../persisted.h"
enum {
    NfcMagicSceneCheckStateCardSearch,
    NfcMagicSceneCheckStateCardFound,
};
#define TAG "NfcPassword"
static byte password[4];

static void on_save(void* context) {
    NfcMagic* app = context;
    memcpy(app->persisted_state->password, password, 4);
    SAVE_MAGIC4_SETTINGS(app->persisted_state);
    view_dispatcher_send_custom_event(
        app->view_dispatcher, 0);
}

void nfc_magic_gen4_scene_password_setup_view(NfcMagic* nfc_magic) {
    ByteInput* input = nfc_magic->byte_input;
    byte_input_set_header_text(input, "Password");
    memcpy(password, nfc_magic->persisted_state->password, 4);
    byte_input_set_result_callback(input, on_save, NULL, nfc_magic, password, 4);
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
}

void nfc_magic_gen4_scene_password_on_enter(void* context) {
    nfc_magic_gen4_scene_password_setup_view((NfcMagic*)context);
}

bool nfc_magic_gen4_scene_password_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    if (event.type == SceneManagerEventTypeCustom) {
        scene_manager_previous_scene(nfc_magic->scene_manager);
        return true;
    }
    return false;
}

void nfc_magic_gen4_scene_password_on_exit(void* context) {
    // NfcMagic* nfc_magic = context;
    UNUSED(context);

    // nfc_magic_gen4_worker_stop(nfc_magic->worker);
    // scene_manager_set_scene_state(
    //     nfc_magic->scene_manager, NfcMagicSceneCheck, NfcMagicSceneCheckStateCardSearch);
    // Clear view
    // byte_input_reset(nfc_magic->byte_input);

    // nfc_magic_gen4_blink_stop(nfc_magic);
}
