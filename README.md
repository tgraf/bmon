# bmon - Bandwidth Monitor

[![Build Status](https://travis-ci.org/tgraf/bmon.svg?branch=master)](https://travis-ci.org/tgraf/bmon)
[![Coverity Status](https://scan.coverity.com/projects/2864/badge.svg)](https://scan.coverity.com/projects/2864)

bmon is a monitoring and debugging tool to capture networking related
statistics and prepare them visually in a human friendly way. It
features various output methods including an interactive curses user
interface and a programmable text output for scripting.

## Download

 * [Latest Release](https://github.com/tgraf/bmon/releases/latest)
 * [Older Releases](https://github.com/tgraf/bmon/releases)

## Debian/Ubuntu Installation

```
git clone https://github.com/tgraf/bmon.git
cd bmon
apt-get install build-essential make libconfuse-dev libnl-3-dev libnl-route-3-dev libncurses-dev pkg-config dh-autoreconf
./autogen.sh
./configure
make
make install
bmon
```

-------------
## New in 3.8
 * Don't disable Netlink if TC stats are unavailable

-------------
### Usage

To run bmon in the default curses mode:

> bmon

There are many other options available and full help is
provided via:

> bmon --help

## Screenshots

![Screenshot 1](https://github.com/tgraf/bmon/raw/gh-pages/images/shot1.png)
![Screenshot 2](https://github.com/tgraf/bmon/raw/gh-pages/images/shot2.png)

## Copyright

> *Copyright (c) 2001-2014 Thomas Graf <tgraf@suug.ch>*
> *Copyright (c) 2013 Red Hat, Inc.*

Please see the [LICENSE.BSD](https://github.com/tgraf/bmon/blob/master/LICENSE.BSD)
and [LICENSE.MIT](https://github.com/tgraf/bmon/blob/master/LICENSE.MIT) files for
additional details.

