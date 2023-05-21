#include "magic_gen4.h"

#include <furi_hal_nfc.h>

#define TAG "GEN4"

#define MAGIC_BUFFER_SIZE 32

#define MAGIC_CMD_WUPA (0x40)
#define MAGIC_CMD_WIPE (0x41)
#define MAGIC_CMD_READ (0x43)
#define MAGIC_CMD_WRITE (0x43)

#define MAGIC_MIFARE_READ_CMD (0x30)
#define MAGIC_MIFARE_WRITE_CMD (0xA0)

#define MAGIC_ACK (0x0A)

#define DEFAULT_TIMEOUT_MS 50

#define min(x, y) ((x) < (y) ? (x) : (y))

bool execute_magic_command(
    const byte* password,
    byte command,
    const byte* payload,
    int payload_size,
    byte* buffer,
    int buffer_size) {
    return execute_magic_command_timeout(
        password, command, payload, payload_size, buffer, buffer_size, DEFAULT_TIMEOUT_MS);
}

bool execute_magic_command_timeout(
    const byte* password,
    byte command,
    const byte* payload,
    int payload_size,
    byte* buffer,
    int buffer_size,
    int timeout) {
    int tx_data_size = 1 + 4 + 1 + payload_size;
    uint8_t tx_data[1 + 4 + 1 + payload_size];
    memset(tx_data, 0, tx_data_size);
    tx_data[0] = 0xCF;
    memcpy(tx_data + 1, password, 4);
    tx_data[5] = command;
    if(payload_size > 0) {
        memcpy(tx_data + 6, payload, payload_size);
    }
    FuriHalNfcTxRxContext tx_rx_ = {};
    FuriHalNfcTxRxContext* tx_rx = &tx_rx_;
    memset(tx_rx->tx_data, 0, sizeof(tx_rx->tx_data));
    memset(tx_rx->tx_parity, 0, sizeof(tx_rx->tx_parity));
    bool retval = true;
    do {
        furi_hal_nfc_activate_nfca(timeout, NULL);
        FURI_LOG_D(TAG, "Executing command");
        memcpy(tx_rx->tx_data, tx_data, tx_data_size);
        tx_rx->tx_bits = tx_data_size * 8;
        tx_rx->tx_rx_type = FuriHalNfcTxRxTypeDefault;
        for(int i = 0; i < tx_rx->tx_bits / 8; ++i) {
            FURI_LOG_E("MAGIC", "Sending byte %02x", tx_rx->tx_data[i]);
        }

        if(!furi_hal_nfc_tx_rx(tx_rx, timeout)) {
            FURI_LOG_E(TAG, "Failed reading version");
            furi_hal_nfc_sleep();
            retval = false;
            break;
        }
        for(int i = 0; i < tx_rx->rx_bits / 8; ++i) {
            FURI_LOG_E("MAGIC", "Got byte %02x", tx_rx->rx_data[i]);
        }
        int to_copy = min(tx_rx->rx_bits / 8, buffer_size);
        if(to_copy > 0) {
            memcpy(buffer, tx_rx->rx_data, to_copy);
        }
    } while(false);

/*    do {
        memset(tx_rx->tx_data, 0, sizeof(tx_rx->tx_data));
        memset(tx_rx->tx_parity, 0, sizeof(tx_rx->tx_parity));
        FURI_LOG_W("MAGIC", "Sending RATS");
        tx_rx->tx_data[0] = 0xE0;
        tx_rx->tx_data[1] = 0;
        tx_rx->tx_bits = 16;
        if(!furi_hal_nfc_tx_rx(tx_rx, timeout)) {
            FURI_LOG_E(TAG, "Failed doing RATS");
            furi_hal_nfc_sleep();
            retval = false;
            break;
        }
        for(int i = 0; i < tx_rx->rx_bits / 8; ++i) {
            FURI_LOG_E("MAGIC", "Got byte %02x", tx_rx->rx_data[i]);
        }
    } while (false);*/


    return retval;
}

bool magic_wupa() {
    bool magic_activated = false;
    uint8_t tx_data[MAGIC_BUFFER_SIZE] = {};
    uint8_t rx_data[MAGIC_BUFFER_SIZE] = {};
    uint16_t rx_len = 0;
    FuriHalNfcReturn ret = 0;

    do {
        // Setup nfc poller
        furi_hal_nfc_exit_sleep();
        furi_hal_nfc_ll_txrx_on();
        furi_hal_nfc_ll_poll();
        ret = furi_hal_nfc_ll_set_mode(
            FuriHalNfcModePollNfca, FuriHalNfcBitrate106, FuriHalNfcBitrate106);
        if(ret != FuriHalNfcReturnOk) break;

        furi_hal_nfc_ll_set_fdt_listen(FURI_HAL_NFC_LL_FDT_LISTEN_NFCA_POLLER);
        furi_hal_nfc_ll_set_fdt_poll(FURI_HAL_NFC_LL_FDT_POLL_NFCA_POLLER);
        furi_hal_nfc_ll_set_error_handling(FuriHalNfcErrorHandlingNfc);
        furi_hal_nfc_ll_set_guard_time(FURI_HAL_NFC_LL_GT_NFCA);

        // Start communication
        tx_data[0] = MAGIC_CMD_WUPA;
        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            7,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_TX_MANUAL | FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON |
                FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP,
            furi_hal_nfc_ll_ms2fc(20));
        if(ret != FuriHalNfcReturnIncompleteByte) break;
        if(rx_len != 4) break;
        if(rx_data[0] != MAGIC_ACK) break;
        magic_activated = true;
    } while(false);

    if(!magic_activated) {
        furi_hal_nfc_ll_txrx_off();
        furi_hal_nfc_start_sleep();
    }

    return magic_activated;
}

bool magic_data_access_cmd() {
    bool write_cmd_success = false;
    uint8_t tx_data[MAGIC_BUFFER_SIZE] = {};
    uint8_t rx_data[MAGIC_BUFFER_SIZE] = {};
    uint16_t rx_len = 0;
    FuriHalNfcReturn ret = 0;

    do {
        tx_data[0] = 12;
        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            8,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_TX_MANUAL | FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON |
                FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP,
            furi_hal_nfc_ll_ms2fc(20));
        if(ret != FuriHalNfcReturnIncompleteByte) break;
        if(rx_len != 4) break;
        if(rx_data[0] != MAGIC_ACK) break;

        write_cmd_success = true;
    } while(false);

    if(!write_cmd_success) {
        furi_hal_nfc_ll_txrx_off();
        furi_hal_nfc_start_sleep();
    }

    return write_cmd_success;
}

bool magic_read_block(uint8_t block_num, MfClassicBlock* data) {
    furi_assert(data);

    bool read_success = false;

    uint8_t tx_data[MAGIC_BUFFER_SIZE] = {};
    uint8_t rx_data[MAGIC_BUFFER_SIZE] = {};
    uint16_t rx_len = 0;
    FuriHalNfcReturn ret = 0;

    do {
        tx_data[0] = MAGIC_MIFARE_READ_CMD;
        tx_data[1] = block_num;
        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            2 * 8,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON,
            furi_hal_nfc_ll_ms2fc(20));

        if(ret != FuriHalNfcReturnOk) break;
        if(rx_len != 16 * 8) break;
        memcpy(data->value, rx_data, sizeof(data->value));
        read_success = true;
    } while(false);

    if(!read_success) {
        furi_hal_nfc_ll_txrx_off();
        furi_hal_nfc_start_sleep();
    }

    return read_success;
}

bool magic_write_blk(uint8_t block_num, MfClassicBlock* data) {
    furi_assert(data);

    bool write_success = false;
    uint8_t tx_data[MAGIC_BUFFER_SIZE] = {};
    uint8_t rx_data[MAGIC_BUFFER_SIZE] = {};
    uint16_t rx_len = 0;
    FuriHalNfcReturn ret = 0;

    do {
        tx_data[0] = MAGIC_MIFARE_WRITE_CMD;
        tx_data[1] = block_num;
        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            2 * 8,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON | FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP,
            furi_hal_nfc_ll_ms2fc(20));
        if(ret != FuriHalNfcReturnIncompleteByte) break;
        if(rx_len != 4) break;
        if(rx_data[0] != MAGIC_ACK) break;

        memcpy(tx_data, data->value, sizeof(data->value));
        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            16 * 8,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON | FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP,
            furi_hal_nfc_ll_ms2fc(20));
        if(ret != FuriHalNfcReturnIncompleteByte) break;
        if(rx_len != 4) break;
        if(rx_data[0] != MAGIC_ACK) break;

        write_success = true;
    } while(false);

    if(!write_success) {
        furi_hal_nfc_ll_txrx_off();
        furi_hal_nfc_start_sleep();
    }

    return write_success;
}

bool magic_wipe() {
    bool wipe_success = false;
    uint8_t tx_data[MAGIC_BUFFER_SIZE] = {};
    uint8_t rx_data[MAGIC_BUFFER_SIZE] = {};
    uint16_t rx_len = 0;
    FuriHalNfcReturn ret = 0;

    do {
        tx_data[0] = MAGIC_CMD_WIPE;
        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            8,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_TX_MANUAL | FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON |
                FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP,
            furi_hal_nfc_ll_ms2fc(2000));

        if(ret != FuriHalNfcReturnIncompleteByte) break;
        if(rx_len != 4) break;
        if(rx_data[0] != MAGIC_ACK) break;

        wipe_success = true;
    } while(false);

    return wipe_success;
}

void magic_test() {
    // uint8_t tx_data[] = {0xCF, 0x08, 0x16, 0x32, 0x64, 0xCF, 0x01};
    //uint8_t tx_data[] = {0xCF, 0x00, 0x00, 0x00, 0x00, 0xCD, 0x00, 0x04, 0x68, 0x7D, 0xEA, 0x8C, 0x70, 0x80, 0x08, 0x44, 0x00, 0x01, 0x01, 0x11, 0x00, 0x34, 0x21};
    //uint8_t tx_data[] = {0xCF, 0x08, 0x16, 0x32, 0x64, 0xFE, 0x42, 0x77, 0x13, 0x18};
    uint8_t tx_data[] = {0xCF, 0x42, 0x77, 0x13, 0x18, 0x35, 0x44, 0x00, 0x08};
    FuriHalNfcTxRxContext tx_rx_ = {};

    FuriHalNfcTxRxContext* tx_rx = &tx_rx_;
    FuriHalNfcReturn ret = 0;
    memset(tx_rx->tx_data, 0, sizeof(tx_rx->tx_data));
    memset(tx_rx->tx_parity, 0, sizeof(tx_rx->tx_parity));
    do {
        furi_hal_nfc_activate_nfca(1000, NULL);
        FURI_LOG_D(TAG, "Reading version");
        memcpy(tx_rx->tx_data, tx_data, sizeof(tx_data));
        // tx_rx->tx_data[0] = 0x60;
        // tx_rx->tx_data[1] = 0x01;
        tx_rx->tx_bits = sizeof(tx_data) * 8;
        tx_rx->tx_rx_type = FuriHalNfcTxRxTypeDefault;
        if(!furi_hal_nfc_tx_rx(tx_rx, 5000)) {
            FURI_LOG_E(TAG, "Failed reading version");
            furi_hal_nfc_sleep();
            break;
        }
        for(int i = 0; i < tx_rx->rx_bits / 8; ++i) {
            FURI_LOG_E("MAGIC", "Got byte %02x", tx_rx->rx_data[i]);
        }
        FURI_LOG_E(TAG, "Got version!");
    } while(false);
    return;
    // uint8_t tx_data[] = {0x60};
    uint8_t rx_data[MAGIC_BUFFER_SIZE] = {};
    uint16_t rx_len = 0;

    do {
        furi_hal_nfc_exit_sleep();
        furi_hal_nfc_ll_txrx_on();
        furi_hal_nfc_ll_poll();
        ret = furi_hal_nfc_ll_set_mode(
            FuriHalNfcModePollNfca, FuriHalNfcBitrate106, FuriHalNfcBitrate106);
        if(ret != FuriHalNfcReturnOk) break;

        furi_hal_nfc_ll_set_fdt_listen(FURI_HAL_NFC_LL_FDT_LISTEN_NFCA_POLLER);
        furi_hal_nfc_ll_set_fdt_poll(FURI_HAL_NFC_LL_FDT_POLL_NFCA_POLLER);
        furi_hal_nfc_ll_set_error_handling(FuriHalNfcErrorHandlingNfc);
        furi_hal_nfc_ll_set_guard_time(FURI_HAL_NFC_LL_GT_NFCA);

        ret = furi_hal_nfc_ll_txrx_bits(
            tx_data,
            sizeof(tx_data) * 8,
            rx_data,
            sizeof(rx_data),
            &rx_len,
            FURI_HAL_NFC_LL_TXRX_FLAGS_AGC_ON | FURI_HAL_NFC_LL_TXRX_FLAGS_CRC_RX_KEEP,
            furi_hal_nfc_ll_ms2fc(5000));
        for(size_t i = 0; i < sizeof(tx_data); ++i) {
            FURI_LOG_D("MAGIC", "Sent data: %02x", tx_data[i]);
        }
        // FURI_LOG_D("MAGIC", "Sent data: %x %x %x %x %x", sendme[0], sendme[1], sendme[2], sendme[3], sendme[4]);
        FURI_LOG_D("MAGIC", "Got ret: %d, num bytes: %d", ret, rx_len);
        for(size_t i = 0; i < rx_len; ++i) {
            FURI_LOG_D("MAGIC", "Got byte %02x", rx_data[i]);
        }

    } while(false);
}

void magic_deactivate() {
    furi_hal_nfc_ll_txrx_off();
    furi_hal_nfc_sleep();
}
