// Sokeriseuranta Pebble Watchface 
// Copyright (C) 2017  Mika Haulo
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


var moment = require('./moment');
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var config_user_email = "";
var config_access_token = "";

var settings = {};
try {
  settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  config_user_email = settings.UserEmail;
  config_access_token = settings.AccessToken;
} 
catch (e) {}

var xhrRequest = function (url, type, email, token, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      callback(this.responseText);
    };
    
    xhr.open(type, url);
    xhr.setRequestHeader("X-User-Email", email);
    xhr.setRequestHeader("X-Access-Token", token);
    xhr.send();
};

function get_glucose() {
  var url = "https://sokeriseuranta.fi/api/nightscout/pebble";
  var user_email = config_user_email;
  var access_token = config_access_token;
  
  xhrRequest(url, 'GET', user_email, access_token, 
    function(responseText) {
      var json = JSON.parse(responseText);
      var current_glucose = json.bgs[0].sgv.replace(".", ",");
      var current_glucose_timestamp = moment.utc(json.bgs[0].datetime).format("H.mm");
      var glucose_delta = json.bgs[0].bgdelta;
      var dictionary = {
        'CURRENT_GLUCOSE': current_glucose,
        'GLUCOSE_DELTA': glucose_delta,
        'CURRENT_GLUCOSE_TIMESTAMP': current_glucose_timestamp,
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Glugose info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending glucose info to Pebble!');
        }
      ); 
    }
  );     
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
    get_glucose();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    get_glucose();
  }                     
);
