/* Name: app.js
* Description: proxy post requests from an incoming request to another specified server
*              always responds with 200 http status code to the incoming server
* Author: Kevin Koos
* Use: Slide Sentinel project for OSU capstone
* Node Version: Developed on v11.10.0 but should work on anything past v4.4.5
*               recommended to use anything later than v6 Node.
* Host: Hosted on Microsoft's Azure Cloud network at: 
*/

const http    = require('http');
const request = require('request');
const url     = require('url');
const path    = require('path');
const port    = process.env.PORT || 3000;
const qs      = require('querystring');
const bcrypt  = require('bcrypt');
const shortid = require('shortid');

const fs      = require('fs');
var data      = fs.readFileSync('./config.json');
var config    = JSON.parse(data);

//bcrypt hashed password
const myhash = '$2b$10$QOGKLUnarqQFHgve0GoMpOs.p6f1P.bMJnXe2YUixneOUP3sNd7OS';


//validate the url for the existence of all the needed parameters to add/change/or remove from config
function validateURL(query) {
    if (query.id == 'NEW') {
        if ('url' in query && 'requesttype' in query && 'pass' in query) {
            return true;
        }
    } else if (query.id == 'EXISTING') {
        if ('url' in query && 'devid' in query && 'requesttype' in query && 'pass' in query) {
            return true;
        }
    } else if (query.id == 'REMOVE') {
        if ('devid' in query && 'pass' in query) {
            return true;
        }
    }
    return false;
}


//validate a id as a devid from the config and return index
function getIdIndex(id) {
    var index = config.devices.findIndex( (ele) => {
        return ele.devid == id;
    });
    
    return index
}


//add url to config and return generated devid
function addURL(query) {
    //delete unnessesary key/vals
    delete query['id'];
    delete query['pass'];
    
    //gen id and construct object
    var str = query;
    var new_id = shortid.generate();
    str['devid'] = new_id;
    config.devices.push(str);
    
    //write to data object
    data = JSON.stringify(config, null, 2);
    
    //async call write to file
    fs.writeFile('./config.json', data, (err) => {
        if (err != null) {
            console.log(err);
        }
    });
    return new_id;
}


//changed existing devid, return true or false on if devid is valid or not
function changeURL(query) {
    //get index of device to change    
    var index = getIdIndex(query.devid);
    
    //devid found, change url and request type
    if (index != -1) {
        config.devices[index].url = query.url;
        config.devices[index].requesttype = query.requesttype;
        
        //async write to file
        data = JSON.stringify(config, null, 2);
        fs.writeFile('./config.json', data, (err) => {
            if (err != null) {
                console.log(err);
            }
        });
    } 
    
    return index;
}


//remove existing devid, return true or false on if devid is valid or not
function removeURL(query) {
    //get index of device to change
    var index = getIdIndex(query.devid);
    
    //remove if index is valid
    if (index != -1) {
        config.devices.splice(index, 1);
        
        //async write to file
        data = JSON.stringify(config, null, 2);
        fs.writeFile('./config.json', data, (err) => {
            if (err != null) {
                console.log(err);
            }
        });
    }
    
    return index;
}


const server = http.createServer((req, res) => {
    
    var user_url = url.parse(req.url, true);
    var filename = user_url.pathname;
    var query = qs.parse(req.url.split('?')[1]);
        
    //SERVE STATIC PAGE
    if (typeof query.id == 'undefined') {
        
        if (filename == '/') {
            filename = '/index.html';
        }

        var filepath = __dirname + filename;
        var file_ext = path.extname(filename);
        var content_type = 'text/html';
        
        //reject someone looking at json
        if (file_ext == '.json') {
            filepath = '';
        }
        
        // handle content type
        switch(file_ext) {
            case '.css':
                content_type = 'text/css';
                break;
            case '.js':
                content_type = 'text/javascript';
                break;
            case '.ico':
                content_type = 'image/x-icon';
                break;
        }
        
        // check filepath exists and serve if is does exists
        if(!fs.existsSync(filepath)) {
            res.statusCode = 404;
            res.setHeader('Content-Type', content_type);
            res.end('404 ERROR: PAGE NOT FOUND');
        } else {
            res.statusCode = 200;
            var file = fs.readFile(filepath, (err, data) => {
                if (err) { throw err; }
                res.setHeader('Content-Type', content_type);
                res.end(data);
            });
            
        }
        
        
    }   
    
    
    // change config
    else if (filename == '/' && typeof query.id !== 'undefined') {

        //check that correct parameters for the action
        if (validateURL(query)) {
            
            //compare user given password to its hash
            bcrypt.compare(query.pass, myhash, (err, response) => {
                res.statusCode = 200;
                res.setHeader('Content-Type', 'text/html');
                if (response == true) {
                    //add url to config
                    if (query.id == 'NEW') {
                        var devid = addURL(query);
                        hosturl = req.headers.host;
                        res.end('URL ADDED, DEV ID: ' + devid + '<br>' + 'URL: ' + hosturl + '/?id=' + devid);
                    //change url in config from dev id
                    } else if (query.id == 'EXISTING') {
                        var ret = changeURL(query);
                        if (ret != -1) {
                            res.end('DEV ID: ' + query.devid + '  CHANGED');
                        } else {
                            res.end('DEVICE ID NOT FOUND');
                        }
                    //remove entry in config by dev id
                    } else if (query.id == 'REMOVE') {
                        var ret = removeURL(query);
                        if (ret != -1) {
                            res.end('DEV ID: REMOVED');
                        } else {
                            res.end('DEVICE ID NOT FOUND');
                        }
                    }
                } else {
                    //password is incorrect
                    res.end('INCORRECT PASSWORD');
                }
            });
           
            
        // send request
        } else {
            //CHECK IF ID IS VALID FROM CONFIG.JSON HERE
            
            var index = getIdIndex(query['id']);
            if (index != -1) {
                
                //delete the id and pass from the query object
                delete query['id'];
                delete query['pass'];
                
                //get needed type and url for request id
                var request_type = config.devices[index].requesttype;
                var request_url = config.devices[index].url;
                
                // POST REQUEST
                if (req.method == 'POST') {                    
                    var body = '';
                    
                    //read url encoded body data to appropriate var
                    req.on('data', (data) => {
                        body += data;
                    });
                    
                    //parse data to a key value pair object and callback
                    req.on('end', () => {
                        var post = qs.parse(body);
                        
                        if (request_type == 'GET') { //send a GET request
                            request.get({url: request_url, qs: post}, (err, resp, body) => {
                                res.statusCode = 200; //HTTP OK
                                res.setHeader('Content-Type', 'text/html');
                                res.end(body);
                            });
                            
                        } else { //send a POST request
                            request.post({url: request_url, form: post}, (err, resp, body) =>{
                                res.statusCode = 200; //HTTP OK
                                res.setHeader('Content-Type', 'text/html');
                                res.end(body);
                            });
                        }
                        
                    });
                    
                // GET REQUEST
                } else if (req.method == 'GET') {
                    
                    if (request_type == 'GET') { //send a GET request
                        request.get({url: request_url, qs: query}, (err, resp, body) => {
                            res.statusCode = 200; //HTTP OK
                            res.setHeader('Content-Type', 'text/html');
                            res.end(body);
                        });
                        
                    } else { //send a POST request
                        request.post({url: request_url, form: query}, (err, resp, body) => {
                            res.statusCode = 200; //HTTP OK
                            res.setHeader('Content-Type', 'text/html');
                            res.end(body);
                        });
                    }
                    
                } else {
                    res.statusCode = 200; //HTTP OK
                    res.setHeader('Content-Type', 'text/html');
                    res.end('INVALID REQUEST METHOD ' + req.method);
                }
                
            } else {
                res.statusCode = 400;
                res.setHeader('Content-Type', 'text/html');
                res.end("INVALID REQUEST ID");
            }
                       
        }
    } 
    // non empty filepath and id param
    else {
        res.statusCode = 404;
        res.setHeader('Content-Type', 'text/html');
        res.end('404 ERROR: Send request to *.net/?id=&ltYOUR ID HERE&gt');
    }
});

//Start the server and listen on the specified port (3000 or env variable)
server.listen(port,() => {
    console.log('Server running on port: ' + port);
});
