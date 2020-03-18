#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of sdrplayDab
 *
 *    sdrplayDab is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    sdrplayDab is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sdrplayDab if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QThread>
#include	<QSettings>
#include	"sdrplay-controller.h"
#include	"sdrplay-handler-v3.h"
#include	"dab-processor.h"

#include	"sdrplay-commands.h"
static
int     RSP1_Table []	= {0, 24, 19, 43};

static
int     RSP1A_Table []	= {0, 6, 12, 18, 20, 26, 32, 38, 57, 62};

static
int     RSP2_Table []	= {0, 10, 15, 21, 24, 34, 39, 45, 64};

static
int	RSPduo_Table []	= {0, 6, 12, 18, 20, 26, 32, 38, 57, 62};

static
int	get_lnaGRdB (int hwVersion, int lnaState) {
	switch (hwVersion) {
	   case 1:
	   default:
	      return RSP1_Table [lnaState];

	   case 2:
	      return RSP2_Table [lnaState];

	   case 255: 
	      return RSP1A_Table [lnaState];

	   case 3:
	      return RSPduo_Table [lnaState];
	}
}

	sdrplayController::sdrplayController	(sdrplayHandler_v3 *parent,
	                                         dabProcessor	*base,
	                                         int	lnaState,
	                                         int	ppmValue,
	                                         int	gainSetPoint,
	                                         bool	agcMode):
	                                            serverjobs (0){

	this	-> parent	= parent;
	this	-> base		= base;

	this	-> lnaState	= lnaState;
	this	-> ppmValue	= ppmValue;
	this	-> gainSetPoint	= gainSetPoint;
	this	-> agcMode	= agcMode;

	connect (this, SIGNAL (deviceReady (bool)),
	         parent, SLOT (deviceReady (bool)));
	connect	(this, SIGNAL (avgValue (float)),
	         parent, SLOT (avgValue (float)));
	connect (this, SIGNAL (dipValue (float)),
	         parent, SLOT (dipValue (float)));
	connect (this, SIGNAL (freq_offset (int)),
	         parent, SLOT (freq_offset (int)));
	connect (this, SIGNAL (freq_error (int)),
	         parent, SLOT (freq_error (int)));
	connect (this, SIGNAL (show_TotalGain (float)),
	         parent, SLOT (show_TotalGain (float)));
	threadRuns. store (false);
	reportIndicator	= false;
	theGain		= -1;
	start ();
}

	sdrplayController::~sdrplayController () {
	threadRuns. store (false);
	while (this -> isRunning ())
	   msleep (10);
}
//
//	The access functions

bool	sdrplayController::restartReader (int newFreq) {
restartRequest r (newFreq);
	if (receiverRuns. load ())
	   return true;
	vfoFrequency	= newFreq;
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return r. result;
}

void	sdrplayController::stopReader () {
stopRequest r;
	if (!receiverRuns. load ())
	   return;
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return ;
	return;
}

void	sdrplayController::setVFOFrequency (int newFreq) {
set_frequencyRequest r (newFreq);
	vfoFrequency	= newFreq;
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return;
}

int	sdrplayController::getVFOFrequency () {
//get_frequencyRequest r;
//	server_queue. push (&r);
//	serverjobs. release (1);
//	while (!r. waiter. tryAcquire (1, 1000))
//	   if (!threadRuns. load ())
//	      return -1;
//	return r. frequency;
	return vfoFrequency;
}

bool	sdrplayController::set_agc (bool sw, int setpoint) {
agcRequest r (sw, setpoint);
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return r. result;
}

bool	sdrplayController::set_GRdB	(int GRdB) {
GRdBRequest r (GRdB);
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return r. result;
}

bool	sdrplayController::set_PPM	(int ppm) {
ppmRequest r (ppm);
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return r. result;
}

bool	sdrplayController::set_LNA	(int lnaValue) {
lnaRequest r (lnaValue);
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return r. result;
}

bool	sdrplayController::set_antenna	(int antenna) {
antennaRequest r (antenna);
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return false;
	return r. result;
}

int	sdrplayController::gainValue	() {
gainvalueRequest r;
	server_queue. push (&r);
	serverjobs. release (1);
	while (!r. waiter. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      return -1;
	return r. gainValue;
}


int	totalOffset	= 0;
void	sdrplayController::setOffset (int offset) {
	vfoFrequency		+= offset;
	totalOffset		+= offset;
	emit freq_offset	(totalOffset);
	emit freq_error		(offset);
	setVFOFrequency (vfoFrequency);
}

static inline
int     constrain (int v, int l, int h) {
        if (v < l)
           return l;
        if (v > h)
           return h;
}

void	sdrplayController::setGains (float lowVal, float highVal) {

	float str = 10 * log10 ((highVal + 0.005)  / denominator);
        float lvv = 10 * log10 ((lowVal  + 0.005)  / denominator);

	if (theGain == -1)
	   return;
	emit avgValue (str);
	emit dipValue (lvv);
	show_TotalGain (theGain);

//	we compute the "error" in the gain setting,
//	and we derive the GRdB value needed to correct that
        int gainCorr    = gainSetPoint - str;
        gainCorr        = constrain (gainCorr, -20, 20);

	int lnaState	= chParams	-> tunerParams. gain.LNAstate;
        int GRdB        = theGain - get_lnaGRdB (hwVersion, lnaState);
        GRdB            = constrain (GRdB + gainCorr, 20, 59);
	
	if ((chParams -> ctrlParams.agc.enable == sdrplay_api_AGC_DISABLE) &&
                   (GRdB != 0)) 
	   set_GRdB (GRdB);
}

//
////////////////////////////////////////////////////////////////////
//
static
void    StreamACallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples,
	                 unsigned int reset,
                         void *cbContext) {
sdrplayController *p	= static_cast<sdrplayController *> (cbContext);
float	denominator	= (float)(p -> denominator);
std::complex<int16_t> localBuf [numSamples];
static int teller	= 0;

	(void)params;
	if (reset)
	   return;
	if (!p -> receiverRuns. load ())
	   return;

	for (int i = 0; i <  (int)numSamples; i ++) {
	   std::complex<float> symb = std::complex<float> (
	                                       (float) (xi [i]) / denominator,
	                                       (float) (xq [i]) / denominator);
	   int res = p -> base -> addSymbol (symb);
	   switch (res) {
	      case GO_ON:
	         continue;
	   
	      case DEVICE_UPDATE: {
	         int    offset;
	         float  lowVal, highVal;
                 if (++ teller > 10) {
                    p -> base -> update_data (&offset, &lowVal, &highVal);
                    p -> setOffset (offset);
                    p -> setGains  (lowVal, highVal);
                    teller      = 0;
                 }
              }

	         
	      continue;
	   
	   case INITIAL_STRENGTH: {
	         float str = 10 * log10 ((p -> base -> initialSignal () + 0.005) / denominator);
//	         p -> set_initialGain (str);
	      }
	      continue;
	   }
	}
}

static
void	StreamBCallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples, unsigned int reset,
                         void *cbContext) {
	(void)xi; (void)xq; (void)params; (void)cbContext;
        if (reset)
           printf ("sdrplay_api_StreamBCallback: numSamples=%d\n", numSamples);
        return;
}

static
void	EventCallback (sdrplay_api_EventT eventId,
                       sdrplay_api_TunerSelectT tuner,
                       sdrplay_api_EventParamsT *params,
                       void *cbContext) {
sdrplayController *p	= static_cast<sdrplayController *> (cbContext);
	(void)tuner;
	p -> theGain	= params -> gainParams. currGain;
	switch (eventId) {
	   case sdrplay_api_GainChange:
	      break;

	   case sdrplay_api_PowerOverloadChange:
	      p -> update_PowerOverload (params);
	      break;

	   default:
	      fprintf (stderr, "event %d\n", eventId);
	      break;
	}
}

void	sdrplayController::
	         update_PowerOverload (sdrplay_api_EventParamsT *params) {
	sdrplay_api_Update (chosenDevice -> dev,
	                    chosenDevice -> tuner,
	                    sdrplay_api_Update_Ctrl_OverloadMsgAck,
	                    sdrplay_api_Update_Ext1_None);
	if (params -> powerOverloadParams.powerOverloadChangeType ==
	                                    sdrplay_api_Overload_Detected) {
//	   fprintf (stderr, "Qt-DAB sdrplay_api_Overload_Detected");
	}
	else {
//	   fprintf (stderr, "Qt-DAB sdrplay_api_Overload Corrected");
	}
}

void	sdrplayController::run		() {
sdrplay_api_ErrT        err;
sdrplay_api_DeviceT     devs [6];
uint32_t                ndev;

	chosenDevice	= nullptr;
	deviceParams	= nullptr;

	denominator		= 2048;		// default
	nrBits			= 12;		// default
	threadRuns. store (false);
	receiverRuns. store (false);
	Handle			= fetchLibrary ();
	if (Handle == nullptr)
	   throw (21);
//	load the functions
	bool success	= loadFunctions ();
	if (!success) {
	   releaseLibrary ();
           throw (23);
        }
	fprintf (stderr, "functions loaded\n");

//	try to open the API
	err	= sdrplay_api_Open ();
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_Open failed %s\n",
	                          sdrplay_api_GetErrorString (err));
	   releaseLibrary ();
	   throw (24);
	}


//	Check API versions match
        err = sdrplay_api_ApiVersion (&apiVersion);
        if (err  != sdrplay_api_Success) {
           fprintf (stderr, "sdrplay_api_ApiVersion failed %s\n",
                                     sdrplay_api_GetErrorString (err));
	   goto closeAPI;
        }

        if (apiVersion != SDRPLAY_API_VERSION) {
           fprintf (stderr, "API versions don't match (local=%.2f dll=%.2f)\n",
                                              SDRPLAY_API_VERSION, apiVersion);
	   goto closeAPI;
	}
	
	fprintf (stderr, "api version %f detected\n", apiVersion);
//
//	lock API while device selection is performed
	sdrplay_api_LockDeviceApi ();
	{  int s	= sizeof (devs) / sizeof (sdrplay_api_DeviceT);
	   err	= sdrplay_api_GetDevices (devs, &ndev, s);
	   if (err != sdrplay_api_Success) {
	      fprintf (stderr, "sdrplay_api_GetDevices failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	      goto unlockDevice_closeAPI;
	   }
	}

	if (ndev == 0) {
	   fprintf (stderr, "no valid device found\n");
	   goto unlockDevice_closeAPI;
	}

	fprintf (stderr, "%d devices detected\n", ndev);
	chosenDevice	= &devs [0];
	err	= sdrplay_api_SelectDevice (chosenDevice);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_SelectDevice failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	   goto unlockDevice_closeAPI;
	}
//
//	we have a device, unlock
	sdrplay_api_UnlockDeviceApi ();

	err	= sdrplay_api_DebugEnable (chosenDevice -> dev, 
	                                         (sdrplay_api_DbgLvl_t)1);
//	retrieve device parameters, so they can be changed if needed
	err	= sdrplay_api_GetDeviceParams (chosenDevice -> dev,
	                                                     &deviceParams);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_GetDeviceParams failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	   goto closeAPI;
	}

	if (deviceParams == nullptr) {
	   fprintf (stderr, "sdrplay_api_GetDeviceParams return null as par\n");
	   goto closeAPI;
	}
//
	vfoFrequency	= Khz (220000);		// default
//	set the parameter values
	chParams	= deviceParams -> rxChannelA;
	deviceParams	-> devParams -> fsFreq. fsHz	= 2048000.0;
	chParams	-> tunerParams. bwType = sdrplay_api_BW_1_536;
	chParams	-> tunerParams. ifType = sdrplay_api_IF_Zero;
//
//	these will change:
	chParams	-> tunerParams. rfFreq. rfHz    = 220000000.0;
	chParams	-> tunerParams. gain.gRdB	= 30;
	chParams	-> tunerParams. gain.LNAstate	= lnaState;
	chParams	-> ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;
	if (agcMode) {
	   chParams    -> ctrlParams. agc. setPoint_dBfs = -30;
	   chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_100HZ;
	}
	else
	   chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_DISABLE;
//
//	assign callback functions
	cbFns. StreamACbFn	= StreamACallback;
	cbFns. StreamBCbFn	= StreamBCallback;
	cbFns. EventCbFn	= EventCallback;

	err	= sdrplay_api_Init (chosenDevice -> dev, &cbFns, this);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_Init failed %s\n",
                                       sdrplay_api_GetErrorString (err));
	   goto unlockDevice_closeAPI;
	}
//
//	Let the parent display the values
	serialNumber	= devs [0]. SerNo;

	hwVersion = devs [0]. hwVer;
	switch (hwVersion) {
	   case 1:		// old RSP
	      lna_upperBound	= 3;
	      deviceLabel	= "RSP-I";
	      denominator	= 2048;
	      nrBits		= 12;
	      has_antennaSelect	= false;
	      break;
	   case 2:		// RSP II
	      lna_upperBound	= 8;
	      deviceLabel 	= "RSP-II";
	      denominator	= 2048;
	      nrBits		= 14;
	      has_antennaSelect	= true;
	      break;
	   case 3:		// RSP-DUO
	      lna_upperBound	= 9;
	      deviceLabel	= "RSP-DUO";
	      denominator	= 2048;
	      nrBits		= 12;
	      has_antennaSelect	= false;
	      break;
	   default:
	   case 255:		// RSP-1A
	      lna_upperBound	= 9;
	      deviceLabel	= "RSP-1A";
	      denominator	= 8192;
	      nrBits		= 14;
	      has_antennaSelect	= false;
	      break;
	}

	reportIndicator		= true;
	threadRuns. store (true);	// it seems we can do some work

	while (threadRuns. load ()) {
	   while (!serverjobs. tryAcquire (1, 1000))
	   if (!threadRuns. load ())
	      goto normal_exit;
//
//	here we assert that theQueue is not empty
	   switch (server_queue. front () -> cmd) {
	      case RESTART_REQUEST: {
	         restartRequest *p = (restartRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. rfFreq. rfHz =
	                                            (float)(p -> frequency);
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Frf,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "restart: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
                 }
	         receiverRuns. store (true);
	         p -> waiter. release (1);
	         break;
	      }
	       
	      case STOP_REQUEST: {
	         stopRequest *p = (stopRequest *)(server_queue. front ());
	         server_queue. pop ();
	         receiverRuns. store (false);
	         p -> waiter. release (1);
	         break;
	      }
	       
	      case SETFREQUENCY_REQUEST: {
	         set_frequencyRequest *p =
	                      (set_frequencyRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. rfFreq. rfHz =
	                                    (float)(p -> frequency);
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Frf,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "restart: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result	= false;
                 }
	         p -> waiter. release (1);
	         break;
	      }

	      case GETFREQUENCY_REQUEST: {
	         get_frequencyRequest *p =
	                      (get_frequencyRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> frequency = 
	                 chParams -> tunerParams. rfFreq. rfHz;
	         p -> result	= true;
	         p -> waiter. release (1);
	         break;
	      }

	      case AGC_REQUEST: {
	         agcRequest *p = 
	                    (agcRequest *)(server_queue. front ());
	         server_queue. pop ();
	         if (p -> agcMode) {
	            chParams    -> ctrlParams. agc. setPoint_dBfs =
	                                             - p -> setPoint;
                    chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_100HZ;
	         }
	         else
	            chParams    -> ctrlParams. agc. enable =
                                             sdrplay_api_AGC_DISABLE;

	         p -> result = true;
	         err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Ctrl_Agc,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "agc: error %s\n",
	                                   sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case GRDB_REQUEST: {
	         GRdBRequest *p =  (GRdBRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. gain. gRdB = p -> GRdBValue;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Gr,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "grdb: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case PPM_REQUEST: {
	         ppmRequest *p = (ppmRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result	= false;
	         deviceParams    -> devParams -> ppm = p -> ppmValue;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Dev_Ppm,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "lna: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case LNA_REQUEST: {
	         lnaRequest *p = (lnaRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         chParams -> tunerParams. gain. LNAstate =
	                                          p -> lnaState;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Tuner_Gr,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success) {
                    fprintf (stderr, "grdb: error %s\n",
                                      sdrplay_api_GetErrorString (err));
	            p -> result = false;
	         }
	         p -> waiter. release (1);
	         break;
	      }

	      case ANTENNASELECT_REQUEST: {
	         antennaRequest *p = (antennaRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = true;
	         deviceParams    -> rxChannelA -> rsp2TunerParams. antennaSel =
                                    p -> antenna == 'A' ?
                                             sdrplay_api_Rsp2_ANTENNA_A:
                                             sdrplay_api_Rsp2_ANTENNA_B;
                 err = sdrplay_api_Update (chosenDevice -> dev,
                                           chosenDevice -> tuner,
                                           sdrplay_api_Update_Rsp2_AntennaControl,
                                           sdrplay_api_Update_Ext1_None);
                 if (err != sdrplay_api_Success)
	            p -> result = false;

	         p -> waiter. release (1);
	         break;
	      }
	
	      case GAINVALUE_REQUEST: {
	         gainvalueRequest *p = 
	                        (gainvalueRequest *)(server_queue. front ());
	         server_queue. pop ();
	         p -> result = false;
	         p -> gainValue = -1;
	         p -> waiter. release (1);
	         break;
	      }

	      default:		// cannot happen
	         break;
	   }
	}

normal_exit:
	err = sdrplay_api_Uninit	(chosenDevice -> dev);
	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "sdrplay_api_Uninit failed %s\n",
	                          sdrplay_api_GetErrorString (err));

	err = sdrplay_api_ReleaseDevice	(chosenDevice);
	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "sdrplay_api_ReleaseDevice failed %s\n",
	                          sdrplay_api_GetErrorString (err));

//	sdrplay_api_UnlockDeviceApi	(); ??
        sdrplay_api_Close               ();
	if (err != sdrplay_api_Success) 
	   fprintf (stderr, "sdrplay_api_Close failed %s\n",
	                          sdrplay_api_GetErrorString (err));

	releaseLibrary			();
	fprintf (stderr, "library released, ready to stop thread\n");
	msleep (200);
	return;

unlockDevice_closeAPI:
	sdrplay_api_UnlockDeviceApi	();
closeAPI:	
	reportIndicator		= true;
	sdrplay_api_ReleaseDevice       (chosenDevice);
        sdrplay_api_Close               ();
	releaseLibrary	();
}
//
//	""simple" functions
bool	sdrplayController::is_threadRunning	() {
	return threadRuns. load ();
}

bool	sdrplayController::is_receiverRunning	() {
	return receiverRuns. load ();
}

bool	sdrplayController::report		() {
	return reportIndicator;
}

bool	sdrplayController::isOK			() {
	return threadRuns. load ();
}

int	sdrplayController::get_lnaRange		() {
	return lna_upperBound;
}

QString	sdrplayController::get_deviceLabel	() {
	return deviceLabel;
}

int	sdrplayController::get_nrBits		() {
	return nrBits;
}

float	sdrplayController::get_apiVersion	() {
	return apiVersion;
}

QString	sdrplayController::get_serialNumber	() {
	return serialNumber;
}

void	sdrplayController::set_gain		(int g) {
	gainSetPoint = g;
}

int	sdrplayController::get_lnaValue		(int lnaState) {
	return get_lnaGRdB (hwVersion, lnaState);
}

HINSTANCE	sdrplayController::fetchLibrary () {
HINSTANCE	Handle	= nullptr;
#ifdef	__MINGW32__
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

	wchar_t *libname = (wchar_t *)L"sdrplay_api.dll";
	Handle	= LoadLibrary (libname);
	if (Handle == nullptr) {
	   if (RegOpenKey (HKEY_LOCAL_MACHINE,
	                   TEXT("Software\\MiricsSDR\\API"),
	                   &APIkey) != ERROR_SUCCESS) {
              fprintf (stderr,
	           "failed to locate API registry entry, error = %d\n",
	           (int)GetLastError());
	      return nullptr;
	   }

	   RegQueryValueEx (APIkey,
	                    (wchar_t *)L"Install_Dir",
	                    nullptr,
	                    nullptr,
	                    (LPBYTE)&APIkeyValue,
	                    (LPDWORD)&APIkeyValue_length);
//	Ok, make explicit it is in the 32/64 bits section
	   wchar_t *x =
	        wcscat (APIkeyValue, (wchar_t *)L"\\x86\\sdrplay_api.dll");
//	        wcscat (APIkeyValue, (wchar_t *)L"\\x64\\sdrplay_api.dll");
	   RegCloseKey(APIkey);

	   Handle	= LoadLibrary (x);
	   if (Handle == nullptr) {
	      fprintf (stderr, "Failed to open sdrplay_api.dll\n");
	      return nullptr;
	   }
	}
#else
	Handle		= dlopen ("libusb-1.0.so", RTLD_NOW | RTLD_GLOBAL);
	Handle		= dlopen ("libsdrplay_api.so", RTLD_NOW);
	if (Handle == nullptr) {
	   fprintf (stderr, "error report %s\n", dlerror());
	   return nullptr;
	}
#endif
	return Handle;
}

void	sdrplayController::releaseLibrary () {
#ifdef __MINGW32__
        FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
}

bool	sdrplayController::loadFunctions () {
	sdrplay_api_Open	= (sdrplay_api_Open_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Open");
	if ((void *)sdrplay_api_Open == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Open\n");
	   return false;
	}

	sdrplay_api_Close	= (sdrplay_api_Close_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Close");
	if (sdrplay_api_Close == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Close\n");
	   return false;
	}

	sdrplay_api_ApiVersion	= (sdrplay_api_ApiVersion_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_ApiVersion");
	if (sdrplay_api_ApiVersion == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_ApiVersion\n");
	   return false;
	}

	sdrplay_api_LockDeviceApi	= (sdrplay_api_LockDeviceApi_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_LockDeviceApi");
	if (sdrplay_api_LockDeviceApi == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_LockdeviceApi\n");
	   return false;
	}

	sdrplay_api_UnlockDeviceApi	= (sdrplay_api_UnlockDeviceApi_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_UnlockDeviceApi");
	if (sdrplay_api_UnlockDeviceApi == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_UnlockdeviceApi\n");
	   return false;
	}

	sdrplay_api_GetDevices		= (sdrplay_api_GetDevices_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetDevices");
	if (sdrplay_api_GetDevices == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetDevices\n");
	   return false;
	}

	sdrplay_api_SelectDevice	= (sdrplay_api_SelectDevice_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_SelectDevice");
	if (sdrplay_api_SelectDevice == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_SelectDevice\n");
	   return false;
	}

	sdrplay_api_ReleaseDevice	= (sdrplay_api_ReleaseDevice_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_ReleaseDevice");
	if (sdrplay_api_ReleaseDevice == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_ReleaseDevice\n");
	   return false;
	}

	sdrplay_api_GetErrorString	= (sdrplay_api_GetErrorString_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetErrorString");
	if (sdrplay_api_GetErrorString == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetErrorString\n");
	   return false;
	}

	sdrplay_api_GetLastError	= (sdrplay_api_GetLastError_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetLastError");
	if (sdrplay_api_GetLastError == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetLastError\n");
	   return false;
	}

	sdrplay_api_DebugEnable		= (sdrplay_api_DebugEnable_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_DebugEnable");
	if (sdrplay_api_DebugEnable == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_DebugEnable\n");
	   return false;
	}

	sdrplay_api_GetDeviceParams	= (sdrplay_api_GetDeviceParams_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_GetDeviceParams");
	if (sdrplay_api_GetDeviceParams == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_GetDeviceParams\n");
	   return false;
	}

	sdrplay_api_Init		= (sdrplay_api_Init_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Init");
	if (sdrplay_api_Init == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Init\n");
	   return false;
	}

	sdrplay_api_Uninit		= (sdrplay_api_Uninit_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Uninit");
	if (sdrplay_api_Uninit == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Uninit\n");
	   return false;
	}

	sdrplay_api_Update		= (sdrplay_api_Update_t)
	                 GETPROCADDRESS (Handle, "sdrplay_api_Update");
	if (sdrplay_api_Update == nullptr) {
	   fprintf (stderr, "Could not find sdrplay_api_Update\n");
	   return false;
	}

	return true;
}
