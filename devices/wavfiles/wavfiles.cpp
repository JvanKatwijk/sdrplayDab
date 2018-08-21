#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the sdrplayDab program
 *    sdrplayDab is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    sdrplayDab is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sdrplayDab; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<time.h>
#include	<QString>
#include	"wavfiles.h"
#include	"dab-processor.h"

static inline
int64_t		getMyTime	(void) {
struct timeval	tv;

	gettimeofday (&tv, NULL);
	return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

#define	__BUFFERSIZE	8 * 32768

	wavFiles::wavFiles (QString f) {
SF_INFO *sf_info;

	fileName	= f;
	myFrame		= new QFrame;
	setupUi (myFrame);
//	myFrame		-> show ();

	tester		= 3;
	_I_Buffer	= new RingBuffer<std::complex<float>>(__BUFFERSIZE);

	sf_info		= (SF_INFO *)alloca (sizeof (SF_INFO));
	sf_info	-> format	= 0;
	filePointer	= sf_open (f. toUtf8 (). data (), SFM_READ, sf_info);
	if (filePointer == NULL) {
	   fprintf (stderr, "file %s no legitimate sound file\n", 
	                                f. toUtf8 ().data ());
	   delete myFrame;
	   throw (24);
	}
	if ((sf_info -> samplerate != 2048000) ||
	    (sf_info -> channels != 2)) {
	   fprintf (stderr, "This is not a recorded dab file, sorry\n");
	   sf_close (filePointer);
	   delete myFrame;
	   throw (25);
	}
	nameofFile	-> setText (f);
	currPos		= 0;
	running. store (false);
}

	wavFiles::~wavFiles (void) {
	running. store (false);
	while (isRunning ())
	   usleep (100);
	sf_close (filePointer);
	delete _I_Buffer;
	delete	myFrame;
}

bool	wavFiles::restartReader	(int32_t f) {
	(void)f;
	start ();
	return true;
}

void	wavFiles::stopReader	(void) {
	running. store (false);
}

int16_t	wavFiles::bitDepth	(void) {
	return 12;
}

void	wavFiles::run (void) {
int32_t	t, i;
std::complex<float>	*bi;
int32_t	bufferSize	= 16384;
int64_t	period;
int64_t	nextStop;

	if (running. load ())
	   return;
	running. store (true);

	period		= (16384 * 1000) / 2048;	// full IQś read
	fprintf (stderr, "Period = %ld\n", period);
	bi		= new std::complex<float> [bufferSize];
	nextStop	= getMyTime ();
	while (running. load ()) {
	   nextStop += period;
	   t = readBuffer (bi, bufferSize);
	   if (t < bufferSize) {
	      for (i = t; i < bufferSize; i ++)
	          bi [i] = 0;
	      t = bufferSize;
	   }
	   for (i = 0; i < bufferSize; i ++)
	      base -> addSymbol (bi [i]);
	   if (nextStop - getMyTime () > 0)
	      usleep (nextStop - getMyTime ());
	}
	fprintf (stderr, "taak voor replay eindigt hier\n"); fflush (stderr);
}
/*
 *	length is number of uints that we read.
 */
int32_t	wavFiles::readBuffer (std::complex<float> *data, int32_t length) {
int32_t	i, n;
float	temp [2 * length];

	n = sf_readf_float (filePointer, temp, length);
	if (n < length) {
	   sf_seek (filePointer, 0, SEEK_SET);
	   fprintf (stderr, "End of file, restarting\n");
	}
	for (i = 0; i < n; i ++)
	   data [i] = std::complex<float> (temp [2 * i], temp [2 * i + 1]);
	return	n & ~01;
}

void	wavFiles::show		(void) {
	myFrame	-> show 	();
}

void	wavFiles::hide		(void) {
	myFrame	-> hide ();
}

bool	wavFiles::isVisible	(void) {
	return myFrame	-> isVisible ();
}

