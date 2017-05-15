/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Item selector interface program for EW-Database                        *
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

#include <QCoreApplication>
#include <QUdpSocket>
#include <QTimer>
#include <QFile>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QFile File(argv[1]);
	QUdpSocket Socket;

	File.open(QFile::ReadOnly | QFile::Text);

	while (!File.atEnd())
	{
		const QString Line = File.readLine();

		if (Line.startsWith(" 5")) Socket.writeDatagram(
					Line.mid(3).trimmed().toUtf8(),
					QHostAddress::LocalHost, 6666);
	}

	QTimer::singleShot(50, &a, &QCoreApplication::quit);

	return a.exec();
}
