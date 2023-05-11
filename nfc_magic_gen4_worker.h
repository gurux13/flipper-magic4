#pragma once

#include <lib/nfc/nfc_device.h>

typedef struct NfcMagicWorker NfcMagicWorker;

typedef enum {
    NfcMagicWorkerStateReady,

    NfcMagicWorkerStateCheck,
    NfcMagicWorkerStateWrite,
    NfcMagicWorkerStateWipe,
    NfcMagicWorkerStateDebug,

    NfcMagicWorkerStateStop,
} NfcMagicWorkerState;

typedef enum {
    NfcMagicWorkerEventSuccess,
    NfcMagicWorkerEventFail,
    NfcMagicWorkerEventCardDetected,
    NfcMagicWorkerEventNoCardDetected,
    NfcMagicWorkerEventWrongCard,
} NfcMagicWorkerEvent;

typedef bool (*NfcMagicWorkerCallback)(NfcMagicWorkerEvent event, void* context);

NfcMagicWorker* nfc_magic_gen4_worker_alloc();

void nfc_magic_gen4_worker_free(NfcMagicWorker* nfc_magic_gen4_worker);

void nfc_magic_gen4_worker_stop(NfcMagicWorker* nfc_magic_gen4_worker);

void nfc_magic_gen4_worker_start(
    NfcMagicWorker* nfc_magic_gen4_worker,
    NfcMagicWorkerState state,
    NfcDeviceData* dev_data,
    NfcMagicWorkerCallback callback,
    void* context);
