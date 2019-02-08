/*--------------------------------------------------------------------
//Originally published by Mogsdad@Stackoverflow
//Modified for jarkomdityaz.appspot.com
//Modified for Hackster.io by Stephen Borsay
//Modified for Slide Sentinel capstone by Kevin Koos
----------------------------------------------------------------------
GET request query:
gscript id = 1SufBxpLD6LKT_I2LWkUPODMCZl3Ns5vwwOwJfA5REmo
https://script.google.com/macros/s/<gscript id>/exec?celData=data_here
--------------------------------------------------------------------*/
//GLOBALS
//var id = '1SufBxpLD6LKT_I2LWkUPODMCZl3Ns5vwwOwJfA5REmo';

/*
 * Clears the data sheet
 */
function clearSheet() {
  var sheet = SpreadsheetApp.getActive().getSheetByName('data');
  sheet.getRange('A1:E1000').clearContent();
}
/*
 * Clear all sheets, not the columns names of the nmea sentence sheets
 */
function clearAllSheets() {
  var sheet = SpreadsheetApp.getActive().getSheetByName('data');
  sheet.getRange('A1:E1000').clearContent();
  sheet = SpreadsheetApp.getActive().getSheetByName('GGA');
  sheet.getRange('A2:N1000').clearContent();
  sheet = SpreadsheetApp.getActive().getSheetByName('STI 030');
  sheet.getRange('A2:N1000').clearContent();
  sheet = SpreadsheetApp.getActive().getSheetByName('STI 032');
  sheet.getRange('A2:N1000').clearContent();
  sheet = SpreadsheetApp.getActive().getSheetByName('RMC');
  sheet.getRange('A2:N1000').clearContent();
  sheet = SpreadsheetApp.getActive().getSheetByName('GSA');
  sheet.getRange('A2:N1000').clearContent();
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

/* These function put parameters into their sheet with their respective columns
 * GGA - base sentence, includes fix data
 * GSA - sentence provides information on satelittles used in the fix
 * RMC - (Recommended Minimum) sentence from NMea with position, velocity, and time
 * STI 030 - proprietary sentence from NavSpark, includes gps positions
 * STI 032 - proprietary sentence from NavSpark, includes rtk data
 */
function putGGA(data_dict, params, rowData) {
  var deg, min;
  rowData[0]  = stripQuotes(data_dict['deviceID']); //deviceID
  rowData[1]  = params[1];  //utc time
  rowData[2]  = params[0];  //type
  rowData[3]  = params[4];  //long
  rowData[4]  = params[2];  //lat
  rowData[5]  = params[9];  //altitude
  rowData[6]  = params[6];  //fix quality
  rowData[7]  = params[7];  //num of satelittes in fix
  rowData[8]  = params[8];  //horizontal dilution of position
  rowData[9]  = params[11]; //height above ellipsoid (sea level)
  rowData[10] = params[15]; //checksum
  
  //adjust lat and long 
  rowData[3] = decimalDegree(rowData[3]);
  rowData[4] = decimalDegree(rowData[4]);
  rowData[3] *= (params[5] == 'E' ? 1 : -1);
  rowData[4] *= (params[3] == 'N' ? 1 : -1);
}

function putGSA(data_dict, params, rowData) {
  rowData[0]  = stripQuotes(data_dict['deviceID']); //deviceID
  rowData[1] = stripQuotes(data_dict['Time']);
  rowData[2] = params[0]; //type
  rowData[3] = params[1]; //fix type
  rowData[4] = params[2]; //fix number
  
  rowData[5] = params[3] + ' ';      //replace what's there previously
  for (var i = 4; i < (12+3); i++) { //satellite PRNs deliminated by spaces
    rowData[5] += params[i] + ' ';
  }
  
  rowData[6] = params[15]; //general error
  rowData[7] = params[16]; //horizontal error
  rowData[8] = params[17]; //vertical error
  rowData[9] = params[18]; //checksum
  
  if (rowData[5] != null && /\S/.test(rowData[5])) {
    rowData[5] = "N/A";
  }
  
}

function putRMC(data_dict, params, rowData) {
  rowData[0]  = stripQuotes(data_dict['deviceID']); //deviceID
  rowData[1] = params[9]; //date
  rowData[2] = params[1]; //time
  rowData[3] = params[0]; //type
  rowData[4] = params[2]; //status
  rowData[5] = params[5]; //long
  rowData[6] = params[3]; //lat
  rowData[7] = params[7]; //velocity
  rowData[8] = params[8]; //angle of velocity
  
  //rowData[9] = params[10] + params[11]; //magnetic variation unused by gps
  rowData[9] = 'N/A';
  rowData[10] = params[13]; //checksum
  
  //adjust lat and long to +/-ddd.sssss
  rowData[5] = decimalDegree(rowData[5]);
  rowData[6] = decimalDegree(rowData[6]);
  rowData[5] *= (params[6] == 'E' ? 1 : -1);
  rowData[6] *= (params[4] == 'N' ? 1 : -1);
}

function putSTI030(data_dict, params, rowData) {
  rowData[0] = stripQuotes(data_dict['deviceID']); //deviceID
  rowData[1] = params[2]; //utc time
  rowData[2] = params[12]; //utc date
  rowData[3] = params[0]; //nmea type
  rowData[4] = params[6]; //long
  rowData[5] = params[4]; //lat
  rowData[6] = params[8]; //altitude
  rowData[7] = params[9]; //ew velocity
  rowData[8] = params[10]; //ns velocity
  rowData[9] = params[11]; //vertical velocity
  rowData[10] = params[13]; //eval mode
  rowData[11] = params[14]; //rtk age
  rowData[12] = params[15]; //rtk ratio
  rowData[13] = params[16]; //checksum
  
  //adjust lat and long to +/-ddd.sssss
  rowData[4] = decimalDegree(rowData[4]);
  rowData[5] = decimalDegree(rowData[5]);

  rowData[4] *= (params[7] == 'E' ? 1 : -1);
  rowData[5] *= (params[5] == 'N' ? 1 : -1);
}

function putSTI032(data_dict, params, rowData) {
  rowData[0] = stripQuotes(data_dict['deviceID']); //deviceID
  rowData[1] = params[3];  //utc date
  rowData[2] = params[2];  //utc time
  rowData[3] = params[4];  //status
  if (params[4] == 'A') {
    rowData[4] = params[5];  //mode
    rowData[5] = params[6];  //east projection
    rowData[6] = params[7];  //north projection
    rowData[7] = params[8];  //up projection
    rowData[8] = params[9];  //baseline length
    rowData[9] = params[10]; //baseline course
  } else { // == 'V'
    for (var i = 4; i < 10; i++) {
      rowData[i] = 'N/A';
    }
  }
  rowData[10] = params[16];//checksum
}

// Automatically called whenever the linked PushingBox Scenario
// has data sent to it in the form of key-value pairs. 
function doGet(e) { 
    
  var result = 'Ok'; // assume success

  if (e.parameter == undefined) {
    result = 'No Parameters';
  }
  else {    
    // Turn the Get Request arguments into a dictionary using the key-value pairs from the request.
    // Adds Date and Time fields to start of dictionary 
    var data_dict   = getDataDict(e.parameter);
        
    // The keys from data_dict is the set of columns in the spreadsheet.
    var ignore_keys = ["sheetID", "tabID", "null"];    
    var column_set = difference(Object.keys(data_dict), ignore_keys);
    
    // The name of the sheet for the data to go to is the IDtag field that is sent.
    var sheet_id    = stripQuotes(data_dict['tabID']);
    var spreadsheet = SpreadsheetApp.openById(data_dict['sheetID']);
    var sheet_list  = spreadsheet.getSheets();
    var sheet       = spreadsheet.getSheetByName(sheet_id);
    
    // If the tabID is new, then create a sheet for it.
    if (sheet == undefined) {
      sheet = spreadsheet.insertSheet(sheet_id);
      sheet_list = spreadsheet.getSheets();
    }
    
    // Variable to hold new row and where to put it in the spreadsheet 
    var new_range;
    var new_row  = sheet.getLastRow() + 1;
    var row_data = [];
    
    if (new_row <= 2) {
      sheet.appendRow(column_set);
      setSheetColumnList(sheet.getName(), column_set);
      new_row += 1;
    }
    
    // If the set of data received is different than previous for the sheet
    // Put a break between the two data sets and label the columns accurately.
    if (!arraysEqual(column_set, getSheetColumnList(sheet.getName()))) {
      if (new_row > 1) new_row += 1;
      new_range = sheet.getRange(new_row, 1, 1, column_set.length);
      new_range.setValues([column_set]);
      new_row += 1;
      setSheetColumnList(sheet.getName(), column_set);
    }
   
    // The data from the rows are the values from the dictionary where the key is the column name.
    for (i = 0; i < column_set.length; i++) {
      row_data.push(data_dict[column_set[i]]);
    }
    
    // Write the row to the sheet.
    new_range = sheet.getRange(new_row, 1, 1, row_data.length);
    new_range.setValues([row_data]); 
    
    // Parse NMEA_Data into segments
    // regex split on '*' and ',' (* for getting checksum) of nmea data
    //var params = e.parameter['NMEA_Data'].split(/[*,]+/);
    var params = stripQuotes(data_dict['NMEA_Data']).split(/[*,]/)
    if (params[0].slice(-3) == 'STI') {
      var sheet = SpreadsheetApp.openById(data_dict['sheetID']).getSheetByName(params[0].slice(-3) + ' ' + String(params[1]));
    } else {
      var sheet = SpreadsheetApp.openById(data_dict['sheetID']).getSheetByName(params[0].slice(-3));
    }
    var newRow = sheet.getLastRow() + 1;
    var rowData = [];
    
    // Call function based on sentence type
    switch(params[0].slice(-3)) {
      case 'GGA':
        putGGA(data_dict, params, rowData);
        break;
      case 'GSA':
        putGSA(data_dict, params, rowData);
        break;
      case 'RMC':
        putRMC(data_dict, params, rowData);
        break;
      case 'STI':
        if (params[1] == '030') {
          putSTI030(data_dict, params, rowData);
        } else if (params[1] == '032') {
          putSTI032(data_dict, params, rowData);
        } else {
          console.log("Unhandled PSTI format: " + String(params[1]));
        }
        break;
      default:
        console.log("Unhandled NMEA format: " + String(params[0]));
    }
    
    var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
    newRange.setValues([rowData]);
    
  }

  // Return result of operation
  return ContentService.createTextOutput(result);
}


// Takes in the arguments of the get request
// Formats it into a dictionary linking keys to their values.
function getDataDict(get_args) {
  
  var data_dict = {};
  var col_list = getMatches(get_args, "key[0-9]+");
  var val_list = getMatches(get_args, "val[0-9]+");
  
  
  // Add date and time columns
  // This could be customized further
  var d = new Date();
  data_dict["UTC"]  = d.toUTCString();
  data_dict["Date"] = d.toLocaleDateString();
  data_dict["Time"] = d.toLocaleTimeString();

  // Populate dictionary
  for (i = 0; i < col_list.length; i++) {
    if (val_list[i] != "null")
      data_dict[col_list[i]] = val_list[i];
  }
  
  // Convert data string to array and then dictionary
  var data_array = data_dict["full_data"].split('~');
  for (i = 0; i < data_array.length; i+=2) {
    data_dict[data_array[i]] = data_array[i+1];
  }
  
  // Remove the data string
  delete data_dict["full_data"];
 
  
  return data_dict;
}

// --- AUXILIARY FUNCTIONS ---
// Returns the subset of args that matched pattern patt as a list.
function getMatches(args, patt) {
  var list = [];
  var match_order = [];
  var key_list = Object.keys(args);
  var match_patt = new RegExp(patt);
  
  for (var key in key_list) {
    if(match_patt.test(key_list[key]))
      match_order.push(key_list[key]);
  }
  match_order.sort();

  for (var match in match_order)
      list.push(args[match_order[match]]);
  return list;
}

//Returns a string of columns delimited by ','
function getColumnString(col_list) {
  var col_string = '';
  for (var i in col_list) {
    col_string += (col_list[i] + ',')
  }
  return col_string.slice(0, col_string.length - 1);
}

// Returns a list of column names
function getColumnList(col_string) {
  return col_string.split(',');
}

// Sets the set of columns that a sheet is using
function setSheetColumnList(sheet_name, col_list) {
  var doc_properties = PropertiesService.getDocumentProperties();
  doc_properties.setProperty(sheet_name, getColumnString(col_list));
}

// Gets the set of columns that a sheet is using
function getSheetColumnList(sheet_name) {
  var doc_properties = PropertiesService.getDocumentProperties();
  return getColumnList(doc_properties.getProperty(sheet_name));
}

// Remove leading and trailing single or double quotes
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}

// Checks if two arrays are equal to each other.
function arraysEqual(a, b) {
  if (a === b)                return true;
  if (a == null || b == null) return false;
  if (a.length != b.length)   return false;
  
  for (var i = 0; i < a.length; ++i) {
    if (a[i] !== b[i]) return false;
  }
  return true;
}

// Finds the difference of two arrays
// Used to remove various columns from being printed
function difference(a1, a2) {
  var result = [];
  for (var i = 0; i < a1.length; i++) 
    if ((a2.indexOf(a1[i]) === -1) && (a1[i][0] != '$'))
      result.push(a1[i]);
  return result;
}
a
