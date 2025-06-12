#include <Wire.h>
#include <SSD1306Wire.h>
#include "heltec.h"
#include <Robojax_HeltecLoRa32.h>
#include "model.h" // <-- Include your ML model header

// Emergency types
enum EmergencyType {
    VITALS_MODE,  // Normal mode - sends vitals + GPS
    SOS_MODE,     // Emergency modes - only send emergency message
    BACKUP_MODE,
    MEDICAL_MODE
};

void OnTxDone(void);
void OnTxTimeout(void);
void cycleEmergencyMode(void);
void transmitData(void);
void displayCurrentMode(void);

// Display setup
SSD1306Wire oledDisplay(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);
Robojax_HeltecLoRa32 robojaxDisplay(&oledDisplay);

#define MENU_BUTTON_PIN 0  // Button on GPIO0
EmergencyType currentMode = VITALS_MODE;
unsigned long lastTransmission = 0;
const unsigned long txInterval = 3000; // Send every 3 seconds

// Variables to store the last transmitted data
int lastTransmittedHR = 0;
float lastTransmittedLat = 0;
float lastTransmittedLon = 0;
int lastRiskRating = 0; // <-- Store last risk rating

// Hardcoded data for normal mode
struct {
    int heartRate = 72;       // Normal resting heart rate
    float latitude = 40.7128; // New York coordinates
    float longitude = -74.0060;
} vitalData;

// LoRa Configuration
#define RF_FREQUENCY 865875000 // Hz
#define TX_OUTPUT_POWER 20     // Power in dBm
#define LORA_SPREADING_FACTOR 9 // SF9 for better range
#define LORA_BANDWIDTH 125E3   // 125 kHz bandwidth
#define LORA_CODINGRATE 5      // 4/5 coding rate
#define BUFFER_SIZE 128

char txpacket[BUFFER_SIZE];
bool transmitting = false;

void setup() {
    Serial.begin(115200);

    // Initialize Heltec board
    Heltec.begin(true /*DisplayEnable*/, true /*LoRaEnable*/, true /*SerialEnable*/);

    pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
    robojaxDisplay.begin();

    // Initialize LoRa with configuration
    Heltec.LoRa.setFrequency(RF_FREQUENCY);
    Heltec.LoRa.setTxPower(TX_OUTPUT_POWER);
    Heltec.LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
    Heltec.LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    Heltec.LoRa.setCodingRate4(LORA_CODINGRATE);
    Heltec.LoRa.enableCrc();

    Serial.println("Transmitter Started");
    Serial.println("Normal Mode: Sending vitals+GPS");
    Serial.println("Press button to cycle modes");

    displayCurrentMode();
}

void loop() {
    // Check button press (with debounce)
    if(digitalRead(MENU_BUTTON_PIN) == LOW) {
        delay(50); // Debounce
        if(digitalRead(MENU_BUTTON_PIN) == LOW) {
            cycleEmergencyMode();
            while(digitalRead(MENU_BUTTON_PIN) == LOW); // Wait for release
        }
    }

    // Transmit at regular intervals
    if(!transmitting && (millis() - lastTransmission >= txInterval)) {
        transmitData();
        lastTransmission = millis();
    }

    displayCurrentMode();
    delay(100);
}

void cycleEmergencyMode() {
    currentMode = (EmergencyType)((currentMode + 1) % 4); // Cycle through all modes

    Serial.print("Mode changed to: ");
    switch(currentMode) {
        case VITALS_MODE: 
            Serial.println("Normal (Vitals+GPS)"); 
            break;
        case SOS_MODE: 
            Serial.println("SOS Emergency"); 
            break;
        case BACKUP_MODE: 
            Serial.println("Backup Request"); 
            break;
        case MEDICAL_MODE: 
            Serial.println("Medical Assistance"); 
            break;
    }
}

void transmitData() {
    if(transmitting) return; // Prevent overlapping transmissions

    memset(txpacket, 0, BUFFER_SIZE);

    // Add small random variations to simulate real data
    int hrVariation = random(-3, 4); // Small HR variation
    float gpsVariation = random(-10, 11) * 0.0001; // Small GPS variation

    if(currentMode == VITALS_MODE) {
        // Store the actual values being transmitted
        lastTransmittedHR = vitalData.heartRate + hrVariation;
        lastTransmittedLat = vitalData.latitude + gpsVariation;
        lastTransmittedLon = vitalData.longitude + gpsVariation;

        // --- ML model: get risk rating ---
        lastRiskRating = get_hr_risk_rating(lastTransmittedHR);

        // Only in normal mode - send vitals, GPS, and risk
        snprintf(txpacket, BUFFER_SIZE,
                "VITALS:HR:%d,RISK:%d,GPS:%.6f,%.6f",
                lastTransmittedHR,
                lastRiskRating,
                lastTransmittedLat,
                lastTransmittedLon);
    } else {
        // Emergency modes - just send emergency message
        const char* emergencyMsg = "";
        switch(currentMode) {
            case SOS_MODE: emergencyMsg = "SOS_EMERGENCY"; break;
            case BACKUP_MODE: emergencyMsg = "NEED_BACKUP"; break;
            case MEDICAL_MODE: emergencyMsg = "MEDICAL_HELP"; break;
        }
        snprintf(txpacket, BUFFER_SIZE, "EMERGENCY:%s", emergencyMsg);
    }

    Serial.printf("Transmitting: %s\n", txpacket);

    transmitting = true;
    Heltec.LoRa.beginPacket();
    Heltec.LoRa.print(txpacket);
    Heltec.LoRa.endPacket();
    transmitting = false;

    OnTxDone(); // Call completion handler
}

void displayCurrentMode() {
    oledDisplay.clear();

    // Display mode header
    const char* modeText = "";
    switch(currentMode) {
        case VITALS_MODE: modeText = "NORMAL"; break;
        case SOS_MODE: modeText = "SOS"; break;
        case BACKUP_MODE: modeText = "BACKUP"; break;
        case MEDICAL_MODE: modeText = "MEDICAL"; break;
    }

    robojaxDisplay.displayLineText(modeText, 0, 0, 16, (currentMode != VITALS_MODE));
    robojaxDisplay.displayText("(TX)", 127, 0, 10, TEXT_ALIGN_RIGHT);

    if(currentMode == VITALS_MODE) {
        // Show vitals, risk, and GPS in normal mode
        String hrText = "HR:" + String(lastTransmittedHR) + "bpm";
        String riskText = "Risk:" + String(lastRiskRating);
        String gpsText = String(lastTransmittedLat, 4) + "," + String(lastTransmittedLon, 4);

        robojaxDisplay.displayText(hrText.c_str(), 0, 18, 14, TEXT_ALIGN_LEFT);
        robojaxDisplay.displayText(riskText.c_str(), 0, 34, 12, TEXT_ALIGN_LEFT);
        robojaxDisplay.displayText(gpsText.c_str(), 0, 50, 10, TEXT_ALIGN_LEFT);
    } else {
        // Show emergency alert
        robojaxDisplay.displayText("EMERGENCY", 0, 25, 14, true);
        robojaxDisplay.displayText("TRANSMITTING...", 0, 45, 10, TEXT_ALIGN_LEFT);
    }

    oledDisplay.display();
}

void OnTxDone(void) {
    Serial.println("TX complete");
}

void OnTxTimeout(void) {
    Serial.println("TX timeout");
}