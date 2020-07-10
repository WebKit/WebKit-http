// META: script=websocket.sub.js

test(function() {
  var asciiWithSep = "/echo";
  var wsocket;
  assert_throws_dom("SYNTAX_ERR", function() {
    wsocket = CreateWebSocketWithAsciiSep(asciiWithSep)
  });
}, "Create WebSocket - Pass a valid URL and a protocol string with an ascii separator character - SYNTAX_ERR is thrown")
