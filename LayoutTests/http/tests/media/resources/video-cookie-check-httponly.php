<?php
    if($_COOKIE["TEST_HTTP_ONLY"])
    {
        $extension = pathinfo($_COOKIE["TEST_HTTP_ONLY"], PATHINFO_EXTENSION);
        if ($extension == 'mp4') {
               header("Content-Type: video/mp4");
               $fileName = "test.mp4";
        } else if ($extension == 'ogv') {
               header("Content-Type: video/ogg");
               $fileName = "test.ogv";
        } else if ($extension == 'ts') {
               header("Content-Type: video/mpegts");
               $fileName = "hls/test.ts";
        } else
               die;
        header("Cache-Control: no-store");
        header("Connection: close");
        $fn = fopen($fileName, "r");
        fpassthru($fn);
        fclose($fn);
        exit;
    }
?>
