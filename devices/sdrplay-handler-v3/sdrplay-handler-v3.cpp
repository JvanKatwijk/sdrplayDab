#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of Qt-DAB
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QThread>
#include	<QSettings>
#include	<QHBoxLayout>
#include	<QLabel>
#include	<QFileDialog>
#include	"sdrplay-handler-v3.h"
#include	"sdrplay-controller.h"
#include	"dab-processor.h"
#include	"radio.h"

	sdrplayHandler_v3::sdrplayHandler_v3  (RadioInterface *mr,
	                                       QSettings *s,
	                                       dabProcessor *base) {
	sdrplaySettings			= s;
	this	-> base			= base;
	myFrame				= new QFrame (nullptr);
	setupUi (this -> myFrame);
	this	-> myFrame	-> show	();
	antennaSelector		-> hide	();
	tunerSelector		-> hide	();
	nrBits			= 12;	// default

	sdrplaySettings		-> beginGroup ("sdrplaySettings");
	lnaState			=
	            sdrplaySettings -> value ("sdrplay-lnastate", 4). toInt();
	lnaGainSetting	-> setValue (lnaState);
	ppmValue		=
	            sdrplaySettings -> value ("sdrplay-ppm", 0). toInt();
	ppmControl	-> setValue (ppmValue);

	agcMode		=
	       sdrplaySettings -> value ("sdrplay-agcMode", 1). toInt() != 0;
	if (agcMode) {
	   agcControl -> setChecked (true);
	}
	sdrplaySettings	-> endGroup	();

//	and be prepared for future changes in the settings
	connect (lnaGainSetting, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lnagainReduction (int)));
	connect (agcControl, SIGNAL (stateChanged (int)),
	         this, SLOT (set_agcControl (int)));
	connect (ppmControl, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmControl (int)));
	connect (antennaSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (set_antennaSelect (const QString &)));
	connect (tunerSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (set_tunerSelect (const QString &)));
	connect (gain_setpoint, SIGNAL (valueChanged (int)),
	         this, SLOT (set_gain (int)));
	theController = new sdrplayController (this, base,
	                                       lnaState,
	                                       ppmValue,
	                                       gain_setpoint -> value (),
	                                       agcMode);
	while (!theController	-> report ())
	   usleep (1000);
	if (!theController	-> isOK ())
	   throw (23);
//
//	OK, the controller runs, let us extract the
//	data to show on the gui

	int high 		= theController -> get_lnaRange ();
	fprintf (stderr, "lnaRange to %d\n", high);
	lnaGainSetting		-> setRange (0, high);
	QString deviceName	= theController -> get_deviceLabel ();
	deviceLabel		-> setText (deviceName);
	nrBits			= theController -> get_nrBits ();
	float	apiVersion	= theController -> get_apiVersion ();
	api_version		-> display (apiVersion);
	QString	Serial		= theController	-> get_serialNumber ();
	serialNumber		-> setText (Serial);
	lnaValueDisplay		-> display (theController -> get_lnaValue (lnaState));
	if (deviceName == "RSP-II")
	   antennaSelector	-> show ();

	debugControl	-> hide ();
	usleep (1000);
}

	sdrplayHandler_v3::~sdrplayHandler_v3 () {
	delete theController;
//
//	thread should be stopped by now
	sdrplaySettings	-> beginGroup ("sdrplaySettings");
	sdrplaySettings -> setValue ("sdrplay-ppm",
	                                           ppmControl -> value ());
	sdrplaySettings -> setValue ("sdrplay-lnastate",
	                                           lnaGainSetting -> value ());
	sdrplaySettings	-> setValue ("sdrplay-agcMode",
	                                  agcControl -> isChecked() ? 1 : 0);
	sdrplaySettings	-> endGroup ();
	sdrplaySettings	-> sync();

	myFrame	-> hide ();
	delete	myFrame;
}

void	sdrplayHandler_v3::avgValue	(float v) {
	averageValue	-> display (v);
}

void	sdrplayHandler_v3::dipValue	(float v) {
	nullValue	-> display (v);
}

void	sdrplayHandler_v3::freq_offset	(int f) {
	freq_offsetDisplay	-> display (f);
}

void	sdrplayHandler_v3::freq_error	(int f) {
	freq_errorDisplay	-> display (f);
}

void	sdrplayHandler_v3::show_TotalGain (float f) {
	reportedGain	-> display (f);
}


int32_t	sdrplayHandler_v3::getVFOFrequency() {
	return theController	-> getVFOFrequency ();
}

void	sdrplayHandler_v3::set_lnagainReduction (int lnaState) {
	fprintf (stderr, "lna state to %d\n", lnaState);
	theController	-> set_LNA (lnaState);
}

void	sdrplayHandler_v3::set_GRdB (int gainValue) {
	theController	-> set_GRdB (gainValue);
}

void	sdrplayHandler_v3::set_agcControl (int dummy) {
bool agcMode	= agcControl -> isChecked();

	(void)dummy;
	theController	-> set_agc (agcMode, 30);
}

void	sdrplayHandler_v3::set_ppmControl (int ppm) {
	theController	-> set_PPM (ppm);
}

void	sdrplayHandler_v3::set_antennaSelect	(const QString &s) {
        if (s == "Antenna A")
	   theController	-> set_antenna ('A');
        else
	   theController	-> set_antenna ('B');
}

void	sdrplayHandler_v3::set_tunerSelect	(const QString &s) {
	fprintf (stderr, "tuner select not implemented\n");
}

bool	sdrplayHandler_v3::restartReader	(int32_t freq) {
	if (!theController -> is_threadRunning () ||
	    theController -> is_receiverRunning ())
	   return true;

	return theController	-> restartReader (freq);
}

void	sdrplayHandler_v3::stopReader	() {
	if (!theController -> is_threadRunning () ||
	    !theController -> is_receiverRunning ())
	   return;
	theController	-> stopReader ();
}

void	sdrplayHandler_v3::set_gain	(int gain) {
	theController	-> set_gain (gain);
}

void	sdrplayHandler_v3::resetBuffer	() {
}

int16_t	sdrplayHandler_v3::bitDepth	() {
	return nrBits;
}

void	sdrplayHandler_v3::show		() {
	this	-> myFrame	-> show ();
}

void	sdrplayHandler_v3::hide		() {
	this	-> myFrame	-> hide ();
}

bool	sdrplayHandler_v3::isHidden	() {
	return !myFrame -> isVisible ();
}

void	sdrplayHandler_v3::deviceReady (bool b) {}


