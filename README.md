
sdrplayDab is experimental software for Windows, Linux and Raspberry Pi for listening to terrestrial Digital Audio Broadcasting (DAB and DAB+).
It is a variant of Qt-DAB, and optimized for use with SDRplay RSP
devices. It uses a completely different approach 
for recognizing DAB frames than used in qt-dab. 

![sdrplayDab](/sdrplay-picture-1.png?raw=true)

------------------------------------------------------------------
New: support for both 2.13 and 3.06 support libraries
-----------------------------------------------------------------

Since future developments for SDRplay devices will be centered
around the 3.xx libraries, while a large number of users are
running with the 2.13 library, support is included for both
libraries.
If support for both libraries is configured, the software will try
the 2.13 library first and if that fails the 3.06 library.

Uncommenting the appropriate lines in the sdrplay-dab.pro file
allows configuration of either or both libraries.

-------------------------------------------------------------------
sdrplayDab-3.3
-------------------------------------------------------------------

* [Introduction](#introduction)
* [Features](#features)
* [Widgets and scopes](#widgets-and-scopes)
* [Presets](#Presets)
* [Comment on some settings](#comment-on-some-settings)
* [Obsolete properties](#Obsolete-properties)
* [Installation](#features)
* [Copyright](#copyright)

-------------------------------------------------------------------
Introduction
-------------------------------------------------------------------

sdrplayDab  is software for the reception of DAB, optimized for the
use of the SDRplay RSP devices.
While there are lot of commonalities with Qt-DAB, the "front-end",
i.e. the interfacing to the input device and the ofdm processing is
completely different and optimized for use with the SDRplay devices.

Version 3.3 is an update and functionality implemented is that of
Qt-DAB 3.3.
The GUI of sdrplay-Dab 3.3 and Qt-3.3 are made lookalikes of each other

------------------------------------------------------------------
Features
------------------------------------------------------------------

  * DAB (mp2) and DAB+ (HE-AAC v1, HE-AAC v2 and LC-AAC) decoding
  * MOT SlideShow (SLS)
  * Dynamic Label (DLS) 
  * Both DAB bands supported): 
  	* VHF Band III (default),
   	* L-Band (only used in Czech Republic and Vatican) (see "obsolete properties")
  * capable of handling alternative frequency lists sspecified by the user
  * Modes I, II and IV (Mode I default, oter modes can be set in the ".ini" file)
  * Spectrum view (incl. constellation diagram, correlation result, TII spectrum)
  * Scanning function (scan the band and show the results on the screen)
  * Detailed information for selected service (SNR, bitrate, frequency, ensemble name, ensemble ID, subchannel ID, used CUs, protection level, CPU usage, program type, language, 4 quality bars)
  * Detailed information for other services by right-clicking on their name (bitrate, subchannel ID, used CU's protection level, program type)
  * Automatic display of TII (Transmitter Identification Information) data when transmitted
  * Presets for easy switching of programs in different ensembles (see *Presets*)
  * Saving audio as uncompressed wave files
  * Saving aac frames from DAB+ services for processing by e.g. VLC
  * Saving the ensemble content (description of audio and data streams, including almost all technical data) into a text file readable by e.g *LibreOfficeCalc*
  * ip output: when configured the ip data - if selected - is sent to a specificied ip address (default: 127.0.0.1:8888)
  * TPEG output: when configured the data is sent to a specified ip address

--------------------------------------------------------------------
New in sdrplayDab-3.3
--------------------------------------------------------------------

Two of the (many) areas that still needed addressing in the handling of
DAB+ contents were handling

  * secondary audio services* and reacting upon a
  * change in  configuration*.

While not encountered here (the Netherlands), *Secondary audio services*
are seen in BBC transmissions. They are now visible as service
and can be selected.

A *change in configuration* may be that bitrates of channels change,
that protection changes, and even that *secondary audio services*
appear or disappear.

Especially interesting is of course  what the software should do
when - after a *change in configuration* - the selected secondary
audio service is gone.

In sdrplayDab-3.3 a mechanism is included to make secondary audio services
visible and selectabe, and to handle a change in configuration.
Such a change will manifest itself as a minor disruption (app 20 msec)
in the signal of the selected service.

While it is known that the DAB transmissions are in Band III, there are
situations where it is desirable to use other frequencies.
If you want to experiment with a modulator, connected to an SDR device
on different frequencies than the default one (or you want just to
have a restricted number of channels from Band III or L Band), sdrplay-DAB
offers a possibility to specify a list of channels to be used.
Specify in a file a list of channels, e.g.

	jan	227360
	twee	220352
	drie	1294000
	vier	252650

and pass the file on with the "-A" command line switch.


Support for the 3.06 library for the RSP's of SDRplay

------------------------------------------------------------------
Widgets and scopes
------------------------------------------------------------------

The picture on top shows sdrplayDab's main window and the other 5 **optional**
widgets:

  * a widget with controls for the attached device,
  * a widget showing the technical information of the *selected service* as well
as some information on the quality of the decoding, 
  * a widget showing the spectrum of the received radio signal and the constellation of the decoded signal,
  * a widget showing the spectrum of the NULL period between successive DAB frames from which the TII is derived,
  * and a widget showing the response(s) from different transmitters in the SFN,

Another - a sixth - widget shows when running a *scan*; the widget will show the contents of the ensembles found in the selected channel.

While the main window is always shown, visibility of the others is
under user control, the main widget contains a button for each of those.

The buttons and other controls on the main widget are equipped with
*tool tips* briefly explaining the (in most cases obvious) function
of the element. (The tooltip on the copyright label shows (a.o) the date the executable was generated.)

The elements in the **left part** of the widget, below the list of services,
 are concerned with selecting a channel and service. 

The channel selector is augmented with a "-" and a "+"
button for selecting the previous resp. next channel.
Similarly,  a second pair of "-" and "+" buttons
is available for selecting the previous resp. the next service 
on the services list.

Some data on the selected service - if any - is to be found on
a separate widget. This widget will show where the data for the
service is to be found in the DAB frames, and how it is encoded.

The further selectors are concentrated on the bottom part of the right side
of the widget. Buttons to make scopes visible, to store input and or
output into a file, to select input device and the audio and to
scan and store a description of the ensemble are in that section.

![sdrplay scan result](/sdrplay-dab-buttons.png?raw=true)

During **scanning**, a separate window will be shown with the results
of the scan as shown in the picture.

![sdrplayDab](/sdrplayDab-scanner.png?raw=true)

----------------------------------------------------------------------
Presets
----------------------------------------------------------------------

A *preset* selector is available to store and retrieve "favorit" services.
Note that the services are encoded as "channel:serviceName" pair:
it sometimes happens that a service appears in more than one ensemble
(as example the "Omroep West" service appears in channels 5B and 8A.)

The presets are stored in an xml file, `.qt-dab-presets.xml'.

*Adding* a service to the *presets* is simply by *clicking with the right mouse
button on the name of the service that is currently selected in the
servicelist* (recall that clicking with the *left* mouse button
well select the service with that name).

Of course, one is also able to *remove* an entry from the presets.
To do this, select the entry (by putting the curson on it without
clicking any of the mouse buttons) and press the *shift* and the *delete*
button on the keyboard simultaneously.

---------------------------------------------------------------------------
Maintaining History 
---------------------------------------------------------------------------

Qt-DAB-3.3 saves all service names found. Pairs Channel:serviceName
will be made (in)visible when touching the appropriate button (the
one labeled with "xx").

The data in stored in a file in xml format.
The *history* can be cleared by a click of the right mouse button,
clicking on a channel:servicename combination with the left
mouse button will cause the QT-DAB software to attempt to set the channel and
select the name.

![Qt-DAB with sdrplay input](/qt-dab-3.3-history.png?raw=true)

---------------------------------------------------------------------------
Comment on some settings
-------------------------------------------------------------------------------

Some settings are preserved between program invocations,
they are stored in a file `.sdrplayDab-dab.ini`, to be found in the home directory.
Settings maintained between program invocations are typically the name 
of the device used, the channel that was used, the name of the service
last used etc.

`saveSlides=1` 
when set to 0 the slides that are attached to audio programs will not be saved. If set to 1 the slides will be saved in a directory `/tmp/qt-pictures` (Linux) or in `%tmp%\qt-pictures` (Windows).

`picturesPath` 
defines the directory where the slides (MOT slideshow) should be stored. Default is the home directory.

`showSlides=1` 
when set to 0 the slides will not be shown.

`has-presetName=1` 
when set the name of the selected service - that is selected when closing down the program - is kept and at the next invocation of the program, an attempt is made to start that particular service. The name of the service is kept as `presetname=xxxx`

The background colors of the spectrum can be changed by setting 

```
displaycolor=blue
gridcolor=red
```
If one uses the rtl_tcp handler, the default value for the "port" is 1234,
a port can be set in the ".ini" file by setting

`rtl_tcp_port=xxx`

In selecting a service from the presetlist that resides on another
channel implies switching to that channel and waiting until the
ensemble in that channel is recognized.
The duration of the delay can be specified  by setting

`switchTime=xxx`

where xxx is the amount of milliseconds

The result of the correlation is shown ny the correlation viewer. The
correlation takes place on the first datablock of a DAB frame, and if
the estimation of the end of the null period is correct, the maximum
in the correlation is 504. Since for DAB a single frequency network is
used, in general more than one transmitter is received and there are
more "peaks" in the correlation.
The distance between the peaks is in units of 1/2048000 seconds.
While the correlationviewer by default shows the correltion result of
the first 1000 samples, this may be reduced by setting

`plotLength=xxx`

where xxx is the number of samples taken into account.

-------------------------------------------------------------------------
Obsolete properties
-------------------------------------------------------------------------

The current DAB standard eliminated the support for Modes other than Mode 1 and Bands other than VHF Band III. The sdrplay-DAB implementation still supports these features, however, since they are obsolete, there are no controls on the GUI anymore (the control for the Mode was already removed from earlier versions). 

Explicitly setting the Mode and/or the Band is possible by
including some command lines in the ".qt-dab.ini" file.

For the Mode, one will add/set a line

	dabMode=Mode x, where x is either 1, 2 or 4

For the Band, one will add/set a line

	dabBand=band, where band is either VHF Band III or L_Band

----------------------------------------------------------------------------
Installation
----------------------------------------------------------------------------

For **windows** an  **installer** is to be found in the releases section, https://github.com/JvanKatwijk/sdrplayDab/releases. The installer will install the executable as well as required libraries.
The installer will also call the official installer for the dll implementing the api for getting access to the SDRplay devices.

For Linux on the RPI a script is available that -when run - will fetch all
required libraries and generate an executable.
Note that the library (libraries) for SDRplay support have to be installed.

Note firther that in this version both the 2.13 and the 3.06 libraries
are supported (if configured).

Assuming support for bot libraries is configured, operation is
	
	- first an attempt is made to load the functions from the support library 2.13. If that succeeds and attempt is made to open the device.
	- in case of success, operation with that library starts
	- in case of failure (i.e. no library is found or no device could be identified), functions from the support library 3.06 are loaded. If that succeeds an attempt is made to open the device.
	- in case of success, operation with that library starts
	- in case of failure (see above), a widget will show where a
file can be selected.

# Copyright

	Copyright (C)  2017 .. 2020
	Jan van Katwijk (J.vanKatwijk@gmail.com)
	Lazy Chair Computing

	The sdrplayDab software is made available under the GPL-2.0.
	The sdrplayDab software, of which the sdrplayDab software is a part, 
	is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

