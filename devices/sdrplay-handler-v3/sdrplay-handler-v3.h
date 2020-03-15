#
/*
 *    Copyright (C) 2014 .. 2019
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

#ifndef __SDRPLAY_HANDLER_V3__
#define	__SDRPLAY_HANDLER_V3__

#include	<QObject>
#include	<QFrame>
#include	<QSettings>
#include	<atomic>
#include	<stdio.h>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	"ui_sdrplay-widget.h"

class	controlQueue;
class	sdrplayController;
class	dabProcessor;
class	RadioInterface;

#ifdef __MINGW32__
#define GETPROCADDRESS  GetProcAddress
#else
#define GETPROCADDRESS  dlsym
#endif

class	sdrplayHandler_v3: public deviceHandler, public Ui_sdrplayWidget {
Q_OBJECT
public:
			sdrplayHandler_v3	(RadioInterface *,
	                                         QSettings *,
	                                              dabProcessor *);
			~sdrplayHandler_v3	();
	bool		restartReader		(int32_t);
	void		stopReader		();
	int32_t		getVFOFrequency		();
	void		resetBuffer		();
	int16_t		bitDepth		();
	void		show			();
	void		hide			();
	bool		isHidden		();

private:
	dabProcessor		*base;
	sdrplayController	*theController;
	controlQueue		*theQueue;
	int32_t			vfoFrequency;
	std::atomic<bool>	running;
	int16_t			hwVersion;
	QSettings		*sdrplaySettings;
	QFrame			*myFrame;
	bool			agcMode;
	int16_t			nrBits;
	int16_t			denominator;

private slots:
	void			set_lnagainReduction	(int);
//	void			set_debugControl	(int);
	void			set_agcControl		(int);
	void			set_ppmControl		(int);
	void			set_antennaSelect	(const QString &);
	void			set_tunerSelect		(const QString &);
	void			set_gain		(int);
public slots:
        void			freq_offset     (int);
        void                    freq_error      (int);
        void                    avgValue        (float);
        void                    dipValue        (float);
	void			set_lnaRange	(int, int);
	void			set_deviceLabel	(const QString &, int);
	void			setDeviceData	(const QString &, int, float);
	void			show_TotalGain	(int);
};
#endif

