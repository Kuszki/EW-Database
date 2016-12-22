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

#include "databasedriver.hpp"

DatabaseDriver::DatabaseDriver(QObject* Parent)
: QObject(Parent)
{
	Database = QSqlDatabase::addDatabase("QIBASE");
}

DatabaseDriver::~DatabaseDriver(void)
{

}

QMap<QString, QString> DatabaseDriver::getAttributes(const QStringList& Keys)
{
	QMap<QString, QString> Res;

	if (Database.isOpen())
	{
		QSqlQuery Query(Database);

		if (Keys.isEmpty()) Query.prepare("SELECT DISTINCT NAZWA, TYTUL FROM EW_OB_DDSTR");
		else Query.prepare(QString("SELECT DISTINCT NAZWA, TYTUL FROM EW_OB_DDSTR WHERE KOD IN ('%1')").arg(Keys.join("','")));

		if (Query.exec()) while (Query.next())
		{
			Res.insert(Query.value(0).toString(), Query.value(1).toString());
		}
	}

	return Res;
}

bool DatabaseDriver::openDatabase(const QString& Server, const QString& Base, const QString& User, const QString& Pass)
{
	if (Database.isOpen()) Database.close();

	Database.setHostName(Server);
	Database.setDatabaseName(Base);
	Database.setUserName(User);
	Database.setPassword(Pass);

	if (Database.open()) emit onConnect();
	else emit onError(Database.lastError().text());

	return Database.isOpen();
}

bool DatabaseDriver::closeDatabase(void)
{
	if (Database.isOpen())
	{
		Database.close(); emit onDisconnect(); return true;
	}
	else
	{
		emit onError(tr("Database is not opened")); return false;
	}
}

void DatabaseDriver::queryAttributes(const QStringList& Keys)
{
	if (Database.isOpen()) emit onAttributesLoad(getAttributes(Keys));
	else emit onError(tr("Database is not opened"));
}
