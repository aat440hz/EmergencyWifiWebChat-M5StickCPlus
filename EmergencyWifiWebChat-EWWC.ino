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

void setup() {
    M5.begin();
    M5.lcd.setRotation(3);
    M5.lcd.println("Page refresh every 60 seconds, chat messages disappear after 10 minutes");
    M5.lcd.printf("Connect to: %s\n", ssid);
    
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    M5.lcd.println(myIP);
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
                        // Serve the chat interface HTML
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.println("<html><head>");
                        client.println("<meta http-equiv='refresh' content='60;url=/'>"); // Auto-refresh to 192.168.4.1 every 60 seconds
                        client.println("</head><body>");
                        client.println("<h1>WiFi Chat</h1>");
                        client.println("<form method='GET'>");
                        client.println("<input type='text' name='message' placeholder='Enter your message'><br>");
                        client.println("<input type='submit' value='Send'>");
                        client.println("</form>");

                        // Display chat messages
                        client.println("<h2>Chat Messages:</h2>");
                        client.println("<ul>");
                        
                        // Display the most recent messages (up to maxMessages) in descending order
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
                            // Add the message only if it's different from the last one
                            addChatMessage(chatMessage);
                        }
                    }
                }
            }
        }

        client.stop();
    }

    // Check if it's time to clear old messages and free up memory
    unsigned long currentTime = millis();
    if (currentTime - lastMemoryClear >= 600000) {  // Clear memory every 10 minutes
        clearOldMessages();
        lastMemoryClear = currentTime;
    }
}

void addChatMessage(String message) {
    if (messageCount < maxMessages) {
        chatMessages[messageCount] = message;
        messageCount++;
    } else {
        // If the message log is full, shift messages to make space for the new message
        for (int i = 0; i < maxMessages - 1; i++) {
            chatMessages[i] = chatMessages[i + 1];
        }
        chatMessages[maxMessages - 1] = message;
    }
}

void clearOldMessages() {
    // Clear all messages and reset the message count
    for (int i = 0; i < maxMessages; i++) {
        chatMessages[i] = "";
    }
    messageCount = 0;
}
