#
/*
 *    Copyright (C) 2014 .. 2017
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
 *    along with sdrplayDab if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"dab-processor.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"radio.h"
#include	"dab-params.h"
/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  *	It is the main interface to the qt-dab program,
  *	local are classes ofdmDecoder, ficHandler and mschandler.
  */
#define BUFSIZE 64
#define BUFMASK (64 - 1)
#define	N	5

static
int	tii_delay	= 10;
static
int	tii_counter	= 0;

static  inline
int16_t valueFor (int16_t b) {
int16_t res     = 1;
        while (--b > 0)
           res <<= 1;
        return res;
}

	dabProcessor::dabProcessor	(RadioInterface	*mr,
	                                 uint8_t	dabMode,
	                                 int16_t	threshold,
	                                 int16_t	diff_length,
	                                 int16_t	bitDepth,
	                                 RingBuffer<float> *responseBuffer,
	                                 RingBuffer<std::complex<float>> *
	                                                   spectrumBuffer,
	                                 RingBuffer<std::complex<float>> *
	                                                   iqBuffer,
	                                 RingBuffer<std::complex<float>> *
	                                                   tiiBuffer,
	                                 QString	picturesPath
	                                 ):
	                                 params (dabMode),
	                                 my_ficHandler (mr, dabMode),
	                                 my_mscHandler (mr, dabMode,
	                                                picturesPath),
	                                 phaseSynchronizer (mr,
	                                                    dabMode, 
	                                                    responseBuffer,
                                                            threshold,
	                                                    diff_length),
	                                 my_TII_Detector (dabMode),
	                                 my_ofdmDecoder (mr,
	                                                 dabMode,
	                                                 iqBuffer,
	                                                 bitDepth) {
int32_t	i;

	this	-> myRadioInterface	= mr;
        this    -> spectrumBuffer       = spectrumBuffer;
	this	-> tiiBuffer		= tiiBuffer;
	this	-> dabMode		= dabMode;
	this	-> T_null		= params. get_T_null ();
	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> T_g			= T_s - T_u;
	this	-> T_F			= params. get_T_F ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> carrierDiff		= params. get_carrierDiff ();
	processorMode			= START;
	nullCount			= 0;
	ofdmBuffer. resize (2 * T_s);
	ofdmBufferIndex			= 0;
	avgSignalValue			= 0;
	avgLocalValue			= 0;
	counter				= 0;
	dataBuffer. resize (BUFSIZE);
	memset (dataBuffer. data (), 0, BUFSIZE * sizeof (float));
	bufferP				= 0;
	fineOffset			= 0;	
	coarseOffset			= 0;	
	totalOffset			= 0;
	correctionNeeded		= true;
	dumpfilePointer. store (nullptr);
        dumpIndex			= 0;
        dumpScale			= valueFor (bitDepth);

	ibits. resize (2 * carriers);
//
//	for the spectrum display we need:
	bufferSize              = 32768;
        localBuffer. resize (bufferSize);
        localCounter            = 0;

	connect (this, SIGNAL (showCoordinates (int, int)),
                 mr,   SLOT   (showCoordinates (int, int)));
	connect (this, SIGNAL (setSynced (char)),
	         myRadioInterface, SLOT (setSynced (char)));
	connect (this, SIGNAL (set_freqOffset (int)),
	         myRadioInterface, SLOT (set_freqOffset (int)));
        connect (this, SIGNAL (show_Spectrum (int)),
                 mr, SLOT (showSpectrum (int)));
	connect (this, SIGNAL (show_tii (int)),
	         myRadioInterface, SLOT (show_tii (int)));
	connect (this, SIGNAL (No_Signal_Found (void)),
	         myRadioInterface, SLOT (No_Signal_Found (void)));
	my_TII_Detector. reset ();
}

	dabProcessor::~dabProcessor	(void) {
}
//
//	Since in this implementation, the callback of the device
//	is the driving force, i.e. pumping symbols into the system,
//	the basic interpretation is using an explicit state-based
//	approach
int	dabProcessor::addSymbol	(std::complex<float> symbol) {
int	retValue	= GO_ON;		// default

static	int dabCounter	= 0;

	avgSignalValue	= 0.9999 * avgSignalValue +
	                  0.0001 * jan_abs (symbol);
	dataBuffer [bufferP] = jan_abs (symbol);
	avgLocalValue	+= jan_abs (symbol) -
	                   dataBuffer [(bufferP - 50) & BUFMASK];
	bufferP		= (bufferP + 1) & BUFMASK;
	dabCounter ++;
	if (dumpfilePointer. load () != nullptr)
	   dump (symbol);

	if (localCounter < bufferSize)
	   localBuffer [localCounter ++] = symbol;
	sampleCount ++;

	if (++sampleCount > INPUT_RATE / N) {
	   sampleCount	= 0;
	   spectrumBuffer -> putDataIntoBuffer (localBuffer. data (),
	                                        localCounter);
	   show_Spectrum	(localCounter);
	   localCounter = 0;
	}

	switch (processorMode) {
	   default:
	   case START:
	      avgSignalValue	= 0;
	      avgLocalValue	= 0;
	      counter		= 0;
	      dipValue		= 0;
	      dipCnt		= 0;
	      fineOffset	= 0;
	      correctionNeeded	= true;
	      coarseOffset	= 0;
	      totalOffset	= 0;
	      attempts		= 0;
	      memset (dataBuffer. data (), 0, BUFSIZE * sizeof (float));
	      bufferP		= 0;
	      processorMode	= INIT;
	      break;

	   case INIT:
	      if (++counter >= 2 * T_F) {
	         processorMode	= LOOKING_FOR_DIP;
	         retValue	= INITIAL_STRENGTH;	
	         counter	= 0;
	      }
	      break;
//
//	After initialization, we start looking for a dip,
//	After recognizing a frame, we pass this and continue
//	at end of dip
	   case LOOKING_FOR_DIP:
	      counter	++;
	      if (avgLocalValue / 50 < avgSignalValue * 0.45) {
	         retValue	= DEVICE_UPDATE;
	         processorMode	= DIP_FOUND;
	         dipValue	= 0;
	         dipCnt		= 0;
	      }
	      else	
	      if (counter > T_F) {
	         counter	= 0;
	         attempts ++;
	         if (attempts > 5) {
	            emit No_Signal_Found ();
	            processorMode	= START;
	         }
	         else {
	            counter		= 0;
	            processorMode	= INIT;
	         }
	      }
	      break;
	      
	   case DIP_FOUND:
	      dipValue		+= jan_abs (symbol);
	      dipCnt		++;
	      if (avgLocalValue / BUFSIZE > avgSignalValue * 0.8) {
	         dipValue		/= dipCnt;
                 processorMode  	= END_OF_DIP;
	         ofdmBufferIndex	= 0;
	      }
	      else 
	      if (dipCnt > T_null + 100) {
	         dipCnt		= 0;
	         attempts ++;
	         if (attempts > 5) {
	            emit No_Signal_Found ();
	            processorMode       = START;
	         }
	         else {
	            counter		= 0;
	            processorMode       = INIT;
	         }
	      }
	      break;

	   case END_OF_DIP:
	      ofdmBuffer [ofdmBufferIndex ++] = symbol;
	      if (ofdmBufferIndex >= T_u) {
	         int startIndex = phaseSynchronizer. findIndex (ofdmBuffer);
	         if (startIndex < 0) {		// no sync
	            if (attempts > 5) {
	               emit No_Signal_Found ();
                       processorMode       = START;
	               break;
                    }
	            else {
	               processorMode = LOOKING_FOR_DIP;
	               break;
	            }
	         }
	         attempts	= 0;	// we made it!!!
	         dabCounter	= dabCounter - T_u + startIndex;
//	         fprintf (stderr, "%d \n", dabCounter);
	         dabCounter	= T_u - startIndex;
	         memmove (ofdmBuffer. data (),
	                  &((ofdmBuffer. data ()) [startIndex]),
                           (T_u - startIndex) * sizeof (std::complex<float>));
	         ofdmBufferIndex  = T_u - startIndex;
	         processorMode	= GO_FOR_BLOCK_0;
	      }
	      break;

	   case GO_FOR_BLOCK_0:
	      ofdmBuffer [ofdmBufferIndex] = symbol;
	      if (++ofdmBufferIndex < T_u)
	         break;
	      my_ofdmDecoder. processBlock_0 (ofdmBuffer);
	      my_mscHandler.  processBlock_0 (ofdmBuffer. data ());
//      Here we look only at the block_0 when we need a coarse
//      frequency synchronization.
	      correctionNeeded     = !my_ficHandler. syncReached ();
	      if (correctionNeeded) {
	         int correction    =
                    phaseSynchronizer. estimate_CarrierOffset (ofdmBuffer);
	         if (correction != 100) {
	            coarseOffset   = correction * carrierDiff;
	            totalOffset	+= coarseOffset;
	            if (abs (totalOffset) > Khz (15)) {
	               totalOffset	= 0;
	               coarseOffset	= 0;
	            }
	         }
	      }
	      else
	         coarseOffset	= 0;
//
//	and prepare for reading the data blocks
	      FreqCorr		= std::complex<float> (0, 0);
	      ofdmSymbolCount	= 1;
	      ofdmBufferIndex	= 0;
	      processorMode	= BLOCK_READING;
	      break;

	   case BLOCK_READING:
	      ofdmBuffer [ofdmBufferIndex ++] = symbol;
	      if (ofdmBufferIndex < T_s) 
	         break;

	      for (int i = (int)T_u; i < (int)T_s; i ++)
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	      if (ofdmSymbolCount < 4) {
	         my_ofdmDecoder. decode (ofdmBuffer,
	                                 ofdmSymbolCount, ibits. data ());
	         my_ficHandler. process_ficBlock (ibits, ofdmSymbolCount);
	      }
	      my_mscHandler. process_Msc  (&((ofdmBuffer. data ()) [T_g]),
	                                   ofdmSymbolCount);
	      ofdmBufferIndex	= 0;
	      if (++ofdmSymbolCount >= nrBlocks) {
	         processorMode	= END_OF_FRAME;
	      }
	      break;

	   case END_OF_FRAME:
	      fineOffset = arg (FreqCorr) / M_PI * carrierDiff / 2;

	      if (fineOffset > carrierDiff / 2) {
	         coarseOffset += carrierDiff;
	         fineOffset -= carrierDiff;
	      }
	      else
	      if (fineOffset < -carrierDiff / 2) {
	         coarseOffset -= carrierDiff;
	         fineOffset += carrierDiff;
	      }
//
//	Once here, we are - without even looking - sure
//	that we are in a dip period
	      processorMode	= PREPARE_FOR_SKIP_NULL_PERIOD;
	      retValue		= DEVICE_UPDATE;
	      break;
//
//	here, we skip the next null period
	   case PREPARE_FOR_SKIP_NULL_PERIOD:
	      nullCount		= 0;
	      dipValue		= jan_abs (symbol);
	      ofdmBuffer [nullCount ++] = symbol;
	      processorMode	= SKIP_NULL_PERIOD;
	      break;

	   case SKIP_NULL_PERIOD:
	      ofdmBuffer [nullCount] = symbol;
	      dipValue		+= jan_abs (symbol);
	      nullCount ++;
	      if (nullCount >= T_null - 1) {
	         processorMode = END_OF_DIP;
	         dipValue	/= T_null;
	         handle_tii_detection (ofdmBuffer);
	      }
	      break;
	}
	return retValue;
}

void	dabProcessor:: reset	(void) {
	processorMode	= START;
	nullCount	= 0;
	correctionNeeded	= true;
	my_ficHandler.  reset ();
	my_mscHandler. reset  ();
}

void	dabProcessor::stop	(void) {
	my_ficHandler.  reset ();
}

void	dabProcessor::coarseCorrectorOn (void) {
	correctionNeeded 	= true;
	coarseOffset	= 0;
}

void	dabProcessor::coarseCorrectorOff (void) {
	correctionNeeded	= false;
}

uint8_t dabProcessor::kindofService           (QString &s) {
	return my_ficHandler. kindofService (s);
}

void	dabProcessor::dataforAudioService     (int16_t c, audiodata *dd) {
	my_ficHandler. dataforAudioService (c, dd);
}

void	dabProcessor::dataforAudioService     (QString &s,audiodata *dd) {
	my_ficHandler. dataforAudioService (s, dd, 0);
}

void	dabProcessor::dataforAudioService     (QString &s,
	                                          audiodata *d, int16_t c) {
	my_ficHandler. dataforAudioService (s, d, c);
}

void	dabProcessor::dataforDataService	(int16_t c, packetdata *dd) {
	my_ficHandler. dataforDataService (c, dd);
}

void	dabProcessor::dataforDataService	(QString &s, packetdata *dd) {
	my_ficHandler. dataforDataService (s, dd, 0);
}

void	dabProcessor::dataforDataService	(QString &s,
	                                            packetdata *d, int16_t c) {
	my_ficHandler. dataforDataService (s, d, c);
}


void	dabProcessor::reset_msc (void) {
	my_mscHandler. reset ();
}

void	dabProcessor::set_audioChannel (audiodata *d,
	                                      RingBuffer<int16_t> *b) {
	my_mscHandler. set_Channel (d, b, (RingBuffer<uint8_t> *)nullptr);
}

void	dabProcessor::set_dataChannel (packetdata *d,
	                                      RingBuffer<uint8_t> *b) {
	my_mscHandler. set_Channel (d, (RingBuffer<int16_t> *)nullptr, b);
}

uint8_t	dabProcessor::get_ecc		(void) {
	return my_ficHandler. get_ecc ();
}

int32_t dabProcessor::get_ensembleId	(void) {
	return my_ficHandler. get_ensembleId ();
}

QString dabProcessor::get_ensembleName	(void) {
	return my_ficHandler. get_ensembleName ();
}

void	dabProcessor::clearEnsemble	(void) {
	my_ficHandler. clearEnsemble ();
}

static int teller	= 0;

void	dabProcessor::update_data (int *freq, float *dip, float *firstSymb) {
int	result	= coarseOffset + fineOffset;
	if (++teller > 10) {
	   teller = 0;
	   set_freqOffset (result);
	}
	*freq		= result;
	*dip		= dipValue;
	*firstSymb	= avgSignalValue;
}

float	dabProcessor::initialSignal	(void) {
	return avgSignalValue;
}

bool    dabProcessor::wasSecond (int16_t cf, dabParams *p) {
        switch (p -> get_dabMode ()) {
           default:
           case 1:
              return (cf & 07) >= 4;
           case 2:
           case 3:
              return (cf & 02);
           case 4:
              return (cf & 03) >= 2;
        }
}

void	dabProcessor::handle_tii_detection
	                      (std::vector<std::complex<float>> b) {
	if (dabMode != 1)
	   return;
	if (wasSecond (my_ficHandler. get_CIFcount (), &params)) {
	   my_TII_Detector. addBuffer (ofdmBuffer);
	   if (++tii_counter >= tii_delay) {
	      int16_t mainId      = -1;
	      int16_t subId       = -1;
	      my_TII_Detector. processNULL (&mainId, &subId);
	      if (mainId > 0)
	         showCoordinates (mainId, subId);
	      tiiBuffer -> putDataIntoBuffer (ofdmBuffer. data (), T_u);
              show_tii (1);
	      tii_counter	= 0;
	      my_TII_Detector. reset ();
	   }
	}

	if ((tii_counter & 02) != 0) {
	   tiiBuffer -> putDataIntoBuffer (ofdmBuffer. data (), T_u);
	   show_tii (1);
	}

	if (tii_counter >= tii_delay)
	   tii_counter = 1;
}

void	dabProcessor:: dump (std::complex<float> temp) {
	dumpBuffer [2 * dumpIndex    ] = real (temp) * dumpScale;
	dumpBuffer [2 * dumpIndex + 1] = imag (temp) * dumpScale;
	if (++dumpIndex >= DUMPSIZE / 2) {
	   sf_writef_short (dumpfilePointer. load (),
	                    dumpBuffer, dumpIndex);
	   dumpIndex = 0;
        }
}


void	dabProcessor::startDumping (SNDFILE *f) {
        dumpfilePointer. store (f);
}

void	dabProcessor::stopDumping (void) {
        dumpfilePointer. store (nullptr);
}


