function CallCommand(cmd)
{
 try {
     var req = new XMLHttpRequest;
     req.open("GET", "/resources/network-simulator.php?command=" + cmd, false);
     req.send(null);
     return req.responseText;
 } catch (ex) {
     return "";
 }
}

function startTest()
{
    if (window.testRunner) {
        testRunner.dumpAsText();
        testRunner.waitUntilDone();
    }
 
    window.setTimeout(endTest, 0);
}

function endTest()
{
    getResourceLog();
    CallCommand("clear-resource-request-log");

    if (window.testRunner)
        testRunner.notifyDone();
}

function getResourceLog()
{
    var log = CallCommand("get-resource-request-log");
    var logLines = log.split('\n');
    logLines.sort();
    document.getElementById('result').innerText = logLines.join('\n');
}

CallCommand("start-resource-request-log");
window.addEventListener('load', startTest, false);
