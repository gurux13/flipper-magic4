#include "../nfc_magic_gen4_i.h"
#include "./nfc_magic_gen4_scene_hex_input.h"
#include "../nfc_magic_gen4_worker_i.h"
#include "../persisted.h"
#include "../warning.h"
enum {
    NfcMagicSceneWriteBlockStateBlockId,
    NfcMagicSceneWriteBlockStateWriteBlock,
    NfcMagicSceneWriteBlockStateWarning,
    NfcMagicSceneWriteBlockStateCardWait,
};

enum {
    NfcMagicSceneWriteBlockEventByteInputSave,
    NfcMagicSceneWriteBlockEventWorkerSuccess,
    NfcMagicSceneWriteBlockEventWorkerFail,

    NfcMagicSceneWriteBlockEventWarningCancel,
    NfcMagicSceneWriteBlockEventWarningOk,
};

#define TAG "NfcWriteBlock"

static byte block_id = 9;
static byte block_data[16] = {0x09, 0x78, 0x00, 0x91, 0x02, 0xBD, 0xAC, 0x19, 0x13};
static NfcMagicWorkerActRequest act_request;

static void on_save(void* context) {
    NfcMagic* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, NfcMagicSceneWriteBlockEventByteInputSave);
}
bool nfc_magic_gen4_write_block_worker_callback(NfcMagicWorkerEvent event, void* context) {
    furi_assert(context);

    NfcMagic* nfc_magic = context;
    switch(event) {
    case NfcMagicWorkerEventSuccess:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneWriteBlockEventWorkerSuccess);
        break;
    case NfcMagicWorkerEventFail:
        view_dispatcher_send_custom_event(
            nfc_magic->view_dispatcher, NfcMagicSceneWriteBlockEventWorkerFail);
        break;
    default:
        break;
    }

    return true;
}

void nfc_magic_gen4_scene_write_block_setup_view(NfcMagic* nfc_magic) {
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneWriteBlock);
    ByteInput* input = nfc_magic->byte_input;
    if(state == NfcMagicSceneWriteBlockStateBlockId) {
        byte_input_set_header_text(input, "Block ID");
        byte_input_set_result_callback(input, on_save, NULL, nfc_magic, &block_id, 1);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
    } else if(state == NfcMagicSceneWriteBlockStateWriteBlock) {
        byte_input_set_header_text(input, "Block data");
        byte_input_set_result_callback(input, on_save, NULL, nfc_magic, block_data, 16);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewByteInput);
    } else if(state == NfcMagicSceneWriteBlockStateWarning) {
        render_warning(
            nfc_magic,
            "Writing to block 0! Continue?",
            NfcMagicSceneWriteBlockEventWarningOk,
            NfcMagicSceneWriteBlockEventWarningCancel);
    } else if(state == NfcMagicSceneWriteBlockStateCardWait) {
        Popup* popup = nfc_magic->popup;
        popup_reset(popup);
        popup_set_icon(nfc_magic->popup, 0, 8, &I_NFC_manual_60x50);
        popup_set_text(
            nfc_magic->popup, "Apply card to\nthe back", 128, 32, AlignRight, AlignCenter);
        view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewPopup);
    }
}

static void nfc_magic_gen4_scene_write_block_start_write(NfcMagic* nfc_magic) {
    scene_manager_set_scene_state(
        nfc_magic->scene_manager, NfcMagicSceneWriteBlock, NfcMagicSceneWriteBlockStateCardWait);
    nfc_magic_gen4_blink_start(nfc_magic);
    nfc_magic_gen4_scene_write_block_setup_view(nfc_magic);
    act_request.command = 0xCD;
    act_request.payload_size = 17;
    act_request.payload[0] = block_id;
    memcpy(act_request.payload + 1, block_data, 16);
    nfc_magic_gen4_worker_start(
        nfc_magic->worker,
        NfcMagicWorkerStateAct,
        &nfc_magic->nfc_dev->dev_data,
        nfc_magic->persisted_state->password,
        &act_request,
        nfc_magic_gen4_write_block_worker_callback,
        nfc_magic);
}

void nfc_magic_gen4_scene_write_block_on_enter(void* context) {
    nfc_magic_gen4_scene_write_block_setup_view((NfcMagic*)context);
}

bool nfc_magic_gen4_scene_write_block_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    int state = scene_manager_get_scene_state(nfc_magic->scene_manager, NfcMagicSceneWriteBlock);

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcMagicSceneWriteBlockEventByteInputSave) {
            if(state == NfcMagicSceneWriteBlockStateBlockId) {
                scene_manager_set_scene_state(
                    nfc_magic->scene_manager, NfcMagicSceneWriteBlock, NfcMagicSceneWriteBlockStateWriteBlock);
                nfc_magic_gen4_scene_write_block_setup_view(context);
            } else if(state == NfcMagicSceneWriteBlockStateWriteBlock) {
                if(block_id == 0) {
                    scene_manager_set_scene_state(
                        nfc_magic->scene_manager,
                        NfcMagicSceneWriteBlock,
                        NfcMagicSceneWriteBlockStateWarning);
                    nfc_magic_gen4_scene_write_block_setup_view(context);
                    return true;
                } else {
                    nfc_magic_gen4_scene_write_block_start_write(nfc_magic);
                }
            } else {
                scene_manager_previous_scene(nfc_magic->scene_manager);
            }
        } else if(event.event == NfcMagicSceneWriteBlockEventWorkerSuccess) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneSuccess);
            return true;
        } else if(event.event == NfcMagicSceneWriteBlockEventWorkerFail) {
            scene_manager_search_and_switch_to_another_scene(
                nfc_magic->scene_manager, NfcMagicSceneWriteFail);
            return true;
        } else if(event.event == NfcMagicSceneWriteBlockEventWarningCancel) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneWriteBlock, NfcMagicSceneWriteBlockStateWriteBlock);
            nfc_magic_gen4_scene_write_block_setup_view(context);
            return true;
        } else if(event.event == NfcMagicSceneWriteBlockEventWarningOk) {
            nfc_magic_gen4_scene_write_block_start_write(nfc_magic);
            return true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        if(state == NfcMagicSceneWriteBlockStateWriteBlock) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneWriteBlock, NfcMagicSceneWriteBlockStateBlockId);
            nfc_magic_gen4_scene_write_block_setup_view(context);
            return true;
        } else if(state == NfcMagicSceneWriteBlockStateWarning) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneWriteBlock, NfcMagicSceneWriteBlockStateWriteBlock);
            nfc_magic_gen4_scene_write_block_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            return true;
        } else if(state == NfcMagicSceneWriteBlockStateCardWait) {
            scene_manager_set_scene_state(
                nfc_magic->scene_manager, NfcMagicSceneWriteBlock, NfcMagicSceneWriteBlockStateWriteBlock);
            nfc_magic_gen4_scene_write_block_setup_view(context);
            nfc_magic_gen4_blink_stop(nfc_magic);
            return true;
        }
    }
    return false;
}

void nfc_magic_gen4_scene_write_block_on_exit(void* context) {
    NfcMagic* nfc_magic = context;
    scene_manager_set_scene_state(nfc_magic->scene_manager, NfcMagicSceneWriteBlock, 0);
    nfc_magic_gen4_blink_stop(nfc_magic);
    nfc_magic_gen4_worker_stop(nfc_magic->worker);
    // Clear view
    widget_reset(nfc_magic->widget);
    popup_reset(nfc_magic->popup);
}
