# README #

This is an Arduino ESP32 project for a simple home automation

The ESP32 one booted up:

- connects to WIFI
- if connected, get date and time from ntp
- if not connected, hour=0
- when hour=6, send a wake-on-lan packet to the home server to start it up
- if not connected, sleep for 20 min
- if connected, sleep for 1 hour
- repeat from start