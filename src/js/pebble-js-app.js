function HTTPGET(url) {
	var req = new XMLHttpRequest();
	req.open("GET", url, false);
	req.send(null);
	return req.responseText;
}

function lulz(url, params) {
	var k = new XMLHttpRequest();

	k.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	k.setRequestHeader("Content-length", params.length);
	k.setRequestHeader("Access-Control-Allow-Origin", "*");
	k.setRequestHeader("Connection", "close");
	k.open("POST", url, true);
	k.send(params);
	return k.responseText;
}

var getWeather = function() {
	//Get weather info
	//var response = HTTPGET("http://api.openweathermap.org/data/2.5/weather?q=London,uk");
	var response = lulz("http://api.openweathermap.org/data/2.5/weather?q=London,uk", "lol");

	//Convert to JSON
	var json = JSON.parse(response);
	
	//Extract the data
	var temperature = Math.round(json.main.temp - 273.15);
	var location = json.name;
	
	//Console output to check all is working.
	console.log("It is " + temperature + " degrees in " + location + " today!");
	
	//Construct a key-value dictionary	
	var dict = {0 : response};
	
	//Send data to watch for display
	Pebble.sendAppMessage(dict);
};

Pebble.addEventListener("ready",
  function(e) {
	//App is ready to receive JS messages
	getWeather();
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
	//Watch wants new data!
	getWeather();
  }
);