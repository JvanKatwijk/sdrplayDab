#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of sdrplayDab program
 *    srplayDab is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    srplayDab is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sdrplayDab if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QObject>
#include	<QSettings>
#include	<QHBoxLayout>
#include	<QLabel>
#include	<complex>
#include	"sdrplay-handler.h"
#include	"sdrplayselect.h"
#include	"dab-processor.h"
#include	"radio.h"

static
int     RSP1_Table [] = {0, 24, 19, 43};

static
int     RSP1A_Table [] = {0, 6, 12, 18, 20, 26, 32, 38, 57, 62};

static
int     RSP2_Table [] = {0, 10, 15, 21, 24, 34, 39, 45, 64};

//static
//int	RSPduo_Table [] = {0, 6, 12, 18, 20, 26, 32, 38, 57, 62};

static
int	get_lnaGRdB (int hwVersion, int lnaState) {
	switch (hwVersion) {
	   case 1:
	      return RSP1_Table [lnaState];

	   case 2:
	      return RSP2_Table [lnaState];

	   default:
	      return RSP1A_Table [lnaState];
	}
}
//
//	here we start
	sdrplayHandler::sdrplayHandler  (RadioInterface *mr,
	                                 QSettings *s) {
mir_sdr_ErrT	err;
float	ver;
mir_sdr_DeviceT devDesc [4];
mir_sdr_GainValuesT gainDesc;
sdrplaySelect	*sdrplaySelector;

	sdrplaySettings			= s;
	this	-> myFrame		= new QFrame (NULL);
	setupUi (this -> myFrame);
	this	-> myFrame	-> show ();
	antennaSelector		-> hide ();
	tunerSelector		-> hide ();
	this	-> inputRate		= Khz (2048);
	_I_Buffer			= NULL;
	libraryLoaded			= false;

#ifdef __MINGW32__
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

	if (RegOpenKey (HKEY_LOCAL_MACHINE,
	                TEXT("Software\\MiricsSDR\\API"),
	                &APIkey) != ERROR_SUCCESS) {
	  fprintf (stderr,
	           "failed to locate API registry entry, error = %d\n",
	           (int)GetLastError());
	   delete myFrame;
	   throw (21);
	}

	RegQueryValueEx (APIkey,
	                 (wchar_t *)L"Install_Dir",
	                 NULL,
	                 NULL,
	                 (LPBYTE)&APIkeyValue,
	                 (LPDWORD)&APIkeyValue_length);
//	Ok, make explicit it is in the 32 bits section
	wchar_t *x = wcscat (APIkeyValue, (wchar_t *)L"\\x86\\mir_sdr_api.dll");
//	wchar_t *x = wcscat (APIkeyValue, (wchar_t *)L"\\x64\\mir_sdr_api.dll");
//	fprintf (stderr, "Length of APIkeyValue = %d\n", APIkeyValue_length);
//	wprintf (L"API registry entry: %s\n", APIkeyValue);
	RegCloseKey(APIkey);

	Handle	= LoadLibrary (x);
	if (Handle == NULL) {
	  fprintf (stderr, "Failed to open mir_sdr_api.dll\n");
	  delete myFrame;
	  throw (22);
	}
#else
	Handle		= dlopen ("libusb-1.0.so", RTLD_NOW | RTLD_GLOBAL);
	Handle		= dlopen ("libmirsdrapi-rsp.so", RTLD_NOW);
	if (Handle == NULL)
	   Handle	= dlopen ("libmir_sdr.so", RTLD_NOW);

	if (Handle == NULL) {
	   fprintf (stderr, "error report %s\n", dlerror ());
	   delete myFrame;
	   throw (23);
	}
#endif
	libraryLoaded	= true;

	bool success = loadFunctions ();
	if (!success) {
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (23);
	}

	err		= my_mir_sdr_ApiVersion (&ver);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error at ApiVersion %s\n",
	                 errorCodes (err). toLatin1 (). data ());
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (24);
	}
	
	if (ver < 2.13) {
	   fprintf (stderr, "sorry, library too old\n");
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (24);
	}

	_I_Buffer	= new RingBuffer<std::complex<float>>(1024 * 1024);
	vfoFrequency	= Khz (220000);		// default
	totalOffset	= 0;

//	See if there are settings from previous incarnations
//	and config stuff

	sdrplaySettings		-> beginGroup ("sdrplaySettings");
	lnaGainSetting		-> setValue (
	            sdrplaySettings -> value ("sdrplay-lnastate", 0). toInt ());
	lnaState	= lnaGainSetting	-> value ();

	ppmControl		-> setValue (
	            sdrplaySettings -> value ("sdrplay-ppm", 0). toInt ());
	gain_setpoint		-> setValue (
	            sdrplaySettings -> value ("gain_setpoint", -35). toInt ());
	bool	debugFlag	=
	            sdrplaySettings -> value ("sdrplay-debug", 0). toInt ();
	if (!debugFlag)
	   debugControl -> hide ();
	sdrplaySettings	-> endGroup ();

	err = my_mir_sdr_GetDevices (devDesc, &numofDevs, uint32_t (4));
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error at GetDevices %s \n",
	                   errorCodes (err). toLatin1 (). data ());

#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (25);
	}

	if (numofDevs == 0) {
	   fprintf (stderr, "Sorry, no device found\n");
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (25);
	}

	if (numofDevs > 1) {
	   sdrplaySelector       = new sdrplaySelect ();
	   for (deviceIndex = 0; deviceIndex < numofDevs; deviceIndex ++) {
#ifndef	__MINGW32__
	      sdrplaySelector ->
	           addtoList (devDesc [deviceIndex]. DevNm);
#else
	      sdrplaySelector ->
	           addtoList (devDesc [deviceIndex]. SerNo);
#endif
	   }
	   deviceIndex = sdrplaySelector -> QDialog::exec ();
	   delete sdrplaySelector;
	}
	else
	   deviceIndex = 0;

	hwVersion = devDesc [deviceIndex]. hwVer;
	fprintf (stderr, "hwVer = %d\n", hwVersion);
	fprintf (stderr, "devicename = %s\n", devDesc [deviceIndex]. DevNm);

	err = my_mir_sdr_SetDeviceIdx (deviceIndex);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error at SetDeviceIdx %s \n",
	                   errorCodes (err). toLatin1 (). data ());

#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (25);
	}

	serialNumber	-> setText (devDesc [deviceIndex]. SerNo);
	api_version	-> display (ver);
//	we know we are only in the frequency range 175 .. 230 Mhz,
//	so we can rely on a single table for the lna reductions.
	switch (hwVersion) {
	   case 1:		// old RSP
	      lnaGainSetting	-> setRange (0, 3);
	      deviceLabel	-> setText ("RSP-I");
	      nrBits		= 12;
	      denominator	= 4096;
	      break;
	   case 2:
	      lnaGainSetting	-> setRange (0, 8);
	      deviceLabel	-> setText ("RSP-II");
	      nrBits		= 12;
	      denominator	= 4096;
	      antennaSelector -> show ();
	      err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	      if (err != mir_sdr_Success) 
	         fprintf (stderr, "error %d in setting antenna\n", err);
	      connect (antennaSelector, SIGNAL (activated (const QString &)),
	               this, SLOT (set_antennaSelect (const QString &)));
	      break;
	   case 3:	
	      lnaGainSetting	-> setRange (0, 9);
	      deviceLabel	-> setText ("RSP-DUO");
	      nrBits		= 14;
	      denominator	= 8192;
	      tunerSelector	-> show ();
	      err	= my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_1);
	      if (err != mir_sdr_Success) 
	         fprintf (stderr, "error %d in setting of rspDuo\n", err);
	      connect (tunerSelector, SIGNAL (activated (const QString &)),
	               this, SLOT (set_tunerSelect (const QString &)));
	      break;
	   default:
	      lnaGainSetting	-> setRange (0, 9);
	      deviceLabel	-> setText ("RSP-1A");
	      nrBits		= 14;
	      denominator	= 16384;
	      break;
	}

//	and be prepared for future changes in the settings
	connect (lnaGainSetting, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lnagainReduction (int)));
	connect (debugControl, SIGNAL (stateChanged (int)),
	         this, SLOT (set_debugControl (int)));
	connect (ppmControl, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmControl (int)));

	lnaGRdBDisplay		-> display (get_lnaGRdB (hwVersion,
	                                         lnaGainSetting -> value ()));
	running. store (false);
}

	sdrplayHandler::~sdrplayHandler	(void) {
	if (!libraryLoaded)
	   return;
	fprintf (stderr, "going to delete\n");
	stopReader ();
	sdrplaySettings	-> beginGroup ("sdrplaySettings");
	sdrplaySettings -> setValue ("sdrplay-ppm", ppmControl -> value ());
	sdrplaySettings -> setValue ("gain_setpoint",
	                                    gain_setpoint -> value ());
	sdrplaySettings -> setValue ("sdrplay-lnastate",
	                                    lnaGainSetting -> value ());
	sdrplaySettings	-> endGroup ();
	sdrplaySettings	-> sync ();
	delete	myFrame;

	if (numofDevs > 0)
	   my_mir_sdr_ReleaseDeviceIdx (deviceIndex);
	if (_I_Buffer != NULL)
	   delete _I_Buffer;
#ifdef __MINGW32__
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
}

void	sdrplayHandler::setOffset		(int32_t offset) {
int	newFrequency	= vfoFrequency + offset;
mir_sdr_ErrT err;

	if (offset != 0) {
	   totalOffset	+= offset;
	   err = my_mir_sdr_SetRf (double (newFrequency), 1, 0);
	   if (err != mir_sdr_Success)
	     fprintf (stderr, "error in update frequency with %d %s\n",
	                     offset, errorCodes (err). toLatin1 (). data ());
	   else
	      vfoFrequency	= newFrequency;
	}
	freq_offsetDisplay	-> display (totalOffset);
	freq_errorDisplay	-> display (offset);
}

void	sdrplayHandler::set_lnagainReduction (int lnaState) {
mir_sdr_ErrT err;

	this	-> lnaState	= lnaState;
	err			= my_mir_sdr_RSP_SetGr (30, lnaState, 0 , 0);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error in lna state (%d) %s\n",
	                              lnaState,
	                              errorCodes (err). toLatin1 (). data ());
	else
	   lnaGRdBDisplay	-> display (get_lnaGRdB (hwVersion, lnaState));
	
}

static
void myStreamCallback (int16_t		*xi,
	               int16_t		*xq,
	               uint32_t		firstSampleNum, 
	               int32_t		grChanged,
	               int32_t		rfChanged,
	               int32_t		fsChanged,
	               uint32_t		numSamples,
	               uint32_t		reset,
	               uint32_t		hwRemoved,
	               void		*cbContext) {
int16_t	i;
sdrplayHandler	*p	= static_cast<sdrplayHandler *> (cbContext);
float	denominator	= (float)(p -> denominator);
static	int teller	= 0;

	if (reset || hwRemoved)
	   return;

	for (i = 0; i < (int)numSamples; i ++) {
	   std::complex<float> symb = std::complex<float> (
	                                       (float) (xi [i]) / denominator,
	                                       (float) (xq [i]) / denominator);
	   int res = p -> base -> addSymbol (symb);
	   if (res == GO_ON)
	      continue;
	   if (res == DEVICE_UPDATE) {
	      mir_sdr_ErrT err;
	      mir_sdr_GainValuesT gains;

	      int	offset;
	      float	lowVal;
	      float	highVal;
	      p -> base -> update_data (&offset, &lowVal, &highVal);
	      if (++ teller > 5) {
	         p -> setOffset (offset);
	         err = p -> my_mir_sdr_GetCurrentGain (&gains);
	         if (err != mir_sdr_Success)
	            fprintf (stderr, "error getting gain values %s\n",
	                               p -> errorCodes (err). toLatin1 (). data ());
	         float str = 10 * log10 ((highVal + 0.005)  / denominator);
	         float lvv = 10 * log10 ((lowVal  + 0.005)  / denominator);
	         int GRdB = str - (p -> gain_setpoint -> value ());
	         if (GRdB != 0) {
	            int ifGain = gains. curr - get_lnaGRdB (p -> hwVersion,
	                                                    p -> lnaState);
	            if (GRdB < -20) 	
	               GRdB = -20;
	            if (GRdB > 20)
	               GRdB = 20;
	            if ((ifGain + GRdB >= 20) && (ifGain + GRdB <= 59)) {
	               err = p -> my_mir_sdr_RSP_SetGr (GRdB,
	                                                p -> lnaState, 0 , 0);
	               if (err != mir_sdr_Success)
	                  fprintf (stderr,
	                           "error updating GainReduction (%d), curr = %f) %s\n",
	                              GRdB, gains. curr,
	                              p -> errorCodes (err). toLatin1 (). data ());
	            }
	         }
	         p -> averageValue -> display (str);
	         p -> nullValue -> display (lvv);
	         p -> reportedGain -> display (gains. curr);
	         teller	= 0;
	      }
	   }
	   else
	   if (res == INITIAL_STRENGTH) {
	      mir_sdr_ErrT err;
	      mir_sdr_GainValuesT gains;
	      err = p -> my_mir_sdr_GetCurrentGain (&gains);
	      if (err != mir_sdr_Success)
	         fprintf (stderr, "error getting gain values %s\n",
	                           p -> errorCodes (err). toLatin1 (). data ());
	      float str	= 10 * log10 ((p -> base -> initialSignal () + 0.005) / denominator);
	      int GRdB = str - (p -> gain_setpoint -> value ()); 
	      fprintf (stderr, "initial strength = %f GRdB %d, observed reduction %f\n",
	                                           str, GRdB,  gains. curr);

	      GRdB += gains. curr -  get_lnaGRdB (p -> hwVersion,
	                                          p -> lnaState);

	      if ((GRdB >= 20) && (GRdB <= 59)) {
	         err = p -> my_mir_sdr_RSP_SetGr (GRdB, p -> lnaState, 1 , 0);
	         if (err != mir_sdr_Success) 
	            fprintf (stderr, "error updating GainReduction (%d) %s\n",
	                                      GRdB,
	                               p -> errorCodes (err). toLatin1 (). data ());
	      }
	   }
	}
	(void)	firstSampleNum;
	(void)	grChanged;
	(void)	rfChanged;
	(void)	fsChanged;
}

void	myGainChangeCallback (uint32_t	GRdB,
	                      uint32_t	lnaGRdB,
	                      void	*cbContext) {
sdrplayHandler	*p	= static_cast<sdrplayHandler *> (cbContext);
	p -> GRdBDisplay	-> display ((int)GRdB);
	(void)lnaGRdB;
//	p -> lnaGRdBDisplay	-> display ((int)lnaGRdB);
}

bool	sdrplayHandler::restartReader	(int32_t frequency) {
int	gRdBSystem;
int	samplesPerPacket;
mir_sdr_ErrT	err;
int	GRdB		= 30;

	if (running. load ())
	   return true;

	vfoFrequency	= frequency;
	totalOffset	= 0;
//	fprintf (stderr, "restart op freq %d\n", frequency);
	err	= my_mir_sdr_StreamInit (&GRdB,
	                                 double (inputRate) / MHz (1),
	                                 double (frequency) / Mhz (1),
	                                 mir_sdr_BW_1_536,
	                                 mir_sdr_IF_Zero,
	                                 lnaState,
	                                 &gRdBSystem,
	                                 mir_sdr_USE_RSP_SET_GR,
	                                 &samplesPerPacket,
	                                 (mir_sdr_StreamCallback_t)myStreamCallback,
	                                 (mir_sdr_GainChangeCallback_t)myGainChangeCallback,
	                                 this);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	   return false;
	}
	err	= my_mir_sdr_SetPpm (double (ppmControl -> value ()));
	if (err != mir_sdr_Success) 
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());

	err		= my_mir_sdr_SetDcMode (4, 1);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	err		= my_mir_sdr_SetDcTrackTime (63);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	running. store (true);
	return true;
}

void	sdrplayHandler::stopReader	(void) {
mir_sdr_ErrT err;

	if (!running. load ())
	   return;

	err	= my_mir_sdr_StreamUninit	();
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	running. store (false);
}

void	sdrplayHandler::resetBuffer	(void) {
	_I_Buffer	-> FlushRingBuffer ();
}

int16_t	sdrplayHandler::bitDepth	(void) {
	return nrBits;
}

bool	sdrplayHandler::loadFunctions	(void) {
	my_mir_sdr_StreamInit	= (pfn_mir_sdr_StreamInit)
	                    GETPROCADDRESS (this -> Handle,
	                                    "mir_sdr_StreamInit");
	if (my_mir_sdr_StreamInit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_StreamInit\n");
	   return false;
	}

	my_mir_sdr_StreamUninit	= (pfn_mir_sdr_StreamUninit)
	                    GETPROCADDRESS (this -> Handle,
	                                    "mir_sdr_StreamUninit");
	if (my_mir_sdr_StreamUninit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_StreamUninit\n");
	   return false;
	}

	my_mir_sdr_SetRf	= (pfn_mir_sdr_SetRf)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetRf");
	if (my_mir_sdr_SetRf == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetRf\n");
	   return false;
	}

	my_mir_sdr_SetFs	= (pfn_mir_sdr_SetFs)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetFs");
	if (my_mir_sdr_SetFs == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetFs\n");
	   return false;
	}

	my_mir_sdr_SetGr	= (pfn_mir_sdr_SetGr)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetGr");
	if (my_mir_sdr_SetGr == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetGr\n");
	   return false;
	}

	my_mir_sdr_RSP_SetGr	= (pfn_mir_sdr_RSP_SetGr)
	                    GETPROCADDRESS (Handle, "mir_sdr_RSP_SetGr");
	if (my_mir_sdr_RSP_SetGr == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_RSP_SetGr\n");
	   return false;
	}

	my_mir_sdr_SetGrParams	= (pfn_mir_sdr_SetGrParams)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetGrParams");
	if (my_mir_sdr_SetGrParams == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetGrParams\n");
	   return false;
	}

	my_mir_sdr_SetDcMode	= (pfn_mir_sdr_SetDcMode)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetDcMode");
	if (my_mir_sdr_SetDcMode == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDcMode\n");
	   return false;
	}

	my_mir_sdr_SetDcTrackTime	= (pfn_mir_sdr_SetDcTrackTime)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetDcTrackTime");
	if (my_mir_sdr_SetDcTrackTime == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDcTrackTime\n");
	   return false;
	}

	my_mir_sdr_SetSyncUpdateSampleNum = (pfn_mir_sdr_SetSyncUpdateSampleNum)
	               GETPROCADDRESS (Handle, "mir_sdr_SetSyncUpdateSampleNum");
	if (my_mir_sdr_SetSyncUpdateSampleNum == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetSyncUpdateSampleNum\n");
	   return false;
	}

	my_mir_sdr_SetSyncUpdatePeriod	= (pfn_mir_sdr_SetSyncUpdatePeriod)
	                GETPROCADDRESS (Handle, "mir_sdr_SetSyncUpdatePeriod");
	if (my_mir_sdr_SetSyncUpdatePeriod == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetSyncUpdatePeriod\n");
	   return false;
	}

	my_mir_sdr_ApiVersion	= (pfn_mir_sdr_ApiVersion)
	                GETPROCADDRESS (Handle, "mir_sdr_ApiVersion");
	if (my_mir_sdr_ApiVersion == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ApiVersion\n");
	   return false;
	}

	my_mir_sdr_AgcControl	= (pfn_mir_sdr_AgcControl)
	                GETPROCADDRESS (Handle, "mir_sdr_AgcControl");
	if (my_mir_sdr_AgcControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_AgcControl\n");
	   return false;
	}

	my_mir_sdr_Reinit	= (pfn_mir_sdr_Reinit)
	                GETPROCADDRESS (Handle, "mir_sdr_Reinit");
	if (my_mir_sdr_Reinit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_Reinit\n");
	   return false;
	}

	my_mir_sdr_SetPpm	= (pfn_mir_sdr_SetPpm)
	                GETPROCADDRESS (Handle, "mir_sdr_SetPpm");
	if (my_mir_sdr_SetPpm == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetPpm\n");
	   return false;
	}

	my_mir_sdr_DebugEnable	= (pfn_mir_sdr_DebugEnable)
	                GETPROCADDRESS (Handle, "mir_sdr_DebugEnable");
	if (my_mir_sdr_DebugEnable == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_DebugEnable\n");
	   return false;
	}

	my_mir_sdr_rspDuo_TunerSel = (pfn_mir_sdr_rspDuo_TunerSel)
	               GETPROCADDRESS (Handle, "mir_sdr_rspDuo_TunerSel");
	if (my_mir_sdr_rspDuo_TunerSel == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_rspDuo_TunerSel\n");
	   return false;
	}

	my_mir_sdr_DCoffsetIQimbalanceControl	=
	                     (pfn_mir_sdr_DCoffsetIQimbalanceControl)
	                GETPROCADDRESS (Handle, "mir_sdr_DCoffsetIQimbalanceControl");
	if (my_mir_sdr_DCoffsetIQimbalanceControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_DCoffsetIQimbalanceControl\n");
	   return false;
	}


	my_mir_sdr_ResetUpdateFlags	= (pfn_mir_sdr_ResetUpdateFlags)
	                GETPROCADDRESS (Handle, "mir_sdr_ResetUpdateFlags");
	if (my_mir_sdr_ResetUpdateFlags == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ResetUpdateFlags\n");
	   return false;
	}

	my_mir_sdr_GetDevices		= (pfn_mir_sdr_GetDevices)
	                GETPROCADDRESS (Handle, "mir_sdr_GetDevices");
	if (my_mir_sdr_GetDevices == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetDevices");
	   return false;
	}

	my_mir_sdr_GetCurrentGain	= (pfn_mir_sdr_GetCurrentGain)
	                GETPROCADDRESS (Handle, "mir_sdr_GetCurrentGain");
	if (my_mir_sdr_GetCurrentGain == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetCurrentGain");
	   return false;
	}

	my_mir_sdr_GetHwVersion	= (pfn_mir_sdr_GetHwVersion)
	                GETPROCADDRESS (Handle, "mir_sdr_GetHwVersion");
	if (my_mir_sdr_GetHwVersion == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetHwVersion");
	   return false;
	}

	my_mir_sdr_RSPII_AntennaControl	= (pfn_mir_sdr_RSPII_AntennaControl)
	                GETPROCADDRESS (Handle, "mir_sdr_RSPII_AntennaControl");
	if (my_mir_sdr_RSPII_AntennaControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_RSPII_AntennaControl");
	   return false;
	}

	my_mir_sdr_SetDeviceIdx	= (pfn_mir_sdr_SetDeviceIdx)
	                GETPROCADDRESS (Handle, "mir_sdr_SetDeviceIdx");
	if (my_mir_sdr_SetDeviceIdx == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDeviceIdx");
	   return false;
	}

	my_mir_sdr_ReleaseDeviceIdx	= (pfn_mir_sdr_ReleaseDeviceIdx)
	                GETPROCADDRESS (Handle, "mir_sdr_ReleaseDeviceIdx");
	if (my_mir_sdr_ReleaseDeviceIdx == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ReleaseDeviceIdx");
	   return false;
	}

	return true;
}

void	sdrplayHandler::set_debugControl (int debugMode) {
	(void)debugMode;
	my_mir_sdr_DebugEnable (debugControl -> isChecked () ? 1 : 0);
}

void	sdrplayHandler::set_ppmControl (int ppm) {
	if (running. load ()) {
	   my_mir_sdr_SetPpm	((float)ppm);
	   my_mir_sdr_SetRf	((float)vfoFrequency, 1, 0);
	}
}

void	sdrplayHandler::set_antennaSelect (const QString &s) {
mir_sdr_ErrT err;

	if (hwVersion != 2)	// should not happen
	   return;

	if (s == "Antenna A")
	   err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	else
	   err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_B);

	if (err != mir_sdr_Success) 
	   fprintf (stderr, "error in set antenna (%s) %s\n",
	                     s. toLatin1 (). data (),
	                     errorCodes (err). toLatin1 (). data ());
}

void	sdrplayHandler::set_tunerSelect (const QString &s) {
mir_sdr_ErrT err;

	if (hwVersion != 3)	// should not happen
	   return;
	if (s == "Tuner 1") 
	   err	= my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_1);
	else
	   err	= my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_2);

	if (err != mir_sdr_Success) 
	   fprintf (stderr, "error %d in selecting  rspDuo\n", err);
}

QString	sdrplayHandler::errorCodes (mir_sdr_ErrT err) {
	switch (err) {
	   case mir_sdr_Success:
	      return "success";
	   case mir_sdr_Fail:
	      return "Fail";
	   case mir_sdr_InvalidParam:
	      return "invalidParam";
	   case mir_sdr_OutOfRange:
	      return "OutOfRange";
	   case mir_sdr_GainUpdateError:
	      return "GainUpdateError";
	   case mir_sdr_RfUpdateError:
	      return "RfUpdateError";
	   case mir_sdr_FsUpdateError:
	      return "FsUpdateError";
	   case mir_sdr_HwError:
	      return "HwError";
	   case mir_sdr_AliasingError:
	      return "AliasingError";
	   case mir_sdr_AlreadyInitialised:
	      return "AlreadyInitialised";
	   case mir_sdr_NotInitialised:
	      return "NotInitialised";
	   case mir_sdr_NotEnabled:
	      return "NotEnabled";
	   case mir_sdr_HwVerError:
	      return "HwVerError";
	   case mir_sdr_OutOfMemError:
	      return "OutOfMemError";
	   case mir_sdr_HwRemoved:
	      return "HwRemoved";
	   default:
	      return "???";
	}
}

void	sdrplayHandler::setEnv	(dabProcessor *p) {
	base	= p;
}

