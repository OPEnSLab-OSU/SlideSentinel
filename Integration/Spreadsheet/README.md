## Set Up
To set this up for another account you must import the spreadsheet and then under tools -> script editors, copy and paste the whole script inside. At the top of the webpage in the url there will be https://docs.google.com/spreadsheets/d/<YOUR SPREADSHEET ID HERE>/edit#gid=0, copy the id string into the id variable at the top of the script. Save the script and then under publish ->  deploy as web app, set the user to be "Me" and set the access permissions to everyone including anonymous. Set version to NEW. Any changed to the script must be saved and deployed as a new version or changes will not be in effect. The script url provided can then be used to send data to sheet. More verbose logging can be accessed through view -> stackdriver logging. This sheet is configured to receive data from Rock7 servers which is hex encoded, this can be disabled for testing by commenting and uncommenting out lines 37 and 38. 
  
## Testing
The script can be tested with a rockblock that is set up with a hub and node or with a http request. Note: due to Rock7's web interface, a middleman is needed to ferry messages to google sheets, see POSTproxy. The script looks for the data query string variable in the body of a request under the key 'data'. Here is one test data message.
 - Hex: 2f4750532c69302c733233333731332e3030302c73343433332e393234303336332c734e2c7331323331362e383932363433362c73572c733132342e3037372c73442c73302e302c73302e3020232f53746174652c73302c732d3339332c733230312c73343034392c73332e33363030303020
 - Ascii: /GPS,i0,s233713.000,s4433.9240363,sN,s12316.8926436,sW,s124.077,sD,s0.0,s0.0 #/State,s0,s-393,s201,s4049,s3.360000 
 
 You can send a test message using the command:
 
 ```
 curl -i -d "data=<YOUR MESSAGE>" <YOUR SPREADSHEET SCRIPT URL>
 ```
