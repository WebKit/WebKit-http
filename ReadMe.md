# Qt Port of WebKit

WebKit is an open source web browser engine. WebKit's HTML and JavaScript code began as a branch of the KHTML and KJS libraries from KDE. As part of KDE framework KHTML was based on Qt but during their porting efforts Apple's engineers made WebKit toolkit independent. QtWebKit is a project aiming at porting this fabulous engine back to Qt.

The Qt port of WebKit currently compiles and runs on Linux, *BSD, Windows and macOS.

## Structure of the project

There are several code repositories associated with QtWebKit

### This repository (development)

Development of code specific to the Qt port happens here. You should clone this repository if you are planning to contribute.

Branches:

* `master` - mirror of WebKit upstream, without any Qt-specific code
* `qtwebkit-stable` - Qt-specific fixes and improvements are integrated here first
* `qtwebkit-5.212` - current release branch, which is `qtwebkit-stable` with commits backported from `master`

### End-user repository (snapshots)

Development repository is very large and contains lots of data that is not required for building and using QtWebKit. Use the following repository if you need to get latest snapshots of QtWebKit code:

http://code.qt.io/cgit/qt/qtwebkit.git/

Branches:

* `5.212` - code snapshots from `qtwebkit-5.212`

### Upstream

Development of the WebKit engine happens at https://webkit.org. All development of cross-platform code, including JavaScript engine and Web platform features, happens there. Code from upstream is getting into QtWebKit development via cherry-picks or merges.

## More information

See https://github.com/annulen/webkit/wiki

## Contacts

* Mailing list: webkit-qt@lists.webkit.org
* IRC: #qtwebkit on irc.freenode.net
* Blog: http://qtwebkit.blogspot.com
