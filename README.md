# SlideSentinel
[![DOI](https://zenodo.org/badge/136069337.svg)](https://zenodo.org/badge/latestdoi/136069337)
## How to Setup
Install the ardiono IDE, follow the adafruit [guide](https://learn.adafruit.com/adafruit-feather-m0-basic-proto/overview), and install LOOM with it dependencies with its guide [here](https://github.com/OPEnSLab-OSU/InternetOfAg/tree/master/Arduino_and_Loom_Setup)

Latest Node code is located in the "integration/DEMO_NODE" directory and the latest hub code is located in the "integration/Hub" directory. Load up the main file from the directory in arduino IDE and connect the device in with the nesseary additions listed out in the bill of materials in the documentation directory. Note: Some additional devices requires configuring the drivers for use with the system.

The spreadsheet visualizer and parser is located at "integration/spreadsheet". A setup guide is included within along with some sample data.

The node application which facilitates traffic from the Rock7 servers to google spreadsheets is located at "integration/POSTproxy". A setup guide is included within. The application is active at https://postproxy.azurewebsites.net/. 

The mapping application is located at "SlideSentinelWebClient/myapp". An installation guide is included. This application hooks into the spreadsheet and pulls data to display is on a map. An active version of the map is avaliable [here](http://home.stallkamp.us:8999)

## Directory Layout
- Devices contains development, testing, and drivers for specific devices in the system.
- Documentation contains an assortment of documentation including data sheets, blog posts, notes, and the capstone documentation.
- Integration contains mostly all the up to date and in use code for the project. Some directories within are backups of previous demontrations of the system in different iterations.
- Phase 1 Legacy Code contains some old hub code when the project was just begining.
- SlideSentinelWebClient contains the node app for the mapping application.
