[![CI](https://github.com/lpenz/gkrellexec/actions/workflows/ci.yml/badge.svg)](https://github.com/lpenz/gkrellexec/actions/workflows/ci.yml)
[![packagecloud](https://img.shields.io/badge/deb-packagecloud.io-844fec.svg)](https://packagecloud.io/app/lpenz/debian/search?q=gkrellexec)

gkrellexec
==========

# Introduction

**gkrellexec** is a plugin for gkrellm that displays the return status of
arbitrary shell commands.

Up to 10 shell commands can be specified, each with a name, timeout and
different times to sleep after a success or a failure.

gkellexec displays a panel in gkrellm, with one line per process, showing its
last exit status (**T** for timeout, **O** for ok, **E** for error) and its
name.


# Installation


## Debian

gkrellexec's debian package is kept in https://packagecloud.io/lpenz/debian.
Follow the link for installation instructions.


## Manually

To build it, run *cmake .* and then *make*. gkrellm's header files must be
installed.

To install, you can either run *make install* so that gkrellexec.so is copied
to the default ``/usr/lib/gkrellm2`` or you can copy it to
``$HOME/.gkrellm2/plugins``.


# Configuration

Enable it in gkrellm's plugin configuration.

On the plugins's configuration screen, there is a tab for each process with the
configurable options. The process is disabled if it has no name. The shell
command is run with ``/bin/sh -c "<command>"``, so that shell syntax is fully
available.

