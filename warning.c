#include "warning.h"
#include <furi.h>

static int send_event_on_ok = 0;
static int send_event_on_cancel = 0;

static void on_btn_press(GuiButtonType btn_type,  InputType input_type,  void * context) {
    if (input_type != InputTypePress) {
        return;
    }
    NfcMagic* nfc_magic = (NfcMagic*)context;
    view_dispatcher_send_custom_event(nfc_magic->view_dispatcher, btn_type == GuiButtonTypeRight ? send_event_on_ok : send_event_on_cancel);
}

void render_warning(NfcMagic* nfc_magic, const char * message, int ok_event, int cancel_event)
{
    send_event_on_cancel = cancel_event;
    send_event_on_ok = ok_event;
    Widget* widget = nfc_magic->widget;
    widget_reset(nfc_magic->widget);
    widget_add_icon_element(widget, 0, 0, &I_WarningDolphin_45x42);
    widget_add_text_scroll_element(widget, 48, 2, (125-48), 50, message);
    widget_add_button_element(widget, GuiButtonTypeLeft, "Cancel", on_btn_press, nfc_magic);
    widget_add_button_element(widget, GuiButtonTypeRight, "OK", on_btn_press, nfc_magic);
    view_dispatcher_switch_to_view(nfc_magic->view_dispatcher, NfcMagicViewWidget);

}