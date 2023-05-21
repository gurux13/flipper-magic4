#include "../nfc_magic_gen4_i.h"
#include "../nfc_magic_gen4_worker_i.h"

void nfc_magic_gen4_scene_magic_info_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    NfcMagic* nfc_magic = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(nfc_magic->view_dispatcher, result);
    }
}

typedef struct {
    byte ultralight_protocol;
    byte uid_length;
    byte password[4];
    byte gtu_mode;
    byte ats_length;
    byte ats[16];
    byte atqa[2];
    byte sak;
    byte ultralight_mode;
    byte max_rw_sectors;
    byte block_zero_wr;
    byte junk[2];
} MagicConfiguration;
FuriString* buffer;

void nfc_magic_gen4_scene_magic_info_on_enter(void* context) {
    NfcMagic* nfc_magic = context;
    Widget* widget = nfc_magic->widget;
    byte* config_b = nfc_magic->worker->dev_data->reader_data.data;
    for (int i = 0; i < 32; ++i) {
        FURI_LOG_W("GEN4", "Have byte: %2X", config_b[i]);
    }
    buffer = furi_string_alloc();
    MagicConfiguration* config = (MagicConfiguration*)config_b;
    #define LINE_GAP 10
    const char * newline = "\n";
    notification_message(nfc_magic->notifications, &sequence_success);
    widget_add_string_element(
        widget, 64, 4, AlignCenter, AlignTop, FontPrimary, "Magic card detected");
    // widget_add_icon_element(widget, 73, 17, &I_DolphinCommon_56x48);
    const char* ul_mode = "Mode: Classic";
    if (config->ultralight_protocol == 1) {
        ul_mode = "Mode: Ultralight";
    } else if (config->ultralight_mode != 0) {
        ul_mode = "Mode: UNKNOWN";
    }
    furi_string_cat(buffer, ul_mode);

    const char* uid_length = "UID Length: UNKNOWN";
    switch (config->uid_length) {
        case 0:
            uid_length = "UID Length: 4 bytes";
            break;
        case 1:
            uid_length = "UID Length: 7 bytes";
            break;
        case 2:
            uid_length = "UID Length: 10 bytes";
            break;
    }
    furi_string_cat(buffer, newline);
    furi_string_cat(buffer, uid_length);
    furi_string_cat(buffer, newline);
    furi_string_cat_printf(buffer, "Password: %02X %02X %02X %02X", config->password[0],config->password[1],config->password[2],config->password[3]);
    furi_string_cat(buffer, newline);
    furi_string_cat(buffer, "Shadow mode: ");
    switch (config->gtu_mode) {
        case 0:
            furi_string_cat(buffer, "ON: Pre-write");
            break;
        case 1:
            furi_string_cat(buffer, "ON: Restore");
            break;
        case 2:
            furi_string_cat(buffer, "OFF");
            break;
        case 3:
            furi_string_cat(buffer, "OFF (h/s RW for UL)");
            break;
        default:
            furi_string_cat(buffer, "UNKNOWN");
    }
    furi_string_cat(buffer, newline);
    furi_string_cat_printf(buffer, "ATS Length: %u", (int)(config->ats_length));
    furi_string_cat(buffer, newline);
    furi_string_cat(buffer, "ATS: ");
    for (size_t i = 0; i < config->ats_length; ++i) {
        furi_string_cat_printf(buffer, "%02X ", config->ats[i]);
    }
    furi_string_cat(buffer, newline);
    furi_string_cat_printf(buffer, "ATQA: %02X %02X\n", config->atqa[0], config->atqa[1]);
    furi_string_cat_printf(buffer, "SAK: %02X\n", config->sak);
    furi_string_cat(buffer, "Ultralight Mode: ");
    switch (config->ultralight_mode) {
        case 0: 
            furi_string_cat(buffer, "UL EV1");
            break;
        case 1: 
            furi_string_cat(buffer, "NTAG");
            break;
        case 2: 
            furi_string_cat(buffer, "UL-C");
            break;
        case 3: 
            furi_string_cat(buffer, "UL");
            break;
        default:
            furi_string_cat(buffer, "UNKNOWN");
            break;

    }
    furi_string_cat(buffer, newline);
    furi_string_cat_printf(buffer, "Sectors r/w: %02X\n", config->max_rw_sectors);
    furi_string_cat(buffer, "Block 0: ");
    switch (config->block_zero_wr) {
        case 0: 
            furi_string_cat(buffer, "WRITEABLE");
            break;
        case 1: 
            furi_string_cat(buffer, "Read-only");
            break;
        case 2: 
            furi_string_cat(buffer, "WRITEABLE(default)");
            break;
    }


    widget_add_text_scroll_element(widget, 3, 18, 122, 30, furi_string_get_cstr(buffer));

    widget_add_button_element(
        widget, GuiButtonTypeLeft, "Retry", nfc_magic_gen4_scene_magic_info_widget_callback, nfc_magic);

    // Setup and start worker
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewWidget);
}

bool nfc_magic_gen4_scene_magic_info_on_event(void* context, SceneManagerEvent event) {
    NfcMagic* nfc_magic = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_previous_scene(nfc_magic->scene_manager);
        }
    }
    if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(nfc_magic->scene_manager, NfcMagicSceneStart);
    }
    return consumed;
}

void nfc_magic_gen4_scene_magic_info_on_exit(void* context) {
    NfcMagic* nfc_magic = context;

    widget_reset(nfc_magic->widget);
    furi_string_free(buffer);
}
