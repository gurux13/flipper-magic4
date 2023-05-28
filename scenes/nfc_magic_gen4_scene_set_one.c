#include "../nfc_magic_gen4_i.h"
#include "./nfc_magic_gen4_scene_hex_input.h"
#include "../nfc_magic_gen4_worker_i.h"
#include "../persisted.h"
#include "../warning.h"
enum {
    NfcMagicSceneOneStateSelectValue,
    NfcMagicSceneOneStateWarning,
    NfcMagicSceneOneStateCardWait,
};

enum {
    NfcMagicSceneOneEventOptionSelected,
    NfcMagicSceneOneEventWorkerSuccess,
    NfcMagicSceneOneEventWorkerFail,

    NfcMagicSceneOneEventWarningCancel,
    NfcMagicSceneOneEventWarningOk,
};
#define MAX_NUMBER_OF_OPTIONS 4
typedef struct {
    byte value;
    const char* name;
} Option;
typedef struct {
    byte opcode;
    const char* name;
    bool is_dangerous;
    size_t num_options;
    Option options[MAX_NUMBER_OF_OPTIONS];
} SetOneOptions;

static SetOneOptions all_options[] = {
    {0x32, "Shadow", true, 4, {{0, "Pre-write"}, {1, "Restore"}, {2, "Disable"}, {3, "H/S mode"}}},
    {0x68, "UID Length", false, 3, {{0, "4 bytes"}, {1, "7 bytes"}, {2, "10 bytes"}}},
    {0x69, "UltraLight", false, 2, {{0, "Mifare Classic"}, {1, "UltraLight/NTAG"}}},
    {0x6A, "UltraLight Mode", false, 3, {{0, "4 bytes"}, {1, "7 bytes"}, {2, "10 bytes"}}},
    {0xCF, "Block 0 Writeable", false, 3, {{0, "Writeable"}, {1, "ReadOnly"}, {2, "Default"}}},
};



#define TAG "NfcSetOne"

static byte requested_operation_idx = 0;
static byte selected_value = 0;
void nfc_magic_gen4_set_one_set_operation(byte operation) {
    requested_operation_idx = 0;
    for(size_t i = 0; i < sizeof(all_options) / sizeof(all_options[0]); ++i) {
        if(all_options[i].opcode == operation) {
            requested_operation_idx = i;
            break;
        }
    }
}

static NfcMagicWorkerActRequest act_request;

static void on_option_selected(void* context, uint32_t index) {
    NfcMagic* app = context;
    selected_value = index;
    view_dispatcher_send_custom_event(app->view_dispatcher, NfcMagicSceneOneEventOptionSelected);
}
bool nfc_magic_gen4_set_one_worker_callback(NfcMagicWorkerEvent event, void* context) {
    furi_assert(context);

    NfcMagic* nfc_magic = context;
    switch(event) {
    case NfcMagicWorkerEventSuccess:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneOneEventWorkerSuccess);
        break;
    case NfcMagicWorkerEventFail:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneOneEventWorkerFail);
        break;
    default:
        break;
    }

    return true;
}

static void render_submenu(NfcMagic* nfc_magic) {
    Submenu* submenu = nfc_magic->submenu;
    submenu_reset(submenu);
    for(size_t i = 0; i < all_options[requested_operation_idx].num_options; ++i) {
        submenu_add_item(
            submenu,
            all_options[requested_operation_idx].options[i].name,
            i,
            on_option_selected,
            nfc_magic);
    }
    submenu_set_selected_item(submenu, 0);
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewMenu);
}

void nfc_magic_gen4_scene_set_one_setup_view(NfcMagic* nfc_magic) {
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetOne);
    if(state == NfcMagicSceneOneStateSelectValue) {
        render_submenu(nfc_magic);
    } else if(state == NfcMagicSceneOneStateWarning) {
        render_warning(
            nfc_magic,
            "This is a potentially dangerous operation. Continue?",
            NfcMagicSceneOneEventWarningOk,
            NfcMagicSceneOneEventWarningCancel);
    } else if(state == NfcMagicSceneOneStateCardWait) {
        Popup* popup = nfc_magic->popup;
        popup_reset(popup);
        popup_set_icon(nfc_magic->popup, 0, 8, &I_NFC_manual_60x50);
        popup_set_text(
            nfc_magic->popup, "Apply card to\nthe back", 128, 32, AlignRight, AlignCenter);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewPopup);
    }
}

static void nfc_magic_gen4_scene_set_one_start_write(NfcMagic* nfc_magic) {
    FURI_LOG_W(TAG, "Starting worker!");
    scene_manager_set_scene_state(
        nfc_magic->scene_manager, NfcMagicSceneSetOne, NfcMagicSceneOneStateCardWait);
    nfc_magic_gen4_blink_start(nfc_magic);
    nfc_magic_gen4_scene_set_one_setup_view(nfc_magic);
    act_request.command = all_options[requested_operation_idx].opcode;
    act_request.payload_size = 1;
    act_request.payload[0] = all_options[requested_operation_idx].options[selected_value].value;
    nfc_magic_gen4_worker_start(
        nfc_magic->worker,
        NfcMagicWorkerStateAct,
        &nfc_magic->nfc_dev->dev_data,
        nfc_magic->persisted_state->password,
        &act_request,
        nfc_magic_gen4_set_one_worker_callback,
        nfc_magic);
}

void nfc_magic_gen4_scene_set_one_on_enter(void* context) {
    nfc_magic_gen4_scene_set_one_setup_view((NfcMagic*)context);
}

bool nfc_magic_gen4_scene_set_one_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetOne);

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicSceneOneEventOptionSelected) {
            if (all_options[requested_operation_idx].is_dangerous) {
                    scene_manager_set_scene_state(
                        nfc_magic->scene_manager,
                        NfcMagicSceneSetOne,
                        NfcMagicSceneOneStateWarning);
                    nfc_magic_gen4_scene_set_one_setup_view(context);                
            } else {
                nfc_magic_gen4_scene_set_one_start_write(nfc_magic);
            }
        } else if(event.event == NfcMagicSceneOneEventWorkerSuccess) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneSuccess);
            return true;
        } else if(event.event == NfcMagicSceneOneEventWorkerFail) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneWriteFail);
            return true;
        } else if(event.event == NfcMagicSceneOneEventWarningCancel) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetOne, NfcMagicSceneOneStateSelectValue);
            nfc_magic_gen4_scene_set_one_setup_view(context);
            return true;
        } else if(event.event == NfcMagicSceneOneEventWarningOk) {
            FURI_LOG_W(TAG, "Got Warning_OK event!");
            nfc_magic_gen4_scene_set_one_start_write(nfc_magic);
            return true;
        }
        return true;
    } else if(event.type == SceneManagerEventTypeBack) {
        if(state == NfcMagicSceneOneStateWarning) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetOne, NfcMagicSceneOneStateSelectValue);
            nfc_magic_gen4_scene_set_one_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            nfc_magic_gen4_worker_stop(nfc_magic->worker);

            return true;
        } else if(state == NfcMagicSceneOneStateCardWait) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetOne, NfcMagicSceneOneStateSelectValue);
            nfc_magic_gen4_scene_set_one_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            nfc_magic_gen4_worker_stop(nfc_magic->worker);

            return true;
        }
    }
    return false;
}

void nfc_magic_gen4_scene_set_one_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    scene_manager_set_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetOne, 0);
    nfc_magic_gen4_blink_stop(nfc_magic);
    nfc_magic_gen4_worker_stop(nfc_magic->worker);
    // Clear view
    // byte_input_reset(nfc_magic->byte_input);
    widget_reset(nfc_magic->widget);
    popup_reset(nfc_magic->popup);
    submenu_reset(nfc_magic->submenu);
}
