#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#*                                                                         *
#*  Edit Firebird database for EWMAPA software                             *
#*  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
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

QT			+=	core gui widgets concurrent sql network

TARGET		=	EW-Database
TEMPLATE		=	app

CONFIG		+=	c++14

SOURCES		+=	main.cpp \
				mainwindow.cpp \
				connectdialog.cpp \
				databasedriver.cpp \
				columnsdialog.cpp \
				groupdialog.cpp \
				filterdialog.cpp \
				filterwidget.cpp \
				queryhlighter.cpp \
				queryeditor.cpp \
				recordmodel.cpp \
				aboutdialog.cpp \
				updatedialog.cpp \
				updatewidget.cpp \
				joindialog.cpp \
				exportdialog.cpp \
				geometrywidget.cpp \
				classdialog.cpp \
				textdialog.cpp \
				mergedialog.cpp

HEADERS		+=	mainwindow.hpp \
				connectdialog.hpp \
				databasedriver.hpp \
				columnsdialog.hpp \
				groupdialog.hpp \
				filterdialog.hpp \
				filterwidget.hpp \
				queryhlighter.hpp \
				queryeditor.hpp \
				recordmodel.hpp \
				aboutdialog.hpp \
				updatedialog.hpp \
				updatewidget.hpp \
				joindialog.hpp \
				exportdialog.hpp \
				geometrywidget.hpp \
				classdialog.hpp \
				textdialog.hpp \
				mergedialog.hpp

FORMS		+=	mainwindow.ui \
				connectdialog.ui \
				columnsdialog.ui \
				groupdialog.ui \
				filterdialog.ui \
				filterwidget.ui \
				aboutdialog.ui \
				updatedialog.ui \
				updatewidget.ui \
				joindialog.ui \
				exportdialog.ui \
				geometrywidget.ui \
				classdialog.ui \
				textdialog.ui \
				mergedialog.ui

RESOURCES		+=	resources.qrc

TRANSLATIONS	+=	ew-database_pl.ts

QMAKE_CXXFLAGS	+=	-std=c++14 -march=native
