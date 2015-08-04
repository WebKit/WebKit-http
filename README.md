WebKit for Wayland
======

To build:

    $ Tools/Scripts/update-webkitwpe-libs
    $ Tools/Scripts/build-webkit --wpe

To run:

    $ Tools/Scripts/run-wpe
    $ Tools/Scripts/run-wpe --url http://www.webkit.org
    # Arguments after -- are passed to Weston
    $ Tools/Scripts/run-wpe -- --fullscreen
