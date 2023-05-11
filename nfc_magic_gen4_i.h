#pragma once

#include "nfc_magic_gen4.h"
#include "nfc_magic_gen4_worker.h"

#include "lib/magic_gen4/magic_gen4.h"

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <notification/notification_messages.h>

#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/loading.h>
#include <gui/modules/text_input.h>
#include <gui/modules/widget.h>
#include <gui/modules/byte_input.h>

#include <input/input.h>

#include "scenes/nfc_magic_gen4_scene.h"
#include "persisted.h"

#include <storage/storage.h>
#include <lib/toolbox/path.h>

#include <lib/nfc/nfc_device.h>
#include "nfc_magic_gen4_icons.h"

#define NFC_APP_FOLDER ANY_PATH("nfc")
typedef unsigned char byte;

enum NfcMagicCustomEvent {
    // Reserve first 100 events for button types and indexes, starting from 0
    NfcMagicCustomEventReserved = 100,

    NfcMagicCustomEventViewExit,
    NfcMagicCustomEventWorkerExit,
    NfcMagicCustomEventByteInputDone,
    NfcMagicCustomEventTextInputDone,
};

struct NfcMagic {
    NfcMagicWorker* worker;
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notifications;
    SceneManager* scene_manager;
    // NfcMagicDevice* dev;
    NfcDevice* nfc_dev;

    FuriString* text_box_store;

    // Common Views
    Submenu* submenu;
    Popup* popup;
    Loading* loading;
    TextInput* text_input;
    Widget* widget;
    ByteInput* byte_input;
    NfcMagic4State* persisted_state;
};

typedef enum {
    NfcMagicViewMenu,
    NfcMagicViewPopup,
    NfcMagicViewLoading,
    NfcMagicViewTextInput,
    NfcMagicViewByteInput,
    NfcMagicViewWidget,
} NfcMagicView;

NfcMagic* nfc_magic_gen4_alloc();

void nfc_magic_gen4_text_store_set(NfcMagic* nfc_magic, const char* text, ...);

void nfc_magic_gen4_text_store_clear(NfcMagic* nfc_magic);

void nfc_magic_gen4_blink_start(NfcMagic* nfc_magic);

void nfc_magic_gen4_blink_stop(NfcMagic* nfc_magic);

void nfc_magic_gen4_show_loading_popup(void* context, bool show);
