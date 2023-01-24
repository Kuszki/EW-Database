#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#*                                                                         *
#*  Edit Firebird database for EWMAPA software                             *
#*  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@o2.pl             *
#*                                                                         *
#*  This program is free software: you can redistribute it and/or modify   *
#*  it under the terms of the GNU General Public License as published by   *
#*  the  Free Software Foundation, either  version 3 of the  License, or   *
#*  (at your option) any later version.                                    *
#*                                                                         *
#*  This  program  is  distributed  in the hope  that it will be useful,   *
#*  but WITHOUT ANY  WARRANTY;  without  even  the  implied  warranty of   *
#*  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   *
#*  GNU General Public License for more details.                           *
#*                                                                         *
#*  You should have  received a copy  of the  GNU General Public License   *
#*  along with this program. If not, see http://www.gnu.org/licenses/.     *
#*                                                                         *
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

QT			+=	core gui widgets concurrent sql network qml

TARGET		=	EW-Database
TEMPLATE		=	app

CONFIG		+=	c++21

SOURCES		+=	main.cpp \
				jshighlighter.cpp \
				mainwindow.cpp \
				connectdialog.cpp \
				databasedriver.cpp \
				columnsdialog.cpp \
				groupdialog.cpp \
				filterdialog.cpp \
				filterwidget.cpp \
				progresswidget.cpp \
				queryhlighter.cpp \
				queryeditor.cpp \
				recordmodel.cpp \
				aboutdialog.cpp \
				sqleditordialog.cpp \
				sqlhighlighter.cpp \
				updatedialog.cpp \
				updatewidget.cpp \
				joindialog.cpp \
				exportdialog.cpp \
				geometrywidget.cpp \
				classdialog.cpp \
				textdialog.cpp \
				mergedialog.cpp \
				cutdialog.cpp \
				batchdialog.cpp \
				batchwidget.cpp \
				labeldialog.cpp \
				harmonizedialog.cpp \
				insertdialog.cpp \
				variablesdialog.cpp \
				redactionwidget.cpp \
				kergdialog.cpp \
				selectordialog.cpp \
				copyfieldswidget.cpp \
				copyfieldsdialog.cpp \
				scriptdialog.cpp \
				breaksdialog.cpp \
				settingsdialog.cpp \
				edgesdialog.cpp

HEADERS		+=	mainwindow.hpp \
				commontemplates.hpp \
				connectdialog.hpp \
				databasedriver.hpp \
				columnsdialog.hpp \
				groupdialog.hpp \
				filterdialog.hpp \
				filterwidget.hpp \
				jshighlighter.hpp \
				progresswidget.hpp \
				queryhlighter.hpp \
				queryeditor.hpp \
				recordmodel.hpp \
				aboutdialog.hpp \
				sqleditordialog.hpp \
				sqlhighlighter.hpp \
				updatedialog.hpp \
				updatewidget.hpp \
				joindialog.hpp \
				exportdialog.hpp \
				geometrywidget.hpp \
				classdialog.hpp \
				textdialog.hpp \
				mergedialog.hpp \
				cutdialog.hpp \
				batchdialog.hpp \
				batchwidget.hpp \
				labeldialog.hpp \
				harmonizedialog.hpp \
				insertdialog.hpp \
				variablesdialog.hpp \
				redactionwidget.hpp \
				kergdialog.hpp \
				selectordialog.hpp \
				copyfieldswidget.hpp \
				copyfieldsdialog.hpp \
				scriptdialog.hpp \
				breaksdialog.hpp \
				settingsdialog.hpp \
				edgesdialog.hpp

FORMS		+=	mainwindow.ui \
				connectdialog.ui \
				columnsdialog.ui \
				groupdialog.ui \
				filterdialog.ui \
				filterwidget.ui \
				aboutdialog.ui \
				sqleditordialog.ui \
				updatedialog.ui \
				updatewidget.ui \
				joindialog.ui \
				exportdialog.ui \
				geometrywidget.ui \
				classdialog.ui \
				textdialog.ui \
				mergedialog.ui \
				cutdialog.ui \
				batchdialog.ui \
				batchwidget.ui \
				labeldialog.ui \
				harmonizedialog.ui \
				insertdialog.ui \
				variablesdialog.ui \
				redactionwidget.ui \
				kergdialog.ui \
				selectordialog.ui \
				copyfieldswidget.ui \
				copyfieldsdialog.ui \
				scriptdialog.ui \
				breaksdialog.ui \
				settingsdialog.ui \
				edgesdialog.ui

RESOURCES		+=	resources.qrc

TRANSLATIONS	+=	ew-database_pl.ts
