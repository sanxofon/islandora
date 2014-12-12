# Islandora JW Player for MP3/Video/Mixed Playlists

## Introduction

A Islandora viewer module using [JW Player](http://www.jwplayer.com/).

This module doesn't do much on its own, its assummed that it will be used in conjunction with the [CD Solution Pack](https://github.com/sanxofon/islandora_solution_pack_cd) (*currently in development*), where it will be provided as a viewer.

**NOTE**: JW Player is free for non comercial use.

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Tuque](https://github.com/islandora/tuque)
* [Libraries API](https://drupal.org/project/libraries)
* [JW Player](www.longtailvideo.com/download/jwplayer-free-6-1-2972.zip)

## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

Install the JW Player available [here](http://www.longtailvideo.com/jw-player/download/) to your `sites/libraries` folder.

## Configuration

Select JW Playlist as a viewer in Administration » Islandora » CD Collection (admin/islandora/cd).
This player must recieve a RSS Playlist file url not an audio file url.

In Islandora, this player will be available as al viewer of the following Mime Type content: *application/rss+xml*

####Variables for display:####

* file -> RSS Playlist file URL
* thumbnail -> TN file url
* width -> Player width in pixeles
* height -> Player height in pixeles
* listbarposition -> left or right position of playlist sidebar
* listbarsize -> Width in pixeles of playlist sidebar

## RSS Playlist Example

    <?xml version="1.0" encoding="utf-8" ?>
    <rss version="2.0" xmlns:jwplayer="http://rss.jwpcdn.com/">
    <channel>
    
      <item>
        <title>Title of track</title>
        <description>Decription of track</description>
        <jwplayer:image>Full url to image thumbnail</jwplayer:image>
        <jwplayer:source file="http://localhost/full_url_of_audio.mp3" />
      </item>
    
    </channel>
    </rss>

## Maintainers

Current maintainers:

* [Santiago Chávez](https://github.com/sanxofon)

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
