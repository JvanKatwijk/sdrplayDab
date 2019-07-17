#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB (formerly SDR-J, JSDR).
 *    Many of the ideas as implemented in Qt-DAB are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are acknowledged.
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *      Main program
 */
#include        <QApplication>
#include        <QSettings>
#include	<QTranslator>
#include        <QDir>
#include	<QDebug>
#include        <unistd.h>
#include        "dab-constants.h"
#include        "radio.h"

#define DEFAULT_INI     ".sdrplay-dab.ini"

#ifndef	GITHASH
#define	GITHASH	"      "
#endif

QString fullPathfor (QString v) {
QString fileName;

	if (v == QString (""))
	   return QString ("/tmp/xxx");

	if (v. at (0) == QChar ('/'))           // full path specified
	   return v;

	fileName = QDir::homePath ();
	fileName. append ("/");
	fileName. append (v);
	fileName = QDir::toNativeSeparators (fileName);

	if (!fileName. endsWith (".ini"))
	   fileName. append (".ini");

	return fileName;
}

void    setTranslator (QString Language);

int     main (int argc, char **argv) {
QString initFileName = fullPathfor (QString (DEFAULT_INI));
RadioInterface  *MyRadioInterface;

// Default values
int16_t         tii_delay       = 20;
QSettings       *dabSettings;           // ini file
int32_t		dataPort	= 8888;
int     opt;

	QCoreApplication::setOrganizationName ("Lazy Chair Computing");
	QCoreApplication::setOrganizationDomain ("Lazy Chair Computing");
	QCoreApplication::setApplicationName ("sdrplayDab");
	QCoreApplication::setApplicationVersion (QString (CURRENT_VERSION) + " Git: " + GITHASH);

	while ((opt = getopt (argc, argv, "i:P:")) != -1) {
	   switch (opt) {
	      case 'i':
	         initFileName = fullPathfor (QString (optarg));
	         break;

	      case 'P':
	         dataPort	= atoi (optarg);
	         break;

	     default:
	         break;
	   }
	}

	dabSettings =  new QSettings (initFileName, QSettings::IniFormat);

/*
 *      Before we connect control to the gui, we have to
 *      instantiate
 */
#if QT_VERSION >= 0x050600
	QGuiApplication::setAttribute (Qt::AA_EnableHighDpiScaling);
#endif
	QApplication a (argc, argv);
//	setting the language
	QString locale = QLocale::system (). name ();
	qDebug() << "main:" <<  "Detected system language" << locale;
	setTranslator (locale);

	a. setWindowIcon (QIcon (":/sdrplay-dab.ico"));

	MyRadioInterface = new RadioInterface (dabSettings,
	                                       dataPort
                                               );
	MyRadioInterface -> show ();
        a. exec ();
/*
 *      done:
 */
	fprintf (stderr, "back in main program\n");
	fflush (stdout);
	fflush (stderr);
	qDebug ("It is done\n");
	delete MyRadioInterface;
	delete dabSettings;
}

void	setTranslator (QString Language) {
QTranslator *Translator = new QTranslator;

//	German is special (as always)
	if ((Language == "de_AT") || (Language ==  "de_CH"))
	   Language = "de_DE";
//
//	what about Dutch?
	bool TranslatorLoaded =
	             Translator -> load (QString(":/i18n/") + Language);
	qDebug() << "main:" <<  "Set language" << Language;
	QCoreApplication::installTranslator (Translator);

	if (!TranslatorLoaded) {
	   qDebug() << "main:" <<  "Error while loading language specifics" << Language << "use English \"en_GB\" instead";
	   Language = "en_GB";
	}

	QLocale curLocale (QLocale ((const QString&)Language));
	QLocale::setDefault (curLocale);
}

