#include "../nfc_magic_gen4_i.h"
#include "./nfc_magic_gen4_scene_hex_input.h"
#include "../nfc_magic_gen4_worker_i.h"
#include "../persisted.h"
#include "../warning.h"
enum {
    NfcMagicSceneAtsStateLength,
    NfcMagicSceneAtsStateAts,
    NfcMagicSceneAtsStateWarning,
    NfcMagicSceneAtsStateCardWait,
};

enum {
    NfcMagicSceneAtsEventByteInputSave,
    NfcMagicSceneAtsEventWorkerSuccess,
    NfcMagicSceneAtsEventWorkerFail,

    NfcMagicSceneAtsEventWarningCancel,
    NfcMagicSceneAtsEventWarningOk,
};

#define TAG "NfcSetAts"

static byte ats_length = 9;
static byte ats[16] = {0x09, 0x78, 0x00, 0x91, 0x02, 0xBD, 0xAC, 0x19, 0x13};
static NfcMagicWorkerActRequest act_request;

static void on_save(void* context) {
    NfcMagic* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, NfcMagicSceneAtsEventByteInputSave);
}
bool nfc_magic_gen4_set_ats_worker_callback(NfcMagicWorkerEvent event, void* context) {
    furi_assert(context);

    NfcMagic* nfc_magic = context;
    switch(event) {
    case NfcMagicWorkerEventSuccess:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneAtsEventWorkerSuccess);
        break;
    case NfcMagicWorkerEventFail:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneAtsEventWorkerFail);
        break;
    default:
        break;
    }

    return true;
}

void nfc_magic_gen4_scene_set_ats_setup_view(NfcMagic* nfc_magic) {
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetAts);
    ByteInput* input = nfc_magic->byte_input;
    if(state == NfcMagicSceneAtsStateLength) {
        byte_input_set_header_text(input, "ATS Length");
        byte_input_set_result_callback(input, on_save, NULL, nfc_magic, &ats_length, 1);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
    } else if(state == NfcMagicSceneAtsStateAts) {
        byte_input_set_header_text(input, "ATS");
        byte_input_set_result_callback(input, on_save, NULL, nfc_magic, ats, ats_length);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
    } else if(state == NfcMagicSceneAtsStateWarning) {
        render_warning(
            nfc_magic,
            "ATS[0] != length\nThis looks wrong.\n\nContinue?",
            NfcMagicSceneAtsEventWarningOk,
            NfcMagicSceneAtsEventWarningCancel);
    } else if(state == NfcMagicSceneAtsStateCardWait) {
        Popup* popup = nfc_magic->popup;
        popup_reset(popup);
        popup_set_icon(nfc_magic->popup, 0, 8, &I_NFC_manual_60x50);
        popup_set_text(
            nfc_magic->popup, "Apply card to\nthe back", 128, 32, AlignRight, AlignCenter);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewPopup);
    }
}

static void nfc_magic_gen4_scene_set_ats_start_write(NfcMagic* nfc_magic) {
    scene_manager_set_scene_state(
        nfc_magic->scene_manager, NfcMagicSceneSetAts, NfcMagicSceneAtsStateCardWait);
    nfc_magic_gen4_blink_start(nfc_magic);
    nfc_magic_gen4_scene_set_ats_setup_view(nfc_magic);
    act_request.command = 0x34;
    act_request.payload_size = ats_length + 1;
    act_request.payload[0] = ats_length;
    memcpy(act_request.payload + 1, ats, ats_length);
    nfc_magic_gen4_worker_start(
        nfc_magic->worker,
        NfcMagicWorkerStateAct,
        &nfc_magic->nfc_dev->dev_data,
        nfc_magic->persisted_state->password,
        &act_request,
        nfc_magic_gen4_set_ats_worker_callback,
        nfc_magic);
}

void nfc_magic_gen4_scene_set_ats_on_enter(void* context) {
    nfc_magic_gen4_scene_set_ats_setup_view((NfcMagic*)context);
}

bool nfc_magic_gen4_scene_set_ats_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetAts);

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicSceneAtsEventByteInputSave) {
            if(state == NfcMagicSceneAtsStateLength) {
                scene_manager_set_scene_state(
                    nfc_magic->scene_manager, NfcMagicSceneSetAts, NfcMagicSceneAtsStateAts);
                nfc_magic_gen4_scene_set_ats_setup_view(context);
            } else if(state == NfcMagicSceneAtsStateAts) {
                if(ats[0] != ats_length) {
                    scene_manager_set_scene_state(
                        nfc_magic->scene_manager,
                        NfcMagicSceneSetAts,
                        NfcMagicSceneAtsStateWarning);
                    nfc_magic_gen4_scene_set_ats_setup_view(context);
                    return true;
                } else {
                    nfc_magic_gen4_scene_set_ats_start_write(nfc_magic);
                }
            } else {
                scene_manager_previous_scene(nfc_magic->scene_manager);
            }
        } else if(event.event == NfcMagicSceneAtsEventWorkerSuccess) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneSuccess);
            return true;
        } else if(event.event == NfcMagicSceneAtsEventWorkerFail) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneWriteFail);
            return true;
        } else if(event.event == NfcMagicSceneAtsEventWarningCancel) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetAts, NfcMagicSceneAtsStateAts);
            nfc_magic_gen4_scene_set_ats_setup_view(context);
            return true;
        } else if(event.event == NfcMagicSceneAtsEventWarningOk) {
            nfc_magic_gen4_scene_set_ats_start_write(nfc_magic);
            return true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        if(state == NfcMagicSceneAtsStateAts) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetAts, NfcMagicSceneAtsStateLength);
            nfc_magic_gen4_scene_set_ats_setup_view(context);
            return true;
        } else if(state == NfcMagicSceneAtsStateWarning) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetAts, NfcMagicSceneAtsStateAts);
            nfc_magic_gen4_scene_set_ats_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            return true;
        } else if(state == NfcMagicSceneAtsStateCardWait) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneSetAts, NfcMagicSceneAtsStateAts);
            nfc_magic_gen4_scene_set_ats_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            return true;
        }
    }
    return false;
}

void nfc_magic_gen4_scene_set_ats_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    scene_manager_set_scene_state(nfc_magic->scene_manager, NfcMagicSceneSetAts, 0);
    nfc_magic_gen4_blink_stop(nfc_magic);
    nfc_magic_gen4_worker_stop(nfc_magic->worker);
    // Clear view
    // byte_input_reset(nfc_magic->byte_input);
    widget_reset(nfc_magic->widget);
    popup_reset(nfc_magic->popup);
}
