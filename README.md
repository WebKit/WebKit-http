WebKit for Wayland
======

To build:

    $ Tools/Scripts/update-webkitwpe-libs
    $ Tools/Scripts/build-webkit --wpe --threaded-compositor

To run:

    $ Tools/Scripts/run-launcher --wpe
    $ Tools/Scripts/run-launcher --wpe --url http://www.webkit.org
    # Arguments after -- are passed to Weston
    $ Tools/Scripts/run-launcher --wpe -- --fullscreen
