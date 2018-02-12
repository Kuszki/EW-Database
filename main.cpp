/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the  Free Software Foundation, either  version 3 of the  License, or   *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This  program  is  distributed  in the hope  that it will be useful,   *
 *  but WITHOUT ANY  WARRANTY;  without  even  the  implied  warranty of   *
 *  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have  received a copy  of the  GNU General Public License   *
 *  along with this program. If not, see http://www.gnu.org/licenses/.     *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>

#include "databasedriver.hpp"
#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	qRegisterMetaType<QList<DatabaseDriver::FIELD>>("QList<FIELD>");
	qRegisterMetaType<QList<DatabaseDriver::TABLE>>("QList<TABLE>");

	qRegisterMetaType<QList<QPair<int,BatchWidget::FUNCTION>>>("QList<QPair<int,BatchWidget::FUNCTION>>");
	qRegisterMetaType<QHash<QString,QHash<int,QString>>>("QHash<QString,QHash<int,QString>>");
	qRegisterMetaType<QHash<int,QHash<int,QVariant>>>("QHash<int,QHash<int,QVariant>>");
	qRegisterMetaType<QHash<QString,QSet<QString>>>("QHash<QString,QSet<QString>>");
	qRegisterMetaType<QList<QHash<int,QVariant>>>("QList<QHash<int,QVariant>>");
	qRegisterMetaType<QList<QMap<int,QVariant>>>("QList<QMap<int,QVariant>>");
	qRegisterMetaType<QHash<QString,QString>>("QHash<QString,QString>");
	qRegisterMetaType<QMap<QString,QString>>("QMap<QString,QString>");
	qRegisterMetaType<QList<QStringList>>("QList<QStringList>");
	qRegisterMetaType<QModelIndexList>("QModelIndexList");
	qRegisterMetaType<QHash<int,int>>("QHash<int,int>");
	qRegisterMetaType<QVector<int>>("QVector<int>");
	qRegisterMetaType<QList<int>>("QList<int>");

	a.setApplicationName("EW-Database");
	a.setOrganizationName("Łukasz \"Kuszki\" Dróżdż");
	a.setOrganizationDomain("https://github.com/Kuszki");
	a.setApplicationVersion("1.0");

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&qtTranslator);

	QTranslator baseTranslator;
	baseTranslator.load("qtbase_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&baseTranslator);

	QTranslator appTranslator;
	appTranslator.load("ew-database_" + QLocale::system().name());
	a.installTranslator(&appTranslator);

	MainWindow w;
	w.show();

	return a.exec();
}
