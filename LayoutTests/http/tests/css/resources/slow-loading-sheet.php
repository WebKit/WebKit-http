<?php
// We sleep here so that we are have enough time to test the different attributes before the stylesheet is fully loaded.
usleep(100);

header("Expires: Thu, 01 Dec 2003 16:00:00 GMT");
header("Cache-Control: no-cache, no-store, must-revalidate");
header("Pragma: no-cache");
header("Content-Type: text/css");

$color = $_GET['color'];

echo "h1 { background-color: $color }\n";
ob_flush();
flush();
?>
