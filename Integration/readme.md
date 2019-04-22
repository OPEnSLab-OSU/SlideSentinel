# Slide Sentinel Phase 1 Prototype

This folder contains all of the code for producing a new Rover and Base station pair. This device is intended for collecting data on landslide movement with sub-centimeter positional accuracy. The system logs rover position at set intervals, and uploads the highest quality positional fix via a satcom upload using the ROCKBLOCK+. 

## Getting Started

Inspect the Rover/Rover.png and Base/Base.png schemtic images for a detailed schematic of all components used in the system build. Using the Arduino IDE load the Base device with Base/Base.ino and the Rover device with Rover/Rover.ino. 

## Hardware

### Base
- ROCKBLOCK+ Iridium Satcom Module
- Freewave Z9-T radio
- RTK - GPS Base

### Rover
- MMA8451 accelerometer with interrups
- DS3231 real time clock with interrupts
- Datalogger
- Latching relay for power management
- Freewave Z9-T radio
- RTK - GPS Rover

## Files
1. **Base:** All files for the Base unit of the system
2. **Rover:** All files for the Rover unit for the system
3. **POSTproxy:** Source code for the post proxy web application. Required for successful Google Spreadsheets HTTP requests.
4. **Spreadsheet:** Google spreadsheet script for displaying recorded data.
5. **EditLib:** Rover unit contains direct library edits. This folder contains copies of editted library files, and original copies of library files for the Base unit.
