/*

This is a simple server router that runs with node.js (http://nodejs.org/)
and processes incoming texts (from Twilio) and posts them to a data feed (on data.sparkfun.com)

Based on code originally written by Noah Veltman, veltman.com

I leaving this running on my server using forever.js
Details on that here: https://github.com/foreverjs/forever
Once installed, the command I run is: sudo forever start server.js

John Keefe
http://johnkeefe.net
March 2015

*/

// establish all of the dependency modules
// install these modules with "npm" the node packet manager
//    npm install express request body-parser twilio

var express = require('express'),
    fs = require('fs'),
    request = require('request'),
    bodyParser = require('body-parser'),
    cookieParser = require('cookie-parser'),
    twilio = require('twilio'),
    app = express(),
    sparkfunTemptexterKeys = require('../api_keys/sparkfun_temptexter_keys'),
    twilioKeys = require('../api_keys/twilio_keys');
    
    // The last two lines read in js files containing my secret keys,
    // mainly so I don't accidentally post them in github.
    // An example of the files is here: https://gist.github.com/jkeefe/f77e0002d8f202371bff
    // Invoked in this program with sparkfunTemtexterKeys.PRIVATE_KEY

// this is needed to read the body!
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
  extended: true
}));

// Accept incoming texts with device, conductivity, battery level
// and then post them to a data.sparkfun.com table.
// Part of the streamlab project.
// Uses the Twilio webhook authentication noted in a function below
app.post(/^\/?incoming-riffle-texts/i, twilio.webhook(twilioKeys.TWILIO_AUTH_TOKEN), function(req, response){
    
  // Receive Text //
    
	// is there a body of information
	if (!req.body) {
		req.send("Posting error");
		return true;
	}
  
  var text_message = req.body.Body;
  
  // strip out spaces (the arduino code pads numbers with spaces)
  text_message = text_message.replace(/ /g,'');
  
  // text messages sent in the format:
  // device_id,conductivity,temp,battery
  // ie: john's_arduino,78.5,51.2,91
  
  // make an array out of the comma-separated values
  var values = text_message.split(',');
  
  // Post to data.sparkfun.com page //
  
  // Format for "posting" is:  http://data.sparkfun.com/input/[publicKey]?private_key=[privateKey]&battery=[value]&device_id=[value]&humidity=[value]&temperature=[value]
  // Public URL is: https://data.sparkfun.com/streams/7JLDOZZvLJFjm3lyoAL4
  
  // build the url to hit sparfun with
  var data_url = "http://data.sparkfun.com/input/7JLDOZZvLJFjm3lyoAL4?private_key=" + sparkfunTemptexterKeys.STREAMLAB_PRIVATE_KEY +
  "&device_id=" + values[0] + 
  "&conductivity=" + values[1] + 
  "&temperature=" + values[2] + 
  "&battery=" + values[3];
  
  console.log("Trying: " + data_url);
  
  // compose the request
  var options = {
    url: data_url,
    method: 'GET'
  };
  
  // start the request/get to data.sparkfun.com
  request(options, function(error, response, body){
    
    if (error || response.statusCode != 200) {
      console.log("Error hitting Sparkfun site: ",error);
    
    } else {
      
      // everything went okay
      
    }
  });  
});

app.use(express.static(__dirname));
 
app.listen(80);
console.log('running!');