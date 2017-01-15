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

const QStringList DatabaseDriver_v2::Operators =
{
	"=", "<>", ">=", ">", "<=", "<",
	"LIKE", "NOT LIKE",
	"IN", "NOT IN",
	"IS NULL", "IS NOT NULL"
};

DatabaseDriver_v2::DatabaseDriver_v2(QObject* Parent)
: QObject(Parent)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Database = QSqlDatabase::addDatabase(Settings.value("driver", "QIBASE").toString());
	Settings.endGroup();
}

DatabaseDriver_v2::~DatabaseDriver_v2(void) {}

QMap<int, DatabaseDriver_v2::FIELD> DatabaseDriver_v2::getFilterList(void) const
{
	QMap<int, FIELD> List; int i = 0;

	for (const auto& Field : Fields)
	{
		if ((Field.Type != INTEGER && Field.Type != MASK) || Field.Dict.size() > 1) List.insert(i, Field); ++i;
	}

	return List;
}

QList<DatabaseDriver_v2::FIELD> DatabaseDriver_v2::loadCommon(bool Emit)
{
	if (!Database.isOpen()) return QList<FIELD>();

	QList<FIELD> Fields =
	{
		{ INTEGER,	"EW_OBIEKTY.OPERAT",	tr("Job name")			},
		{ READONLY,	"EW_OBIEKTY.KOD",		tr("Object code")		},
		{ READONLY, 	"EW_OBIEKTY.NUMER",		tr("Object ID")		},
		{ DATE,		"EW_OBIEKTY.DTU",		tr("Creation date")		},
		{ DATE, 		"EW_OBIEKTY.DTW",		tr("Modification date")	}
	};

	QMap<QString, QString> Dict =
	{
		{ "EW_OBIEKTY.OPERAT",		"SELECT UID, NUMER FROM EW_OPERATY ORDER BY NUMER"	},
		{ "EW_OBIEKTY.KOD",			"SELECT KOD, OPIS FROM EW_OB_OPISY ORDER BY OPIS"		}
	};

	if (Emit) emit onSetupProgress(0, Dict.size());

	int j = 0; for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		auto& Field = getFieldByName(Fields, i.key());

		if (Field.Name.isEmpty()) continue;

		QSqlQuery Query(i.value(), Database);

		if (Query.exec()) while (Query.next()) Field.Dict.insert(Query.value(0), Query.value(1).toString());

		if (Emit) emit onUpdateProgress(++j);
	}

	return Fields;
}

QList<DatabaseDriver_v2::TABLE> DatabaseDriver_v2::loadTables(bool Emit)
{
	if (!Database.isOpen()) return QList<TABLE>();

	QSqlQuery Query(Database);
	QList<TABLE> List;
	int Step = 0;

	if (Emit)
	{
		if (Query.exec("SELECT COUNT(*) FROM EW_OB_OPISY") && Query.next())
		{
			emit onSetupProgress(0, Query.value(0).toInt());
		}
	}

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

		if (Emit) emit onUpdateProgress(++Step);
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
			"EW_OB_DDSTR.KOD = :table");

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

QMap<QVariant, QString> DatabaseDriver_v2::loadDict(const QString& Field, const QString& Table) const
{
	if (!Database.isOpen()) return QMap<QVariant, QString>();

	QSqlQuery Query(Database);
	QMap<QVariant, QString> List;

	Query.prepare(
		"SELECT "
			"EW_OB_DDSL.WARTOSC, EW_OB_DDSL.OPIS "
		"FROM "
		    "EW_OB_DDSL "
		"INNER JOIN "
		    "EW_OB_DDSTR "
		"ON "
		    "EW_OB_DDSL.UIDP = EW_OB_DDSTR.UID "
		"WHERE "
		    "EW_OB_DDSTR.NAZWA = :field AND EW_OB_DDSTR.KOD = :table");

	Query.bindValue(":field", Field);
	Query.bindValue(":table", Table);

	if (Query.exec()) while (Query.next()) List.insert(Query.value(0), Query.value(1).toString());

	Query.prepare(
		"SELECT "
			"EW_OB_DDSL.WARTOSC, EW_OB_DDSL.OPIS "
		"FROM "
		    "EW_OB_DDSL "
		"INNER JOIN "
		    "EW_OB_DDSTR "
		"ON "
		    "EW_OB_DDSL.UIDP = EW_OB_DDSTR.UIDSL "
		"WHERE "
		    "EW_OB_DDSTR.NAZWA = :field AND EW_OB_DDSTR.KOD = :table");

	Query.bindValue(":field", Field);
	Query.bindValue(":table", Table);

	if (Query.exec()) while (Query.next()) List.insert(Query.value(0), Query.value(1).toString());

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
		for (auto& Field : Tab.Fields) Tab.Headers.append(List.indexOf(Field.Label));
	});

	return List;
}

QMap<QString, QStringList> DatabaseDriver_v2::getDeleteGroups(const QList<int>& Indexes) const
{
	if (!Database.isOpen()) return QMap<QString, QStringList>();

	QMap<QString, QStringList> List;
	QSqlQuery Query(Database);

	List.insert("EW_OBIEKTY", QStringList());

	for (const auto& ID : Indexes)
	{
		Query.prepare(QString(
			"SELECT "
				"EW_OB_OPISY.DANE_DOD "
			"FROM "
				"EW_OB_OPISY "
			"INNER JOIN "
				"EW_OBIEKTY "
			"ON "
				"EW_OB_OPISY.KOD = EW_OBIEKTY.KOD "
			"WHERE "
				"EW_OBIEKTY.UID = :id"));

		Query.bindValue(":id", ID);

		if (Query.exec() && Query.next())
		{
			const QString Table = Query.value(0).toString();

			if (!List.contains(Table)) List.insert(Table, QStringList());

			List["EW_OBIEKTY"].append(QString::number(ID));
			List[Table].append(QString::number(ID));
		}
	}

	return List;
}

QList<int> DatabaseDriver_v2::getUsedFields(const QString& Filter) const
{
	if (Filter.isEmpty()) return QList<int>(); QList<int> Used;

	QRegExp Exp(QString("\\b(\\S+)\\b\\s*(?:%1)").arg(Operators.join('|')));

	int i = 0; while ((i = Exp.indexIn(Filter, i)) != -1)
	{
		const QString Field = Exp.capturedTexts().last();
		const auto& Ref = getFieldByName(Fields, Field);

		Used.append(Fields.indexOf(Ref));

		i += Exp.matchedLength();
	}

	return Used;
}

bool DatabaseDriver_v2::hasAllIndexes(const DatabaseDriver_v2::TABLE& Tab, const QList<int>& Used)
{
	for (const auto& Index : Used) if (Index >= Common.size())
	{
		if (!Tab.Indexes.contains(Index)) return false;
	}

	return true;
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
		emit onBeginProgress(tr("Loading database informations")); emit onLogin(true);

		Common = loadCommon(false);
		Tables = loadTables(true);

		Fields = normalizeFields(Tables, Common);
		Headers = normalizeHeaders(Tables, Common);

		emit onEndProgress(); emit onConnect(Fields, Tables, Headers);
	}
	else
	{
		emit onError(Database.lastError().text()); emit onLogin(false);
	}

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

void DatabaseDriver_v2::updateData(const QString& Filter, QList<int> Used)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	if (Used.isEmpty()) Used = getUsedFields(Filter);

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	RecordModel* Model = new RecordModel(Headers, this); int Step = 0;

	for (const auto& Table : Tables) if (hasAllIndexes(Table, Used))
	{
		QSqlQuery Query(Database); QStringList Attribs;

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(QString("EW_DATA.%1").arg(Field.Name));

		QString Exec = QString(
			"SELECT "
				"EW_OBIEKTY.UID, %1 "
			"FROM "
				"EW_OBIEKTY "
			"INNER JOIN "
				"%2 EW_DATA "
			"ON "
				"EW_OBIEKTY.UID = EW_DATA.UIDO "
			"WHERE "
				"EW_OBIEKTY.STATUS = 0")
				    .arg(Attribs.join(", "))
				    .arg(Table.Data);

		if (!Filter.isEmpty()) Exec.append(QString(" AND (%1)").arg(Filter));

		if (Query.exec(Exec)) while (Query.next())
		{
			QMap<int, QVariant> Values; int i = 1;

			const int Index = Query.value(0).toInt();

			for (int j = 0; j < Common.size(); ++j)
			{
				const QVariant Value = Query.value(i++);

				if (Common[j].Dict.isEmpty()) Values.insert(j, Value);
				else Values.insert(j, Common[j].Dict[Value]);
			}

			for (const auto ID : Table.Headers)
			{
				const QVariant Value = Query.value(i++);

				if (Fields[ID].Dict.isEmpty()) Values.insert(ID, Value);
				else Values.insert(ID, Fields[ID].Dict[Value]);
			}

			if (!Values.isEmpty()) Model->addItem(Index, Values);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver_v2::removeData(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRemove(); return; }

	QMap<QString, QStringList> Tasks = getDeleteGroups(Model->getUids(Items));
	QSqlQuery Query(Database); int Step = 0;

	emit onBeginProgress(tr("Removing data"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		Query.exec(QString(
			"DELETE FROM "
				"%1 "
			"WHERE "
				"UID IN ('%2')")
				 .arg(i.key())
				 .arg(i.value().join("', '")));

		emit onUpdateProgress(++Step);
	}

	for (const auto Item : Items) Model->removeItem(Item);

	emit onEndProgress();
	emit onDataRemove();
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

const DatabaseDriver_v2::FIELD& getFieldByName(const QList<DatabaseDriver_v2::FIELD>& Fields, const QString& Name)
{
	static DatabaseDriver_v2::FIELD Dummy = DatabaseDriver_v2::FIELD();

	for (auto& Field : Fields) if (Field.Name == Name) return Field;

	return Dummy;
}
