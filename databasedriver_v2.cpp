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

#include "databasedriver_v2.hpp"

DatabaseDriver_v2::DatabaseDriver_v2(QObject* Parent)
: QObject(Parent)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Database = QSqlDatabase::addDatabase(Settings.value("driver", "QIBASE").toString());
	Settings.endGroup();
}

DatabaseDriver_v2::~DatabaseDriver_v2(void) {}

QMap<int, QStringList> DatabaseDriver_v2::getAttribList(void) const
{
	QMap<int, QStringList> List;

	for (int i = 0; i < Fields.size(); ++i) List.insert(i, QStringList());

	int i = 0; for (const auto& Field : Fields) for (const auto& Table : Tables)
	{
		if (Table.Fields.contains(Field)) List[i].append(Table.Name);
	}

	return List;
}

QMap<int, DatabaseDriver_v2::FIELD> DatabaseDriver_v2::getFilterList(void) const
{
	QMap<int, FIELD> List; int i = 0;

	for (const auto& Field : Fields)
	{
		if ((Field.Type != FOREGIN && Field.Type != MASK) || Field.Dict.size() > 1) List.insert(i, Field); ++i;
	}

	return List;
}

QList<DatabaseDriver_v2::FIELD> DatabaseDriver_v2::loadCommon(void) const
{
	if (!Database.isOpen()) return QList<FIELD>();

	QList<FIELD> Fields =
	{
		{ INT,		"EW_OBIEKTY.UID",		tr("Database ID")		},
		{ FOREGIN,	"EW_OBIEKTY.OPERAT",	tr("Job name")			},
		{ READONLY,	"EW_OBIEKTY.KOD",		tr("Object code")		},
		{ READONLY, 	"EW_OB_OPISY.OPIS",		tr("Code description")	},
		{ READONLY, 	"EW_OBIEKTY.NUMER",		tr("Object ID")		},
		{ DATE,		"EW_OBIEKTY.DTU",		tr("Creation date")		},
		{ DATE, 		"EW_OBIEKTY.DTW",		tr("Modification date")	}
	};

	QMap<QString, QString> Dict =
	{
		{ "EW_OBIEKTY.OPERAT",		"SELECT UID, NUMER FROM EW_OPERATY ORDER BY NUMER" }
	};

	for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		QSqlQuery Query(i.value(), Database);

		auto& Ref = getFieldByName(Fields, i.key()).Dict;

		if (Query.exec()) while (Query.next())
		{
			Ref.insert(
				Query.value(0).toInt(),
				Query.value(1).toString()
			);
		}
	}

	return Fields;
}

QList<DatabaseDriver_v2::TABLE> DatabaseDriver_v2::loadTables(void) const
{
	if (!Database.isOpen()) return QList<TABLE>();

	QSqlQuery Query(Database);
	QList<TABLE> List;

	Query.prepare(
		"SELECT "
			"EW_OB_OPISY.KOD, EW_OB_OPISY.OPIS, EW_OB_OPISY.DANE_DOD "
		"FROM "
			"EW_OB_OPISY");

	if (Query.exec()) while (Query.next())
	{
		const QString Data = Query.value(0).toString();

		List.append(
		{
			Query.value(0).toString(),
			Query.value(1).toString(),
			Query.value(2).toString(),
			loadFields(Data)
		});
	}

	return List;
}

QList<DatabaseDriver_v2::FIELD> DatabaseDriver_v2::loadFields(const QString& Table) const
{
	if (!Database.isOpen()) return QList<FIELD>();

	QSqlQuery Query(Database);
	QList<FIELD> List;

	Query.prepare(
		"SELECT "
			"EW_OB_DDSTR.NAZWA, EW_OB_DDSTR.TYTUL, EW_OB_DDSTR.TYP "
		"FROM "
			"EW_OB_DDSTR "
		"WHERE "
			"EW_OB_DDSTR.KOD = :table "
		"ORDER BY "
			"EW_OB_DDSTR.TYTUL");

	Query.bindValue(":table", Table);

	if (Query.exec()) while (Query.next())
	{
		const QString Data = Query.value(0).toString();

		List.append(
		{
			TYPE(Query.value(2).toInt()),
			Query.value(0).toString(),
			Query.value(1).toString(),
			loadDict(Data, Table)
		});
	}

	return List;
}

QMap<int, QString> DatabaseDriver_v2::loadDict(const QString& Field, const QString& Table) const
{
	if (!Database.isOpen()) return QMap<int, QString>();

	QSqlQuery Query(Database);
	QMap<int, QString> List;

	Query.prepare(
		"SELECT "
			"EW_OB_DDSL.WARTOSC, EW_OB_DDSL.OPIS "
		"FROM "
		    "EW_OB_DDSL "
		"INNER JOIN "
		    "EW_OB_DDSTR "
		"ON "
		    "EW_OB_DDSL.UIDP = EW_OB_DDSTR.UIDSL OR EW_OB_DDSL.UIDP = EW_OB_DDSTR.UID "
		"WHERE "
		    "EW_OB_DDSTR.NAZWA = :field AND EW_OB_DDSTR.KOD = :table "
		"ORDER BY "
			"EW_OB_DDSL.OPIS");

	Query.bindValue(":field", Field);
	Query.bindValue(":table", Table);

	if (Query.exec()) while (Query.next())
	{
		List.insert(
			Query.value(0).toInt(),
			Query.value(1).toString()
		);
	}

	return List;
}

QList<DatabaseDriver_v2::FIELD> DatabaseDriver_v2::normalizeFields(QList<DatabaseDriver_v2::TABLE>& Tabs, const QList<DatabaseDriver_v2::FIELD>& Base) const
{
	QList<FIELD> List;

	for (const auto& Field : Base)
	{
		if (!List.contains(Field)) List.append(Field);
	}

	for (const auto& Tab : Tabs) for (const auto& Field : Tab.Fields)
	{
		if (!List.contains(Field)) List.append(Field);
	}

	QtConcurrent::blockingMap(Tabs, [&List] (TABLE& Tab) -> void
	{
		for (auto& Field : Tab.Fields) Tab.Indexes.append(List.indexOf(Field));
	});

	return List;
}

QStringList DatabaseDriver_v2::normalizeHeaders(QList<DatabaseDriver_v2::TABLE>& Tabs, const QList<DatabaseDriver_v2::FIELD>& Base) const
{
	QStringList List;

	for (const auto& Field : Base)
	{
		if (!List.contains(Field.Label)) List.append(Field.Label);
	}

	for (const auto& Tab : Tabs) for (const auto& Field : Tab.Fields)
	{
		if (!List.contains(Field.Label)) List.append(Field.Label);
	}

	QtConcurrent::blockingMap(Tabs, [&List] (TABLE& Tab) -> void
	{
		for (auto& Field : Tab.Fields) Tab.Headers.append(List.indexOf(Field.Name));
	});

	return List;
}

bool DatabaseDriver_v2::openDatabase(const QString& Server, const QString& Base, const QString& User, const QString& Pass)
{
	if (Database.isOpen()) Database.close();

	Database.setHostName(Server);
	Database.setDatabaseName(Base);
	Database.setUserName(User);
	Database.setPassword(Pass);

	if (Database.open())
	{
		emit onBeginProgress(tr("Loading database informations"));
		emit onSetupProgress(0, 4);
		emit onUpdateProgress(0);

		Common = loadCommon(); emit onUpdateProgress(1);
		Tables = loadTables(); emit onUpdateProgress(2);

		Fields = normalizeFields(Tables, Common); emit onUpdateProgress(3);
		Headers = normalizeHeaders(Tables, Common); emit onUpdateProgress(4);

		emit onEndProgress();
		emit onConnect(getFilterList(), getAttribList(), Headers);
	}
	else emit onError(Database.lastError().text());

	return Database.isOpen();
}

bool DatabaseDriver_v2::closeDatabase(void)
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

bool operator == (const DatabaseDriver_v2::FIELD& One, const DatabaseDriver_v2::FIELD& Two)
{
	return
	(
		One.Type == Two.Type &&
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Dict == Two.Dict
	);
}

bool operator == (const DatabaseDriver_v2::TABLE& One, const DatabaseDriver_v2::TABLE& Two)
{
	return
	(
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Data == Two.Data &&
		One.Fields == Two.Fields
	);
}

DatabaseDriver_v2::FIELD& getFieldByName(QList<DatabaseDriver_v2::FIELD>& Fields, const QString& Name)
{
	static DatabaseDriver_v2::FIELD Dummy = DatabaseDriver_v2::FIELD();

	for (auto& Field : Fields) if (Field.Name == Name) return Field;

	return Dummy;
}
