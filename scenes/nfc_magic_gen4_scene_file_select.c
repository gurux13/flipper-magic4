#include "../nfc_magic_gen4_i.h"

static bool nfc_magic_gen4_scene_file_select_is_file_suitable(NfcDevice* nfc_dev) {
    return (nfc_dev->format == NfcDeviceSaveFormatMifareClassic) &&
           (nfc_dev->dev_data.mf_classic_data.type == MfClassicType1k) &&
           (nfc_dev->dev_data.nfc_data.uid_len == 4);
}

void nfc_magic_gen4_scene_file_select_on_enter(void* context) {
    NfcMagic* nfc_magic = context;
    // Process file_select return
    nfc_device_set_loading_callback(nfc_magic->nfc_dev, nfc_magic_gen4_show_loading_popup, nfc_magic);

    if(!furi_string_size(nfc_magic->nfc_dev->load_path)) {
        furi_string_set_str(nfc_magic->nfc_dev->load_path, NFC_APP_FOLDER);
    }

    if(nfc_file_select(nfc_magic->nfc_dev)) {
        if(nfc_magic_gen4_scene_file_select_is_file_suitable(nfc_magic->nfc_dev)) {
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicSceneWriteConfirm);
        } else {
            scene_manager_next_scene(nfc_magic->scene_manager, NfcMagicSceneWrongCard);
        }
    } else {
        scene_manager_previous_scene(nfc_magic->scene_manager);
    }
}

bool nfc_magic_gen4_scene_file_select_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nfc_magic_gen4_scene_file_select_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    nfc_device_set_loading_callback(nfc_magic->nfc_dev, NULL, nfc_magic);
}
