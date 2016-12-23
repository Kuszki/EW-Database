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

const QMap<QString, QString> DatabaseDriver::commonAttribs =
{
	{"EW_OBIEKTY.KOD",			tr("Object code")},
	{"EW_OBIEKTY.NUMER",		tr("Object ID")},
	{"EW_OBIEKTY.POZYSKANIE",	tr("Source of data")},
	{"EW_OBIEKTY.DTU",			tr("Creation date")},
	{"EW_OBIEKTY.DTW",			tr("Modification date")},
	{"EW_OBIEKTY.DTR",			tr("Delete date")},
	{"EW_OBIEKTY.STATUS",		tr("Object status")},
	{"EW_OPERATY.NUMER",		tr("Job name")},
	{"EW_OB_OPISY.OPIS",		tr("Code description")}
};

QStringList DatabaseDriver::getAttribTables(void)
{
	if (!Database.isOpen()) return QStringList();

	QSqlQuery Query(Database); QStringList List;

	if (Query.exec("SELECT DISTINCT DANE_DOD FROM EW_OB_OPISY")) while (Query.next())
	{
		List.append(Query.value(0).toString());
	}

	return List;
}

QStringList DatabaseDriver::getTableFields(const QString& Table)
{
	if (!Database.isOpen()) return QStringList();

	QSqlQuery Query(Database); QStringList List;

	Query.prepare(QString(
			"SELECT "
				"EW_OB_DDSTR.NAZWA "
			"FROM "
				"EW_OB_DDSTR "
			"INNER JOIN "
				"EW_OB_OPISY "
			"ON "
				"EW_OB_DDSTR.KOD=EW_OB_OPISY.KOD "
			"WHERE "
				"EW_OB_OPISY.DANE_DOD='%1'")
			    .arg(Table));

	if (Query.exec()) while (Query.next())
	{
		List.append(Query.value(0).toString());
	}

	return List;
}

QStringList DatabaseDriver::getDataQueries(const QStringList& Tables, const QMap<QString, QString>& Map)
{
	QStringList Queries;

	for (const auto& Table : Tables)
	{
		const QStringList Fields = getTableFields(Table);

		Queries.append(QString(
			"SELECT "
				"EW_OBIEKTY.KOD, "
				"EW_OBIEKTY.NUMER, "
				"EW_OBIEKTY.POZYSKANIE, "
				"EW_OBIEKTY.DTU, "
				"EW_OBIEKTY.DTW, "
				"EW_OBIEKTY.DTR, "
				"EW_OBIEKTY.STATUS, "
				"EW_OPERATY.NUMER, "
				"EW_OB_OPISY.OPIS, "
				"%1 "
			"FROM "
				"EW_OBIEKTY "
			"LEFT JOIN "
				"EW_OPERATY "
			"ON "
				"EW_OBIEKTY.OPERAT=EW_OPERATY.UID "
			"LEFT JOIN "
				"EW_OB_OPISY "
			"ON "
				"EW_OBIEKTY.KOD=EW_OB_OPISY.KOD "
			"INNER JOIN "
				"%2 "
			"ON "
				"EW_OBIEKTY.UID=%2.UIDO")
				    .arg(Fields.join(", "))
				    .arg(Table));
	}

	return Queries;
}

DatabaseDriver::DatabaseDriver(QObject* Parent)
: QObject(Parent)
{
	Database = QSqlDatabase::addDatabase("QIBASE");
}

DatabaseDriver::~DatabaseDriver(void) {}

QMap<QString, QString> DatabaseDriver::getAttributes(const QStringList& Keys)
{
	QMap<QString, QString> Res;

	if (Database.isOpen())
	{
		QSqlQuery Query(Database); QString Text = "SELECT DISTINCT NAZWA, TYTUL FROM EW_OB_DDSTR";

		if (!Keys.isEmpty()) Text.append(QString(" WHERE KOD IN ('%1')").arg(Keys.join("','")));

		if (Query.exec(Text)) while (Query.next()) Res.insert(Query.value(0).toString(), Query.value(1).toString());
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

/* SQL Query for common data:

SELECT
    EW_OBIEKTY.KOD,
    EW_OBIEKTY.NUMER,
    EW_OBIEKTY.POZYSKANIE,
    EW_OBIEKTY.DTU,
    EW_OBIEKTY.DTW,
    EW_OBIEKTY.DTR,
    EW_OBIEKTY.STATUS,
    EW_OPERATY.NUMER,
    EW_OB_OPISY.OPIS
FROM EW_OBIEKTY
    LEFT JOIN EW_OPERATY ON EW_OBIEKTY.OPERAT=EW_OPERATY.UID
    LEFT JOIN EW_OB_OPISY ON EW_OBIEKTY.KOD=EW_OB_OPISY.KOD;

*/
