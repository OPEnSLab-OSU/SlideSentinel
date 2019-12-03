var path = require('path');
var express = require('express');
var app = express();
var dir = path.join(__dirname, 'public');
console.log(dir);
app.use(express.static(dir));
const readline = require('readline');
const {google} = require('googleapis');
// If modifying these scopes, delete token.json.
const SCOPES = ['https://www.googleapis.com/auth/spreadsheets.readonly'];
// The file token.json stores the user's access and refresh tokens, and is
// created automatically when the authorization flow completes for the first
// time.
const TOKEN_PATH = 'token.json';
var fs = require("fs");
var http = require("http");
var url = require("url");
var credentials;
var spreadSheet;
var sheetNames =[];
//const SSID = '1SufBxpLD6LKT_I2LWkUPODMCZl3Ns5vwwOwJfA5REmo' //demo sheet v1
//const SSID = '1xCLHJguvSOqCUXEvNzqJkmj2Ieb0RGNLa9eSj50y3YQ' //my sheet
const SSID = '10DkOqGIkRkOmSKsknAcFsjAp9_pjuEXmgO6n-IdT67c' //extended demo sheet v2


function loadSheetNames(callback)
{
	credentials = fs.readFileSync("credentials.json", "utf8");
	authorize(JSON.parse(credentials), getSheetNames,callback);
	return;
}
function loadData(callback) {
  
  credentials = fs.readFileSync("credentials.json", "utf8");
  authorize(JSON.parse(credentials), getData,callback);
  return;
}
/**
 * Create an OAuth2 client with the given credentials, and then execute the
 * given callback function.
 * @param {Object} credentials The authorization client credentials.
 * @param {function} callback The callback to call with the authorized client.
 */
function authorize(credentials, getdatacallback,writecallback) {
  const {client_secret, client_id, redirect_uris} = credentials.installed;
  const oAuth2Client = new google.auth.OAuth2(
      client_id, client_secret, redirect_uris[0]);

  // Check if we have previously stored a token.
  fs.readFile(TOKEN_PATH, (err, token) => {
    if (err) return getNewToken(oAuth2Client, getdatacallback,writecallback);
    oAuth2Client.setCredentials(JSON.parse(token));
    getdatacallback(oAuth2Client,writecallback);
  });
}
/**
 * Get and store new token after prompting for user authorization, and then
 * execute the given callback with the authorized OAuth2 client.
 * @param {google.auth.OAuth2} oAuth2Client The OAuth2 client to get token for.
 * @param {getEventsCallback} callback The callback for the authorized client.
 */
function getNewToken(oAuth2Client, callback,writecallback) {
  const authUrl = oAuth2Client.generateAuthUrl({
    access_type: 'offline',
    scope: SCOPES,
  });
  console.log('Authorize this app by visiting this url:', authUrl);
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });
  rl.question('Enter the code from that page here: ', (code) => {
    rl.close();
    oAuth2Client.getToken(code, (err, token) => {
      if (err) return console.error('Error while trying to retrieve access token', err);
      oAuth2Client.setCredentials(token);
      // Store the token to disk for later program executions
      fs.writeFile(TOKEN_PATH, JSON.stringify(token), (err) => {
        if (err) console.error(err);
        console.log('Token stored to', TOKEN_PATH);
      });
      callback(oAuth2Client,writecallback);
    });
  });
}
/**
 * Prints the names and majors of students in a sample spreadsheet:
 * @see https://docs.google.com/spreadsheets/d/1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms/edit
 * @param {google.auth.OAuth2} auth The authenticated Google OAuth client.
 */
function getData(auth,writecallback) {
  service = google.sheets({version: 'v4', auth});
  
  //my test sheet id 1xCLHJguvSOqCUXEvNzqJkmj2Ieb0RGNLa9eSj50y3YQ
  //apparaently other sheets can be referened in ranges?
  //found here:https://stackoverflow.com/questions/46385047/get-and-update-cells-in-individual-google-sheet-specific-sheet-in-spreadsheet
  //https://developers.google.com/sheets/api/guides/concepts
  service.spreadsheets.values.get({
    spreadsheetId: SSID, 
    range: customSheetName,
  }, (err, res) => {
    if (err) return console.log('The API returned an error: ' + err);
	rows = res.data.values;
    if (rows.length) {
      console.log('Data Retrieved.');
	  chartData = JSON.stringify(rows);
	  writecallback();
    } else {
      console.log('No data found.');
    }
	return;
  });
}

function getSheetNames(auth, writecallback)
{
	service = google.sheets({version: 'v4', auth});
	sheet_metaData = service.spreadsheets.get({
    spreadsheetId: SSID,
  }, (err, res) => {
		if (err)
		{			
			return console.log('The API returned an error: ' + err);
		}
		else
		{
			sheets = res.data.sheets;
			for (var i = 0;i<sheets.length;i++)
			{
				tmp = sheets[i].properties.title;
				sheetNames[i] = tmp;
			}
			writecallback();
		}
	});
	
	return;
}
app.get('/', function (req, res) {
	
    res.writeHead(200);
    html = fs.readFileSync("index.html", "utf8");
    res.write(html);
    res.end();
})
app.get('/index.css', function(req,res) {
	script = fs.readFileSync("index.css", "utf8");
    res.write(script);
	res.end();
})
app.get('/index.js', function(req,res) {
	script = fs.readFileSync("index.js", "utf8");
    res.write(script);
	res.end();
})
app.get('/main.js', function(req,res) {
	var writeDataCallBack = function() {
		res.write(chartData);
		res.end();
	}
	customSheetName = req.url.split("?",2)[1];
	loadData(writeDataCallBack);

})
app.get('/loadSheetNames', function(req,res) {
	var writeSheetNamesCallBack = function() {
		res.write(sheetNames.toString());
		res.end();
	}
	
	loadSheetNames(writeSheetNamesCallBack);
})
app.listen(9001)