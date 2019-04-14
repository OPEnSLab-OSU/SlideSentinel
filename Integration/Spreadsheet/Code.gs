// This is a snapshot of the google sheets script for the April-May Slide Sentinel demonstration
// any sensitive information has been removed


/*--------------------------------------------------------------------
//
Originally published by Mogsdad@Stackoverflow
//Modified for jarkomdityaz.appspot.com
//Modified for Hackster.io by Stephen Borsay
//Modified for Slide Sentinel capstone by Kevin Koos
----------------------------------------------------------------------

FORMAT OF DATA STRINGS
'GPS', NODE ID, UTC, LAT, LONG, ALT, MODE, AGE, RATIO, 'State', NODE ID, ACCEL X, ACCEL Y, ACCEL Z, VOLTAGE
Most fields are from the STI 030 nmea format
--------------------------------------------------------------------*/
//GLOBALS
var field_delim = ',';
var node_delim = '#';

// forward GET requests to POST request handler
function doGet(e) {
  doPost(e);
}

// Responds to POST requests
function doPost(e) { 
  
  console.log( JSON.stringify(e) );  // view parameters (stackdriver)
  
  var result = 'Ok'; // assume success

  if (e.parameter == undefined) {
    result = 'No Parameters';
  } else {
    var id = 'REDACTED';//docs.google.com/spreadsheetURL/d
    var sheet = SpreadsheetApp.openById(id).getActiveSheet();
    
     
    //ignore other fields from iridium metadata and just get the data
    var rawdata = JSON.stringify(hex2a(e.parameter['data']));
    //var rawdata = e.parameter['data'];
    
    //strip quotes
    rawdata = stripQuotes(rawdata);
       
    if(rawdata[0] != '#') {
      rawdata = '#' + rawdata;
    }
    
    console.log(rawdata);
    
    //split on node deliminator to get list of nodes
    var nodes = rawdata.split(node_delim); 
 
    //split on field deliminator to get a list of lists of fields
    var data = nodes.map(function(elem) {
    return elem.split(field_delim);      
    });

    data.shift();
    console.log(data);
  
  
  for (var i = 0; i < data.length; i++) {
    for (var j = 0; j < data[i].length; j++){
      data[i][j] = data[i][j].substring(1);
    }
  }
    
  var rowData = [];
  var newRow = sheet.getLastRow() + 1;
  var gpsQueue = [];
  var stateQueue = [];
  var newRange;

  
  
  //iterate over 
  var stringtype;
  for (var i = 0; i < data.length; i++) { 
    stringtype = data[i][0];
    data[i].shift(); //get rid of the indicating string now
    switch(stringtype) {
      case 'GPS':
        gpsQueue.push(data[i]);
        break;
      case 'State':
        stateQueue.push(data[i]);
        break;
    }
    
  } 
  
  var numNode = Math.max(gpsQueue.length, stateQueue.length);

    for (var i = 0; i < numNode; i++) {
      newRow = sheet.getLastRow() + 1;
      rowData = [];
      
      // if just state string then add info and add to row immediatly
      if (gpsQueue.length == 0) {
        var d = new Date();
        rowData.push(d.toDateString());
        rowData.push(d.toLocaleTimeString().slice(0, -3));
        rowData.push(stateQueue[0][0]);
        for (var i = 3; i < 9; i++) {
          rowData.push('N/A');
        }
        rowData = rowData.concat(stateQueue.shift());
        rowData.splice(9, 1);
        
      } else {
        
        // just gps string, concat and skip checking node id of state string
        if (stateQueue.length == 0) {
          rowData = rowData.concat(gpsQueue.shift());
          for (var j = 8; j < 13; j++) {
            rowData.push('N/A');
          }
        } else {
          //push on first gps and state string.    
          rowData = rowData.concat(gpsQueue.shift());
          rowData = rowData.concat(stateQueue.shift());
          
          //check node ids, if they dont match continue on
          if (rowData[0] != rowData[10]) {
            console.log(rowData[0]);
            console.log(rowData[10]);
            continue;
          }
        }
        
        console.log(rowData);
        
        // swap node id and time
        rowData.swap(0,1);
        
        // convert ddm to decimal degree format
        rowData[2] = decimalDegree(rowData[2]);
        rowData[4] = decimalDegree(rowData[4]);
        
        // mode indicator
        rowData[7] = modeIndicator(rowData[7]);
        
        // adjust for N/S or E/W for lat and long
        rowData[3] *= (rowData[3] == 'N' ? 1 : -1);
        rowData[4] *= (rowData[5] == 'E' ? 1 : -1);
        // remove them from row
        rowData.splice(5, 1);
        rowData.splice(3, 1);
        
        // remove second node id
        rowData.splice(8, 1);
        
        // get local date and time of the recorded time    
        var datetime = getDateTime(new Date(), rowData[0]);
        rowData[0] = datetime.time;
        rowData.unshift(datetime.date);
        
        
      }
      
      // Write new row below
      newRange = sheet.getRange(newRow, 1, 1, rowData.length);
      newRange.setValues([rowData]);
    }
  
    // sort the newly added data in ascending order from date and time
    var range = sheet.getRange("A2:M");
    range.sort([{column: 1, ascending: true}, {column: 2, ascending: true}]);
    
  }
  // Return result of operation
  return ContentService.createTextOutput(result);
  
}

/*
 * Remove leading and trailing single or double quotes
 */
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}

/* 
 * Convert hex to an ascii string 
 */
function hex2a(hexx) {
    var hex = hexx.toString();//force conversion
    var str = '';
    for (var i = 0; (i < hex.length && hex.substr(i, 2) !== '00'); i += 2)
        str += String.fromCharCode(parseInt(hex.substr(i, 2), 16));
    return str;
}

/*
 * Convert degree + decimal minutes (ddmm.mmmm) to decimal degrees 
 */
function decimalDegree(ddm) {
  var deg = Math.floor(ddm / 100);
  var min = (ddm-(deg*100)) / 60;
  
  return (deg  + min);
}

/*
 * Figures if the time that a data point was recorded has was recorded the day of receiving or
 * the day before. Formats date and time as well with am and pm. Will work in any timezone
 * local time = utc time - 8 hrs
 * returns correct data and time strings in obj
 */
function getDateTime(date_received, utctime_recorded) {
  var rectime = utctime_recorded.match(/.{1,2}/g);
  var offset = date_received.getTimezoneOffset(); //get offset in # of minutes
  var mytime = date_received.toTimeString();
  
  // get the local hour/min time of the time recorded
  rectime[0] = String((Number(rectime[0]) - Math.floor(offset/60)).mod(24)); 
  rectime[1] = String((Number(rectime[1]) - (offset).mod(60)).mod(60));
  
  // get the universal ms time of received time and supposed recorded time on same day
  var this_time = date_received.getTime();
  var new_date = new Date();
  new_date.setHours(rectime[0], rectime[1], rectime[2]);
  var new_time = new_date.getTime();
  
  // check if the recorded time was taken on the same day or not and set its current date
  var mydate;
  if ( new_time > this_time) {
    mydate = new Date(date_received.getTime() - 86400000);
  } else {
    mydate = new Date(date_received.getTime());
  }

  // time string to print
  var rectimenew = String(Number(rectime[0]).mod(12)) + ':' + rectime[1] + ':' + rectime[2];
  
  // am or pm
  if ( rectime[0] >= 12 ) {
    rectimenew = rectimenew + ' PM';
  } else {
    rectimenew = rectimenew + ' AM';
  }
  
  return { "date": mydate.toLocaleDateString() , "time": rectimenew };
} 

/*
 * Add a proper mod operator to numbers
 */
Number.prototype.mod = function(n) {
    return ((this%n)+n)%n;
};

/*
 * swap function for arrays.
 */ 
Array.prototype.swap = function(x,y) {
  var b = this[x];
  this[x] = this[y];
  this[y] = b;
  return this;
}

/*
 * Convert Navspark STI 030 mode indicator to its real meaning
 */
function modeIndicator(mode) {
  var str;
  switch(mode) {
    case 'N':
      str = 'Data Not Valid';
      break;
    case 'A':
      str = 'Autonomous';
      break;
    case 'D':
      str = 'Differential';
      break;
    case 'E':
      str = 'Estimated';
      break;
    case 'M':
      str = 'Manual';
      break;
    case 'S':
      str = 'Simulation';
      break;
    case 'F':
      str = 'Float RTK';
      break;
    case 'R':
      str = 'RTK';
      break;
  }
  return str;
}

/*
 * Clears the data sheet
 */
function clearSheet() {
  var sheet = SpreadsheetApp.getActive().getSheetByName('data');
  sheet.getRange('A2:M').clearContent();
}
