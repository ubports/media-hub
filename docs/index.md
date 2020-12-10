---
title: "Media Hub"
table_of_contents: False
---

# About Media Hub

Media Hub is a simple and lightweight media playback service.  It implements the
[MPRIS DBus
interface](https://specifications.freedesktop.org/mpris-spec/latest/) and plays
audio and video files. Among other features, it supports:

 * Centralized system media playback and control
 * Track lists
 * Various simultaneous clients
 * Hardware video decoding

## Installing Media Hub

It can be installed as a snap doing

```text
$ snap install media-hub
```

## Using Media Hub

It can be used jointly with mediaplayer-app. To install this client:

```text
$ snap install --devmode mediaplayer-app
$ snap connect mediaplayer-app:mpris media-hub:mpris
```

After this, files in Music or Videos folders can be played:

```text
$ mediaplayer-app Music/file.mp3
```

## Configure gstreamer1.0 backend

Some properties of the playbin object can be configured by creating a file in $HOME/.config/media-hub/media-hub.conf containing:
```text
# Playbin buffer-duration, in nano seconds, or 0 to disble
export PLAYBIN_BUFFER_DURATION=5000000000
# Playbin buffer-size, in bytes or 0 to disable
export PLAYBIN_BUFFER_SIZE=0
# Additional flags. See GstPlayFlags
# 0x100: Enable buffering
export PLAYBIN_ADDITIONAL_FLAGS=0x100
```
