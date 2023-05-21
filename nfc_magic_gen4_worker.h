#pragma once

#include <lib/nfc/nfc_device.h>
typedef unsigned char byte;

typedef struct NfcMagicWorker NfcMagicWorker;
typedef struct NfcMagicWorkerActRequest NfcMagicWorkerActRequest;

typedef enum {
    NfcMagicWorkerStateReady,

    NfcMagicWorkerStateIdentify,
    NfcMagicWorkerStateAct,
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

extern byte last_received_data[128];

typedef bool (*NfcMagicWorkerCallback)(NfcMagicWorkerEvent event, void* context);

NfcMagicWorker* nfc_magic_gen4_worker_alloc();

void nfc_magic_gen4_worker_free(NfcMagicWorker* nfc_magic_gen4_worker);

void nfc_magic_gen4_worker_stop(NfcMagicWorker* nfc_magic_gen4_worker);

void nfc_magic_gen4_worker_start(
    NfcMagicWorker* nfc_magic_gen4_worker,
    NfcMagicWorkerState state,
    NfcDeviceData* dev_data,
    const byte* password,
    NfcMagicWorkerActRequest* act_request,
    NfcMagicWorkerCallback callback,
    void* context);
