#!/bin/bash
mkdir -p ~/.steam/steamcmd
cd ~/.steam/steamcmd
curl -L "https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz" | tar -xz
./steamcmd.sh +quit   # first run, lets it update itself
