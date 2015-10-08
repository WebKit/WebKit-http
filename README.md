WebKit for Wayland
======

To build:

    $ Tools/Scripts/update-webkitwpe-libs
    $ Tools/Scripts/build-webkit --wpe --threaded-compositor

To run (assuming you're executing under a Wayland compositor):

    $ Tools/Scripts/run-wpe
    $ Tools/Scripts/run-wpe http://www.bouncyballs.org

To run under a specific Weston instance:

    $ weston --socket=wpe-test
    $ WAYLAND_DISPLAY=wpe-test Tools/Scripts/run-wpe
