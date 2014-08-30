# bmon - Bandwidth Monitor

[![Build Status](https://travis-ci.org/tgraf/bmon.svg?branch=master)](https://travis-ci.org/tgraf/bmon)
[![Coverity Status](https://scan.coverity.com/projects/2864/badge.svg)](https://scan.coverity.com/projects/2864)

bmon is a monitoring and debugging tool to capture networking related
statistics and prepare them visually in a human friendly way. It
features various output methods including an interactive curses user
interface and a programmable text output for scripting.

## Changes


## New in v3.5
 * Fixes for all defects identified by coverity
 * Fix accuracy issue on total rate calculation
 * Travis-CI support
 * Various other small bugfixes

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

> *Copyright (c) 2001-2014 Thomas Graf <tgraf@suug.ch>
> Copyright (c) 2013 Red Hat, Inc.*

Please see the [LICENSE](https://github.com/tgraf/bmon/blob/master/LICENSE)
file for additional details.

