#include "nfc_magic_gen4_worker_i.h"

#include "lib/magic_gen4/magic_gen4.h"

#define TAG "NfcMagicWorker"

byte last_received_data[128] = {0};

static void nfc_magic_gen4_worker_change_state(
    NfcMagicWorker* nfc_magic_gen4_worker,
    NfcMagicWorkerState state) {
    furi_assert(nfc_magic_gen4_worker);

    nfc_magic_gen4_worker->state = state;
}

NfcMagicWorker* nfc_magic_gen4_worker_alloc() {
    NfcMagicWorker* nfc_magic_gen4_worker = malloc(sizeof(NfcMagicWorker));

    // Worker thread attributes
    nfc_magic_gen4_worker->thread = furi_thread_alloc_ex(
        "NfcMagicWorker", 8192, nfc_magic_gen4_worker_task, nfc_magic_gen4_worker);

    nfc_magic_gen4_worker->callback = NULL;
    nfc_magic_gen4_worker->context = NULL;

    nfc_magic_gen4_worker_change_state(nfc_magic_gen4_worker, NfcMagicWorkerStateReady);

    return nfc_magic_gen4_worker;
}

void nfc_magic_gen4_worker_free(NfcMagicWorker* nfc_magic_gen4_worker) {
    furi_assert(nfc_magic_gen4_worker);

    furi_thread_free(nfc_magic_gen4_worker->thread);
    free(nfc_magic_gen4_worker);
}

void nfc_magic_gen4_worker_stop(NfcMagicWorker* nfc_magic_gen4_worker) {
    furi_assert(nfc_magic_gen4_worker);

    nfc_magic_gen4_worker_change_state(nfc_magic_gen4_worker, NfcMagicWorkerStateStop);
    furi_thread_join(nfc_magic_gen4_worker->thread);
}

void nfc_magic_gen4_worker_start(
    NfcMagicWorker* nfc_magic_gen4_worker,
    NfcMagicWorkerState state,
    NfcDeviceData* dev_data,
    const byte* password,
    NfcMagicWorkerActRequest* act_request,
    NfcMagicWorkerCallback callback,
    void* context) {
    furi_assert(nfc_magic_gen4_worker);
    furi_assert(dev_data);

    furi_hal_nfc_deinit();
    furi_hal_nfc_init();

    nfc_magic_gen4_worker->callback = callback;
    nfc_magic_gen4_worker->context = context;
    nfc_magic_gen4_worker->dev_data = dev_data;
    nfc_magic_gen4_worker->password = password;
    nfc_magic_gen4_worker->act_request = act_request;
    nfc_magic_gen4_worker_change_state(nfc_magic_gen4_worker, state);
    furi_thread_start(nfc_magic_gen4_worker->thread);
}

int32_t nfc_magic_gen4_worker_task(void* context) {
    NfcMagicWorker* nfc_magic_gen4_worker = context;

    if(nfc_magic_gen4_worker->state == NfcMagicWorkerStateIdentify) {
        nfc_magic_gen4_worker_identify(nfc_magic_gen4_worker);
    } else if(nfc_magic_gen4_worker->state == NfcMagicWorkerStateWrite) {
        nfc_magic_gen4_worker_write(nfc_magic_gen4_worker);
    } else if(nfc_magic_gen4_worker->state == NfcMagicWorkerStateAct) {
        nfc_magic_gen4_worker_act(nfc_magic_gen4_worker);
    } else if(nfc_magic_gen4_worker->state == NfcMagicWorkerStateWipe) {
        nfc_magic_gen4_worker_wipe(nfc_magic_gen4_worker);
    } else if(nfc_magic_gen4_worker->state == NfcMagicWorkerStateDebug) {
        nfc_magic_gen4_worker_debug(nfc_magic_gen4_worker);
    }

    nfc_magic_gen4_worker_change_state(nfc_magic_gen4_worker, NfcMagicWorkerStateReady);

    return 0;
}

void nfc_magic_gen4_worker_write(NfcMagicWorker* nfc_magic_gen4_worker) {
    bool card_found_notified = false;
    FuriHalNfcDevData nfc_data = {};
    MfClassicData* src_data = &nfc_magic_gen4_worker->dev_data->mf_classic_data;

    while(nfc_magic_gen4_worker->state == NfcMagicWorkerStateWrite) {
        if(furi_hal_nfc_detect(&nfc_data, 200)) {
            if(!card_found_notified) {
                nfc_magic_gen4_worker->callback(
                    NfcMagicWorkerEventCardDetected, nfc_magic_gen4_worker->context);
                card_found_notified = true;
            }
            furi_hal_nfc_sleep();
            if(!magic_wupa()) {
                FURI_LOG_E(TAG, "No card response to WUPA (not a magic card)");
                nfc_magic_gen4_worker->callback(
                    NfcMagicWorkerEventWrongCard, nfc_magic_gen4_worker->context);
                break;
            }
            furi_hal_nfc_sleep();
        }
        if(magic_wupa()) {
            if(!magic_data_access_cmd()) {
                FURI_LOG_E(TAG, "No card response to data access command (not a magic card)");
                nfc_magic_gen4_worker->callback(
                    NfcMagicWorkerEventWrongCard, nfc_magic_gen4_worker->context);
                break;
            }
            for(size_t i = 0; i < 64; i++) {
                FURI_LOG_D(TAG, "Writing block %d", i);
                if(!magic_write_blk(i, &src_data->block[i])) {
                    FURI_LOG_E(TAG, "Failed to write %d block", i);
                    nfc_magic_gen4_worker->callback(
                        NfcMagicWorkerEventFail, nfc_magic_gen4_worker->context);
                    break;
                }
            }
            nfc_magic_gen4_worker->callback(
                NfcMagicWorkerEventSuccess, nfc_magic_gen4_worker->context);
            break;
        } else {
            if(card_found_notified) {
                nfc_magic_gen4_worker->callback(
                    NfcMagicWorkerEventNoCardDetected, nfc_magic_gen4_worker->context);
                card_found_notified = false;
            }
        }
        furi_delay_ms(300);
    }
    magic_deactivate();
}

void nfc_magic_gen4_worker_identify(NfcMagicWorker* nfc_magic_gen4_worker) {
    bool card_found_notified = false;

    while(nfc_magic_gen4_worker->state == NfcMagicWorkerStateIdentify) {
        if(execute_magic_command(
               nfc_magic_gen4_worker->password,
               0xC6,
               NULL,
               0,
               nfc_magic_gen4_worker->dev_data->reader_data.data,
               sizeof(nfc_magic_gen4_worker->dev_data->reader_data.data))) {
            if(!card_found_notified) {
                nfc_magic_gen4_worker->callback(
                    NfcMagicWorkerEventCardDetected, nfc_magic_gen4_worker->context);
                card_found_notified = true;
            }

            nfc_magic_gen4_worker->callback(
                NfcMagicWorkerEventSuccess, nfc_magic_gen4_worker->context);
            break;
        } else {
            if(card_found_notified) {
                nfc_magic_gen4_worker->callback(
                    NfcMagicWorkerEventNoCardDetected, nfc_magic_gen4_worker->context);
                card_found_notified = false;
            }
        }
        furi_delay_ms(100);
    }
    magic_deactivate();
}

void nfc_magic_gen4_worker_act(NfcMagicWorker* nfc_magic_gen4_worker) {
    if (nfc_magic_gen4_worker->act_request == NULL) {
        nfc_magic_gen4_worker->callback(
                NfcMagicWorkerEventFail, nfc_magic_gen4_worker->context);
        return;
    }
    NfcMagicWorkerActRequest* request = nfc_magic_gen4_worker->act_request;
    while(nfc_magic_gen4_worker->state == NfcMagicWorkerStateAct) {
        if(execute_magic_command(
               nfc_magic_gen4_worker->password,
               request->command,
               request->payload,
               request->payload_size,
               nfc_magic_gen4_worker->dev_data->reader_data.data,
               sizeof(nfc_magic_gen4_worker->dev_data->reader_data.data))) {
            nfc_magic_gen4_worker->callback(
                NfcMagicWorkerEventSuccess, nfc_magic_gen4_worker->context);
            break;
        }
        furi_delay_ms(100);
    }
    magic_deactivate();
}

void nfc_magic_gen4_worker_wipe(NfcMagicWorker* nfc_magic_gen4_worker) {
    FURI_LOG_E("MAGIC", "IN WIPE");

    magic_test();
    return;
    MfClassicBlock block;
    memset(&block, 0, sizeof(MfClassicBlock));
    block.value[0] = 0x01;
    block.value[1] = 0x02;
    block.value[2] = 0x03;
    block.value[3] = 0x04;
    block.value[4] = 0x04;
    block.value[5] = 0x08;
    block.value[6] = 0x04;

    while(nfc_magic_gen4_worker->state == NfcMagicWorkerStateWipe) {
        magic_deactivate();
        furi_delay_ms(300);
        if(!magic_wupa()) continue;
        if(!magic_wipe()) continue;
        if(!magic_data_access_cmd()) continue;
        if(!magic_write_blk(0, &block)) continue;
        nfc_magic_gen4_worker->callback(
            NfcMagicWorkerEventSuccess, nfc_magic_gen4_worker->context);
        break;
    }
    magic_deactivate();
}

void nfc_magic_gen4_worker_debug(NfcMagicWorker* nfc_magic_gen4_worker) {
    if(!nfc_magic_gen4_worker) {
        return;
    }
    magic_test();
}