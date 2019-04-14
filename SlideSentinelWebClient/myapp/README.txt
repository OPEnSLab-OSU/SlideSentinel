Installation: The web client requires node.js and express for node as well as several other packages. Install node.js and run npm install. The app will bind to port 9000 
by default, this port must be open for the system to be accessible  from outside the immediate network.


Usage: On first time start up the app must create credential files in order to access a google spreadsheet. If no credential files are present the app will prompt for an email
address and password. To start the webclient use the following command "node main.js"

Notes
index.css: style sheet for main page
index.html: html sheet for main page
index.js: Front end javascript to run content on main page, this includes creating the map area, map markers, and navigation buttons.

main.js Back end javascript to handle basic route handling and to load data from a google spreadsheet.

A live version of the webclient should be accessible at home.stallkamp.us:9000

