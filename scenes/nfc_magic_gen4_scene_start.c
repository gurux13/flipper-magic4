#include "../nfc_magic_gen4_i.h"
#include "../nfc_magic_gen4_set_one.h"
#include "../nfc_magic_gen4_set_bytes.h"

#define TAG "Start"

typedef struct {
    char* name;
    int scene_id;
    int scene_selector_extra;
} SubmenuItem;

SubmenuItem submenu_items[] = {
    {"Password XX:XX:XX:XX", NfcMagicScenePassword, -1},
    {"Change Password", NfcMagicSceneSetBytes, NfcMagicSetBytesOpPassword},
    {"Get Magic Tag Config", NfcMagicSceneIdentify, -1},
    {"Set ATS", NfcMagicSceneSetAts, -1},
    {"Set ATQA&SAK", NfcMagicSceneSetBytes, NfcMagicSetBytesOpATQA},
    {"Set Shadow Mode", NfcMagicSceneSetOne, NfcMagicSetOneOpShadow},
    {"Set UID length", NfcMagicSceneSetOne, NfcMagicSetOneOpUidLength},
    {"Switch UltraLight/Classic", NfcMagicSceneSetOne, NfcMagicSetOneOpUlEnable},
    {"Set UltraLight Mode", NfcMagicSceneSetOne, NfcMagicSetOneOpUlMode},
    {"Set # sector", NfcMagicSceneSetBytes, NfcMagicSetBytesOpMaxSectors},
    {"Set block 0 Writeability", NfcMagicSceneSetOne, NfcMagicSetOneOpDirectWrite},
    {"Backdoor Write Block", NfcMagicSceneWriteBlock, -1},
};

void nfc_magic_gen4_scene_start_submenu_callback(void* context, uint32_t index) {
    NfcMagic* nfc_magic = context;
    view_dispatcher_send_custom_event(nfc_magic->view_dispatcher, index);
}

void update_password(NfcMagic* nfc_magic) {
    byte* password = nfc_magic->persisted_state->password;
    char* replace_at = strchr(submenu_items[0].name, ' ') + 1;
    furi_assert(replace_at - submenu_items[0].name < (int)strlen(submenu_items[0].name));
    snprintf(
        replace_at,
        1 + strlen(submenu_items[0].name) - (replace_at - submenu_items[0].name),
        "%02X:%02X:%02X:%02X",
        password[0],
        password[1],
        password[2],
        password[3]);
    
}

void nfc_magic_gen4_scene_start_on_enter(void* context) {
    FURI_LOG_I(TAG, "Start::on_enter");
    NfcMagic* nfc_magic = context;
    submenu_reset(nfc_magic->submenu);
    update_password(nfc_magic);
    Submenu* submenu = nfc_magic->submenu;
    for(size_t i = 0; i < COUNT_OF(submenu_items); ++i) {
        submenu_add_item(
            submenu,
            submenu_items[i].name,
            i,
            nfc_magic_gen4_scene_start_submenu_callback,
            nfc_magic);
    }

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneStart));
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewMenu);
}

bool nfc_magic_gen4_scene_start_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    bool consumed = false;
    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(nfc_magic->scene_manager, NfcMagicSceneStart, event.event);
        if(event.event > COUNT_OF(submenu_items)) {
            FURI_LOG_W(TAG, "Invalid submenu index: %lu", event.event);
            return false;
        }
        SubmenuItem* item = &submenu_items[event.event];
        if(item->scene_id == NfcMagicSceneSetOne) {
            nfc_magic_gen4_set_one_set_operation(item->scene_selector_extra);
        }
        else if (item->scene_id == NfcMagicSceneSetBytes) {
            nfc_magic_gen4_set_bytes_set_operation(item->scene_selector_extra);
        }
        scene_manager_next_scene(nfc_magic->scene_manager, item->scene_id);
        consumed = true;
    }
    return consumed;
}

void nfc_magic_gen4_scene_start_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    submenu_reset(nfc_magic->submenu);
}
