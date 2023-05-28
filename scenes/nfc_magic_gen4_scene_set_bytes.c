#include "../nfc_magic_gen4_i.h"
#include "./nfc_magic_gen4_scene_hex_input.h"
#include "../nfc_magic_gen4_worker_i.h"
#include "../nfc_magic_gen4_set_bytes.h"
#include "../persisted.h"
#include "../warning.h"
#include "../common.h"
enum {
    NfcMagicSceneSetBytesStateEnterValue,
    NfcMagicSceneSetBytesStateWarning,
    NfcMagicSceneSetBytesStateCardWait,
};

enum {
    NfcMagicSceneSetBytesEventBytesDone,
    NfcMagicSceneSetBytesEventWorkerSuccess,
    NfcMagicSceneSetBytesEventWorkerFail,

    NfcMagicSceneSetBytesEventWarningCancel,
    NfcMagicSceneSetBytesEventWarningOk,
};

typedef struct {
    byte opcode;
    const char* name;
    int byte_length;
} SetBytesOptions;

static SetBytesOptions all_options[] = {
    {NfcMagicSetBytesOpATQA, "ATQA(2b) SAK(1b)", 3},
    {NfcMagicSetBytesOpMaxSectors, "Max r/w sectors", 1},
    {NfcMagicSetBytesOpPassword, "Password", 4}};

#define MAX_SIMULTANEOUS_WARNINGS 4
static const char* warnings[MAX_SIMULTANEOUS_WARNINGS + 1];
static int current_warning_idx = 0;

#define IS_POWER_OF_2(x) ((x) && ((((x)-1) & (x)) == 0))

#define TAG "NfcSetBytes"
#define MAX_BYTES 4
static byte requested_operation_idx = 0;
static byte entered_bytes[MAX_BYTES] = {0};
void nfc_magic_gen4_set_bytes_set_operation(byte operation) {
    requested_operation_idx = 0;
    for(size_t i = 0; i < COUNT_OF(all_options); ++i) {
        if(all_options[i].opcode == operation) {
            requested_operation_idx = i;
            break;
        }
    }
}
static void on_operation_requested(NfcMagic* nfc_magic) {
    switch(all_options[requested_operation_idx].opcode) {
    case NfcMagicSetBytesOpATQA:
        entered_bytes[0] = 0x00;
        entered_bytes[1] = 0x44;
        entered_bytes[2] = 0x08;
        break;
    case NfcMagicSetBytesOpMaxSectors:
        entered_bytes[0] = 0x3F;
        break;
    case NfcMagicSetBytesOpPassword:
        memcpy(entered_bytes, nfc_magic->persisted_state->password, 4);
        break;
    }
}

static void fill_warnings() {
    static const char atqa_enable_ats[] = "SAK bit 6 is set, make sure ATS is enabled! Continue?";
    static const char atqa_weird_anticoll[] =
        "ATQA byte 2 doesn't have anticollision specified! Continue?";
    static const char atqa_invalid_sak[] =
        "SAK bit 3 is set, this enables anticol cascade 2 the card does not support, CARD MAY BECOME UNREADABLE! Ignore?";
    static const char password_warning[] =
        "You are changing the password. If password is lost, card will be unrecoverable! Continue?";
    int warning_id = 0;
    switch(all_options[requested_operation_idx].opcode) {
    case NfcMagicSetBytesOpATQA: {
        byte sak = entered_bytes[2];
        if(sak & (1 << 2)) {
            warnings[warning_id++] = atqa_invalid_sak;
        }
        if(sak & (1 << 5)) {
            warnings[warning_id++] = atqa_enable_ats;
        }
        if(!IS_POWER_OF_2(entered_bytes[1] & 31)) {
            warnings[warning_id++] = atqa_weird_anticoll;
        }
        break;
    }
    case NfcMagicSetBytesOpPassword:
        warnings[warning_id++] = password_warning;
        break;
    }
    warnings[warning_id] = NULL;
}

void on_success(NfcMagic* nfc_magic) {
    if(all_options[requested_operation_idx].opcode == NfcMagicSetBytesOpPassword) {
        memcpy(nfc_magic->persisted_state->password, entered_bytes, 4);
        SAVE_MAGIC4_SETTINGS(nfc_magic->persisted_state);
    }
}

static NfcMagicWorkerActRequest act_request;

static void on_byes_saved(void* context) {
    NfcMagic* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, NfcMagicSceneSetBytesEventBytesDone);
}
bool nfc_magic_gen4_set_bytes_worker_callback(NfcMagicWorkerEvent event, void* context) {
    furi_assert(context);

    NfcMagic* nfc_magic = context;
    switch(event) {
    case NfcMagicWorkerEventSuccess:
        on_success(nfc_magic);
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneSetBytesEventWorkerSuccess);
        break;
    case NfcMagicWorkerEventFail:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneSetBytesEventWorkerFail);
        break;
    default:
        break;
    }

    return true;
}

static void render_bytes(NfcMagic* nfc_magic) {
    ByteInput* byte_input = nfc_magic->byte_input;
    byte_input_set_header_text(byte_input, all_options[requested_operation_idx].name);
    byte_input_set_result_callback(
        byte_input,
        on_byes_saved,
        NULL,
        nfc_magic,
        entered_bytes,
        all_options[requested_operation_idx].byte_length);
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
}

void nfc_magic_gen4_scene_set_bytes_setup_view(NfcMagic* nfc_magic) {
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetBytes);
    if(state == NfcMagicSceneSetBytesStateEnterValue) {
        render_bytes(nfc_magic);
    } else if(state == NfcMagicSceneSetBytesStateWarning) {
        render_warning(
            nfc_magic,
            warnings[current_warning_idx],
            NfcMagicSceneSetBytesEventWarningOk,
            NfcMagicSceneSetBytesEventWarningCancel);
    } else if(state == NfcMagicSceneSetBytesStateCardWait) {
        Popup* popup = nfc_magic->popup;
        popup_reset(popup);
        popup_set_icon(nfc_magic->popup, 0, 8, &I_NFC_manual_60x50);
        popup_set_text(
            nfc_magic->popup, "Apply card to\nthe back", 128, 32, AlignRight, AlignCenter);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewPopup);
    }
}

static void nfc_magic_gen4_scene_set_bytes_start_write(NfcMagic* nfc_magic) {
    FURI_LOG_W(TAG, "Starting worker!");
    scene_manager_set_scene_state(
        nfc_magic->scene_manager, NfcMagicSceneSetBytes, NfcMagicSceneSetBytesStateCardWait);
    nfc_magic_gen4_blink_start(nfc_magic);
    nfc_magic_gen4_scene_set_bytes_setup_view(nfc_magic);
    act_request.command = all_options[requested_operation_idx].opcode;
    act_request.payload_size = all_options[requested_operation_idx].byte_length;
    memcpy(act_request.payload, entered_bytes, act_request.payload_size);
    if (act_request.command == NfcMagicSetBytesOpATQA) {
        act_request.payload[0] ^= act_request.payload[1];
        act_request.payload[1] ^= act_request.payload[0];
        act_request.payload[0] ^= act_request.payload[1];
    }
    nfc_magic_gen4_worker_start(
        nfc_magic->worker,
        NfcMagicWorkerStateAct,
        &nfc_magic->nfc_dev->dev_data,
        nfc_magic->persisted_state->password,
        &act_request,
        nfc_magic_gen4_set_bytes_worker_callback,
        nfc_magic);
}

void nfc_magic_gen4_scene_set_bytes_on_enter(void* context) {
    on_operation_requested((NfcMagic*) context);
    nfc_magic_gen4_scene_set_bytes_setup_view((NfcMagic*)context);
}

bool nfc_magic_gen4_scene_set_bytes_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetBytes);

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicSceneSetBytesEventBytesDone) {
            fill_warnings();
            current_warning_idx = 0;
            if(warnings[0] != NULL) {
                scene_manager_set_scene_state(
                    nfc_magic->scene_manager,
                    NfcMagicSceneSetBytes,
                    NfcMagicSceneSetBytesStateWarning);
                nfc_magic_gen4_scene_set_bytes_setup_view(context);
            } else {
                nfc_magic_gen4_scene_set_bytes_start_write(nfc_magic);
            }
        } else if(event.event == NfcMagicSceneSetBytesEventWorkerSuccess) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneSuccess);
            return true;
        } else if(event.event == NfcMagicSceneSetBytesEventWorkerFail) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneWriteFail);
            return true;
        } else if(event.event == NfcMagicSceneSetBytesEventWarningCancel) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager,
                NfcMagicSceneSetBytes,
                NfcMagicSceneSetBytesStateEnterValue);
            nfc_magic_gen4_scene_set_bytes_setup_view(context);
            return true;
        } else if(event.event == NfcMagicSceneSetBytesEventWarningOk) {
            FURI_LOG_W(TAG, "Got Warning_OK event!");
            ++current_warning_idx;
            if(warnings[current_warning_idx] != NULL) {
                nfc_magic_gen4_scene_set_bytes_setup_view(context);
            } else {
                nfc_magic_gen4_scene_set_bytes_start_write(nfc_magic);
            }
            return true;
        }
        return true;
    } else if(event.type == SceneManagerEventTypeBack) {
        if(state == NfcMagicSceneSetBytesStateWarning) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager,
                NfcMagicSceneSetBytes,
                NfcMagicSceneSetBytesStateEnterValue);
            nfc_magic_gen4_scene_set_bytes_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            nfc_magic_gen4_worker_stop(nfc_magic->worker);

            return true;
        } else if(state == NfcMagicSceneSetBytesStateCardWait) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager,
                NfcMagicSceneSetBytes,
                NfcMagicSceneSetBytesStateEnterValue);
            nfc_magic_gen4_scene_set_bytes_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            nfc_magic_gen4_worker_stop(nfc_magic->worker);

            return true;
        }
    }
    return false;
}

void nfc_magic_gen4_scene_set_bytes_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    scene_manager_set_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetBytes, 0);
    nfc_magic_gen4_blink_stop(nfc_magic);
    nfc_magic_gen4_worker_stop(nfc_magic->worker);
    // Clear view
    // byte_input_reset(nfc_magic->byte_input);
    widget_reset(nfc_magic->widget);
    popup_reset(nfc_magic->popup);
}
