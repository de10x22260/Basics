// --- Constants and Definitions ---

// Define the expected SYNC byte for COMChip communication
#define COMCHIP_SYNC_BYTE           0x55

// Define the Command ID for the 'Get Battery Status' response from COMChip
#define COMCHIP_CID_GET_STATUS_RESP 0x81

// Define the expected minimum frame length for the 'Get Battery Status' response
// SYNC (1) + CID (1) + Status Byte (1) + Voltage (2) + Byte2 (1) + Checksum (1) = 7 bytes
#define COMCHIP_STATUS_FRAME_LEN    7

// Bit masks for the Status Byte (Byte0 in the response data)
#define STATUS_BIT_BATTERY_ERROR    (1 << 7) // Bit 7: 1 = Battery has error
#define STATUS_BIT_UNDER_VOLTAGE    (1 << 6) // Bit 6: 1 = Under voltage detected
#define STATUS_BIT_NOT_SUPPORTED    (1 << 5) // Bit 5: 1 = Battery not supported

// --- Checksum Calculation Function ---
// This function is based on the ODM_com_checksum_calc routine from the document.
// It calculates an 8-bit checksum for a given buffer of data.
uint8_t calculate_checksum(uint8_t cid, const uint8_t* data_buffer, uint8_t len) {
    uint16_t tmp = cid;
    uint8_t i;

    for (i = 0; i < len; i++) {
        tmp += data_buffer[i];
        // The document's example uses tmp -= 255u; for overflow.
        // A more standard modulo 256 is tmp %= 256;
        // We'll stick to the document's logic for strict adherence.
        if (tmp >= 256u) {
            tmp -= 255u; // This is an unusual overflow handling, but as per document
        }
    }
    tmp = (~tmp) & 0x00FFu; // Bitwise NOT and mask to 8 bits
    return (uint8_t)tmp;
}

// --- Data Packet Structure (for easier access to parsed data) ---
typedef struct {
    uint16_t battery_voltage_mV;
    bool     has_battery_error;
    bool     is_under_voltage;
    bool     is_battery_supported;
    // Add other status interpretations from Byte2 if needed (e.g., discharged status)
    // uint8_t  discharge_status;
} BatteryStatusData;

// --- Function to Process Received Data Packet ---
// This function takes a raw byte array representing the received packet
// and attempts to parse and validate it.
bool process_comchip_status_packet(const uint8_t* received_packet, uint16_t packet_len, BatteryStatusData* out_data) {

    // 1. Frame Size Verification
    if (packet_len != COMCHIP_STATUS_FRAME_LEN) {
        printf("Error: Invalid frame size. Expected %d bytes, got %d.\n", COMCHIP_STATUS_FRAME_LEN, packet_len);
        return false;
    }

    // 2. Sync Byte Verification
    if (received_packet[0] != COMCHIP_SYNC_BYTE) {
        printf("Error: Invalid SYNC byte. Expected 0x%02X, got 0x%02X.\n", COMCHIP_SYNC_BYTE, received_packet[0]);
        return false;
    }

    // 3. CID Verification (Response CID)
    if (received_packet[1] != COMCHIP_CID_GET_STATUS_RESP) {
        printf("Error: Invalid CID. Expected 0x%02X, got 0x%02X.\n", COMCHIP_CID_GET_STATUS_RESP, received_packet[1]);
        return false;
    }

    // 4. Checksum Verification
    // The data for checksum calculation starts from the CID byte (received_packet[1])
    // and includes all data bytes up to the byte *before* the checksum byte.
    // So, it's CID + Status Byte + Voltage High + Voltage Low + Byte2.
    // The length of this data is COMCHIP_STATUS_FRAME_LEN - 3 (minus SYNC, Checksum, and the CID itself is passed separately)
    // Or, more simply, the data buffer for checksum is from received_packet[1] up to received_packet[packet_len - 2].
    // The length of the data buffer for checksum is packet_len - 2 (total - SYNC - CS).
    uint8_t calculated_cs = calculate_checksum(
        received_packet[1], // CID
        &received_packet[2], // Pointer to the first data byte (Status Byte)
        COMCHIP_STATUS_FRAME_LEN - 3 // Length of data bytes (Status + Voltage + Byte2)
    );

    uint8_t received_cs = received_packet[packet_len - 1]; // Last byte is the checksum

    if (calculated_cs != received_cs) {
        printf("Error: Checksum mismatch. Calculated 0x%02X, Received 0x%02X.\n", calculated_cs, received_cs);
        return false;
    }

    // If all verifications pass, extract and interpret the data
    printf("Packet verified successfully!\n");

    uint8_t status_byte = received_packet[2]; // Byte0 (Status)
    uint8_t voltage_high_byte = received_packet[3]; // Byte0 (Battery voltage HIGH)
    uint8_t voltage_low_byte = received_packet[4];  // Byte1 (Battery voltage LOW)
    // uint8_t byte2_status = received_packet[5]; // Byte2 (discharged/not discharged)

    // 5. Calculate Battery Voltage
    // Assuming voltage is 16-bit, High-byte first (Big-Endian) or Low-byte first (Little-Endian)
    // The document's table shows "Byte0 (HIGH)" and "Byte1 (LOW)" which usually implies Big-Endian
    // However, in the example `8C A0` for 35904mV, if 8C is HIGH and A0 is LOW, then 0x8CA0 is 35904.
    // If A0 is HIGH and 8C is LOW, then 0xA08C is 41100.
    // Let's assume the common convention for "HIGH" and "LOW" bytes as Big-Endian for calculation.
    out_data->battery_voltage_mV = (uint16_t)(voltage_high_byte << 8) | voltage_low_byte;

    // 6. Check for Battery Alarm and other Status Flags
    out_data->has_battery_error = (status_byte & STATUS_BIT_BATTERY_ERROR) != 0;
    out_data->is_under_voltage = (status_byte & STATUS_BIT_UNDER_VOLTAGE) != 0;
    out_data->is_battery_supported = (status_byte & STATUS_BIT_NOT_SUPPORTED) == 0; // If bit 5 is 1, it's NOT supported

    // You can add logic here to interpret byte2_status for discharged/not discharged
    // out_data->discharge_status = byte2_status;

    return true; // Packet processed successfully
}

// --- Example Usage (Conceptual Main Function) ---
int main() {
    // Example: A hypothetical received packet for 'Get Battery Status' response
    // SYNC | CID  | Status | Volt_H | Volt_L | Byte2  | Checksum
    // Let's use the example from earlier: 0x81 0x00 0x96 0xFE 0x00 (for 38500mV, no error, OK voltage, supported, not discharged)
    // Calculated Checksum for 0x81 0x00 0x96 0xFE 0x00 using the provided algorithm:
    // tmp = 0x81 + 0x00 + 0x96 + 0xFE + 0x00 = 0x215
    // tmp = 0x215 - 255 = 0xC0 (after first subtraction)
    // tmp = 0xC0 - 255 = 0xC0 (no further subtraction)
    // tmp = ~0xC0 = 0xFFFFFF3F (on 32-bit system)
    // tmp = 0xFFFFFF3F & 0x00FF = 0x3F
    // So, checksum would be 0x3F.

    // Let's use a packet with a known checksum for testing
    // SYNC | CID  | Status | Volt_H | Volt_L | Byte2  | Checksum
    // 0x55 | 0x81 | 0x00   | 0x96   | 0xFE   | 0x00   | 0x3F
    uint8_t good_packet[] = {0x55, 0x81, 0x00, 0x96, 0xFE, 0x00, 0x3F}; // 38500mV, no error, OK voltage
    uint16_t good_packet_len = sizeof(good_packet) / sizeof(good_packet[0]);

    // Example: A packet with an error (e.g., under voltage, Bit 6 set in status byte)
    // Status byte: 0x40 (01000000 binary)
    // Recalculate checksum for 0x81 0x40 0x96 0xFE 0x00
    // tmp = 0x81 + 0x40 + 0x96 + 0xFE + 0x00 = 0x255
    // tmp = 0x255 - 255 = 0x00
    // tmp = ~0x00 = 0xFFFFFFFF
    // tmp = 0xFFFFFFFF & 0x00FF = 0xFF
    uint8_t undervoltage_packet[] = {0x55, 0x81, 0x40, 0x96, 0xFE, 0x00, 0xFF}; // 38500mV, under voltage
    uint16_t undervoltage_packet_len = sizeof(undervoltage_packet) / sizeof(undervoltage_packet[0]);

    // Example: A packet with incorrect checksum
    uint8_t bad_checksum_packet[] = {0x55, 0x81, 0x00, 0x96, 0xFE, 0x00, 0x11}; // Bad checksum
    uint16_t bad_checksum_packet_len = sizeof(bad_checksum_packet) / sizeof(bad_checksum_packet[0]);

    // Example: A packet with incorrect length
    uint8_t short_packet[] = {0x55, 0x81, 0x00, 0x96, 0xFE, 0x00}; // Too short
    uint16_t short_packet_len = sizeof(short_packet) / sizeof(short_packet[0]);

    BatteryStatusData status_data;

    printf("--- Processing Good Packet ---\n");
    if (process_comchip_status_packet(good_packet, good_packet_len, &status_data)) {
        printf("Battery Voltage: %u mV\n", status_data.battery_voltage_mV);
        printf("Battery Error: %s\n", status_data.has_battery_error ? "YES" : "NO");
        printf("Under Voltage: %s\n", status_data.is_under_voltage ? "YES" : "NO");
        printf("Battery Supported: %s\n", status_data.is_battery_supported ? "YES" : "NO");
    }
    printf("\n");

    printf("--- Processing Under Voltage Packet ---\n");
    if (process_comchip_status_packet(undervoltage_packet, undervoltage_packet_len, &status_data)) {
        printf("Battery Voltage: %u mV\n", status_data.battery_voltage_mV);
        printf("Battery Error: %s\n", status_data.has_battery_error ? "YES" : "NO");
        printf("Under Voltage: %s\n", status_data.is_under_voltage ? "YES" : "NO");
        printf("Battery Supported: %s\n", status_data.is_battery_supported ? "YES" : "NO");
    }
    printf("\n");

    printf("--- Processing Bad Checksum Packet ---\n");
    process_comchip_status_packet(bad_checksum_packet, bad_checksum_packet_len, &status_data);
    printf("\n");

    printf("--- Processing Short Packet ---\n");
    process_comchip_status_packet(short_packet, short_packet_len, &status_data);
    printf("\n");

    return 0;
}
