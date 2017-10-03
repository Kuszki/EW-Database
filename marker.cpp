/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Item marker interface program for EW-Database                          *
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
#include <QTextStream>
#include <QTextCodec>
#include <QUdpSocket>
#include <QDateTime>
#include <QSettings>
#include <QTimer>
#include <QFile>
#include <QDir>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);  int i = 0;
	QFile Output; QTextStream Stream(&Output);
	QUdpSocket* Socket = new QUdpSocket(&a);
	QFile File(argv[1]); QByteArray Array;
	QString Database; int Port(0);

	qsrand(QDateTime::currentMSecsSinceEpoch());
	Stream.setCodec(QTextCodec::codecForLocale());

	if (File.open(QFile::ReadOnly | QFile::Text)) while (!File.atEnd())
	{
		const QString Line = QString::fromLocal8Bit(File.readLine()); ++i;

		if (i == 2)
		{
			Output.setFileName(Line.trimmed());
			Output.open(QFile::WriteOnly | QFile::Text);
		}
		else if (i == 3)
		{
			Database = Line.trimmed();
		}
	}

	if (!Database.isEmpty())
	{
		QSettings Settings("EW-Database");
		Settings.beginGroup("Sockets");

		for (const auto& K : Settings.childGroups()) if (!Port)
		{
			if (Database.contains(K, Qt::CaseInsensitive))
			{
				Settings.beginGroup(K);
				Port = Settings.value("marker").toInt();
				Settings.endGroup();
			}
		}

		if (Port) while (!Socket->bind(QHostAddress::LocalHost, qrand() & 0xFFFF));
	}

	if (Output.isOpen())
	{
		Socket->writeDatagram(QString::number(Socket->localPort()).toUtf8(),
						  QHostAddress::LocalHost, Port);

		Stream << "0 4\nKOD\n**\n";
	}

	a.connect(Socket, &QUdpSocket::readyRead, [&Socket, &Stream, &Array, &a] (void) -> void
	{
		Array.resize(Socket->pendingDatagramSize());
		Socket->readDatagram(Array.data(), Array.size());

		Stream << Array; if (Array.endsWith("\n\n")) a.quit();
	});

	QTimer::singleShot(10000, &a, &QCoreApplication::quit); return a.exec();
}
