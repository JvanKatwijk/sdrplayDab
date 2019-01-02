
sdrplayDab is experimental software for Windows, Linux and Raspberry Pi for listening to terrestrial Digital Audio Broadcasting (DAB and DAB+).
It is derived from Qt-DAB, and designed for sole use with SDRplay RSP
devices. It uses a different approach for gain setting and
for recognizing DAB frames than used in qt-dab. 

![sdrplayDab](/sdrplay-dab-1.png?raw=true)


![sdrplayDab](/sdrplay-dab-2.png?raw=true)


-----------------------------------------------------------------------------
Windows
------------------------------------------------------------------------------

For Windows, the releases section contains an *installer*, setup-sdrplayDab.exe.
The installer will install the executable of the program together with the required
dll's. Note that the installer will call the official installer for the dll implementing
the api to get access to the SDRplay device

------------------------------------------------------------------------------------
Linux
------------------------------------------------------------------------------------

For Linux, however, one has to create an executable. The approach is the same as for Qt-DAB
(although one obviously does not need to install airspy or dabstick handlers).


------------------------------------------------------------------------------

The GUI of sdrplayDab is almost equal to the GUI of qt-dab.
Since the device to be used is the SDRplay RSP, there
is no device selector. Starting the program without a connected device
will open a menu for file selection (however, note that only files
created by sdrplayDab are suitable for processing).

New is that on start up no widget is presented for the SDRplay control.
I really like to see as much as possible about the settings 
and data of the selected service, I can imagine, however, that others just
want to listen, without too many widgets on the screen.

As with the widgets for Technical Details and others, touching a button -
the one labeled "device handler" - will cause the widget for the SDRplay
control to be visible (and touching it again will hide it).

Note that apart from device handling and ofdm handling, the code
is the code from qt-dab. It is most likely that this code will
be merged with the qt-dab code base.


--------------------------------------------------------------------------

While it is possible to read in and process a file, neither the
main program nor the file handler will do frequency correction,
so the data in the file should not have an offset in the frequency.

With sdrplayDab one is able to dump the frequency and gain corrected input
into a file, so these files can be processed successfully by sdrplayDab.


