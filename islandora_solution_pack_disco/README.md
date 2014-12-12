# Disco Solution Pack 

## Introduction

Adds a collection for CD/LP Multitrack Audio Object, could include the CD booklet also.
The CD object needs to be ingested with a zip file containing the audio tracks in WAV, MP3 or FLAC format.
Creates a RSS Playlist datastream of the MP3 audio derivatives. It could be associated with the [Islandora JWPlaylist Viewer](https://github.com/sanxofon/islandora_jwplaylist).

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Tuque](https://github.com/islandora/tuque)
* [ImageMagick](https://drupal.org/project/imagemagick)
* [XPDF:pdftotext](http://www.foolabs.com/xpdf/) -  Debian/Ubuntu `sudo apt-get install pdftotext`
* [Lame](http://lame.sourceforge.net) (Debian/Ubuntu `sudo apt-get install lame`)
* [OGGENC](http://www.vorbis.com/) (Debian/Ubuntu `sudo apt-get install vorbis-tools`)

*To successfully create derivative data streams ImageMagick (TN & JPG) needs to be installed on the server.*

## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Configuration

Configure the image-tool kit to use ImageMagick rather than GD in Administration > Configuration > Media > Image Toolkit (admin/config/media/image-toolkit). If GD is selected, TN and JPG datastreams will not be generated.
Set the path for `lame` and select a viewer in Administration » Islandora » Audio Collection (admin/islandora/audio).
Configure thumbnail and CD cover image sizes, and set the path for `pdftotext` if extract text stream is selected in Administration » Islandora » PDF Collection (admin/islandora/pdf).

## Troubleshooting/Issues

Having problems or solved a problem? Check out the Islandora google groups for a solution.

* [Islandora Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora)
* [Islandora Dev Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora-dev)

## Maintainers/Sponsors
Current maintainers:

* [Santiago Chavez Novaro](https://github.com/sanxofon/)

## Development

If you would like to contribute to this module, please check out our helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as our [Developers](http://islandora.ca/developers) section on the [Islandora.ca](http://islandora.ca) site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
