/*
 * Mode 22 PID Brute Force Scanner
 * Target: 2010 Subaru Legacy 2.5GT (EJ255)
 * Hardware: ESP32 + MCP2515
 * Library:  mcp_can.h by coryjfowler
 *   https://github.com/coryjfowler/MCP_CAN_lib
 *
 * No session opener — ECU responds to mode 22 in its default power-on state.
 * TesterPresent keepalive sent every 2s to prevent session timeout mid-scan.
 * Between scan passes, keepalive is sent continuously so the session survives.
 *
 * Wiring (default):
 *   MCP2515 CS  -> GPIO 5
 *   SPI SCK     -> GPIO 18
 *   SPI MOSI    -> GPIO 23
 *   SPI MISO    -> GPIO 19
 */

#include <SPI.h>
#include <mcp_can.h>

// ── Pin config ────────────────────────────────────────────────────────────────
#define CAN_CS_PIN 10

// ── Scan range ────────────────────────────────────────────────────────────────
const uint16_t PID_START = 0x0000;
const uint16_t PID_END = 0x00FF;  // expand to 0x1FFF for a fuller pass

// ── CAN IDs ───────────────────────────────────────────────────────────────────
const uint32_t ECU_REQ_ID = 0x7E0;
const uint32_t ECU_RESP_ID = 0x7E8;

// ── Timing ────────────────────────────────────────────────────────────────────
const uint16_t RESPONSE_TIMEOUT_MS = 50;
const uint16_t INTER_REQUEST_MS = 10;
const uint16_t KEEPALIVE_INTERVAL_MS = 2000;
const uint16_t RESTART_DELAY_MS = 3000;

MCP_CAN CAN(CAN_CS_PIN);
unsigned long lastKeepaliveMs = 0;

// ─────────────────────────────────────────────────────────────────────────────

void sendTesterPresent() {
  // 0x3E = TesterPresent, 0x80 = suppressPositiveResponse — ECU won't reply
  byte txBuf[8] = { 0x02, 0x3E, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 };
  CAN.sendMsgBuf(ECU_REQ_ID, 0, 8, txBuf);
}

void keepaliveIfNeeded() {
  if (millis() - lastKeepaliveMs >= KEEPALIVE_INTERVAL_MS) {
    sendTesterPresent();
    lastKeepaliveMs = millis();
  }
}

bool waitForResponse(byte &len, byte *buf, uint32_t timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      unsigned long rxId;
      CAN.readMsgBuf(&rxId, &len, buf);
      if (rxId == ECU_RESP_ID) return true;
    }
  }
  return false;
}

void logResponse(uint16_t pid, byte len, byte *buf) {
  byte sid = buf[1];
  bool positive = (sid == 0x62);
  bool negative = (sid == 0x7F && buf[2] == 0x22);

  // Drop "requestOutOfRange" (0x31) silently — PID doesn't exist
  if (negative && buf[3] == 0x31) return;

  Serial.print(F("0x"));
  Serial.print(pid, HEX);
  Serial.print(F(",0x"));
  Serial.print(sid, HEX);
  Serial.print(F(","));

  for (byte i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print(F("0"));
    Serial.print(buf[i], HEX);
    if (i < len - 1) Serial.print(F(" "));
  }

  Serial.print(F(","));
  if (positive) {
    byte dataLen = buf[0] - 3;
    Serial.print(F("POSITIVE ["));
    Serial.print(dataLen);
    Serial.print(F(" data byte(s)]"));
  } else if (negative) {
    Serial.print(F("NEGATIVE NRC=0x"));
    Serial.print(buf[3], HEX);
    if (buf[3] == 0x22) Serial.print(F(" (conditionsNotCorrect — PID exists!)"));
  } else {
    Serial.print(F("UNKNOWN SID"));
  }

  Serial.println();
}

// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  Serial.println(F("=== Mode 22 PID Scanner ==="));
  Serial.print(F("Range: 0x"));
  Serial.print(PID_START, HEX);
  Serial.print(F(" - 0x"));
  Serial.println(PID_END, HEX);
  Serial.println(F("No session opener — default session only"));
  Serial.println(F("---"));

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println(F("[WARN] CAN init failed, retrying... (try MCP_16MHZ if this loops)"));
    delay(1000);
  }
  CAN.setMode(MCP_NORMAL);
  Serial.println(F("[OK] CAN bus ready"));
  Serial.println(F("PID,RESP_SID,BYTES_HEX,NOTE"));

  // Send an initial keepalive immediately
  sendTesterPresent();
  lastKeepaliveMs = millis();
}

// ─────────────────────────────────────────────────────────────────────────────

void loop() {
  for (uint16_t pid = PID_START; pid <= PID_END; pid++) {
    keepaliveIfNeeded();

    byte txBuf[8] = {
      0x03, 0x22,
      (byte)(pid >> 8),
      (byte)(pid & 0xFF),
      0x00, 0x00, 0x00, 0x00
    };

    if (CAN.sendMsgBuf(ECU_REQ_ID, 0, 8, txBuf) != CAN_OK) {
      Serial.print(F("# TX error on PID 0x"));
      Serial.println(pid, HEX);
      delay(50);
      continue;
    }

    byte rxLen = 0;
    byte rxBuf[8];
    if (waitForResponse(rxLen, rxBuf, RESPONSE_TIMEOUT_MS)) {
      logResponse(pid, rxLen, rxBuf);
    }

    delay(INTER_REQUEST_MS);
  }

  // Keep session alive during the restart delay — send keepalive every 500ms
  Serial.println(F("# Scan complete. Restarting in 3s..."));
  unsigned long restartStart = millis();
  while (millis() - restartStart < RESTART_DELAY_MS) {
    keepaliveIfNeeded();
    delay(100);
  }
}
