
// Script added as module so the following call will be run when the DOM is already parsed
const scriptTextarea = document.getElementById('scriptTextarea');
const scriptRunButton = document.getElementById('scriptRunButton');
const scriptLoadButton = document.getElementById('scriptLoadButton');
const scriptSaveButton = document.getElementById('scriptSaveButton');
const armingStateSelect = document.getElementById('armingStateSelect');
const usbDeviceTypeSelect = document.getElementById('usbDeviceTypeSelect');
const configSaveButton = document.getElementById('configSaveButton');

const themeToggle = document.getElementById("themeToggle");

const SCRIPT_ACTION_RUN = 0;
const SCRIPT_ACTION_SAVE = 1;

function postScript(script, action) {
	console.log("Sending POST /script endpoint");

	// unregister subscription from the server
	let xhr = new XMLHttpRequest();
	xhr.open("POST", "script", true);
	xhr.setRequestHeader("Content-Type", "application/json");

	let scriptReq = {
        "script": script,
        "action": action,
    };

	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			if(xhr.status === 200)
			{
				//var json = JSON.parse(xhr.responseText);
				//console.log(xhr.responseText);
				console.log("POST /script endpoint response: " + xhr.responseText);
			}
			else
			{
				console.error("POST /script endpoint error: " + xhr.statusText);
			}
		}
	};

	xhr.send(JSON.stringify(scriptReq));
}

function getScript() {
	console.log("Sending GET /script endpoint");

	// unregister subscription from the server
	let xhr = new XMLHttpRequest();
	xhr.open("GET", "script", true);

	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			if(xhr.status === 200)
			{
				var json = JSON.parse(xhr.responseText);
				console.log("GET /script endpoint response: " + xhr.responseText);
				scriptTextarea.value = json.script;
				autoGrow(scriptTextarea);
			}
			else
			{
				console.error("GET /script endpoint error: " + xhr.statusText);
			}
		}
	};

	xhr.send();
}


function getConfig() {
	console.log("Sending GET /config endpoint");

	// unregister subscription from the server
	let xhr = new XMLHttpRequest();
	xhr.open("GET", "config", true);

	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			if(xhr.status === 200)
			{
				var json = JSON.parse(xhr.responseText);
				console.log("GET /config endpoint response: " + xhr.responseText);
				armingStateSelect.value = json.armingState;
				usbDeviceTypeSelect.value = json.usbDeviceType;
			}
			else
			{
				console.error("GET /config endpoint error: " + xhr.statusText);
			}
		}
	};

	xhr.send();
}

function postConfig() {
	console.log("Sending POST /config endpoint");

	// unregister subscription from the server
	let xhr = new XMLHttpRequest();
	xhr.open("POST", "config", true);
	xhr.setRequestHeader("Content-Type", "application/json");

	let configReq = {
		"armingState": parseInt(armingStateSelect.value),
		"usbDeviceType": parseInt(usbDeviceTypeSelect.value),
	};

	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			if(xhr.status === 200)
			{
				//var json = JSON.parse(xhr.responseText);
				//console.log(xhr.responseText);
				console.log("POST /config endpoint response: " + xhr.responseText);
			}
			else
			{
				console.error("POST /config endpoint error: " + xhr.statusText);
			}
		}
	};

	xhr.send(JSON.stringify(configReq));
}

function autoGrow(element) {
	element.style.height = 'auto';
	element.style.height = element.scrollHeight + 'px';
}

scriptTextarea.addEventListener('input', () => autoGrow(scriptTextarea));

scriptRunButton.addEventListener('click', () => postScript(scriptTextarea.value, SCRIPT_ACTION_RUN));
scriptSaveButton.addEventListener('click', () => postScript(scriptTextarea.value, SCRIPT_ACTION_SAVE));
scriptLoadButton.addEventListener('click', () => getScript());
configSaveButton.addEventListener('click', () => postConfig());

// Theme toggle functionality
function setTheme(mode) {
	document.body.classList.remove("light", "dark");
	if (mode) document.body.classList.add(mode);
	localStorage.setItem("theme", mode);
	updateToggleLabel();
}
  
function updateToggleLabel() {
	const isDark = document.body.classList.contains("dark");
	themeToggle.checked  = isDark ? true : false;
}

// Apply saved theme on load
const savedTheme = localStorage.getItem("theme");
if (savedTheme) {
	setTheme(savedTheme);
} else {
	updateToggleLabel(); // default to system mode
}

themeToggle?.addEventListener("click", () => {
	const isDark = document.body.classList.contains("dark");
	setTheme(isDark ? "light" : "dark");
});

updateToggleLabel();
getScript();
getConfig();
