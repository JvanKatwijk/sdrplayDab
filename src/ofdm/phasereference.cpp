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
 *    along with sdrplayDab-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"phasereference.h" 
#include	"string.h"
#include	"radio.h"
/**
  *	\class phaseReference
  *	Implements the correlation that is used to identify
  *	the "first" element (following the cyclic prefix) of
  *	the first non-null block of a frame
  *	The class inherits from the phaseTable.
  */

	phaseReference::phaseReference (RadioInterface *mr,
	                                uint8_t		dabMode,
	                                int16_t		threshold,
	                                int16_t		diff_length,
	                                int16_t		depth,
	                                RingBuffer<float> *b):
	                                     phaseTable (dabMode),
	                                     params (dabMode),
	                                     my_fftHandler (dabMode) {
int32_t	i;
float	Phi_k;

	this	-> response	= b;
	this	-> threshold	= threshold;
	this	-> diff_length	= diff_length;
	this	-> T_u		= params. get_T_u ();
	this	-> depth	= depth;
	this	-> carriers	= params. get_carriers ();

	prevTable.		resize (T_u);
	refTable.		resize (T_u);
	fft_buffer		= my_fftHandler. getVector ();

	framesperSecond		= 2048000 / params. get_T_F ();
	displayCounter		= 0;

	memset (refTable. data (), 0, sizeof (std::complex<float>) * T_u);
	memset (prevTable. data (), 0, sizeof (std::complex<float>) * T_u);

	for (i = 1; i <= params. get_carriers () / 2; i ++) {
	   Phi_k =  get_Phi (i);
	   refTable [i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
	   Phi_k = get_Phi (-i);
	   refTable [T_u - i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
	}
//
	connect (this, SIGNAL (showImpulse (int)),
	         mr,   SLOT   (showImpulse (int)));
	connect (this, SIGNAL (showIndex   (int)),
	         mr,   SLOT   (showIndex   (int)));
	connect (this, SIGNAL (showPhases  (float, float)),
	         mr,   SLOT   (showPhases  (float, float)));
}

	phaseReference::~phaseReference (void) {
}

/**
  *	\brief findIndex
  *	the vector v contains "T_u" samples that are believed to
  *	belong to the first non-null block of a DAB frame.
  *	We correlate the data in this vector with the predefined
  *	data, and if the maximum exceeds a threshold value,
  *	we believe that that indicates the first sample we were
  *	looking for.
  */

static std::complex<float> oldsum	= std::complex<float> (0, 0);
static bool xxxx			= false;

static	float	angle_1			= 0;
static	float	angle_2			= 0;
static	float	theSum			= 0;
void	phaseReference::computeAngle (std::vector<std::complex<float>> v) {
std::complex<float> sum	= std::complex<float>(0, 0);
int	i;

	memcpy (fft_buffer, v. data (), T_u * sizeof (std::complex<float>));
	my_fftHandler. do_FFT ();
//
	for (i = -carriers / 2; i < carriers / 2; i ++) {
	   if (i != 0)
	      sum +=  fft_buffer [(T_u + i) % T_u] *
	                                 conj (prevTable [(T_u + i) % T_u]);
	}
	memcpy (prevTable. data (),
	                 fft_buffer, T_u * sizeof (std::complex<float>));
	if (xxxx) {
	   float a = arg (sum) + M_PI;
	   float b = arg (oldsum) + M_PI;
	   angle_1	= 0.95 * angle_1 + 0.05 * a;
	   angle_2	= 0.95 * angle_2 + 0.05 * b;	
	   showPhases (angle_1, angle_2);
	}
	oldsum = sum;
	xxxx = !xxxx;
}

int32_t	phaseReference::findIndex (std::vector <std::complex<float>> v) {
int32_t	i;
int32_t	maxIndex	= -1;
int32_t	oldIndex	= -1;
float	sum		= 0;
float	Max		= -1000;
float	lbuf [T_u];
float	mbuf [T_u];
std::vector<int> resultVector;

	memcpy (fft_buffer, v. data (), T_u * sizeof (std::complex<float>));
	my_fftHandler. do_FFT ();
//
//	into the frequency domain, now correlate
	for (i = 0; i < T_u; i ++) 
	   fft_buffer [i] *= conj (refTable [i]);
//	and, again, back into the time domain
	my_fftHandler. do_IFFT ();
/**
  *	We compute the average and the max signal values
  */
	for (i = 0; i < T_u; i ++) {
	   lbuf [i] = jan_abs (fft_buffer [i]);
	   mbuf [i] = lbuf [i];
	   sum	+= lbuf [i];
	   if (lbuf [i] > Max) {
	      maxIndex = i;
	      Max = lbuf [i];
	   }
	}

	if (Max < threshold * sum / T_u)
	   return (- abs (Max * T_u / sum) - 1);
	else
	   resultVector. push_back (maxIndex);

	for (int k = 0; k < depth; k ++) {
	   float MMax = 4 * threshold * sum / T_u;
	   int  lIndex = -1;
	   for (i = 0; i < T_u / 2; i ++) {
	      if (lbuf [i] > MMax) {
	         MMax = lbuf [i];
	         lIndex = i;
	      }
	   }
	   if (lIndex > 0) {
	      resultVector . push_back (lIndex);
	      for (i = lIndex - 15; i < lIndex + 15; i ++)
	         lbuf [i] = 0;
	   }
	   else
	      break;
	}

	if (response != NULL) {
	   if (++displayCounter > framesperSecond / 4) {
	      response  -> putDataIntoBuffer (mbuf, T_u);
	      showImpulse (T_u);
	      displayCounter    = 0;
	      if (resultVector. at (0) > 0) {
	         showIndex (-1);
	         for (i = 1; i < resultVector. size (); i ++)
	            showIndex (resultVector. at (i));
	         showIndex (0);
	      }
	   }
	}
	return resultVector. at (0);
}

#define	SEARCH_RANGE	(2 * 35)
int16_t	phaseReference::estimate_CarrierOffset (std::vector<std::complex<float>> v) {
int16_t	i, j;
std::complex<float> temp;
float	maxCorr	= 0;
int	maxIndex = -1;

	for (i = T_u - SEARCH_RANGE / 2;
	     i < T_u + SEARCH_RANGE / 2; i ++) {
	   temp = std::complex<float> (0, 0);
	   for (j = 0; j < diff_length; j ++)
	      temp += fft_buffer [(i + j) % T_u] *
	                          refTable [(i + j) % T_u];
	   if (abs (temp) > maxCorr) {
	      maxIndex = i - (T_u - SEARCH_RANGE);
	      maxCorr  = abs (temp);
	   }
	}

	return maxIndex - T_u;
}
//
//	NOT USED, just for some tests
//	An alternative way to compute the small frequency offset
//	is to look at the phase offset of subsequent carriers
//	in block 0, compared to the values as computed from the
//	reference block.
//	The values are reasonably close to the values computed
//	on the fly
#define	LLENGTH	100
float	phaseReference::estimate_FrequencyOffset (std::vector <std::complex<float>> v) {
int16_t	i, j;
float pd	= 0;

	for (i = - LLENGTH / 2 ; i < LLENGTH / 2; i ++) {
	   std::complex<float> a1 = refTable [(T_u + i) % T_u] * conj (refTable [(T_u + i + 1) % T_u]);
	   std::complex<float> a2 = fft_buffer [(T_u + i) % T_u] * conj (fft_buffer [(T_u + i + 1) % T_u]);
	   pd += arg (a2) - arg (a1);
	}
	return pd / LLENGTH;
}

