#include <SPI.h>
#include <mcp_can.h>

MCP_CAN CAN(53);

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing CAN...");
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN init OK");
  } else {
    Serial.println("CAN init FAILED");
    while (1)
      ;  // halt
  }
  CAN.setMode(MCP_NORMAL);
  Serial.println("Mode set to NORMAL");
}

void sendRPM(uint16_t rpm) {
  byte data[8] = {
    (byte)(rpm & 0xFF),
    (byte)((rpm >> 8) & 0xFF),
    0x00, 0x00, 0x00, 0x10, 0x27, 0x62
  };
  byte result = CAN.sendMsgBuf(0x231, 0, 8, data);
  if (result == CAN_OK) {
    Serial.print("Sent RPM: ");
    Serial.println(rpm);
  } else {
    Serial.print("Send FAILED, error: ");
    Serial.println(result, HEX);
  }
}

void loop() {
  for (uint16_t rpm = 500; rpm <= 7000; rpm += 50) {
    sendRPM(rpm);
    delay(35);
  }
  for (uint16_t rpm = 7000; rpm >= 500; rpm -= 50) {
    sendRPM(rpm);
    delay(35);
  }
}