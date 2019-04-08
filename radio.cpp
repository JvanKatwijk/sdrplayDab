#
/*
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB (formerly SDR-J, JSDR).
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<QSettings>
#include	<QMessageBox>
#include	<QFileDialog>
#include	<QDebug>
#include	<QDateTime>
#include	<QFile>
#include	<QStringList>
#include	<QStringListModel>
#include	<QDir>
#include	<QMouseEvent>
#include	<fstream>
#include	"dab-constants.h"
#include	<iostream>
#include	<numeric>
#include	<unistd.h>
#include	<vector>
#include	"radio.h"
#include	"band-handler.h"
#include	"ensemble-printer.h"
#include        "audio-descriptor.h"
#include        "data-descriptor.h"
#include	"device-handler.h"
#include	"sdrplay-handler.h"
#include	"wavfiles.h"
#ifdef	TCP_STREAMER
#include	"tcp-streamer.h"
#elif	QT_AUDIO
#include	"Qt-audio.h"
#else
#include	"audiosink.h"
#endif
#include	"ui_technical_data.h"
#include	"spectrum-viewer.h"
#include	"impulse-viewer.h"
#include        "tii-viewer.h"

std::vector<size_t> get_cpu_times (void) {
	std::ifstream proc_stat ("/proc/stat");
	proc_stat. ignore (5, ' ');    // Skip the 'cpu' prefix.
	std::vector<size_t> times;
	for (size_t time; proc_stat >> time; times. push_back (time));
	return times;
}
 
bool get_cpu_times (size_t &idle_time, size_t &total_time) {
	const std::vector <size_t> cpu_times = get_cpu_times ();
	if (cpu_times. size () < 4)
           return false;
	idle_time  = cpu_times [3];
	total_time = std::accumulate (cpu_times. begin (), cpu_times. end (), 0);
	return true;
}
/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is initiated by gui buttons
  */

	RadioInterface::RadioInterface (QSettings	*Si,
	                                int32_t		dataPort,
	                                QWidget		*parent):
	                                        QMainWindow (parent) {
int16_t	latency;
int16_t k;
QString h;

	dabSettings		= Si;
	running. store (false);
	isSynced		= UNSYNCED;
	dabBand			= BAND_III;
        spectrumBuffer          = new RingBuffer<std::complex<float>> (2 * 32768);
	iqBuffer		= new RingBuffer<std::complex<float>> (2 * 1536);
	responseBuffer		= new RingBuffer<float> (32768);
	tiiBuffer               = new RingBuffer<std::complex<float>> (32768);
	audioBuffer		= new RingBuffer<int16_t>(16 * 32768);

/**	threshold is used in the phaseReference class 
  *	as threshold for checking the validity of the correlation result
  *	3 is a reasonable value
  */
	threshold	=
	           dabSettings -> value ("threshold", 3). toInt ();
//
//	latency is used to allow different settings for different
//	situations wrt the output buffering. For windows and the RPI
//	this need to be a pretty large value (e.g. 5 to 10)
	latency		=
	           dabSettings -> value ("latency", 5). toInt ();

	diff_length	=
	           dabSettings	-> value ("diff_length", DIFF_LENGTH). toInt ();

	int	tii_depth	= dabSettings -> value ("tii_depth", 1). toInt ();
        int	echo_depth	= dabSettings -> value ("echo_depth", 1). toInt ();

	dataBuffer		= new RingBuffer<uint8_t>(32768);
///	The default, most likely to be overruled
#ifdef	_SEND_DATAGRAM_
	ipAddress		= dabSettings -> value ("ipAddress", "127.0.0.1"). toString ();
	port			= dabSettings -> value ("port", 8888). toInt ();
#endif
//
	has_presetName		= dabSettings -> value ("has-presetName", 1). toInt () != 0;
	if (has_presetName) 
	   presetName		= dabSettings -> value ("presetname", ""). toString ();
	currentName		= QString ("");

	saveSlides	= dabSettings -> value ("saveSlides", 1). toInt ();
	showSlides	= dabSettings -> value ("showPictures", 1). toInt ();
	if (saveSlides != 0)
	   set_picturePath ();

///////////////////////////////////////////////////////////////////////////

//	The settings are done, now creation of the GUI parts
	setupUi (this);
//
	dataDisplay	= new QFrame (nullptr);
	techData. setupUi (dataDisplay);
	dataDisplay		->  hide ();
#ifdef	__MINGW32__
	techData. cpuLabel	-> hide ();
	techData. cpuMonitor	-> hide ();
#endif
//
#ifdef	DATA_STREAMER
	dataStreamer		= new tcpServer (dataPort);
#else
	(void)dataPort;
#endif

//	Where do we leave the audio out?
	streamoutSelector	-> hide ();
#ifdef	TCP_STREAMER
	soundOut		= new tcpStreamer	(20040);
#elif	QT_AUDIO
	soundOut		= new Qt_Audio ();
#else
// just sound out
	soundOut		= new audioSink		(latency);

	((audioSink *)soundOut)	-> setupChannels (streamoutSelector);
	streamoutSelector	-> show ();
	bool err;
	h	= dabSettings -> value ("soundchannel", "default"). toString ();
	k	= streamoutSelector -> findText (h);
	if (k != -1) {
	   streamoutSelector -> setCurrentIndex (k);
	   err = !((audioSink *)soundOut) -> selectDevice (k);
	}

	if ((k == -1) || err)
	   ((audioSink *)soundOut)	-> selectDefaultDevice ();
#endif

	my_spectrumViewer	= new spectrumViewer (this, dabSettings,
	                                       spectrumBuffer,
	                                       iqBuffer);

	my_impulseViewer	= new impulseViewer (this,
	                                             responseBuffer);

	my_tiiViewer            = new tiiViewer (this, tiiBuffer);

//
//	restore some settings from previous incarnations
	QString t       =
                dabSettings     -> value ("dabBand", "VHF Band III"). toString ();
        k       = bandSelector -> findText (t);
        if (k != -1)
           bandSelector -> setCurrentIndex (k);
        dabBand         = bandSelector -> currentText () == "VHF Band III" ?
                                             BAND_III : L_BAND;

        theBand. setupChannels  (channelSelector, dabBand);

        dabMode       = dabSettings   -> value ("dabMode", "Mode 1"). toString ();
//
	QPalette p	= techData. ficError_display -> palette ();
	p. setColor (QPalette::Highlight, Qt::green);
	techData. ficError_display	-> setPalette (p);
	techData. frameError_display	-> setPalette (p);
	techData. rsError_display	-> setPalette (p);
	techData. aacError_display	-> setPalette (p);
	techData. rsError_display	-> hide ();
	techData. aacError_display	-> hide ();
	techData. motAvailable		-> 
                           setStyleSheet ("QLabel {background-color : red}");
//
//
	dumpfilePointer		= nullptr;
	audioDumping		= false;
	audiofilePointer	= nullptr;
	ficBlocks		= 0;
	ficSuccess		= 0;
	pictureLabel		= nullptr;

	scanning		= false;

	ensemble.setStringList (Services);
	ensembleDisplay	-> setModel (&ensemble);
	Services << " ";
	ensemble. setStringList (Services);
	ensembleDisplay	-> setModel (&ensemble);
//
	connect (streamoutSelector, SIGNAL (activated (int)),
	         this,  SLOT (set_streamSelector (int)));
//	
//	display the version
	QString v = "sdrplay-dab -" + QString (CURRENT_VERSION);
	QString versionText = "sdrplay-dab version: " + QString(CURRENT_VERSION);
        versionText += " Build on: " + QString(__TIMESTAMP__) + QString (" ") + QString (GITHASH);
	versionName	-> setText (v);
	versionName	-> setToolTip (versionText);

//	and start the timer(s)
//	The displaytimer is there to show the number of
//	seconds running and handle - if available - the tii data
	displayTimer. setInterval (1000);
	connect (&displayTimer, SIGNAL (timeout (void)),
	         this, SLOT (updateTimeDisplay (void)));
	displayTimer. start (1000);
	numberofSeconds		= 0;
//
//	timer for scanning
	signalTimer. setSingleShot (true);
	signalTimer. setInterval (5000);
	connect (&signalTimer, SIGNAL (timeout (void)),
	         this, SLOT (No_Signal_Found (void)));
	
	h       = dabSettings -> value ("channel", "12C"). toString ();
        k           = channelSelector -> findText (h);
        if (k != -1) {
           channelSelector -> setCurrentIndex (k);
        }

	antennaSwitch		-> hide ();
	phaseOffsetDisplay	-> hide ();
	antennaSwitch_mode	= false;
	inputDevice	= nullptr;
	try {
	   inputDevice	= new sdrplayHandler (this, dabSettings);
#ifdef	__SHOW_PHASEDIFFERENCE__
	   if (inputDevice -> isSDRPLAY_2 ()) {
	      connect (antennaSwitch, SIGNAL (clicked (void)),
	               this, SLOT (antennaSwitcher (void)));
	      antennaSwitch -> show ();
	   }
	   
#endif
 	}
	catch (int e) {
	   fprintf (stderr, "no sdrplay device, trying to open a file\n");
	}

	if (inputDevice	== nullptr) {
	   try {
	      QString file = QFileDialog::getOpenFileName (this,
	                                                tr ("Open file ..."),
	                                                QDir::homePath (),
	                                                tr ("raw data (*.sdr)"));
	      if (file == QString (""))
	         throw (21);

	      file	= QDir::toNativeSeparators (file);
	      inputDevice	= new wavFiles (file);
	      hideButtons ();
	   }
	   catch (int e) {
	      QMessageBox::warning (this, tr ("Warning"),
	                               tr ("file not found"));
	   }
	}

	if (inputDevice == nullptr) {
	   QMessageBox:: warning (this, tr ("Warning"),
	                                tr ("no device, fatal"));
	   exit (21);
	}

	secondariesVector. resize (0);
	connectGUI ();
//	It is time for some action
	my_dabProcessor = new dabProcessor   (this,
	                                      convert (dabMode),
	                                      threshold,
	                                      diff_length,
	                                      inputDevice -> bitDepth (),
                                              tii_depth,
	                                      echo_depth,
	                                      responseBuffer,
	                                      spectrumBuffer,
	                                      iqBuffer,
	                                      tiiBuffer,
	                                      picturesPath
	                                      );

	inputDevice	-> setEnv (my_dabProcessor);
	int32_t frequency	= theBand. Frequency (dabBand,
	                                 channelSelector -> currentText ());
	bool r	= inputDevice -> restartReader (frequency);
	if (!r) {
	   fprintf (stderr, "problems in starting device\n");
	   exit (21);
	}

	if (has_presetName && (presetName != QString (""))) {
	   presetTimer. setSingleShot (true);
	   presetTimer. setInterval (8000);
	   connect (&presetTimer, SIGNAL (timeout (void)),
	            this, SLOT (setPresetStation (void)));
	   presetTimer. start (8000);
	}

	clearEnsemble ();		// the display
	qApp    -> installEventFilter (this);
        currentService  = nullptr;

	running. store (true);
}

	RadioInterface::~RadioInterface (void) {
	fprintf (stderr, "radioInterface is deleted\n");
}
//
/**
  *	\brief At the end, we might save some GUI values
  *	The QSettings could have been the class variable as well
  *	as the parameter
  */
void	RadioInterface::dumpControlState (QSettings *s) {
	if (s == nullptr)	// cannot happen
	   return;

	if (has_presetName)
	   s	-> setValue ("presetname", currentName);
	s	-> setValue ("channel",
	                      channelSelector -> currentText ());
	s	-> setValue ("soundchannel",
	                               streamoutSelector -> currentText ());
	s	-> setValue ("dabBand", bandSelector -> currentText ());
	s	-> sync ();
}

//
//
///////////////////////////////////////////////////////////////////////////////
//	
//	The public slots are called from other places within the dab software
//	so please provide some implementation, perhaps an empty one
//
//	a slot called by the ofdmprocessor
void	RadioInterface::set_freqOffset (int v) {
//	if (running. load ())
//	   correctorDisplay	-> display (v);
}

void	RadioInterface::signalTimer_out (void) {
	No_Signal_Found ();
}

void	RadioInterface::No_Signal_Found (void) {
	signalTimer. stop ();
	if (!running. load () || !scanning)
	   return;
//
//	we stop the thread from running,
//	Increment the frequency
//	and restart
	inputDevice     -> stopReader ();
        my_dabProcessor -> stop ();
        Increment_Channel ();
        clearEnsemble ();
        int32_t frequency = theBand. Frequency (dabBand,
                                         channelSelector -> currentText ());
        inputDevice     -> restartReader (frequency);

	signalTimer. start (5000);
}
//
//	In case the scanning button was pressed, we
//	set it off as soon as we have a signal found
void	RadioInterface::Yes_Signal_Found (void) {
	if (!running. load ())
	   return;

	signalTimer. stop ();
	if (!scanning)
	   return;

	set_Scanning ();
}

void    RadioInterface::set_Scanning    (void) {
        if (!running. load ())
           return;

        presetTimer. stop ();
        setStereo (false);
        if (!running. load ())
           return;

        scanning        = !scanning;
        if (scanning) {
           scanButton -> setText ("scanning");
	   inputDevice		-> stopReader ();
	   my_dabProcessor	-> stop ();
	   clearEnsemble ();
	   Increment_Channel	();
	   int32_t frequency = theBand. Frequency (dabBand,
                                         channelSelector -> currentText ());
	   inputDevice		-> restartReader (frequency);
           signalTimer. start (5000);
        }
        else
           scanButton -> setText ("Scan band");
}

//
//	Increment channel is called during scanning.
//	The approach taken here is to increment the current index
//	in the combobox and select the new frequency.
//	To avoid disturbance, we disconnect the combobox
//	temporarily, since otherwise changing the channel would
//	generate a signal
void	RadioInterface::Increment_Channel (void) {
int32_t	tunedFrequency;
	if (!running. load ())
	   return;

int	cc	= channelSelector -> currentIndex ();

	cc	+= 1;
	if (cc >= channelSelector -> count ())
	   cc = 0;
//
//	To avoid reaction of the system on setting a different value
	disconnect (channelSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (selectChannel (const QString &)));
	channelSelector -> setCurrentIndex (cc);
	connect    (channelSelector, SIGNAL (activated (const QString &)),
	              this, SLOT (selectChannel (const QString &)));
}

///////////////////////////////////////////////////////////////////////////
/**
  *	clearEnsemble
  *	on changing settings, we clear all things in the gui
  *	related to the ensemble.
  *	The function is called from "deep" within the handling code
  *	Potentially a dangerous approach, since the fic handler
  *	might run in a separate thread and generate data to be displayed
  */
void	RadioInterface::clearEnsemble	(void) {

	if (!running. load ())
	   return;

//	it obviously means: stop processing
	my_dabProcessor	-> clearEnsemble ();
	my_dabProcessor	-> reset ();
	clear_showElements	();
}

//
//	a slot, called by the fic/fib handlers
void	RadioInterface::addtoEnsemble (const QString &s) {
	if (!running. load ())
	   return;

	Services << s;
	Services. removeDuplicates ();
	ensemble. setStringList (Services);
	ensembleDisplay	-> setModel (&ensemble);
}

//
///	a slot, called by the fib processor
void	RadioInterface::nameofEnsemble (int id, const QString &v) {
QString s;
	if (!running. load ())
	   return;

	(void)v;
	ensembleId	-> display (id);
	ensembleLabel	= v;
	ensembleName    -> setAlignment(Qt::AlignCenter);
	ensembleName	-> setText (v);
	my_dabProcessor	-> coarseCorrectorOff ();
	Yes_Signal_Found ();
}

///////////////////////////////////////////////////////////////////////
/**
  *	\brief show_successRate
  *	a slot, called by the MSC handler to show the
  *	percentage of frames that could be handled
  */
void	RadioInterface::show_frameErrors (int s) {
	if (running. load ())
	   techData. frameError_display	-> setValue (100 - 4 * s);
}

void	RadioInterface::show_rsErrors (int s) {
	if (running. load ())
	   techData. rsError_display	-> setValue (100 - 4 * s);
}
	
void	RadioInterface::show_aacErrors (int s) {
	if (running. load ())
	   techData. aacError_display	-> setValue (100 - 4 * s);
}
	
void	RadioInterface::show_ficSuccess (bool b) {
	if (!running. load ())	
	   return;
	if (b)
	   ficSuccess ++;
	if (++ficBlocks >= 100) {
	   techData. ficError_display	-> setValue (ficSuccess);
	   ficSuccess	= 0;
	   ficBlocks	= 0;
	}
}

void	RadioInterface::show_motHandling (bool b) {
	if (!running. load ())
	   return;
	if (b) {
	   techData. motAvailable -> 
	               setStyleSheet ("QLabel {background-color : green}");
	}
	else {
	   techData. motAvailable ->
	               setStyleSheet ("QLabel {background-color : red}");
	}
}
	

///	just switch a color, obviously GUI dependent, but called
//	from the ofdmprocessor
void	RadioInterface::setSynced	(char b) {
}

//	showLabel is triggered by the message handler
//	the GUI may decide to ignore this
void	RadioInterface::showLabel	(QString s) {
	if (running. load ())
	   dynamicLabel	-> setText (s);
}

void	RadioInterface::setStereo	(bool s) {
	if (!running. load ())
	   return;
	if (s) 
	   stereoLabel -> 
	               setStyleSheet ("QLabel {background-color : green}");

	else
	   stereoLabel ->
	               setStyleSheet ("QLabel {background-color : red}");
}
//
//////////////////////////////////////////////////////////////////////////
//
void	checkDir (QString &s) {
int16_t	ind	= s. lastIndexOf (QChar ('/'));
int16_t	i;
QString	dir;
	if (ind == -1)		// no slash, no directory
	   return;

	for (i = 0; i < ind; i ++)
	   dir. append (s [i]);

	if (QDir (dir). exists ())
	   return;
	QDir (). mkpath (dir);
}
//	showMOT is triggered by the MOT handler,
//	the GUI may decide to ignore the data sent
//	since data is only sent whenever a data channel is selected
void	RadioInterface::showMOT		(QByteArray data,
	                                 int subtype, QString pictureName) {
const char *type;
	if (!running. load ())
	   return;
	if (pictureLabel == nullptr) 
	   pictureLabel	= new QLabel (nullptr);

	type = subtype == 0 ? "GIF" :
	       subtype == 1 ? "JPG" :
//	       subtype == 1 ? "JPEG" :
	       subtype == 2 ? "BMP" : "PNG";

	QPixmap p;
	p. loadFromData (data, type);
	if (saveSlides && (pictureName != QString (""))) {
	   pictureName	= QDir::toNativeSeparators (pictureName);
	   FILE *x = fopen (pictureName. toLatin1 (). data (), "w+b");
	   if (x == nullptr)
	      fprintf (stderr, "cannot write file %s\n",
	                            pictureName. toLatin1 (). data ());
	   else {
	      fprintf (stderr, "going to write file %s\n",
	                            pictureName. toLatin1 (). data ());
	      (void)fwrite (data. data (), 1, data.length (), x);
	      fclose (x);
	   }
	}

//	pictureLabel -> setFrameRect (QRect (0, 0, p. height (), p. width ()));

	if (showSlides) {
	   pictureLabel ->  setPixmap (p);
	   pictureLabel ->  show ();
	}
}
//
//	sendDatagram is triggered by the ip handler,
//
void	RadioInterface::sendDatagram	(int length) {
uint8_t localBuffer [length];
	if (dataBuffer -> GetRingBufferReadAvailable () < length) {
	   fprintf (stderr, "Something went wrong\n");
	   return;
	}
	dataBuffer -> getDataFromBuffer (localBuffer, length);
#ifdef	_SEND_DATAGRAM_
	if (running. load ())
	   dataOut_socket. writeDatagram ((const char *)localBuffer, length,
	                                   QHostAddress (ipAddress),
	                                   port);
#endif
}
//
//
void	RadioInterface::handle_tdcdata (int frametype, int length) {
#ifdef DATA_STREAMER
uint8_t localBuffer [length + 8];
#endif
	if (!running. load ())
	   return;
	if (dataBuffer -> GetRingBufferReadAvailable () < length) {
	   fprintf (stderr, "Something went wrong\n");
	   return;
	}
#ifdef	DATA_STREAMER
	dataBuffer -> getDataFromBuffer (&localBuffer [8], length);
	localBuffer [0] = 0xFF;
	localBuffer [1] = 0x00;
	localBuffer [2] = 0xFF;
	localBuffer [3] = 0x00;
	localBuffer [4] = (length & 0xFF) >> 8;
	localBuffer [5] = length & 0xFF;
	localBuffer [6] = 0x00;
	localBuffer [7] = frametype == 0 ? 0 : 0xFF;
	if (running. load ())
	   dataStreamer -> sendData (localBuffer, length + 8);
#endif
}

/**
  *	\brief changeinConfiguration
  *	No idea yet what to do, so just give up
  *	with what we were doing. The user will -eventually -
  *	see the new configuration from which he can select
  */
void	RadioInterface::changeinConfiguration	(void) {
	if (running. load ()) {
	   soundOut		-> stop ();
	   fprintf (stderr, "in changeinConf\n");
	   inputDevice		-> stopReader ();
	   inputDevice		-> resetBuffer ();
	   my_dabProcessor	-> reset ();
	   clear_showElements	();
	}
}
//
//	In order to not overload with an enormous amount of
//	signals, we trigger this function at most 10 times a second
//
void	RadioInterface::newAudio	(int amount, int rate) {
	if (running. load ()) {
	   int16_t vec [amount];
	   while (audioBuffer -> GetRingBufferReadAvailable () > amount) {
	      audioBuffer -> getDataFromBuffer (vec, amount);
	      soundOut	-> audioOut (vec, amount, rate);
	   }
	}
}
//
//	This function is only used in the Gui to clear
//	the details of a selection
void	RadioInterface::clear_showElements (void) {
	if (!running. load ())
	   return;
	Services		= QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay		-> setModel (&ensemble);
	my_dabProcessor		-> clearEnsemble ();

	ensembleLabel		= QString ();
	ensembleName		-> setText (ensembleLabel);
	dynamicLabel		-> setText ("");
	
//	Then the various displayed items
	ensembleName		-> setText ("   ");

	techData. frameError_display	-> setValue (0);
	techData. rsError_display	-> setValue (0);
	techData. aacError_display	-> setValue (0);
	techData. ficError_display	-> setValue (0);
	techData. ensemble 		-> setText (QString (""));
	techData. programName		-> setText (QString (""));
	techData. frequency		-> display (0);
	techData. bitrateDisplay	-> display (0);
	techData. startAddressDisplay	-> display (0);
	techData. lengthDisplay		-> display (0);
	techData. subChIdDisplay	-> display (0);
//	techData. protectionlevelDisplay -> display (0);
	techData. uepField		-> setText (QString (""));
	techData. ASCTy			-> setText (QString (""));
	techData. language		-> setText (QString (""));
	techData. programType		-> setText (QString (""));
	techData. motAvailable		-> 
	               setStyleSheet ("QLabel {background-color : red}");
	techData. transmitter_coordinates -> setText (" ");

//	snrDisplay		-> display (0);
	if (pictureLabel != nullptr)
	   delete pictureLabel;
	pictureLabel = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//	

/**
  *	\brief TerminateProcess
  *	Pretty critical, since there are many threads involved
  *	A clean termination is what is needed, regardless of the GUI
  */
void	RadioInterface::TerminateProcess (void) {
	running. store (false);
#ifdef	DATA_STREAMER
	fprintf (stderr, "going to close the dataStreamer\n");
	delete		dataStreamer;
#endif
	displayTimer. stop ();
	signalTimer.  stop ();
	presetTimer.  stop ();

	if (audioDumping) {
	   soundOut	-> stopDumping ();
	   sf_close (audiofilePointer);
	}

	if (inputDevice != nullptr) 
	   inputDevice		-> stopReader ();	// might be concurrent

	if (my_dabProcessor != nullptr)
	   my_dabProcessor	-> stop ();		// definitely concurrent
	if (currentService != nullptr)
           delete currentService;
	currentService	= nullptr;
	soundOut		-> stop ();
	dataDisplay		->  hide ();
//	everything should be halted by now
	dumpControlState (dabSettings);
	delete		soundOut;
	if (inputDevice != nullptr)
	   delete		inputDevice;
	fprintf (stderr, "going to delete dabProcessor\n");
	if (my_dabProcessor != nullptr)
	   delete		my_dabProcessor;
	fprintf (stderr, "deleted dabProcessor\n");
	delete	dataDisplay;
	delete	my_spectrumViewer;
	delete	my_tiiViewer;
	delete	my_impulseViewer;
	if (pictureLabel != nullptr)
	   delete pictureLabel;
	pictureLabel = nullptr;		// signals may be pending, so careful
	delete	iqBuffer;
	delete	spectrumBuffer;
	delete	responseBuffer;
//	close ();
	fprintf (stderr, ".. end the radio silences\n");
}

//
/**
  *	\brief selectChannel
  *	Depending on the GUI the user might select a channel
  *	or some magic will cause a channel to be selected
  */
void	RadioInterface::selectChannel (QString s) {
int32_t	tunedFrequency;

	presetTimer. stop ();
	setStereo (false);

	if (currentService != nullptr)
	   delete currentService;
	currentService	= nullptr;
	stop_sourceDumping ();
	if (!my_tiiViewer -> isHidden ())
	   my_tiiViewer -> hide ();

	soundOut		-> stop ();
	inputDevice		-> stopReader ();
	clear_showElements ();
	secondariesVector. resize (0);
	
	tunedFrequency		= theBand. Frequency (dabBand, s);
	if (!inputDevice	-> restartReader (tunedFrequency)) {
	   QMessageBox::warning (this, tr ("Warning"),
                                tr ("Device will not restart\n"));
	   return;
	}
	my_dabProcessor	-> reset ();
	running. store (true);
	dabSettings	-> setValue ("channel", s);
}

static size_t previous_idle_time	= 0;
static size_t previous_total_time	= 0;

void	RadioInterface::updateTimeDisplay (void) {
	if (!running. load ())
	   return;

	numberofSeconds ++;
	int16_t	numberHours	= numberofSeconds / 3600;
	int16_t	numberMinutes	= (numberofSeconds / 60) % 60;
	QString text = QString ("runtime ");
	text. append (QString::number (numberHours));
	text. append (" hr, ");
	text. append (QString::number (numberMinutes));
	text. append (" min");
	timeDisplay	-> setText (text);
#ifndef	__MINGW32__
	if ((numberofSeconds % 2) == 0) {
	   size_t idle_time, total_time;
	   get_cpu_times (idle_time, total_time);
	   const float idle_time_delta = idle_time - previous_idle_time;
           const float total_time_delta = total_time - previous_total_time;
           const float utilization = 100.0 * (1.0 - idle_time_delta / total_time_delta);
	   techData. cpuMonitor -> display (utilization);
           previous_idle_time = idle_time;
           previous_total_time = total_time;
	}
#endif
#ifdef	SHOW_MISSED
	if ((numberofSeconds % 10) == 0) {
	   int xxx = ((audioSink *)soundOut)	-> missed ();
	   fprintf (stderr, "missed %d\n", xxx);
	}
#endif
}

void	RadioInterface::autoCorrector_on (void) {
//	first the real stuff
	if (!running. load ())
	   return;

	clear_showElements	();
	my_dabProcessor		-> clearEnsemble ();
	my_dabProcessor		-> coarseCorrectorOn ();
	my_dabProcessor		-> reset ();
}

void	RadioInterface::set_bandSelect (QString s) {
bool	localRunning = running. load ();

	if (localRunning) {
	   running. store (false);
	   inputDevice	-> stopReader ();
	   fprintf (stderr, "stopReader in band select\n");
	   inputDevice	-> resetBuffer ();
	   clearEnsemble ();
	}

	if (s == "VHF Band III")
	   dabBand	= BAND_III;
	else
	   dabBand	= L_BAND;

	theBand. setupChannels (channelSelector, dabBand);
	if (localRunning) {
	   my_dabProcessor -> reset ();
	   int32_t tunedFrequency	=
	         theBand. Frequency (dabBand, channelSelector -> currentText ());
	   inputDevice	   -> restartReader (tunedFrequency);
	   running. store (true);
	}
}
//
//	Selecting a service is easy, the fib is asked to
//	hand over the relevant data in two steps
void	RadioInterface::selectService (QModelIndex s) {
QString	currentProgram = ensemble. data (s, Qt::DisplayRole). toString ();
	presetTimer. stop ();
	selectService (currentProgram);
}
//
//	Might be called from the GUI as well as from an internal call
void	RadioInterface::selectService (QString s) {
	serviceLabel -> setText (" ");

	if (!my_dabProcessor -> is_audioService (s) &&
            !my_dabProcessor -> is_packetService (s))
           return;

	my_dabProcessor -> reset_msc ();
	currentName = s;
	setStereo (false);
//	soundOut	-> stop ();

//	dataDisplay	-> hide ();
	techData. rsError_display	-> hide ();
	techData. aacError_display	-> hide ();
	techData. motAvailable		-> 
	               setStyleSheet ("QLabel {background-color : red}");



	if (my_dabProcessor -> is_audioService (s)) {
	   audiodata d;
	   my_dabProcessor -> dataforAudioService (s, &d, 0);
	   if (!d. defined) {
	      QMessageBox::warning (this, tr ("Warning"),
	                            tr ("unknown bitrate for this program\n"));
	      return;
	   }

	   int32_t frequency	= theBand. Frequency (dabBand,
	                                    channelSelector -> currentText ());
	   show_techData (ensembleLabel, s, frequency / 1000000.0, &d);
	   serviceLabel -> setAlignment(Qt::AlignCenter);
	   serviceLabel -> setText (s);
	   my_dabProcessor -> set_audioChannel (&d, audioBuffer);
	   for (int i = 1; i < 5; i ++) {
	      packetdata pd;
	      my_dabProcessor -> dataforPacketService (s, &pd, i);
	      if (pd. defined) {
	         my_dabProcessor -> set_dataChannel (&pd, dataBuffer);
	      }
	   }

	   soundOut	-> restart ();
	   showLabel (QString (" "));
	}
	else
	if (my_dabProcessor -> is_packetService (s)) {
           packetdata pd;
           my_dabProcessor -> dataforPacketService (s, &pd, 0);
           if ((!pd. defined) ||
                    (pd.  DSCTy == 0) || (pd. bitRate == 0)) {
              fprintf (stderr, "d. DSCTy = %d, d. bitRate = %d\n",
                                       pd. DSCTy, pd. bitRate);
              QMessageBox::warning (this, tr ("sdr"),
                                    tr ("still insufficient data for this service\n"));
              return;
           }

	   my_dabProcessor -> set_dataChannel (&pd, dataBuffer);
	   switch (pd. DSCTy) {
	      default:
	         showLabel (QString ("unimplemented Data"));
	         break;
	      case 5:
	         fprintf (stderr, "selected apptype %d\n", 
	                                           pd. appType);
	         showLabel (QString ("Transp. Channel not implemented"));
	         break;
	      case 60:
	         showLabel (QString ("MOT partially implemented"));
	         break;
	      case 59: {
#ifdef	_SEND_DATAGRAM_
	            QString text = QString ("Embedded IP: UDP data to ");
	            text. append (ipAddress);
	            text. append (" ");
	            QString n = QString::number (port);
	            text. append (n);
	            showLabel (text);
#else
	            showLabel ("Embedded IP not supported ");
#endif
	         }
	         break;
	      case 44:
	         showLabel (QString ("Journaline"));
	         break;
	   }
	}
	else {
	   QMessageBox::warning (this, tr ("Warning"),
 	                               tr ("unknown service\n"));
	   return;
	}

	if (pictureLabel != nullptr)
	   delete pictureLabel;
	pictureLabel = nullptr;
}
//

//	signal, received from ofdm_decoder that a buffer is filled
//	with amount values ready for display
void	RadioInterface::showIQ	(int amount) {
	if (running. load ())
	   my_spectrumViewer	-> showIQ (amount);
}

void	RadioInterface::showSpectrum	(int32_t amount) {
int32_t frequency = theBand. Frequency (dabBand,
	                                 channelSelector -> currentText ());
	if (running. load ())
	   my_spectrumViewer -> showSpectrum (amount, frequency);
}

void	RadioInterface::showQuality	(float q) {
	if (running. load ())
	   my_spectrumViewer	-> showQuality (q);
}

void	RadioInterface::showImpulse (int amount) {
	if (running. load ())
	   my_impulseViewer -> showImpulse (amount);
}

void	RadioInterface::showIndex (int ind) {
	if (!running. load ())
	   return;
	my_impulseViewer -> showIndex (ind);
}

void    RadioInterface::show_tii (int amount) {
        if (!running. load ())
	   return;
        my_tiiViewer -> showSpectrum (amount);
	my_tiiViewer -> showSecondaries (secondariesVector);
}

void	RadioInterface::set_audioDump (void) {
SF_INFO	*sf_info	= (SF_INFO *)alloca (sizeof (SF_INFO));

	if (audioDumping) {
	   soundOut	-> stopDumping ();
	   sf_close (audiofilePointer);
	   audioDumping = false;
	   audioDumpButton	-> setText ("Save audio");
	   return;
	}

	QString file = QFileDialog::getSaveFileName (this,
	                                        tr ("Save file ..."),
	                                        QDir::homePath (),
	                                        tr ("PCM wave file (*.wav)"));
	if (file == QString (""))
	   return;
	file		= QDir::toNativeSeparators (file);
	if (!file.endsWith (".wav", Qt::CaseInsensitive))
	   file.append (".wav");
	sf_info		-> samplerate	= 48000;
	sf_info		-> channels	= 2;
	sf_info		-> format	= SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	audiofilePointer	= sf_open (file. toUtf8 (). data (),
	                                   SFM_WRITE, sf_info);
	if (audiofilePointer == nullptr) {
	   qDebug () << "Cannot open " << file. toUtf8 (). data ();
	   return;
	}

	audioDumpButton		-> setText ("WRITING");
	audioDumping		= true;
}

void	RadioInterface:: set_streamSelector (int k) {
#if	not defined (TCP_STREAMER) &&  not defined (QT_AUDIO)
	((audioSink *)(soundOut)) -> selectDevice (k);
#else
	(void)k;
#endif
}

void	RadioInterface::toggle_show_data (void) {
	if (dataDisplay -> isHidden ())
	   dataDisplay -> show ();
	else
	   dataDisplay -> hide ();
}

uint8_t	RadioInterface::convert (QString s) {
	if (s == "Mode 1")
	   return 1;
	if (s == "Mode 2")
	   return 2;
	if (s == "Mode 3")
	   return 3;
	if (s == "Mode 4")
	   return 4;
	return 1;
}

void	RadioInterface::showButtons	(void) {
	if (!running. load ())
	   return;

	scanButton	-> show ();
	channelSelector	-> show	();
	bandSelector	-> show ();
	nextChannelButton	-> show ();
	techData. frequency	-> show ();

}

void	RadioInterface::hideButtons	(void) {
//	if (!running. load ())
//	   return;
	scanButton	-> hide ();
	channelSelector	-> hide ();
	bandSelector	-> hide ();
	nextChannelButton	-> hide ();
	techData. frequency	-> hide ();
}

void	RadioInterface::setSyncLost	(void) {
}

void	RadioInterface::showCoordinates (int data) {
int	mainId	= data >> 8;
int	subId	= data & 0xFF;
QString a = "Estimate: ";
QString b = "  ";

	if (!running. load ())
	   return;

	a	.append (QString::number (mainId));
	b	.append (QString::number (subId));
	a. append (b);
	techData. transmitter_coordinates -> setText (a);
}

void    RadioInterface::showSecondaries (int data) {
int     i;

        if (!running. load ())
           return;

        if (data == -1)
           secondariesVector. resize (0);
        else
           secondariesVector. push_back (data);
}

void	RadioInterface::showEnsembleData (void) {
QString currentChannel	= channelSelector -> currentText ();
int32_t frequency = theBand. Frequency (dabBand,
	                                 channelSelector -> currentText ());
ensemblePrinter	my_Printer;

	if (!running. load () || (ensembleLabel == QString ("")))
	   return;
	QString fileName = QFileDialog::getSaveFileName (this,
	                                        tr ("Save file ..."),
	                                        QDir::homePath (),
	                                        tr ("Text (*.txt)"));
	fileName	= QDir::toNativeSeparators (fileName);
	FILE *file_P	= fopen (fileName. toUtf8 (). data (), "w");

	if (file_P == nullptr) {
	   fprintf (stderr, "Could not open file %s\n",
	                              fileName. toUtf8 (). data ());
	   return;
	}
	my_Printer. showEnsembleData (currentChannel, frequency,
	                              Services, my_dabProcessor, file_P);

	fclose (file_P);
}

void	RadioInterface::setPresetStation (void) {

	if (ensembleLabel == QString (""))
	   return;

	if (presetName == QString (""))		// should not happen
	   return;

	for (const auto& service: Services)
	   if (service. contains (presetName)) {
	      selectService (presetName);
	      return;
	   }
}

void	RadioInterface::set_picturePath (void) {
QString defaultPath	= QDir::tempPath ();

	if (defaultPath. endsWith ("/"))
	   defaultPath. append ("qt-pictures/");
	else
	   defaultPath. append ("/qt-pictures/");

	picturesPath	=
	        dabSettings	-> value ("pictures", defaultPath). toString ();

	if ((picturesPath != "") && (!picturesPath. endsWith ("/")))
	   picturesPath. append ("/");
	QDir testdir (picturesPath);

	if (!testdir. exists ())
	   testdir. mkdir (picturesPath);
}

void	RadioInterface::set_sourceDump		(void) {
	if (dumpfilePointer != nullptr)
	   stop_sourceDumping ();
	else
	   start_sourceDumping ();
}

void	RadioInterface::stop_sourceDumping	(void) {
	if (dumpfilePointer != nullptr) {
	   my_dabProcessor -> stopDumping ();
	   sf_close (dumpfilePointer);
	   dumpfilePointer	= nullptr;
	   dumpButton      -> setText ("Dump to raw file");
	}
}

void	RadioInterface::start_sourceDumping	(void) {
SF_INFO *sf_info        = (SF_INFO *)alloca (sizeof (SF_INFO));

	if (dumpfilePointer != nullptr)
	   return;

//      Dumping is GUI dependent and may be ignored
	QString file = QFileDialog::getSaveFileName (this,
	                                     tr ("Save file ..."),
	                                     QDir::homePath (),
	                                     tr ("raw data (*.sdr)"));
	if (file == QString (""))       // apparently cancelled
	   return;
	file    = QDir::toNativeSeparators (file);
	if (!file.endsWith (".sdr", Qt::CaseInsensitive))
	   file.append (".sdr");
	sf_info -> samplerate   = INPUT_RATE;
	sf_info -> channels     = 2;
	sf_info -> format       = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	dumpfilePointer = sf_open (file. toUtf8 (). data (),
	                                   SFM_WRITE, sf_info);
	if (dumpfilePointer == nullptr) {
	   qDebug () << "cannot open " << file. toUtf8 (). data ();
	   return;
	}

	dumpButton      -> setText ("writing");
	my_dabProcessor -> startDumping (dumpfilePointer);
}

void	RadioInterface::set_nextChannel (void) {
//	we stop the thread from running,
//	Increment the frequency
//	and restart
	inputDevice	-> stopReader ();
	my_dabProcessor -> stop ();
	Increment_Channel ();
	clearEnsemble ();
	int32_t frequency = theBand. Frequency (dabBand,
	                                 channelSelector -> currentText ());
	inputDevice	-> restartReader (frequency);
}

void    RadioInterface::set_tiiSwitch (void) {
        if (my_tiiViewer -> isHidden ())
           my_tiiViewer -> show ();
        else
           my_tiiViewer -> hide ();
}

void	RadioInterface::set_irSwitch (void) {
	if (my_impulseViewer -> isHidden ())
	   my_impulseViewer	-> show ();
	else
	   my_impulseViewer	-> hide ();
}

void	RadioInterface::set_spectrumSwitch (void) {
	if (my_spectrumViewer -> isHidden ())
	   my_spectrumViewer -> show ();
	else
	   my_spectrumViewer -> hide ();
}

void	RadioInterface::set_devicehandlerSwitch	(void) {
	if (inputDevice	-> isVisible ())
	   inputDevice	-> hide ();
	else
	   inputDevice	-> show ();
}

//
//	When changing (or setting) a device, we do not want anybody
//	to have the buttons on the GUI touched, so
//	we just disconnet them and (re)connect them as soon as
//	a device is operational
void	RadioInterface::connectGUI (void) {
	connect (showProgramData, SIGNAL (clicked (void)),
	         this, SLOT (toggle_show_data (void)));
	connect (saveEnsembleData, SIGNAL (clicked (void)),
	         this, SLOT (showEnsembleData (void)));
	connect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
	         this, SLOT (selectService (QModelIndex)));
	connect (resetButton, SIGNAL (clicked (void)),
	         this, SLOT (autoCorrector_on (void)));
	connect (scanButton, SIGNAL (clicked (void)),
	         this, SLOT (set_Scanning (void)));
	connect (nextChannelButton, SIGNAL (clicked (void)),
	         this, SLOT (set_nextChannel (void)));
	connect (bandSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (set_bandSelect (const QString &)));
	connect (dumpButton, SIGNAL (clicked (void)),
	         this, SLOT (set_sourceDump (void)));
	connect (audioDumpButton, SIGNAL (clicked (void)),
	         this, SLOT (set_audioDump (void)));
	connect (tiiButton, SIGNAL (clicked (void)),
	         this, SLOT (set_tiiSwitch (void)));
	connect (show_irButton, SIGNAL (clicked (void)),
	         this, SLOT (set_irSwitch (void)));
	connect (show_spectrumButton, SIGNAL (clicked (void)),
	         this, SLOT (set_spectrumSwitch (void)));
	connect (show_deviceHandler, SIGNAL (clicked (void)),
	         this, SLOT (set_devicehandlerSwitch (void)));
	connect (channelSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (selectChannel (const QString &)));
}

void	RadioInterface::disconnectGUI (void) {
	disconnect (showProgramData, SIGNAL (clicked (void)),
	            this, SLOT (toggle_show_data (void)));
	disconnect (saveEnsembleData, SIGNAL (clicked (void)),
	            this, SLOT (showEnsembleData (void)));
	disconnect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
	            this, SLOT (selectService (QModelIndex)));
	disconnect (resetButton, SIGNAL (clicked (void)),
	            this, SLOT (autoCorrector_on (void)));
	disconnect (scanButton, SIGNAL (clicked (void)),
	            this, SLOT (set_Scanning (void)));
	disconnect (nextChannelButton, SIGNAL (clicked (void)),
	            this, SLOT (set_nextChannel (void)));
	disconnect (bandSelector, SIGNAL (activated (const QString &)),
	            this, SLOT (set_bandSelect (const QString &)));
	disconnect (dumpButton, SIGNAL (clicked (void)),
	            this, SLOT (set_sourceDump (void)));
	disconnect (audioDumpButton, SIGNAL (clicked (void)),
	            this, SLOT (set_audioDump (void)));
	disconnect (tiiButton, SIGNAL (clicked (void)),
	            this, SLOT (set_tiiSwitch (void)));
	disconnect (show_irButton, SIGNAL (clicked (void)),
	            this, SLOT (set_irSwitch (void)));
}

void	RadioInterface::show_techData (QString		ensembleLabel, 
	                               QString 		serviceName, 
	                               float 		Frequency,
	                               audiodata	*d) {
	techData. ensemble	-> setText (ensembleLabel);
	techData. programName	-> setText (serviceName);
	techData. frequency	-> display (Frequency);
	techData. bitrateDisplay -> display (d -> bitRate);
	techData. startAddressDisplay -> display (d -> startAddr);
	techData. lengthDisplay	-> display (d -> length);
	techData. subChIdDisplay -> display (d -> subchId);
	uint16_t h = d -> protLevel;
	QString protL;
	if (!d -> shortForm) {
	   protL = "EEP ";
	   protL. append (QString::number ((h & 03) + 1));
	   if ((h & (1 << 2)) == 0)
	      protL. append ("-A");
	   else
	      protL. append ("-B");
	}
	else  {
	   protL = "UEP ";
	   protL. append (QString::number (h));
	}
	techData. uepField	-> setText (protL);
	techData. ASCTy		-> setText (d -> ASCTy == 077 ? "DAB+" : "DAB");
	if (d -> ASCTy == 077) {
	   techData. rsError_display -> show ();
	   techData. aacError_display -> show ();
	}
	techData. language ->
	   setText (the_textMapper.
	               get_programm_language_string (d -> language));
	techData. programType ->
	   setText (the_textMapper.
	               get_programm_type_string (d -> programType));
	if (d -> fmFrequency == -1) {
	   techData. fmLabel	-> hide ();
	   techData. fmFrequency	-> hide ();
	}
	else {
	   techData. fmLabel	-> show ();
	   techData. fmFrequency	-> show ();
	   QString f	= QString::number (d -> fmFrequency);
	   f. append (" KHz");
	   techData. fmFrequency	-> setText (f);
	}
	
}

void	RadioInterface::antennaSwitcher (void) {
	if (!inputDevice -> isSDRPLAY_2 ())
	   return;
	if (antennaSwitch_mode) {
	   phaseOffsetDisplay	-> hide ();
	   ((sdrplayHandler *)inputDevice)	-> antennaSwitcher (false);
	   my_dabProcessor	-> set_phaseComputing (false);
	   antennaSwitch	-> setText ("off");
	}
	else {
	   phaseOffsetDisplay	-> show ();
	   ((sdrplayHandler *)inputDevice)	-> antennaSwitcher (true);
	   my_dabProcessor	-> set_phaseComputing (true);
	   antennaSwitch	-> setText ("on");
	}
	antennaSwitch_mode	= !antennaSwitch_mode;
}

void	RadioInterface::showPhases	(float left, float right) {
	phaseOffsetDisplay	-> display (0.5 - left / (left + right));
}
	   
#include <QCloseEvent>
void RadioInterface::closeEvent (QCloseEvent *event) {

        QMessageBox::StandardButton resultButton =
                        QMessageBox::question (this, "dabRadio",
                                               tr("Are you sure?\n"),
                                               QMessageBox::No | QMessageBox::Yes,
                                               QMessageBox::Yes);
        if (resultButton != QMessageBox::Yes) {
           event -> ignore();
        } else {
           TerminateProcess ();
           event -> accept ();
        }
}

bool	RadioInterface::eventFilter (QObject *obj, QEvent *event) {
	if ((obj == this -> ensembleDisplay -> viewport ()) &&
	    (event -> type () == QEvent::MouseButtonPress )) {
	   QMouseEvent *ev = static_cast<QMouseEvent *>(event);
           if (ev -> buttons () & Qt::RightButton) {
	      audiodata ad;
	      packetdata pd;
	      QString serviceName =
	           this -> ensembleDisplay -> indexAt (ev -> pos()). data ().toString ();
	      my_dabProcessor -> dataforAudioService (serviceName, &ad, 0);
              if (ad. defined) {
	         if (currentService != NULL)
	            delete currentService;
	         currentService	= new audioDescriptor (&ad);
	         return true;
	      }
	      my_dabProcessor -> dataforPacketService (serviceName, &pd, 0);
              if (pd. defined) {
	         if (currentService != NULL)
	            delete currentService;
	         currentService	= new dataDescriptor (&pd);
	         return true;
	      }
	   }
	}
	return QMainWindow::eventFilter (obj, event);
}

void    RadioInterface::showTime        (const QString &s) {
        localTimeDisplay        -> setText (s);
}

