function postScript() {
    console.log("Sending POST /script endpoint");

    // unregister subscription from the server
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "script", true);
    xhr.setRequestHeader("Content-Type", "application/json");

    var scriptText = `\
    GUI r
    DELAY 500
    STRING notepad.exe
    ENTER
    DELAY 500
    CTRL n
    DELAY 50
    STRING Hello World!`;

    // Create an unsubscription request json with the unique client ID
    let scriptReq = {
        "script": scriptText,
        "action": 0,
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
