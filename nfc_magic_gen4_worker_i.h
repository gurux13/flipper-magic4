#pragma once

#include <furi.h>

#include "nfc_magic_gen4_worker.h"

struct NfcMagicWorkerActRequest {
    byte command;
    byte payload_size;
    byte payload[32];
};

struct NfcMagicWorker {
    FuriThread* thread;

    NfcDeviceData* dev_data;

    NfcMagicWorkerCallback callback;
    void* context;
    const byte* password;
    NfcMagicWorkerActRequest* act_request;

    NfcMagicWorkerState state;
};


int32_t nfc_magic_gen4_worker_task(void* context);

void nfc_magic_gen4_worker_identify(NfcMagicWorker* nfc_magic_gen4_worker);

void nfc_magic_gen4_worker_write(NfcMagicWorker* nfc_magic_gen4_worker);

void nfc_magic_gen4_worker_wipe(NfcMagicWorker* nfc_magic_gen4_worker);
void nfc_magic_gen4_worker_debug(NfcMagicWorker* nfc_magic_gen4_worker);
void nfc_magic_gen4_worker_act(NfcMagicWorker* nfc_magic_gen4_worker);  
