/* Name: app.js
 * Description: proxy post requests from an incoming request to another specified server
 *              always responds with 200 http status code to the incoming server
 * Author: Kevin Koos
 * Use: Slide Sentinel project for OSU capstone
 */

const http    = require('http');
const request = require('request');
const port    = process.env.PORT || 3000;
const qs      = require('querystring');
const config  = require('./config.json');

const server = http.createServer((req, res) => {
    res.statusCode = 200; //HTTP OK
    res.setHeader('Content-Type', 'text/html');
    res.end();
    
    if (req.method == 'POST') {
        var body = '';
        
        //read post data into body variable
        req.on('data', (data) => {
            body += data;
        });
        
        //parse data to a key value pair object and callback
        req.on('end', () => {
            var post = qs.parse(body);
            
            //send get request with post parameters as query key values to supplied postURL
            request.get({url: config.postURL, qs: post}, (err, resp, body) => {
                //console.log(err,body);
            });
        });
    }
});


server.listen(port,() => {
    console.log('Server running on port: ' + port);
});
