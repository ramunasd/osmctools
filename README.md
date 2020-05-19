# OSM C tools

This repository serves as package for few OSM data processing tools, originally made by Markus Weber.

## Why?
The aim of this package is to have one place for all these great tools, for easy update/compile/install process. Original repository
does not contains working compile scripts, not all tools is included and are outdated.

## Author
Author of all included tools is [Markus Weber](http://m.m.i24.cc/). All tools (except packaging configs) is completely
made by him and only him even if commit author is someone else.

Some tools can be found in [authors repository](https://gitlab.com/osm-c-tools/osmctools).

## License
All tools is distributed under GNU APGL v3 license and is publicly accessible from [wiki](http://wiki.osm.org/).
This package contains latest version of all tools without any modifications or customizations.

## Tools

* osmassignpoly
* osmchange - updates an .osm file using one or more .osc files
* [osmconvert](http://wiki.openstreetmap.org/wiki/Osmconvert) - reads and converts OSM data to the selected output file format
* [osmfilter](http://wiki.openstreetmap.org/wiki/Osmfilter) - filters OSM data
* osmgeobase
* osmposition
* osmrelpoly
* [osmupdate](http://wiki.openstreetmap.org/wiki/Osmupdate) - cares about updating an .osm, .o5m or .pbf file
* [pbftoosm](http://wiki.openstreetmap.org/wiki/Pbftoosm) - converts .pbf file into .osm XML format

## Install

```
  git clone https://github.com/ramunasd/osmctools.git
  cd osmctools
  autoreconf --install
  ./configure
  make install
```

## Docker

For those using the Docker workflow, there is a Dockerfile provided to build the tools.

There is an official [Automated Build](https://docs.docker.com/docker-hub/builds/) from source on Docker Hub at [ramunasd/osmctools](https://hub.docker.com/r/ramunasd/osmctools/), automatically built on each commit.
