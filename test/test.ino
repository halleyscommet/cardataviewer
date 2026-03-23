#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
#define CAN_INT_PIN 2

MCP_CAN CAN(CAN_CS_PIN);

// ── helpers ──────────────────────────────────────────────────────────────────

void sendRequest(uint32_t addr, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3 = 0x00) {
  uint8_t data[8] = { b0, b1, b2, b3, 0x00, 0x00, 0x00, 0x00 };
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
  uint32_t rxId;
  uint8_t buf[8];
  if (readResponseWithTimeout(&rxId, buf, 200))
    if (buf[1] == 0x41 && buf[2] == 0x0C)
      return ((buf[3] * 256) + buf[4]) / 4.0;
  return -1;
}

// vehicle speed — PID 0x0D — formula: A (km/h)
float requestSpeed() {
  sendRequest(0x7DF, 0x02, 0x01, 0x0D);
  uint32_t rxId;
  uint8_t buf[8];
  if (readResponseWithTimeout(&rxId, buf, 200))
    if (buf[1] == 0x41 && buf[2] == 0x0D)
      return buf[3];
  return -1;
}

// coolant temp — PID 0x05 — formula: A-40 (°C)
float requestCoolantTemp() {
  sendRequest(0x7DF, 0x02, 0x01, 0x05);
  uint32_t rxId;
  uint8_t buf[8];
  if (readResponseWithTimeout(&rxId, buf, 200))
    if (buf[1] == 0x41 && buf[2] == 0x05)
      return buf[3] - 40.0;
  return -1;
}

// engine load — PID 0x04 — formula: (A*100)/255 (%)
float requestEngineLoad() {
  sendRequest(0x7DF, 0x02, 0x01, 0x04);
  uint32_t rxId;
  uint8_t buf[8];
  if (readResponseWithTimeout(&rxId, buf, 200))
    if (buf[1] == 0x41 && buf[2] == 0x04)
      return (buf[3] * 100.0) / 255.0;
  return -1;
}

// ── subaru SSM mode 22 ───────────────────────────────────────────────────────

float requestSSM_Boost() {
  uint8_t data[8] = { 0x05, 0xA8, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00 };
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
  return -99.0;  // Return -99 if no response is received
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
  float rpm     = requestRPM();
  float speed   = requestSpeed();
  float coolant = requestCoolantTemp();
  float load    = requestEngineLoad();

  Serial.println("=== OBD2 ===");
  Serial.print("RPM:         "); Serial.println(rpm);
  Serial.print("Speed:       "); Serial.print(speed);   Serial.println(" km/h");
  Serial.print("Coolant:     "); Serial.print(coolant); Serial.println(" C");
  Serial.print("Engine load: "); Serial.print(load);    Serial.println(" %");

  delay(100);
}