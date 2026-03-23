#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
#define CAN_INT_PIN 2

MCP_CAN CAN(CAN_CS_PIN);

// helpers

void sendRequest(uint32_t addr, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6) {
  uint8_t data[8] = { b0, b1, b2, b3, b4, b5, b6, 0x00 };

  CAN.sendMsgBuf(addr, 0, 8, data);
}

bool readResponse(uint32_t* rxId, uint8_t* buf, uint32_t timeout_ms) {
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

// mode 01

float requestRPM() {
  sendRequest(0x7DF, 0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00);

  uint32_t rxId;
  uint8_t buf[8];

  if (readResponse(&rxId, buf, 200)) {
    if (buf[1] == 0x41 && buf[2] == 0x0C) {
      return ((buf[3] << 8) | buf[4]) * 0.25;
    }
  }

  return -1;
}

float requestMAP() {
  sendRequest(0x7DF, 0x02, 0x01, 0x0B, 0x00, 0x00, 0x00, 0x00);

  uint32_t rxId;
  uint8_t buf[8];

  if (readResponse(&rxId, buf, 200)) {
    if (buf[1] == 0x41 && buf[2] == 0x0B) {
      return buf[3];
    }
  }

  return -1;
}

float requestABP() {
  sendRequest(0x7DF, 0x02, 0x01, 0x33, 0x00, 0x00, 0x00, 0x00);

  uint32_t rxId;
  uint8_t buf[8];

  if (readResponse(&rxId, buf, 200)) {
    if (buf[1] == 0x41 && buf[2] == 0x33) {
      return buf[3];
    }
  }

  return -1;
}

float calculateBoost() {
  float map = requestMAP();
  float abp = requestABP();

  if (map < 0 || abp < 0) return -1;

  float kpa = map - abp;
  float psi = kpa / 6.895;

  return psi;
}

// main

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(CAN_CS_PIN, OUTPUT);
  digitalWrite(CAN_CS_PIN, HIGH);
  delay(500);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN init failed, trying again...");
    delay(1000);
  }

  CAN.setMode(MCP_NORMAL);
  Serial.println("CAN READY!");
}

void loop() {
  float rpm   = requestRPM();
  float boost = calculateBoost();

  Serial.println("----- MODE 01 -----");
  Serial.print("RPM:   "); Serial.println(rpm);
  Serial.print("Boost: "); Serial.print(boost); Serial.println(" PSI");

  delay(100);
}