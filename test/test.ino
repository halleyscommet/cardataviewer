#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
#define CAN_INT_PIN 2

MCP_CAN CAN(CAN_CS_PIN);

// ── helpers ──────────────────────────────────────────────────────────────────

void sendRequest(uint32_t addr, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3 = 0x00) {
    uint8_t data[8] = {b0, b1, b2, b3, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(addr, 0, 8, data);
}

bool readResponseWithTimeout(uint32_t* rxId, uint8_t* buf, uint32_t timeout_ms) {
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        if (CAN.checkReceive() == CAN_MSGAVAIL) {
            uint8_t len;
            CAN.readMsgBuf(rxId, &len, buf);
            if (*rxId == 0x7E8) return true;
        }
    }
    return false;
}

bool isNegativeResponse(uint8_t* buf) {
    return buf[1] == 0x7F;
}

// ── standard OBD2 mode 01 ────────────────────────────────────────────────────

// RPM — PID 0x0C — formula: ((A*256)+B)/4
float requestRPM() {
    sendRequest(0x7DF, 0x02, 0x01, 0x0C);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200))
        if (buf[1] == 0x41 && buf[2] == 0x0C)
            return ((buf[3] * 256) + buf[4]) / 4.0;
    return -1;
}

// vehicle speed — PID 0x0D — formula: A (km/h)
float requestSpeed() {
    sendRequest(0x7DF, 0x02, 0x01, 0x0D);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200))
        if (buf[1] == 0x41 && buf[2] == 0x0D)
            return buf[3];
    return -1;
}

// coolant temp — PID 0x05 — formula: A-40 (°C)
float requestCoolantTemp() {
    sendRequest(0x7DF, 0x02, 0x01, 0x05);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200))
        if (buf[1] == 0x41 && buf[2] == 0x05)
            return buf[3] - 40.0;
    return -1;
}

// throttle position — PID 0x11 — formula: (A*100)/255 (%)
float requestThrottle() {
    sendRequest(0x7DF, 0x02, 0x01, 0x11);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200))
        if (buf[1] == 0x41 && buf[2] == 0x11)
            return (buf[3] * 100.0) / 255.0;
    return -1;
}

// engine load — PID 0x04 — formula: (A*100)/255 (%)
float requestEngineLoad() {
    sendRequest(0x7DF, 0x02, 0x01, 0x04);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200))
        if (buf[1] == 0x41 && buf[2] == 0x04)
            return (buf[3] * 100.0) / 255.0;
    return -1;
}

// intake air temp — PID 0x0F — formula: A-40 (°C)
float requestIntakeTemp() {
    sendRequest(0x7DF, 0x02, 0x01, 0x0F);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200))
        if (buf[1] == 0x41 && buf[2] == 0x0F)
            return buf[3] - 40.0;
    return -1;
}

// ── subaru SSM mode 22 ───────────────────────────────────────────────────────

// boost / manifold pressure — PID 0x0008 — formula: ((A*256)+B-32768)/133.3 kPa → psi
float requestBoostPSI() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62 && buf[2] == 0x00 && buf[3] == 0x08) {
            float kpa = ((buf[4] * 256) + buf[5] - 32768) / 133.3;
            return kpa * 0.145;
        }
    }
    return -1;
}

float test() {
    // 0x05 = Length, 0xA8 = Direct Read, 0x00, 0x00, 0x00, 0x11 (Address for Boost)
    uint8_t data[8] = {0x05, 0xA8, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data); 

    uint32_t rxId; 
    uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 500)) {
        // Successful SSM response ID is 0x7E8. 
        // The first byte of data should be 0xE8 (A8 + 0x40)
        if (buf[1] != 0xE8) return -1.1; 

        // In an SSM Direct Read (A8), the data usually sits in buf[5]
        float psi = (buf[5] - 128) * 0.145038;
        return psi;
    }
    return -1.2;
}

float requestSSM_Boost() {
    uint8_t data[8] = {0x05, 0xA8, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);

    uint32_t rxId;
    uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (buf[1] == 0xE8) {
            uint8_t rawValue = buf[2];
            
            int zero_offset = 215; 
            
            float psi = (rawValue - zero_offset) * 0.145038;
            return psi;
        }
    }
    return -99.0; // Return -99 if no response is received
}

// knock correction — PID 0x001C — formula: A/2 (degrees)
float requestKnockCorrection() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62)
            return buf[4] / 2.0;
    }
    return -1;
}

// DAM — PID 0x001D — formula: A/2
float requestDAM() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62)
            return buf[4] / 2.0;
    }
    return -1;
}

// ignition timing — PID 0x000E — formula: (A-128)/2 (degrees)
float requestIgnitionTiming() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62)
            return (buf[4] - 128) / 2.0;
    }
    return -1;
}

// battery voltage — PID 0x0019 — formula: A/10 (V)
float requestBatteryVoltage() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62)
            return buf[4] / 10.0;
    }
    return -1;
}

// oil temp — PID 0x0065 — formula: A-40 (°C)
float requestOilTemp() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62)
            return buf[4] - 40.0;
    }
    return -1;
}

// MAF — PID 0x0012 — formula: ((A*256)+B)/100 (g/s)
float requestMAF() {
    uint8_t data[8] = {0x03, 0x22, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00};
    CAN.sendMsgBuf(0x7E0, 0, 8, data);
    uint32_t rxId; uint8_t buf[8];
    if (readResponseWithTimeout(&rxId, buf, 200)) {
        if (isNegativeResponse(buf)) return -1;
        if (buf[1] == 0x62)
            return ((buf[4] * 256) + buf[5]) / 100.0;
    }
    return -1;
}

// ── setup & loop ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(CAN_CS_PIN, OUTPUT);
    digitalWrite(CAN_CS_PIN, HIGH);
    delay(500);

    while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
        Serial.println("CAN init failed, retrying...");
        delay(1000);
    }

    CAN.setMode(MCP_NORMAL);
    Serial.println("CAN READY!");
}

void loop() {
    // standard OBD2
    float rpm     = requestRPM();
    float speed   = requestSpeed();
    float coolant = requestCoolantTemp();
    float throttle = requestThrottle();
    float load    = requestEngineLoad();
    float intake  = requestIntakeTemp();
    float kpa = requestSSM_Boost();

    // subaru SSM
    // float boost   = requestBoostPSI();
    // float knock   = requestKnockCorrection();
    // float dam     = requestDAM();
    // float timing  = requestIgnitionTiming();
    // float battery = requestBatteryVoltage();
    // float oil     = requestOilTemp();
    // float maf     = requestMAF();

    Serial.println("=== OBD2 ===");
    Serial.print("RPM:          "); Serial.println(rpm);
    Serial.print("Speed:        "); Serial.print(speed);    Serial.println(" km/h");
    Serial.print("Coolant:      "); Serial.print(coolant);  Serial.println(" C");
    Serial.print("Throttle:     "); Serial.print(throttle); Serial.println(" %");
    Serial.print("Engine load:  "); Serial.print(load);     Serial.println(" %");
    Serial.print("Intake temp:  "); Serial.print(intake);   Serial.println(" C");

    Serial.print("Boost:        "); Serial.print(kpa);      Serial.println(" PSI");

    // Serial.println("=== Subaru SSM ===");
    // Serial.print("Boost:        "); Serial.print(boost);    Serial.println(" psi");
    // Serial.print("Knock corr:   "); Serial.print(knock);    Serial.println(" deg");
    // Serial.print("DAM:          "); Serial.println(dam);
    // Serial.print("Timing:       "); Serial.print(timing);   Serial.println(" deg");
    // Serial.print("Battery:      "); Serial.print(battery);  Serial.println(" V");
    // Serial.print("Oil temp:     "); Serial.print(oil);      Serial.println(" C");
    // Serial.print("MAF:          "); Serial.print(maf);      Serial.println(" g/s");
    // Serial.println("---");

    delay(100);
}