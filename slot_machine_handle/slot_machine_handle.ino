#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <FastLED.h>

const char* preferred_ssid = "ApeFest";         // Preferred WiFi network
const char* preferred_password = "apefest-wifi-password";

const char* backup_ssid = "MILL";                  // Backup WiFi network
const char* backup_password = "mill-wifi-password";

WiFiMulti WiFiMulti;

unsigned long lastDebounceTime[3] = {0, 0, 0};  // debounce timers for each pin
unsigned long debounceDelay = 50;               // debounce time
bool lastButtonState[3] = {HIGH, HIGH, HIGH};   // previous states of each pin
const int buttonPins[3] = {32, 26, 39};         // the GPIO pins we are monitoring

unsigned long startTime = 0;
bool usingPreferredNetwork = true;
bool wifiAttempted = false;

CRGB mainLED;

void setup() {    
    M5.begin();   // Initialize M5 Atom

    FastLED.addLeds<NEOPIXEL, 27>(&mainLED, 1);
    FastLED.setBrightness(10);

    Serial.begin(115200);
    Serial.println("start");

    for (int i = 0; i < 3; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);    // Setup pins as INPUT with pull-up resistors
    }

    // Add preferred network first
    WiFiMulti.addAP(preferred_ssid, preferred_password);
    startTime = millis();   // Record the start time for trying the preferred network

    Serial.print("Attempting to connect to preferred WiFi: ");
    Serial.println(preferred_ssid);

    // Set LED to Yellow while attempting to connect to WiFi
    mainLED = CRGB::Yellow;
    FastLED.show();
}

void loop() {
    // Attempt connection to WiFi
    if (WiFiMulti.run() != WL_CONNECTED) {
        // If 60 seconds (60000 milliseconds) have passed and still not connected, switch to backup network
        if (usingPreferredNetwork && millis() - startTime >= 60000) {
            Serial.println("Switching to backup WiFi network...");
            WiFi.disconnect(true);  // Disconnect from current WiFi, clearing the state
            WiFiMulti.addAP(backup_ssid, backup_password);  // Add the backup network
            usingPreferredNetwork = false;
            startTime = millis();  // Reset the start time for the backup network
            Serial.print("Attempting to connect to backup WiFi: ");
            Serial.println(backup_ssid);
        } else {
            Serial.println("Still trying to connect to WiFi...");
            Serial.println(backup_ssid);
        }
        delay(1000);  // Wait before retrying
        return;  // Skip rest of the loop until WiFi is connected
    } else {
        if (!wifiAttempted) {
            Serial.println("WiFi connected.");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            wifiAttempted = true;

            // Set LED to Green when WiFi is connected
            mainLED = CRGB::Green;
            FastLED.show();
        }
    }

    // Check each pin for a state change
    for (int i = 0; i < 3; i++) {
        bool buttonState = digitalRead(buttonPins[i]);

        // Debouncing logic
        if (buttonState != lastButtonState[i]) {
            lastDebounceTime[i] = millis();  // Reset the debounce timer for this pin
        }

        if ((millis() - lastDebounceTime[i]) > debounceDelay) {
            if (buttonState == LOW) {
                // Set LED to Blue while button is pressed
                mainLED = CRGB::Blue;
                FastLED.show();

                // HTTP request logic
                const char* host = "http://pi-matrix.local/animate/ApeFestSlotMachine";
                const uint16_t port = 80;
                Serial.print("Connecting to: ");
                Serial.println(host);

                HTTPClient http;
                // Set a timeout of 300000 milliseconds (5 minutes)
+               http.setTimeout(300000);
                
                http.begin(host);

                int httpCode = http.GET();
                if (httpCode > 0) {
                    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
                    if (httpCode == HTTP_CODE_OK) {
                        String payload = http.getString();
                        Serial.println(payload);
                    }
                } else {
                    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                }

                http.end();
                delay(3000);  // Delay to prevent multiple requests on a single press

                // Restore LED to Green after HTTP request
                mainLED = CRGB::Green;
                FastLED.show();
            }
        }

        lastButtonState[i] = buttonState;  // Update the previous state for this pin
    }
}
