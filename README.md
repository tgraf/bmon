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
## CentOS (6) Installation

```
git clone https://github.com/tgraf/bmon.git
cd bmon
yum install make libconfuse-devel libnl3-devel libnl-route3-devel ncurses-devel
./autogen.sh
./configure
make
make install
bmon
```

## OSX Installation

### Brew
```
brew install bmon
```

### Compile yourself
Install libconfuse
```
wget https://github.com/martinh/libconfuse/releases/download/v2.8/confuse-2.8.zip
unzip confuse-2.8.zip && cd confuse-2.8
PATH=/usr/local/opt/gettext/bin:$PATH ./configure
make
make install
```

Install bmon
```
git clone https://github.com/tgraf/bmon.git
cd bmon
./autogen.sh
./configure
make
make install
bmon
```

-------------
## New in 4.0
 * Use monotonic clock instead of realtime clock
 * Pick default selected interface based on policy
 * Collect RX NoHandler statistics if available (Linux)
 * CentOS installation instructions
 * Proper stdout flush in ASCII mode
 * Bugfixes

-------------
### Usage

To run bmon in the default curses mode:

> bmon

There are many other options available and full help is
provided via:

> bmon --help

## Screenshots

![Screenshot 1](https://github.com/tgraf/bmon/raw/gh-pages/images/shot3.png)
![Screenshot 2](https://github.com/tgraf/bmon/raw/gh-pages/images/shot1.png)
![Screenshot 3](https://github.com/tgraf/bmon/raw/gh-pages/images/shot2.png)

## Copyright

Various authors, see git commit log.

> *Copyright (c) 2001-2016 Thomas Graf <tgraf@suug.ch>*
> *Copyright (c) 2013 Red Hat, Inc.*

Please see the [LICENSE.BSD](https://github.com/tgraf/bmon/blob/master/LICENSE.BSD)
and [LICENSE.MIT](https://github.com/tgraf/bmon/blob/master/LICENSE.MIT) files for
additional details.

