if (window.testRunner)
    testRunner.dumpChildFramesAsText();

var iframe = document.createElement('iframe');
document.body.appendChild(iframe);

iframe.contentDocument.write('<script>top.testPassed("script ran")</script>');
iframe.contentDocument.write('PASS');
iframe.contentDocument.close();
