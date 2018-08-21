
sdrplayDab is experimental software for Windows, Linux and Raspberry Pi for listening to terrestrial Digital Audio Broadcasting (DAB and DAB+).
It is derived from qt-dab, and designed for sole use with sdrplay RSP
devices. It uses a different approach for recognizing DAB frames than
used in qt-dab.

![sdrplayDab](/sdrplay-dab.png?raw=true)

Note that apart from device handling and ofdm handling, the code
is the code from qt-dab.
It is most likely that this code will be merged with the qt-dab code base.

Note that while it is possible to read in and process a file, neither the
main program nor the file handler will do frequency correction.

With sdrplayDab one is able to dump the frequency and gain corrected input
into a file, so these files can be processed successfully by sdrplayDab.



