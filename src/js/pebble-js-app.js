var global_veraData = {}
var global_message_codes = {
	scene_data:101,
	scene_execution_success:102,
	scene_execution_failure:103,
	connection_failure:104
}

Pebble.addEventListener("ready",
	function(e) {
		initializeApp();
	}
);

Pebble.addEventListener("appmessage",
  function(e) {
  	sceneIndex = e.payload["1"];
  	doVeraRequest("data_request?id=lu_action&output_format=json&serviceId=urn:micasaverde-com:serviceId:HomeAutomationGateway1&action=RunScene&SceneNum=" + global_veraData.scenes[sceneIndex].id, function(result){
		if (result["u:RunSceneResponse"]["OK"] === "OK"){
			//console.log("Successfully Ran Scene");
			var message = {"0":global_message_codes.scene_execution_success, "1":sceneIndex};
			Pebble.sendAppMessage(message);
		}
		else{
			//console.log("Error running scene");
			var message = {"0":global_message_codes.scene_execution_error, "1":sceneIndex};
			Pebble.sendAppMessage(message);
		}	
	  	
  	});
  }
);

Pebble.addEventListener("showConfiguration",
	function(e){
		Pebble.openURL("http://www.thinkpeopletech.com/VeraPebble/mios.html");	
	}
);

Pebble.addEventListener("webviewclosed",
	function(e) {
		var configuration = JSON.parse(decodeURIComponent(e.response));
		window.localStorage.setItem("username", configuration.username);
		window.localStorage.setItem("password", configuration.password);
		initializeApp();
	}
);

function initializeApp(){
	var username = window.localStorage.getItem("username");
	var password = window.localStorage.getItem("password");
	doGetRequest("https://sta1.mios.com/locator_json.php?username="+username, function(result){
		if (result.units.length == 0){
			sendPebbleConnectionError();
			return;
		}
		var ipAddress = result.units[0].ipAddress;
		if (ipAddress.length < 7){
			global_veraData.url = "https://" + result.units[0].active_server + "/" + username + "/" + password + "/";
		}
		else{
			global_veraData.url = "http://" + ipAddress + ":3480/";
		}
		getSceneData();		
	});
}

function sendPebbleConnectionError(){
	var message = {"0":global_message_codes.connection_failure};
	Pebble.sendAppMessage(message);
}

function getSceneData(){
	doVeraRequest("data_request?id=sdata", function(result){
		global_veraData.scenes = result.scenes; 
				
				var message = {"0":global_message_codes.scene_data};
				for (var i = 0; i < global_veraData.scenes.length; i++){
					message[(i+1).toString()] = result.scenes[i].name;
				}
				Pebble.sendAppMessage(message);
		
		});
}

function doVeraRequest(request, callback){
	var url = global_veraData.url + request;
	doGetRequest(url, callback);
}

function doGetRequest(request, callback){
	//console.log("Doing request: " + request);
	var req = new XMLHttpRequest();
	req.open('GET', request, true);
	req.onload = function(e) {
		if (req.readyState == 4 && req.status == 200) {
			if(req.status == 200) {
				//console.log("Got Response: " + req.responseText);
				var result = JSON.parse(req.responseText);
				callback(result);
			} 
			else{ 
				sendPebbleConnectionError();
			}
		}
		else{
			sendPebbleConnectionError();
		}
	}
	req.send(null);
}