#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the sdrplayDab program
 *
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

#ifndef __SDRPLAY_HANDLER__
#define	__SDRPLAY_HANDLER__

#include	<QObject>
#include	<QFrame>
#include	<QSettings>
#include	<atomic>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	"ui_sdrplay-widget.h"
#include	"mirsdrapi-rsp.h"

class	dabProcessor;
class	RadioInterface;

typedef void (*mir_sdr_StreamCallback_t)(int16_t	*xi,
	                                 int16_t	*xq,
	                                 uint32_t	firstSampleNum, 
	                                 int32_t	grChanged,
	                                 int32_t	rfChanged,
	                                 int32_t	fsChanged,
	                                 uint32_t	numSamples,
	                                 uint32_t	reset,
	                                 uint32_t	hwRemoved,
	                                 void		*cbContext);
typedef	void	(*mir_sdr_GainChangeCallback_t)(uint32_t	gRdB,
	                                        uint32_t	lnaGRdB,
	                                        void		*cbContext);

///////////////////////////////////////////////////////////////////////////
class	sdrplayHandler: public deviceHandler, public Ui_sdrplayWidget {
Q_OBJECT
public:
			sdrplayHandler		(RadioInterface *,
	                                         QSettings *, dabProcessor *);
			~sdrplayHandler		(void);
	void		setOffset		(int);
	bool		restartReader		(int32_t);
	void		stopReader		(void);
	int32_t		getVFOFrequency		(void);
	void		resetBuffer		(void);
	int16_t		bitDepth		(void);
	void		show			(void);
	void		hide			(void);
	bool		isHidden		(void);
	bool		isSDRPLAY_2		(void);
	void		antennaSwitcher		(bool);
//
//	The buffer should be visible by the callback function
	RingBuffer<std::complex<float>>	*_I_Buffer;
	float		denominator;
	dabProcessor	*base;
	void		set_initialGain	(float);
	void		setGains	(float, float);
	QString		errorCodes	(mir_sdr_ErrT);
	int		lnaState;
	int16_t		hwVersion;
	bool		theSwitch;
	char		selectedAntenna;
private:
	uint32_t	numofDevs;
	int16_t		deviceIndex;
	QSettings	*sdrplaySettings;
	QFrame		*myFrame;
	int32_t		inputRate;
	int32_t		vfoFrequency;
	int32_t		totalOffset;
	std::atomic<bool>	running;
	int16_t		nrBits;
private slots:
	void		set_lnagainReduction	(int);
	void		set_debugControl	(int);
	void		set_agcControl		(int);
	void		set_ppmControl		(int);
	void		set_antennaSelect	(const QString &);
	void		set_tunerSelect		(const QString &);
};
#endif

