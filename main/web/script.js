
// Script added as module so the following call will be run when the DOM is already parsed
const scriptTextarea = document.getElementById('scriptTextarea');
const scriptRunButton = document.getElementById('scriptRunButton');
const scriptLoadButton = document.getElementById('scriptLoadButton');
const scriptSaveButton = document.getElementById('scriptSaveButton');

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

function autoGrow(element) {
	element.style.height = 'auto';
	element.style.height = element.scrollHeight + 'px';
}

scriptTextarea.addEventListener('input', () => autoGrow(scriptTextarea));
autoGrow(scriptTextarea);

scriptRunButton.addEventListener('click', () => postScript(scriptTextarea.value, SCRIPT_ACTION_RUN));
scriptSaveButton.addEventListener('click', () => postScript(scriptTextarea.value, SCRIPT_ACTION_SAVE));
