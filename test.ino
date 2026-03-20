#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
#define CAN_INT_PIN 2

MCP_CAN CAN(CAN_CS_PIN);

void sendRequest(uint32_t addr, uint8_t b0, uint8_t b1, uint8_t b2) {
  uint8_t data[8] = {b0, b1, b2, 0x00, 0x00, 0x00, 0x00, 0x00};
  CAN.sendMsgBuf(addr, 0, 8, data);
}

bool readResponse(uint32_t* rxId, uint8_t* buf) {
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    uint8_t len;
    CAN.readMsgBuf(rxId, &len, buf);
    return true;
  }
  return false;
}

bool isNegativeResponse(uint8_t* buf) {
  return buf[1] == 0x7F;
}

float requestRPM() {
  sendRequest(0x7DF, 0x02, 0x01, 0x0C);
  delay(10);

  uint32_t rxId;
  uint8_t buf[8];

  if (readResponse(&rxId, buf)) {
    if (rxId == 0x7E8 && buf[1] == 0x41 && buf[2] == 0x0C) {
      return ((buf[3] * 256) + buf[4]) / 4.0;
    }
  }
  return -1;
}

void setup() {
  Serial.begin(115200);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN init failed, retrying...");
    delay(1000);
  }

  CAN.setMode(MCP_NORMAL);
  Serial.println("CAN READY!");
}

void loop() {
  float rpm = requestRPM();

  Serial.print("RPM: ");        Serial.println(rpm);

  delay(100);
}