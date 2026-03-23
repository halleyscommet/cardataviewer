#include <SPI.h>
#include <mcp_can.h>

MCP_CAN CAN(10);  // CS pin

void setup() {
  Serial.begin(115200);
  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    delay(100);
  }
  CAN.setMode(MCP_NORMAL);
}

void sendRPM(uint16_t rpm) {
  byte data[8] = {
    (byte)(rpm & 0xFF),
    (byte)((rpm >> 8) & 0xFF),
    0x00, 0x00, 0x00, 0x10, 0x27, 0x62
  };
  CAN.sendMsgBuf(0x231, 0, 8, data);
}

void loop() {
  for (uint16_t rpm = 500; rpm <= 7000; rpm += 50) {
    sendRPM(rpm);
    delay(20);
  }
  for (uint16_t rpm = 7000; rpm >= 500; rpm -= 50) {
    sendRPM(rpm);
    delay(20);
  }
}