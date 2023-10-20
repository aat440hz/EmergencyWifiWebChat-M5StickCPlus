#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

const char *ssid = "EmergencyWifiWebChat";
const char *password = "66666666";

WiFiServer server(80);

const int maxMessages = 5;
String chatMessages[maxMessages];
int messageCount = 0;
unsigned long lastMemoryClear = 0;  // Variable to track the last time memory was cleared

bool ledOn = false;  // Variable to track the LED state

// Define the GPIO pin number for the onboard LED (replace with the actual GPIO pin number)
const int onboardLedPin = 10;

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.println("WiFi Chat AP");
    M5.Lcd.printf("Connect to: %s\n", ssid);

    pinMode(onboardLedPin, OUTPUT);  // Set the LED pin as an output

    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    M5.Lcd.println(myIP);
    server.begin();
}

void loop() {
    WiFiClient client = server.available();

    if (client) {
        String currentLine = "";
        String chatMessage = "";  // To store user chat messages

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);

                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.println("<html><head>");
                        client.println("<meta http-equiv='refresh' content='60;url=/'>");
                        client.println("</head><body>");
                        client.println("<h1>WiFi Chat</h1>");
                        client.println("<form method='GET'>");
                        client.println("<input type='text' name='message' placeholder='Enter your message'><br>");
                        client.println("<input type='submit' value='Send'>");
                        client.println("</form>");

                        client.println("<h2>Chat Messages:</h2>");
                        client.println("<ul>");

                        for (int i = messageCount - 1; i >= max(0, messageCount - maxMessages); i--) {
                            client.print("<li>");
                            client.print(chatMessages[i]);
                            client.println("</li>");
                        }

                        client.println("</ul>");
                        client.println("</body></html>");
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }

                if (currentLine.startsWith("GET /?message=")) {
                    int startPos = currentLine.indexOf('=') + 1;
                    int endPos = currentLine.indexOf(" HTTP/");

                    if (startPos != -1 && endPos != -1) {
                        chatMessage = currentLine.substring(startPos, endPos);
                        if (chatMessage != chatMessages[messageCount - 1]) {
                            addChatMessage(chatMessage);
                        }

                        if (chatMessage == "led_on") {
                            digitalWrite(onboardLedPin, LOW);
                            ledOn = true;
                        } else if (chatMessage == "led_off") {
                            digitalWrite(onboardLedPin, HIGH);
                            ledOn = false;
                        }

                        if (chatMessage == "beep") {
                            beepRapidly(3000);  // Beep rapidly for 3 seconds
                        }
                    }
                }
            }
        }

        client.stop();
    }

    unsigned long currentTime = millis();
    if (currentTime - lastMemoryClear >= 300000) {
        clearOldMessages();
        lastMemoryClear = currentTime;
    }
}

void addChatMessage(String message) {
    if (messageCount < maxMessages) {
        chatMessages[messageCount] = message;
        messageCount++;
    } else {
        for (int i = 0; i < maxMessages - 1; i++) {
            chatMessages[i] = chatMessages[i + 1];
        }
        chatMessages[maxMessages - 1] = message;
    }
}

void clearOldMessages() {
    for (int i = 0; i < maxMessages; i++) {
        chatMessages[i] = "";
    }
    messageCount = 0;
}

void beepRapidly(unsigned long duration) {
    unsigned long startTime = millis();
    while (millis() - startTime < duration) {
        M5.Beep.tone(4000);  // Activate the internal buzzer with a 4000 Hz tone
        delay(100); // Beep for 100 ms
        M5.Beep.mute();  // Stop the buzzer
        delay(100); // Pause for 100 ms
    }
}
