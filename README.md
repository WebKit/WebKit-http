WPEWebKit
======
[![HTML5 score](https://img.shields.io/badge/HTML5%20score-440-blue.svg)](https://html5test.com/)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=55UJZHTXW8VTE)

## Building prerequisites

For **Ubuntu 16.04 LTS (Xenial Xerus)**, please type:

    $ sudo apt-get install intltool libtool-bin autoconf automake autopoint cmake gcc g++ bison flex gawk gperf ruby git libversion-perl libgnutls-dev libexpat-dev libxml2-dev libxslt-dev libsqlite3-dev libjpeg9-dev libfreetype6-dev libharfbuzz-dev libxcb-xkb-dev libwayland-dev libgbm-dev libgles2-mesa-dev libgstreamer1.0-dev libwebp-dev yasm ninja-build luajit libluajit-5.1-dev weston


## Building

From the root directory, please type:

    $ Tools/Scripts/update-webkitwpe-libs
    $ Tools/Scripts/build-webkit --wpe


## Running

If it is not already the case, you will need to execute a **Wayland compositor**.

To do this quickly under **Ubuntu 16.04 LTS (Xenial Xerus)**, please type:

    $ weston --socket=wpe

Then, to run WPEWebKit:

    $ WAYLAND_DISPLAY=wpe Tools/Scripts/run-minibrowser --wpe
    $ WAYLAND_DISPLAY=wpe Tools/Scripts/run-minibrowser --wpe http://www.bouncyballs.org
