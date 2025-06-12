# ESP32 LoRaWAN Emergency Communicator

A portable device for emergency communication using LoRaWAN, transmitting vital signs and GPS data. It includes an ML model for heart rate risk assessment.

## Features

*   LoRaWAN communication for long-range data transmission.
*   GPS tracking for location monitoring.
*   Heart rate monitoring.
*   On-device ML model for real-time heart rate risk assessment.
*   Multiple emergency modes (SOS, Backup, Medical).
*   OLED display for status and data visualization.
*   Button for mode selection.

## Hardware and Software Components

### Hardware

*   Heltec ESP32 LoRa V2 board (or similar)
*   GPS module (if not integrated)
*   Heart rate sensor (if not integrated)

### Software

*   PlatformIO IDE
*   Heltec ESP32 library
*   Robojax HeltecLoRa32 library
*   TensorFlow Lite for Microcontrollers

## Project Structure

*   `src/main.cpp`: Main application logic.
*   `include/model.h` & `include/model.cpp`: TensorFlow Lite model for heart rate risk assessment.
*   `lib/`: Contains external libraries.
*   `platformio.ini`: Project configuration file.

## Setup and Usage

### Building and Uploading

1.  Install [PlatformIO IDE](https://platformio.org/platformio-ide).
2.  Clone this repository.
3.  Open the project in PlatformIO IDE.
4.  Configure LoRaWAN settings in `src/main.cpp` (e.g., `RF_FREQUENCY`, LoRaWAN keys if applicable).
5.  Build and upload the firmware to your ESP32 board.

### Operation

*   The device starts in **VITALS_MODE**, transmitting heart rate, GPS, and risk assessment data.
*   Press the button on GPIO0 to cycle through emergency modes:
    *   **SOS_MODE**: Transmits an SOS emergency message.
    *   **BACKUP_MODE**: Transmits a request for backup.
    *   **MEDICAL_MODE**: Transmits a request for medical assistance.
*   The OLED display shows the current mode and relevant data.

## Future Improvements

*   Integrate additional sensors (e.g., temperature, fall detection).
*   Improve the accuracy and features of the ML model.
*   Enhance power efficiency for longer battery life.
*   Implement OTA (Over-The-Air) updates.
