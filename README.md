# bmon - Bandwidth Monitor

[![Build Status](https://travis-ci.org/tgraf/bmon.svg?branch=master)](https://travis-ci.org/tgraf/bmon)

bmon is a monitoring and debugging tool to capture networking related
statistics and prepare them visually in a human friendly way. It
features various output methods including an interactive curses user
interface and a programmable text output for scripting.

## Changes

### New in 3.4
 * Bugfixes
   * blank screen with config file
   * quick-help toggle with '?' in curses
 * Better bmon.conf example

## New in 3.3
 * MacOS X port
 * Only initialize curses module if actually used
 * Assorted bug and spelling fixes
 * Various build fixes

### Usage

To run bmon in the default curses mode:

> bmon

There are many other options available and full help is
provided via:

> bmon --help

## Screenshots

![Screenshot 1](https://github.com/tgraf/bmon/raw/gh-pages/images/shot1.png =512x)
![Screenshot 2](https://github.com/tgraf/bmon/raw/gh-pages/images/shot2.png =512x)

## Copyright

> *Copyright (c) 2001-2014 Thomas Graf <tgraf@suug.ch>
> Copyright (c) 2013 Red Hat, Inc.*

Please see the [LICENSE](https://github.com/tgraf/bmon/blob/master/LICENSE)
file for additional details.
