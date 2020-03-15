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
#include	"control-queue.h"
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
	theQueue		= new controlQueue ();
	nrBits			= 12;	// default

	theQueue	-> add (STOP_REQUEST);
	sdrplaySettings		-> beginGroup ("sdrplaySettings");
	lnaGainSetting		-> setValue (
	            sdrplaySettings -> value ("sdrplay-lnastate", 4). toInt());
	theQueue -> add (LNA_REQUEST, lnaGainSetting -> value ());

	ppmControl		-> setValue (
	            sdrplaySettings -> value ("sdrplay-ppm", 0). toInt());
	theQueue -> add (PPM_REQUEST, ppmControl -> value ());

	agcMode		=
	       sdrplaySettings -> value ("sdrplay-agcMode", 1). toInt() != 0;
	if (agcMode) 
	   theQueue -> add (AGC_REQUEST, true, 30);
	else
	   theQueue -> add (AGC_REQUEST, false);
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
	theController = new sdrplayController (this, theQueue, base);
	denominator	= 2048;
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
	delete	theQueue;
}

void	sdrplayHandler_v3::freq_offset	(int freqOffset) {
	freq_offsetDisplay	-> display (freqOffset);
}

void	sdrplayHandler_v3::freq_error	(int freqError) {
	freq_errorDisplay	-> display (freqError);
}

void	sdrplayHandler_v3::avgValue	(float v) {
	averageValue	-> display (v);
}

void	sdrplayHandler_v3::dipValue	(float v) {
	nullValue	-> display (v);
}
void	sdrplayHandler_v3::set_lnaRange	(int low, int high) {
	lnaGainSetting	-> setRange (low, high);
}

void	sdrplayHandler_v3::set_deviceLabel (const QString &s, int nrBits) {
	this	-> nrBits = nrBits;
	deviceLabel	-> setText (s);
}

void	sdrplayHandler_v3::setDeviceData (const QString &s, int hw, float api) {
	api_version	-> display (api);
	serialNumber	-> setText (s);
	(void)hw;
}

void	sdrplayHandler_v3::show_TotalGain	(int g) {
	reportedGain	-> display (g);
}

int32_t	sdrplayHandler_v3::getVFOFrequency() {
	return vfoFrequency;
}

void	sdrplayHandler_v3::set_lnagainReduction (int lnaState) {
	theQueue	-> add (LNA_REQUEST, lnaState);
}

void	sdrplayHandler_v3::set_gain (int gainValue) {
	theQueue -> add (GAIN_SETPOINT, gainValue);
}

void	sdrplayHandler_v3::set_agcControl (int dummy) {
bool agcMode	= agcControl -> isChecked();

	(void)dummy;
	if (agcMode) 
	   theQueue	-> add (AGC_REQUEST, true, 30);
	else
	   theQueue	-> add (AGC_REQUEST, false);
}

void	sdrplayHandler_v3::set_ppmControl (int ppm) {
	theQueue	-> add (PPM_REQUEST, ppm);
}

void	sdrplayHandler_v3::set_antennaSelect	(const QString &s) {
        if (s == "Antenna A")
	   theQueue	-> add (ANTENNASELECT_REQUEST, 'A');
        else
	   theQueue	-> add (ANTENNASELECT_REQUEST, 'B');
}

void	sdrplayHandler_v3::set_tunerSelect	(const QString &s) {
	fprintf (stderr, "tuner select not implemented\n");
}

bool	sdrplayHandler_v3::restartReader	(int32_t freq) {
	if (!theController -> is_threadRunning () ||
	    theController -> is_receiverRunning ())
	   return true;

	theQueue	-> add (RESTART_REQUEST, freq);
	vfoFrequency	= freq;
	return true;
}

void	sdrplayHandler_v3::stopReader	() {

	if (!theController -> is_threadRunning () ||
	    !theController -> is_receiverRunning ())
	   return;
	theQueue	-> add (STOP_REQUEST);
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

