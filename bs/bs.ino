#include <SPI.h>
#include <mcp_can.h>

MCP_CAN CAN(53);

void setup() {
  Serial.begin(115200);
  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) delay(100);
  CAN.setMode(MCP_NORMAL);
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() < 4) return;

    // parse: "0231 00 B9 02 00 00 10 27 62"
    uint32_t pid = strtol(line.substring(0, 4).c_str(), NULL, 16);
    byte data[8];
    for (int i = 0; i < 8; i++) {
      data[i] = strtol(line.substring(5 + i * 3, 7 + i * 3).c_str(), NULL, 16);
    }
    CAN.sendMsgBuf(pid, 0, 8, data);
  }
}