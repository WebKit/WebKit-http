<?php
if ($_SERVER["REQUEST_METHOD"] == "OPTIONS")
    die("Got unexpected preflight request");

header("Content-Type: text/event-stream");

$count = intval($_GET["count"]);

if ($count == 2)
    header("Access-Control-Allow-Origin: http://some.other.origin:80");
else if ($count == 3)
    header("Access-Control-Allow-Origin: *");
else if ($count > 3)
    header("Access-Control-Allow-Origin: " . $_SERVER["HTTP_ORIGIN"]);

if ($_SERVER["HTTP_LAST_EVENT_ID"] != "77")
    echo "id: 77\ndata: DATA1\nretry: 0\n\n";
else
    echo "data: DATA2\n\n";
?>
