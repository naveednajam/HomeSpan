#include WebOta.cpp


void setupOTA(){
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    if(mdns_init()!= ESP_OK){
      Serial.println("mDNS failed to start");
      return;
     }
     
     MDNS.addService("http", "tcp", 8888);
     
     
    server.on("/", HTTP_GET, []() {
      otaServer.sendHeader("Connection", "close");
      otaServer.send(200, "text/html", serverIndex);
    });
    
    otaServer.on("/update", HTTP_POST, []() {
      otaServer.sendHeader("Connection", "close");
      otaServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin()) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      } else {
        Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
      }
    });
    otaServer.begin();
    
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
}

void webotaloop(void) {
  otaServer.handleClient();
  delay(1);
}
