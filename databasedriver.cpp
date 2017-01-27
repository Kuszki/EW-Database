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

const QStringList DatabaseDriver::Operators =
{
	"=", "<>", ">=", ">", "<=", "<",
	"LIKE", "NOT LIKE",
	"IN", "NOT IN",
	"IS NULL", "IS NOT NULL"
};

DatabaseDriver::DatabaseDriver(QObject* Parent)
: QObject(Parent)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Database = QSqlDatabase::addDatabase(Settings.value("driver", "QIBASE").toString());
	Settings.endGroup();
}

DatabaseDriver::~DatabaseDriver(void) {}

QList<DatabaseDriver::FIELD> DatabaseDriver::loadCommon(bool Emit)
{
	if (!Database.isOpen()) return QList<FIELD>();

	QList<FIELD> Fields =
	{
		{ INTEGER,	"EW_OBIEKTY.OPERAT",	tr("Job name")			},
		{ READONLY,	"EW_OBIEKTY.KOD",		tr("Object code")		},
		{ READONLY,	"EW_OBIEKTY.NUMER",		tr("Object ID")		},
		{ DATETIME,	"EW_OBIEKTY.DTU",		tr("Creation date")		},
		{ DATETIME,	"EW_OBIEKTY.DTW",		tr("Modification date")	},
		{ INTEGER,	"EW_OBIEKTY.OSOU",		tr("Created by")		},
		{ INTEGER,	"EW_OBIEKTY.OSOW",		tr("Modified by")		}
	};

	QMap<QString, QString> Dict =
	{
		{ "EW_OBIEKTY.OPERAT",		"SELECT UID, NUMER FROM EW_OPERATY ORDER BY NUMER"	},
		{ "EW_OBIEKTY.KOD",			"SELECT KOD, OPIS FROM EW_OB_OPISY ORDER BY OPIS"		},
		{ "EW_OBIEKTY.OSOU",		"SELECT ID, NAME FROM EW_USERS ORDER BY NAME"		},
		{ "EW_OBIEKTY.OSOW",		"SELECT ID, NAME FROM EW_USERS ORDER BY NAME"		}
	};

	if (Emit) emit onSetupProgress(0, Dict.size());

	int j = 0; for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		auto& Field = getItemByField(Fields, i.key(), &FIELD::Name);

		if (Field.Name.isEmpty()) continue;

		QSqlQuery Query(i.value(), Database); Query.setForwardOnly(true);

		if (Query.exec()) while (Query.next()) Field.Dict.insert(Query.value(0), Query.value(1).toString());

		if (Emit) emit onUpdateProgress(++j);
	}

	return Fields;
}

QList<DatabaseDriver::TABLE> DatabaseDriver::loadTables(bool Emit)
{
	if (!Database.isOpen()) return QList<TABLE>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QList<TABLE> List; int Step = 0;

	if (Emit && Query.exec("SELECT COUNT(*) FROM EW_OB_OPISY") && Query.next())
	{
		emit onSetupProgress(0, Query.value(0).toInt());
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

QList<DatabaseDriver::FIELD> DatabaseDriver::loadFields(const QString& Table) const
{
	if (!Database.isOpen()) return QList<FIELD>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
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
			QString("EW_DATA.%1").arg(Query.value(0).toString()),
			Query.value(1).toString(),
			loadDict(Data, Table)
		});
	}

	return List;
}

QMap<QVariant, QString> DatabaseDriver::loadDict(const QString& Field, const QString& Table) const
{
	if (!Database.isOpen()) return QMap<QVariant, QString>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
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
		    "EW_OB_DDSTR.NAZWA = :field AND EW_OB_DDSTR.KOD = :table "
		"ORDER BY "
			"EW_OB_DDSL.OPIS");

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
		    "EW_OB_DDSTR.NAZWA = :field AND EW_OB_DDSTR.KOD = :table "
		"ORDER BY "
			"EW_OB_DDSL.OPIS");

	Query.bindValue(":field", Field);
	Query.bindValue(":table", Table);

	if (Query.exec()) while (Query.next()) List.insert(Query.value(0), Query.value(1).toString());

	return List;
}

QList<DatabaseDriver::FIELD> DatabaseDriver::normalizeFields(QList<DatabaseDriver::TABLE>& Tabs, const QList<DatabaseDriver::FIELD>& Base) const
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

QStringList DatabaseDriver::normalizeHeaders(QList<DatabaseDriver::TABLE>& Tabs, const QList<DatabaseDriver::FIELD>& Base) const
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

QMap<QString, QList<int>> DatabaseDriver::getClassGroups(const QList<int>& Indexes, bool Common, int Index)
{
	if (!Database.isOpen()) return QMap<QString, QList<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMap<QString, QList<int>> List; int Step = 0;

	emit onBeginProgress(tr("Preparing queries"));
	emit onSetupProgress(0, Indexes.count());

	if (Common) List.insert("EW_OBIEKTY", QList<int>());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, "
			"EW_OB_OPISY.KOD, EW_OB_OPISY.DANE_DOD "
		"FROM "
			"EW_OB_OPISY "
		"INNER JOIN "
			"EW_OBIEKTY "
		"ON "
			"EW_OB_OPISY.KOD = EW_OBIEKTY.KOD "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0");

	if (Query.exec()) while (Query.next()) if (Indexes.contains(Query.value(0).toInt()))
	{
		const QString Table = Query.value(Index + 1).toString();
		const int ID = Query.value(0).toInt();

		if (!List.contains(Table)) List.insert(Table, QList<int>());

		List[Table].append(ID);

		if (Common) List["EW_OBIEKTY"].append(ID);

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); return List;
}

QList<int> DatabaseDriver::getUsedFields(const QString& Filter) const
{
	if (Filter.isEmpty()) return QList<int>(); QList<int> Used;

	QRegExp Exp(QString("\\b(\\S+)\\b\\s*(?:%1)").arg(Operators.join('|')));

	int i = 0; while ((i = Exp.indexIn(Filter, i)) != -1)
	{
		const QString Field = Exp.capturedTexts().last();
		const auto& Ref = getItemByField(Fields, Field, &FIELD::Name);

		Used.append(Fields.indexOf(Ref));

		i += Exp.matchedLength();
	}

	return Used;
}

QList<int> DatabaseDriver::getCommonFields(const QStringList& Classes) const
{
	QSet<int> Disabled, All;

	for (const auto& Class : Classes) All.unite(getItemByField(Tables, Class, &TABLE::Data).Indexes.toSet());
	for (const auto& Class : Classes) Disabled.unite(QSet<int>(All).subtract(getItemByField(Tables, Class, &TABLE::Data).Indexes.toSet()));

	All.subtract(Disabled).toList();

	for (int i = 0; i < Common.size(); ++i) All.insert(i);

	return All.toList();
}

bool DatabaseDriver::hasAllIndexes(const DatabaseDriver::TABLE& Tab, const QList<int>& Used)
{
	for (const auto& Index : Used) if (Index >= Common.size())
	{
		if (!Tab.Indexes.contains(Index)) return false;
	}

	return true;
}

bool DatabaseDriver::openDatabase(const QString& Server, const QString& Base, const QString& User, const QString& Pass)
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

		emit onEndProgress(); emit onConnect(Fields, Tables, Headers, Common.size());
	}
	else
	{
		emit onError(Database.lastError().text()); emit onLogin(false);
	}

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

void DatabaseDriver::reloadData(const QString& Filter, QList<int> Used)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	if (Used.isEmpty()) Used = getUsedFields(Filter);

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	RecordModel* Model = new RecordModel(Headers, this); int Step = 0;

	for (const auto& Table : Tables) if (hasAllIndexes(Table, Used))
	{
		QSqlQuery Query(Database); QStringList Attribs; Query.setForwardOnly(true);

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

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
				Values.insert(j, getDataFromDict(Query.value(i++), Common[j].Dict, Common[j].Type));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Values.insert(Table.Headers[j], getDataFromDict(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
			}

			if (!Values.isEmpty()) Model->addItem(Index, Values);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::updateData(RecordModel* Model, const QModelIndexList& Items, const QMap<int, QVariant>& Values)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataUpdate(); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 1);
	const QList<int> Used = Values.keys(); int Step = 0; QStringList All;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	for (int i = 0; i < Common.size(); ++i) if (Values.contains(i))
	{
		All.append(QString("%1 = '%2'").arg(Common[i].Name).arg(Values[i].toString()));
	}

	emit onBeginProgress(tr("Updating common data"));
	emit onSetupProgress(0, Tasks.first().size());

	if (!All.isEmpty()) for (const auto& Index : Tasks.first())
	{
		Query.exec(QString(
			"UPDATE "
				"EW_OBIEKTY "
			"SET "
				"%1 "
			"WHERE "
				"UID = '%2'")
				 .arg(All.join(", "))
				 .arg(Index));

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating special data"));
	emit onSetupProgress(0, Tasks.size() - 1);

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		QStringList Updates;

		for (const auto& Index : Used) if (Table.Indexes.contains(Index))
		{
			Updates.append(QString("%1 = '%2'").arg(Fields[Index].Name).arg(Values[Index].toString()));
		}

		if (!Updates.isEmpty()) for (const auto& Index : i.value())
		{
			Query.exec(QString(
				"UPDATE "
					"%1 EW_DATA "
				"SET "
					"%2 "
				"WHERE "
					"EW_DATA.UIDO = '%3'")
					 .arg(i.key())
					 .arg(Updates.join(", "))
					 .arg(Index));
			qDebug() << Query.lastQuery();
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size() - 1);

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		QStringList Attribs;

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

		Query.prepare(QString(
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
				    .arg(Table.Data));

		if (Query.exec()) while (Query.next()) if (i.value().contains(Query.value(0).toInt()))
		{
			QMap<int, QVariant> Values; int i = 1;

			const int Index = Query.value(0).toInt();

			for (int j = 0; j < Common.size(); ++j)
			{
				Values.insert(j, getDataFromDict(Query.value(i++), Common[j].Dict, Common[j].Type));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Values.insert(Table.Headers[j], getDataFromDict(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
			}

			if (!Values.isEmpty()) Model->setData(Index, Values);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataUpdate();
}

void DatabaseDriver::removeData(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRemove(); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 1);
	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0;

	emit onBeginProgress(tr("Removing data"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		for (const auto& Index : i.value())
		{
			Query.exec(QString(
				"DELETE FROM "
					"EW_OBIEKTY "
				"WHERE "
					"UID = '%1'")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"%1 "
				"WHERE "
					"UIDO = '%2'")
					 .arg(i.key())
					 .arg(Index));
		}

		emit onUpdateProgress(++Step);
	}

	for (const auto Item : Items) Model->removeItem(Item);

	emit onEndProgress();
	emit onDataRemove();
}

void DatabaseDriver::splitData(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& From, int Type)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataSplit(0); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);
	QList<int> Points; QMap<int, QList<int>> Objects; int Step = 0; int Count = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	Type = Type == 0 ? 2 : Type == 1 ? 4 : Type == 2 ? 3 : 0;

	if (!Tasks.contains(Point) || !Tasks.contains(From)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, EW_OBIEKTY.ID "
		"FROM "
			"EW_OBIEKTY "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 4 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Point);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Point].contains(Query.value(0).toInt()))
		{
			Points.append(Query.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks[From].size());

	Query.prepare(
		"SELECT "
			"EW_OB_ELEMENTY.UIDO, EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = :typ AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":typ", Type);
	Query.bindValue(":kod", From);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[From].contains(Query.value(0).toInt()))
		{
			const int ID = Query.value(0).toInt();

			if (!Objects.contains(ID)) Objects.insert(ID, QList<int>());

			Objects[ID].append(Query.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Splitting data"));
	emit onSetupProgress(0, Objects.size());

	for (auto i = Objects.constBegin(); i != Objects.constEnd(); ++i)
	{
		for (const auto& Index : i.value()) if (Points.contains(Index))
		{
			Query.exec(QString(
				"DELETE FROM "
					"EW_OB_ELEMENTY "
				"WHERE "
					"UIDO = '%1' AND "
					"IDE = '%2'")
					 .arg(i.key())
					 .arg(Index));

			Count += 1;
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataSplit(Count);
}

void DatabaseDriver::joinCircles(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& Circle, bool Override)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataJoin(0); return; }

	struct POINT { int ID; double X; double Y; }; int Step = 0; int Count = 0;

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);
	QList<POINT> Points; QList<int> Joined; QMap<int, QSet<int>> Geometry, Insert;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	if (!Tasks.contains(Point) || !Tasks.contains(Circle)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Checking used items"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT DISTINCT "
			"EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OB_ELEMENTY "
		"WHERE "
			"EW_OB_ELEMENTY.TYP = 1");

	if (Query.exec()) while (Query.next())
	{
		Joined.append(Query.value(0).toInt());
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, EW_OBIEKTY.ID, "
			"EW_TEXT.POS_X, EW_TEXT.POS_Y "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"INNER "
			"JOIN EW_TEXT "
		"ON "
			"EW_OB_ELEMENTY.IDE = EW_TEXT.ID "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 4 AND "
			"EW_OB_ELEMENTY.N = 0 AND "
			"EW_TEXT.STAN_ZMIANY = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Point);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Point].contains(Query.value(0).toInt()))
		{
			if (!Joined.contains(Query.value(1).toInt())) Points.append(
			{
				Query.value(1).toInt(),
				Query.value(2).toDouble(),
				Query.value(3).toDouble()
			});
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks[Circle].size());

	Query.prepare(
		"SELECT "
			"EW_OB_ELEMENTY.UIDO, EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"WHERE "
			"EW_OB_ELEMENTY.TYP = 1 AND "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Circle);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Circle].contains(Query.value(0).toInt()))
		{
			const int ID = Query.value(0).toInt();

			if (!Geometry.contains(ID)) Geometry.insert(ID, QSet<int>());
			if (!Insert.contains(ID)) Insert.insert(ID, QSet<int>());

			Geometry[ID].insert(Query.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, Tasks[Circle].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, "
			"EW_POLYLINE.P0_X, EW_POLYLINE.P0_Y, "
			"EW_POLYLINE.P1_X, EW_POLYLINE.P1_Y, "
			"EW_POLYLINE.P1_FLAGS "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"INNER JOIN "
			"EW_POLYLINE "
		"ON "
			"EW_OB_ELEMENTY.IDE = EW_POLYLINE.ID "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 3 AND "
			"EW_POLYLINE.STAN_ZMIANY = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Circle);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Circle].contains(Query.value(0).toInt())) for (const auto P : Points)
		{
			const bool Circle = (double(Query.value(1).toDouble() + Query.value(3).toDouble()) / 2.0) == P.X &&
							Query.value(2).toDouble() == P.Y && Query.value(4).toDouble() == P.Y;

			if (Query.value(5).toInt() == 4 && Circle)
			{
				const int ID = Query.value(0).toInt();

				if (!Geometry[ID].contains(P.ID)) Insert[ID].insert(P.ID);
			}
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Joining data"));
	emit onSetupProgress(0, Insert.size());

	for (auto i = Insert.constBegin(); i != Insert.constEnd(); ++i)
	{
		for (const auto& P : i.value())
		{
			Query.exec(QString(
				"INSERT INTO "
					"EW_OB_ELEMENTY (UIDO, IDE, TYP, N, ATRYBUT) "
				"VALUES "
					"('%1', '%2', 1, (SELECT MAX(N) FROM EW_OB_ELEMENTY WHERE UIDO = '%1') + 1, 0)")
					 .arg(i.key())
					 .arg(P));

			if (!Override) continue;

			Query.exec(QString(
				"UPDATE "
					"EW_OBIEKTY "
				"SET "
					"OPERAT = (SELECT OPERAT FROM EW_OBIEKTY WHERE UID = '%1') "
				"WHERE "
					"ID = '%2'")
					 .arg(i.key())
					 .arg(P));
		}

		Count += i.value().size();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	if (Override) for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		QStringList Attribs;

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

		Query.prepare(QString(
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
				    .arg(Table.Data));

		if (Query.exec()) while (Query.next()) if (i.value().contains(Query.value(0).toInt()))
		{
			QMap<int, QVariant> Values; int i = 1;

			const int Index = Query.value(0).toInt();

			for (int j = 0; j < Common.size(); ++j)
			{
				Values.insert(j, getDataFromDict(Query.value(i++), Common[j].Dict, Common[j].Type));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Values.insert(Table.Headers[j], getDataFromDict(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
			}

			if (!Values.isEmpty()) Model->setData(Index, Values);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataJoin(Count);
}

void DatabaseDriver::joinLines(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& Line, bool Override)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataJoin(0); return; }

	struct POINT { int ID; double X; double Y; }; int Step = 0; int Count = 0;

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);
	QList<POINT> Points; QList<int> Joined; QMap<int, QSet<int>> Geometry, Insert;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	if (!Tasks.contains(Point) || !Tasks.contains(Line)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Checking used items"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT DISTINCT "
			"EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OB_ELEMENTY "
		"WHERE "
			"EW_OB_ELEMENTY.TYP = 1");

	if (Query.exec()) while (Query.next())
	{
		Joined.append(Query.value(0).toInt());
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, EW_OBIEKTY.ID, "
			"EW_TEXT.POS_X, EW_TEXT.POS_Y "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"INNER "
			"JOIN EW_TEXT "
		"ON "
			"EW_OB_ELEMENTY.IDE = EW_TEXT.ID "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 4 AND "
			"EW_OB_ELEMENTY.N = 0 AND "
			"EW_TEXT.STAN_ZMIANY = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Point);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Point].contains(Query.value(0).toInt()))
		{
			if (!Joined.contains(Query.value(1).toInt())) Points.append(
			{
				Query.value(1).toInt(),
				Query.value(2).toDouble(),
				Query.value(3).toDouble()
			});
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks[Line].size());

	Query.prepare(
		"SELECT "
			"EW_OB_ELEMENTY.UIDO, EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"WHERE "
			"EW_OB_ELEMENTY.TYP = 1 AND "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue("kod", Line);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Line].contains(Query.value(0).toInt()))
		{
			const int ID = Query.value(0).toInt();

			if (!Geometry.contains(ID)) Geometry.insert(ID, QSet<int>());
			if (!Insert.contains(ID)) Insert.insert(ID, QSet<int>());

			Geometry[ID].insert(Query.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, Tasks[Line].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, "
			"EW_POLYLINE.P0_X, EW_POLYLINE.P0_Y, "
			"EW_POLYLINE.P1_X, EW_POLYLINE.P1_Y "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"INNER JOIN "
			"EW_POLYLINE "
		"ON "
			"EW_OB_ELEMENTY.IDE = EW_POLYLINE.ID "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 2 AND "
			"EW_POLYLINE.STAN_ZMIANY = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Line);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Line].contains(Query.value(0).toInt())) for (const auto P : Points)
		{
			if ((Query.value(1).toDouble() == P.X && Query.value(2).toDouble() == P.Y) ||
			    (Query.value(3).toDouble() == P.X && Query.value(4).toDouble() == P.Y))
			{
				const int ID = Query.value(0).toInt();

				if (!Geometry[ID].contains(P.ID)) Insert[ID].insert(P.ID);
			}
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Joining data"));
	emit onSetupProgress(0, Insert.size());

	for (auto i = Insert.constBegin(); i != Insert.constEnd(); ++i)
	{
		for (const auto& P : i.value())
		{
			Query.exec(QString(
				"INSERT INTO "
					"EW_OB_ELEMENTY (UIDO, IDE, TYP, N, ATRYBUT) "
				"VALUES "
					"('%1', '%2', 1, (SELECT MAX(N) FROM EW_OB_ELEMENTY WHERE UIDO = '%1') + 1, 0)")
					 .arg(i.key())
					 .arg(P));

			if (!Override) continue;

			Query.exec(QString(
				"UPDATE "
					"EW_OBIEKTY "
				"SET "
					"OPERAT = (SELECT OPERAT FROM EW_OBIEKTY WHERE UID = '%1') "
				"WHERE "
					"ID = '%2'")
					 .arg(i.key())
					 .arg(P));
		}

		Count += i.value().size();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	if (Override) for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		QStringList Attribs;

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

		Query.prepare(QString(
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
				    .arg(Table.Data));

		if (Query.exec()) while (Query.next()) if (i.value().contains(Query.value(0).toInt()))
		{
			QMap<int, QVariant> Values; int i = 1;

			const int Index = Query.value(0).toInt();

			for (int j = 0; j < Common.size(); ++j)
			{
				Values.insert(j, getDataFromDict(Query.value(i++), Common[j].Dict, Common[j].Type));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Values.insert(Table.Headers[j], getDataFromDict(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
			}

			if (!Values.isEmpty()) Model->setData(Index, Values);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataJoin(Count);
}

void DatabaseDriver::joinPoints(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& Join, bool Override)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataJoin(0); return; }

	struct POINT { int ID; double X; double Y; }; int Step = 0; int Count = 0;

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);
	QList<POINT> Points; QList<int> Joined; QMap<int, QSet<int>> Geometry, Insert;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	if (!Tasks.contains(Point) || !Tasks.contains(Join)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Checking used items"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT DISTINCT "
			"EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OB_ELEMENTY "
		"WHERE "
			"EW_OB_ELEMENTY.TYP = 1");

	if (Query.exec()) while (Query.next())
	{
		Joined.append(Query.value(0).toInt());
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, EW_OBIEKTY.ID, "
			"EW_TEXT.POS_X, EW_TEXT.POS_Y "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"INNER "
			"JOIN EW_TEXT "
		"ON "
			"EW_OB_ELEMENTY.IDE = EW_TEXT.ID "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 4 AND "
			"EW_OB_ELEMENTY.N = 0 AND "
			"EW_TEXT.STAN_ZMIANY = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Point);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Point].contains(Query.value(0).toInt()))
		{
			if (!Joined.contains(Query.value(1).toInt())) Points.append(
			{
				Query.value(1).toInt(),
				Query.value(2).toDouble(),
				Query.value(3).toDouble()
			});
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks[Join].size());

	Query.prepare(
		"SELECT "
			"EW_OB_ELEMENTY.UIDO, EW_OB_ELEMENTY.IDE "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"WHERE "
			"EW_OB_ELEMENTY.TYP = 1 AND "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Join);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Join].contains(Query.value(0).toInt()))
		{
			const int ID = Query.value(0).toInt();

			if (!Geometry.contains(ID)) Geometry.insert(ID, QSet<int>());
			if (!Insert.contains(ID)) Insert.insert(ID, QSet<int>());

			Geometry[ID].insert(Query.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, Tasks[Join].size());

	Query.prepare(
		"SELECT "
			"EW_OBIEKTY.UID, EW_OBIEKTY.UID, "
			"EW_TEXT.POS_X, EW_TEXT.POS_Y "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_ELEMENTY "
		"ON "
			"EW_OBIEKTY.UID = EW_OB_ELEMENTY.UIDO "
		"INNER JOIN "
			"EW_TEXT "
		"ON "
			"EW_OB_ELEMENTY.IDE = EW_TEXT.ID "
		"WHERE "
			"EW_TEXT.STAN_ZMIANY = 0 AND "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ = 4 AND "
			"EW_OBIEKTY.KOD = :kod");

	Query.bindValue(":kod", Join);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks[Join].contains(Query.value(0).toInt())) for (const auto P : Points)
		{
			if (Query.value(2).toDouble() == P.X && Query.value(3).toDouble() == P.Y)
			{
				const int ID = Query.value(1).toInt();

				if (!Geometry[ID].contains(P.ID)) Insert[ID].insert(P.ID);
			}
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Joining data"));
	emit onSetupProgress(0, Insert.size());

	for (auto i = Insert.constBegin(); i != Insert.constEnd(); ++i)
	{
		for (const auto& P : i.value())
		{
			Query.exec(QString(
				"INSERT INTO "
					"EW_OB_ELEMENTY (UIDO, IDE, TYP, N, ATRYBUT) "
				"VALUES "
					"('%1', '%2', 1, (SELECT MAX(N) FROM EW_OB_ELEMENTY WHERE UIDO = '%1') + 1, 0)")
					 .arg(i.key())
					 .arg(P));

			if (!Override) continue;

			Query.exec(QString(
				"UPDATE "
					"EW_OBIEKTY "
				"SET "
					"OPERAT = (SELECT OPERAT FROM EW_OBIEKTY WHERE UID = '%1') "
				"WHERE "
					"ID = '%2'")
					 .arg(i.key())
					 .arg(P));
		}

		Count += i.value().size();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	if (Override) for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		QStringList Attribs;

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

		Query.prepare(QString(
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
				    .arg(Table.Data));

		if (Query.exec()) while (Query.next()) if (i.value().contains(Query.value(0).toInt()))
		{
			QMap<int, QVariant> Values; int i = 1;

			const int Index = Query.value(0).toInt();

			for (int j = 0; j < Common.size(); ++j)
			{
				Values.insert(j, getDataFromDict(Query.value(i++), Common[j].Dict, Common[j].Type));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Values.insert(Table.Headers[j], getDataFromDict(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
			}

			if (!Values.isEmpty()) Model->setData(Index, Values);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataJoin(Count);
}

void DatabaseDriver::getPreset(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPresetReady(QList<QMap<int, QVariant>>(), QList<int>()); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 1);
	const QList<int> Used = getCommonFields(Tasks.keys());
	QList<QMap<int, QVariant>> Values; int Step = 0;

	emit onBeginProgress(tr("Preparing edit data"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);

		QSqlQuery Query(Database); QStringList Attribs; Query.setForwardOnly(true);

		for (const auto& Field : Common) Attribs.append(Field.Name);
		for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

		Query.prepare(QString(
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
					.arg(i.key()));

		if (Query.exec()) while (Query.next()) if (i.value().contains(Query.value(0).toInt()))
		{
			QMap<int, QVariant> Value; int i = 1;

			for (int j = 0; j < Common.size(); ++j)
			{
				Value.insert(j, Query.value(i++));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Value.insert(Table.Indexes[j], Query.value(i++));
			}

			if (!Value.isEmpty()) Values.append(Value);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onPresetReady(Values, Used);
}

void DatabaseDriver::getJoins(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onJoinsReady(QMap<QString, QString>(), QMap<QString, QString>(), QMap<QString, QString>()); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 0);
	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMap<QString, QString> Points, Lines, Circles; int Step = 0;

	if (Query.exec("SELECT COUNT(*) FROM EW_OBIEKTY WHERE RODZAJ IN (2, 4)") && Query.next())
	{
		Step = Query.value(0).toInt();
	}

	emit onBeginProgress(tr("Preparing classes"));
	emit onSetupProgress(0, Step); Step = 0;

	Query.prepare(
		"SELECT DISTINCT "
			"EW_OBIEKTY.UID, EW_OBIEKTY.RODZAJ, "
			"EW_OB_OPISY.KOD, EW_OB_OPISY.OPIS "
		"FROM "
			"EW_OBIEKTY "
		"INNER JOIN "
			"EW_OB_OPISY "
		"ON "
			"EW_OBIEKTY.KOD = EW_OB_OPISY.KOD "
		"WHERE "
			"EW_OBIEKTY.STATUS = 0 AND "
			"EW_OBIEKTY.RODZAJ IN (2, 3, 4)");

	if (Query.exec()) while (Query.next())
	{
		if (Tasks.first().contains(Query.value(0).toInt())) switch (Query.value(1).toInt())
		{
			case 2:
				Lines.insert(Query.value(2).toString(), Query.value(3).toString());
			break;
			case 3:
				Circles.insert(Query.value(2).toString(), Query.value(3).toString());
			break;
			case 4:
				Points.insert(Query.value(2).toString(), Query.value(3).toString());
			break;
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onJoinsReady(Points, Lines, Circles);
}

bool operator == (const DatabaseDriver::FIELD& One, const DatabaseDriver::FIELD& Two)
{
	return
	(
		One.Type == Two.Type &&
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Dict == Two.Dict
	);
}

bool operator == (const DatabaseDriver::TABLE& One, const DatabaseDriver::TABLE& Two)
{
	return
	(
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Data == Two.Data &&
		One.Fields == Two.Fields
	);
}

QVariant getDataFromDict(QVariant Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type)
{
	if (Type == DatabaseDriver::BOOL)
	{
		return Value.toBool() ? DatabaseDriver::tr("Yes") : DatabaseDriver::tr("No");
	}
	else if (Dict.isEmpty())
	{
		return Value;
	}
	else if (Type == DatabaseDriver::MASK)
	{
		QStringList Values; const int Bits = Value.toInt();

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (Bits & (1 << i.key().toInt())) Values.append(i.value());
		}

		return Values.join(", ");
	}
	else if (Dict.contains(Value))
	{
		return Dict[Value];
	}
	else return Value;
}

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	static Type Dummy = Type();
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
	return Dummy;
}

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	static Type Dummy = Type();
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
	return Dummy;
}
