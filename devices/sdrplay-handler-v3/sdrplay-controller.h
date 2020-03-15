#
/*
 *    Copyright (C) 2014 .. 2019
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

#ifndef	__SDRPLAY_CONTROLLER__
#define	__SDRPLAY_CONTROLLER__

#include	<QThread>
#include	<dab-constants.h>
#include	<ringbuffer.h>
#include	<sdrplay_api.h>
class	sdrplayHandler_v3;
class	controlQueue;
class	dabProcessor;


class	sdrplayController:public QThread {
Q_OBJECT
public:
		sdrplayController (sdrplayHandler_v3 *,
	                           controlQueue *,
	                           dabProcessor *);
		~sdrplayController	();
	bool	is_threadRunning	();
	bool	is_receiverRunning	();
	RingBuffer<std::complex<int16_t>> *_I_Buffer;
	int	denominator;
	std::atomic<bool>	receiverRuns;
	dabProcessor			*base;
	void	update_PowerOverload (sdrplay_api_EventParamsT *params);
	void	setOffset	(int);
	void	setGains	(float, float);
	void	setTotalGain	(int);
	float	theGain;
	int	gainSetPoint;
private:
	sdrplay_api_Open_t              sdrplay_api_Open;
        sdrplay_api_Close_t             sdrplay_api_Close;
        sdrplay_api_ApiVersion_t        sdrplay_api_ApiVersion;
        sdrplay_api_LockDeviceApi_t     sdrplay_api_LockDeviceApi;
        sdrplay_api_UnlockDeviceApi_t   sdrplay_api_UnlockDeviceApi;
        sdrplay_api_GetDevices_t        sdrplay_api_GetDevices;
        sdrplay_api_SelectDevice_t      sdrplay_api_SelectDevice;
        sdrplay_api_ReleaseDevice_t     sdrplay_api_ReleaseDevice;
        sdrplay_api_GetErrorString_t    sdrplay_api_GetErrorString;
	sdrplay_api_GetLastError_t      sdrplay_api_GetLastError;
        sdrplay_api_DebugEnable_t       sdrplay_api_DebugEnable;
        sdrplay_api_GetDeviceParams_t   sdrplay_api_GetDeviceParams;
        sdrplay_api_Init_t              sdrplay_api_Init;
        sdrplay_api_Uninit_t            sdrplay_api_Uninit;
        sdrplay_api_Update_t            sdrplay_api_Update;

	sdrplay_api_DeviceT             *chosenDevice;
	sdrplay_api_DeviceParamsT       *deviceParams;
	sdrplay_api_CallbackFnsT        cbFns;
	sdrplay_api_RxChannelParamsT    *chParams;

	void				run		();
        uint32_t			numofDevs;
        int16_t				deviceIndex;
        int32_t				vfoFrequency;
        HINSTANCE			Handle;
        int16_t				hwVersion;
        int16_t				nrBits;

	sdrplayHandler_v3		*parent;
	controlQueue			*theQueue;
	std::atomic<bool>		threadRuns;
	HINSTANCE			fetchLibrary	();
	void				releaseLibrary	();
	bool				loadFunctions	();
	
signals:
	void				freq_offSet	(int);
	void				freq_error	(int);
        void				avgValue	(float);
        void				dipValue	(float);
	void		set_lnaRange	(int, int);
	void		set_deviceLabel	(const QString &, int);
	void		setDeviceData	(const QString &, int, float);
	void		show_TotalGain	(int);
};

#endif
