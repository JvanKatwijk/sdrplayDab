######################################################################
# Automatically generated by qmake (2.01a) Tue Oct 6 19:48:14 2009
# but modified by me to accomodate for the includes for qwt, hamlib and
# portaudio
######################################################################

TEMPLATE	= app
TARGET		= sdrplayDab-3.3
QT		+= widgets xml
#CONFIG		+= console
CONFIG		-= console
QMAKE_CXXFLAGS	+= -std=c++11
QMAKE_CFLAGS	+=  -flto -ffast-math
MAKE_CXXFLAGS	+=  -flto -ffast-math
#QMAKE_CFLAGS	+=  -g
#QMAKE_CXXFLAGS	+=  -g
#QMAKE_LFLAGS	+=  -g
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
RC_ICONS	=  sdrplay-dab.ico
RESOURCES	+= resources.qrc

TRANSLATIONS = i18n/de_DE.ts

DEPENDPATH += . \
	      ./src \
	      ./includes \
	      ./service-description \
	      ./src/ofdm \
	      ./src/protection \
	      ./src/backend \
	      ./src/backend/audio \
	      ./src/backend/data \
	      ./src/backend/data/mot \
	      ./src/backend/data/journaline \
	      ./src/output \
	      ./src/support \
	      ./src/support/viterbi-jan \
	      ./src/support/viterbi-spiral \
	      ./includes/ofdm \
	      ./includes/protection \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/backend/data/mot \
	      ./includes/backend/data/journaline \
	      ./includes/output \
	      ./includes/support \
	      ./devices \
	      ./devices/wavfiles \
	      ./includes/scopes-qwt6 \
              ./spectrum-viewer \
	      ./correlation-viewer \
	      ./tii-viewer

INCLUDEPATH += . \
	      ./ \
	      ./src \
	      ./includes \
	      ./service-description \
	      ./includes/protection \
	      ./includes/ofdm \
	      ./includes/backend \
	      ./includes/backend/audio \
	      ./includes/backend/data \
	      ./includes/backend/data/mot \
	      ./includes/backend/data/journaline \
	      ./includes/output \
	      ./includes/support \
	      ./includes/support/viterbi-jan \
	      ./includes/support/viterbi-spiral \
	      ./devices \
	      ./devices/wavfiles \
	      ./includes/scopes-qwt6 \
              ./spectrum-viewer \
	      ./correlation-viewer \
	      ./tii-viewer

# Input
HEADERS += ./radio.h \
	   ./dab-processor.h \
	   ./service-description/service-descriptor.h \
	   ./service-description/audio-descriptor.h \
	   ./service-description/data-descriptor.h \
	   ./devices/device-handler.h \
           ./devices/wavfiles/wavfiles.h \
	   ./includes/dab-constants.h \
	   ./includes/country-codes.h \
	   ./includes/ofdm/ofdm-decoder.h \
	   ./includes/ofdm/phasereference.h \
	   ./includes/ofdm/phasetable.h \
	   ./includes/ofdm/freq-interleaver.h \
#	   ./includes/ofdm/tii_table.h \
	   ./includes/ofdm/tii_detector.h \
	   ./includes/ofdm/fic-handler.h \
	   ./includes/ofdm/fib-decoder.h  \
	   ./includes/ofdm/fib-table.h \
	   ./includes/ofdm/dab-config.h \
	   ./includes/protection/protTables.h \
	   ./includes/protection/protection.h \
	   ./includes/protection/eep-protection.h \
	   ./includes/protection/uep-protection.h \
	   ./includes/backend/msc-handler.h \
	   ./includes/backend/galois.h \
	   ./includes/backend/reed-solomon.h \
	   ./includes/backend/rscodec.h \
	   ./includes/backend/charsets.h \
	   ./includes/backend/firecode-checker.h \
	   ./includes/backend/frame-processor.h \
	   ./includes/backend/backend.h \
	   ./includes/backend/backend-driver.h \
	   ./includes/backend/backend-deconvolver.h \
	   ./includes/backend/audio/mp2processor.h \
	   ./includes/backend/audio/mp4processor.h \
	   ./includes/backend/audio/bitWriter.h \
	   ./includes/backend/data/data-processor.h \
	   ./includes/backend/data/pad-handler.h \
	   ./includes/backend/data/virtual-datahandler.h \
	   ./includes/backend/data/tdc-datahandler.h \
	   ./includes/backend/data/ip-datahandler.h \
	   ./includes/backend/data/mot/mot-handler.h \
	   ./includes/backend/data/mot/mot-object.h \
	   ./includes/backend/data/mot/mot-dir.h \
	   ./includes/backend/data/journaline-datahandler.h \
	   ./includes/backend/data/journaline/dabdatagroupdecoder.h \
	   ./includes/backend/data/journaline/crc_8_16.h \
	   ./includes/backend/data/journaline/log.h \
	   ./includes/backend/data/journaline/newssvcdec_impl.h \
	   ./includes/backend/data/journaline/Splitter.h \
	   ./includes/backend/data/journaline/dabdgdec_impl.h \
	   ./includes/backend/data/journaline/newsobject.h \
	   ./includes/backend/data/journaline/NML.h \
#	   ./includes/output/fir-filters.h \
	   ./includes/output/audio-base.h \
	   ./includes/output/newconverter.h \
	   ./includes/output/audiosink.h \
	   ./includes/support/viterbi-jan/viterbi-handler.h \
	   ./includes/support/viterbi-spiral/viterbi-spiral.h \
           ./includes/support/fft-handler.h \
	   ./includes/support/ringbuffer.h \
#	   ./includes/support/Xtan2.h \
	   ./includes/support/dab-params.h \
	   ./includes/support/band-handler.h \
	   ./includes/support/text-mapper.h \
	   ./includes/support/dab_tables.h \
	   ./includes/support/ensemble-printer.h \
	   ./includes/support/preset-handler.h \
	   ./includes/support/presetcombobox.h \
	   ./includes/support/smallcombobox.h \
	   ./includes/support/smallpushbutton.h \
	   ./includes/support/verysmallpushbutton.h \
	   ./includes/support/smallqlistview.h \
	   ./includes/support/history-handler.h \
	   ./includes/support/scanner-table.h \
	   ./includes/scopes-qwt6/spectrogramdata.h \
	   ./includes/scopes-qwt6/iqdisplay.h \
	   ./spectrum-viewer/spectrum-viewer.h \
	   ./correlation-viewer/correlation-viewer.h \
	   ./tii-viewer/tii-viewer.h

FORMS	+= ./forms/technical_data.ui
FORMS	+= ./forms/dabradio.ui 
FORMS	+= ./forms/audio-description.ui
FORMS	+= ./forms/data-description.ui
FORMS	+= ./spectrum-viewer/scopewidget.ui
FORMS	+= ./correlation-viewer/correlation-widget.ui
FORMS	+= ./tii-viewer/tii-widget.ui
FORMS	+= ./devices/wavfiles/filereader-widget.ui 

SOURCES += ./main.cpp \
	   ./radio.cpp \
	   ./dab-processor.cpp \
	   ./service-description/audio-descriptor.cpp \
	   ./service-description/data-descriptor.cpp \
	   ./devices/device-handler.cpp \
	   ./devices/wavfiles/wavfiles.cpp \
	   ./src/ofdm/ofdm-decoder.cpp \
	   ./src/ofdm/phasereference.cpp \
	   ./src/ofdm/phasetable.cpp \
	   ./src/ofdm/freq-interleaver.cpp \
#	   ./src/ofdm/tii_table.cpp \
	   ./src/ofdm/tii_detector.cpp \
	   ./src/ofdm/fic-handler.cpp \
	   ./src/ofdm/fib-decoder.cpp  \
	   ./src/protection/protTables.cpp \
	   ./src/protection/protection.cpp \
	   ./src/protection/eep-protection.cpp \
	   ./src/protection/uep-protection.cpp \
	   ./src/backend/msc-handler.cpp \
	   ./src/backend/galois.cpp \
	   ./src/backend/reed-solomon.cpp \
	   ./src/backend/rscodec.cpp \
	   ./src/backend/charsets.cpp \
	   ./src/backend/firecode-checker.cpp \
#	   ./src/backend/frame-processor.cpp \
	   ./src/backend/backend.cpp \
           ./src/backend/backend-driver.cpp \
           ./src/backend/backend-deconvolver.cpp \
	   ./src/backend/audio/mp2processor.cpp \
	   ./src/backend/audio/mp4processor.cpp \
	   ./src/backend/audio/bitWriter.cpp \
	   ./src/backend/data/pad-handler.cpp \
	   ./src/backend/data/data-processor.cpp \
#	   ./src/backend/data/virtual-datahandler.cpp \
	   ./src/backend/data/tdc-datahandler.cpp \
	   ./src/backend/data/ip-datahandler.cpp \
	   ./src/backend/data/mot/mot-handler.cpp \
	   ./src/backend/data/mot/mot-object.cpp \
	   ./src/backend/data/mot/mot-dir.cpp \
	   ./src/backend/data/journaline-datahandler.cpp \
	   ./src/backend/data/journaline/crc_8_16.c \
	   ./src/backend/data/journaline/log.c \
	   ./src/backend/data/journaline/newssvcdec_impl.cpp \
	   ./src/backend/data/journaline/Splitter.cpp \
	   ./src/backend/data/journaline/dabdgdec_impl.c \
	   ./src/backend/data/journaline/newsobject.cpp \
	   ./src/backend/data/journaline/NML.cpp \
	   ./src/output/audio-base.cpp \
	   ./src/output/newconverter.cpp \
	   ./src/output/audiosink.cpp \
	   ./src/support/viterbi-jan/viterbi-handler.cpp \
	   ./src/support/viterbi-spiral/viterbi-spiral.cpp \
           ./src/support/fft-handler.cpp \
#	   ./src/support/Xtan2.cpp \
	   ./src/support/dab-params.cpp \
	   ./src/support/band-handler.cpp \
	   ./src/support/text-mapper.cpp \
	   ./src/support/dab_tables.cpp \
	   ./src/support/ensemble-printer.cpp \
	   ./src/support/preset-handler.cpp \
	   ./src/support/presetcombobox.cpp \
	   ./src/support/smallcombobox.cpp \
	   ./src/support/smallpushbutton.cpp \
	   ./src/support/verysmallpushbutton.cpp \
	   ./src/support/smallqlistview.cpp \
	   ./src/support/history-handler.cpp \
	   ./src/support/scanner-table.cpp \
	   ./src/scopes-qwt6/iqdisplay.cpp \
	   ./spectrum-viewer/spectrum-viewer.cpp \
	   ./correlation-viewer/correlation-viewer.cpp \
	   ./tii-viewer/tii-viewer.cpp
#
#
unix {
DESTDIR		= ./linux-bin
exists ("./.git") {
   GITHASHSTRING = $$system(git rev-parse --short HEAD)
   !isEmpty(GITHASHSTRING) {
       message("Current git hash = $$GITHASHSTRING")
       DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
   }
}
isEmpty(GITHASHSTRING) {
    DEFINES += GITHASH=\\\"------\\\"
}

INCLUDEPATH	+= /usr/local/include
INCLUDEPATH	+= /usr/local/include /usr/include/qt4/qwt /usr/include/qt5/qwt /usr/include/qt4/qwt /usr/include/qwt /usr/local/qwt-6.1.4-svn/
#
CONFIG		+= sdrplay-v2
CONFIG		+= sdrplay-v3

LIBS		+= -lfftw3f  -lfftw3 -lusb-1.0 -ldl  #
LIBS		+= -lportaudio
LIBS		+= -lz
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
#correct this for the correct path to the qwt6 library on your system
#LIBS		+= -lqwt
LIBS		+= -lqwt-qt5
#
CONFIG		+= faad
#CONFIG		+= fdk-aac
#very experimental, simple server for connecting to a tdc handler
#CONFIG		+= datastreamer

#to handle output of embedded an IP data stream, uncomment
CONFIG		+= send_datagram

#if you want to listen remote, uncomment
#CONFIG		+= tcp-streamer		# use for remote listening
#otherwise, if you want to use the default qt way of sound out
#CONFIG		+= qt-audio
#comment both out if you just want to use the "normal" way

CONFIG		+= try-epg		# do not use
DEFINES		+= PRESET_NAME
DEFINES		+= __THREADED_BACKEND
#DEFINES	+= SHOW_MISSING

#For x64 linux system uncomment SSE
#For any other system comment SSE out and uncomment NO_SSE
#CONFIG	+= SSE
CONFIG	+= NO_SSE
}
#
# an attempt to have it run under W32 through cross compilation
win32 {
#DESTDIR	= ../../../dab-win
DESTDIR		=  ../../windows-sdrplay
#DESTDIR	= /usr/shared/sdr-j-development/windows-qt-dab
# includes in mingw differ from the includes in fedora linux

exists ("./.git") {
   GITHASHSTRING = $$system(git rev-parse --short HEAD)
   !isEmpty(GITHASHSTRING) {
       message("Current git hash = $$GITHASHSTRING")
       DEFINES += GITHASH=\\\"$$GITHASHSTRING\\\"
   }
}
isEmpty(GITHASHSTRING) {
    DEFINES += GITHASH=\\\"------\\\"
}

INCLUDEPATH	+= /usr/i686-w64-mingw32/sys-root/mingw/include
INCLUDEPATH	+= /usr/local/include /usr/include/qt4/qwt /usr/include/qt5/qwt /usr/include/qt4/qwt /usr/include/qwt /usr/local/qwt-6.1.4-svn/
INCLUDEPATH	+= /mingw32/include
INCLUDEPATH	+= /mingw32/include/qwt
INCLUDEPATH	+= /usr/local/include
LIBS		+= -L/usr/i686-w64-mingw32/sys-root/mingw/lib
#INCLUDEPATH	+= /mingw/include
#INCLUDEPATH	+= /mingw64/include/qwt
#INCLUDEPATH	+= C:/msys64/mingw64/include/qwt

#	choose one of:
CONFIG		+= sdrplay-v2
#CONFIG		+= sdrplay-v3

LIBS		+= -lfftw3f -lfftw3
LIBS		+= -lportaudio
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -lusb-1.0
LIBS		+= -lz
#correct this for the correct path to the qwt6 library on your system
#mingw64 wants the first one, cross compiling mingw64-32 the second one
#LIBS		+= -lqwt
LIBS		+= -lqwt-qt5
CONFIG		+= faad
CONFIG		+= NO_SSE

#very experimental, simple server for connecting to a tdc handler
#CONFIG		+= datastreamer

#if you want to listen remote, uncomment
#CONFIG		+= tcp-streamer		# use for remote listening
#otherwise, if you want to use the default qt way of soud out
#CONFIG		+= qt-audio
#comment both out if you just want to use the "normal" way

CONFIG		+= try-epg		# do not use
DEFINES		+= PRESET_NAME
}

sdrplay-v2   	{
DEPENDPATH	+= ./devices/sdrplay-handler-v2 
INCLUDEPATH	+= ./devices/sdrplay-handler-v2 
HEADERS		+= ./devices/sdrplay-handler-v2/sdrplay-handler.h \
	           ./devices/sdrplay-handler-v2/sdrplayselect.h 
SOURCES		+= ./devices/sdrplay-handler-v2/sdrplay-handler.cpp \
	           ./devices/sdrplay-handler-v2/sdrplayselect.cpp 
FORMS		+= ./devices/sdrplay-widget.ui
DEFINES		+= SDRPLAY_V2
}

sdrplay-v3	{
DEPEND_PATH	+= ./devices/sdrplay-handler-v3 
INCLUDEPATH	+= ./devices/sdrplay-handler-v3 \
	           ./devices/sdrplay-handler-v3/include 
HEADERS		+= ./devices/sdrplay-handler-v3/sdrplay-handler-v3.h \
	           ./devices/sdrplay-handler-v3/sdrplay-commands.h 
SOURCES		+= ./devices/sdrplay-handler-v3/sdrplay-handler-v3.cpp 
FORMS		+= ./devices/sdrplay-widget.ui
DEFINES		+= SDRPLAY_V3
}

send_datagram {
	DEFINES		+= _SEND_DATAGRAM_
	QT		+= network
}

try-epg	{
	DEFINES		+= TRY_EPG
	QT		+= xml
	DEPENDPATH	+= ./src/backend/data/epg \
	                   ./includes/backend/data/epg 
	INCLUDEPATH	+= ./includes/backend/data/epg 
	HEADERS		+= ./includes/backend/data/epg/epgdec.h 
	SOURCES		+= ./src/backend/data/epg/epgdec.cpp 
}

tcp-streamer	{
	DEFINES		+= TCP_STREAMER
	QT		+= network
	HEADERS		+= ./includes/output/tcp-streamer.h
	SOURCES		+= ./src/output/tcp-streamer.cpp
}

qt-audio	{
	DEFINES		+= QT_AUDIO
	QT		+= multimedia
	HEADERS		+= ./includes/output/Qt-audio.h \
	                   ./includes/output/Qt-audiodevice.h
	SOURCES		+= ./src/output/Qt-audio.cpp \
	                   ./src/output/Qt-audiodevice.cpp
}

datastreamer	{
	DEFINES		+= DATA_STREAMER
	INCLUDEPATH	+= ./server-thread
	HEADERS		+= ./server-thread/tcp-server.h
	SOURCES		+= ./server-thread/tcp-server.cpp
}


# for RPI2 use:
NEON_RPI2	{
	DEFINES		+= NEON_AVAILABLE
	QMAKE_CFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	QMAKE_CXXFLAGS	+=  -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4  
	HEADERS		+= ./src/support/viterbi-spiral/spiral-neon.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-neon.c
}

# for RPI3 use:
NEON_RPI3	{
	DEFINES		+= NEON_AVAILABLE
#	QMAKE_CFLAGS	+=  -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits
#	QMAKE_CXXFLAGS	+=  -mcpu=cortex-a53 -mfloat-abi=hard -mfpu=neon-fp-armv8 -mneon-for-64bits
	HEADERS		+= ./src/support/viterbi-spiral/spiral-neon.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-neon.c
}

SSE	{
	DEFINES		+= SSE_AVAILABLE
	HEADERS		+= ./src/support/viterbi-spiral/spiral-sse.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-sse.c
}

NO_SSE	{
	HEADERS		+= ./src/support/viterbi-spiral/spiral-no-sse.h
	SOURCES		+= ./src/support/viterbi-spiral/spiral-no-sse.c
}

faad	{
	DEFINES		+= __WITH_FAAD__
	HEADERS		+= ./includes/backend/audio/faad-decoder.h 
	SOURCES		+= ./src/backend/audio/faad-decoder.cpp 
	LIBS		+= -lfaad
}

fdk-aac	{
	DEFINES		+= __WITH_FDK_AAC__
	INCLUDEPATH	+= /usr/local/include/fdk-aac
	HEADERS		+= ./includes/backend/audio/fdk-aac.h 
	SOURCES		+= ./src/backend/audio/fdk-aac.cpp 
	LIBS		+= -lfdk-aac
}

