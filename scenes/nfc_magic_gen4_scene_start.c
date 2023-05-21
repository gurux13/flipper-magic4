#include "../nfc_magic_gen4_i.h"
enum SubmenuIndex {
    SubmenuIndexPassword,
    SubmenuIndexIdentify,
    SubmenuIndexSetAts,
    SubmenuIndexWriteGen1A,
    SubmenuIndexWipe,
};

void nfc_magic_gen4_scene_start_submenu_callback(void* context, uint32_t index) {
    NfcMagic* nfc_magic = context;
    view_dispatcher_send_custom_event(nfc_magic->view_dispatcher, index);
}

void nfc_magic_gen4_scene_start_on_enter(void* context) {
    NfcMagic* nfc_magic = context;
    static char password_text [32];
    byte* password = nfc_magic->persisted_state->password;
    snprintf(password_text, 32, "Password %02X:%02X:%02X:%02X", password[0], password[1], password[2], password[3]);
    Submenu* submenu = nfc_magic->submenu;
    submenu_add_item(
        submenu,
        password_text,
        SubmenuIndexPassword,
        nfc_magic_gen4_scene_start_submenu_callback,
        nfc_magic);
    submenu_add_item(
        submenu,
        "Identify Magic Tag",
        SubmenuIndexIdentify,
        nfc_magic_gen4_scene_start_submenu_callback,
        nfc_magic);
    submenu_add_item(
        submenu,
        "Set ATS",
        SubmenuIndexSetAts,
        nfc_magic_gen4_scene_start_submenu_callback,
        nfc_magic);
    submenu_add_item(
        submenu,
        "Write Gen1A",
        SubmenuIndexWriteGen1A,
        nfc_magic_gen4_scene_start_submenu_callback,
        nfc_magic);
    submenu_add_item(
        submenu, "Wipe", SubmenuIndexWipe, nfc_magic_gen4_scene_start_submenu_callback, nfc_magic);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneStart));
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewMenu);
}

bool nfc_magic_gen4_scene_start_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneStart, event.event);
        if (event.event == SubmenuIndexPassword) {
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicScenePassword);
            consumed = true;
        }
        if(event.event == SubmenuIndexIdentify) {
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicSceneIdentify);
            consumed = true;
        } 
        else if(event.event == SubmenuIndexSetAts) {
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicSceneSetAts);
            consumed = true;
        }
        else if(event.event == SubmenuIndexWriteGen1A) {
            // Explicitly save state in each branch so that the
            // correct option is reselected if the user cancels
            // loading a file.
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicSceneFileSelect);
            consumed = true;
        } else if(event.event == SubmenuIndexWipe) {
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicSceneWipe);
            consumed = true;
        }
    }

    return consumed;
}

void nfc_magic_gen4_scene_start_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    submenu_reset(nfc_magic->submenu);
}
