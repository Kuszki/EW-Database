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

	QHash<QString, QString> Dict =
	{
		{ "EW_OBIEKTY.OPERAT",		"SELECT UID, NUMER FROM EW_OPERATY"	},
		{ "EW_OBIEKTY.KOD",			"SELECT KOD, OPIS FROM EW_OB_OPISY"	},
		{ "EW_OBIEKTY.OSOU",		"SELECT ID, NAME FROM EW_USERS"		},
		{ "EW_OBIEKTY.OSOW",		"SELECT ID, NAME FROM EW_USERS"		}
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
			"O.KOD, O.OPIS, O.DANE_DOD, O.OPCJE, "
			"F.NAZWA, F.TYTUL, F.TYP, "
			"D.WARTOSC, D.OPIS "
		"FROM "
			"EW_OB_OPISY O "
		"INNER JOIN "
			"EW_OB_DDSTR F "
		"ON "
			"O.KOD = F.KOD "
		"INNER JOIN "
			"EW_OB_DDSTR S "
		"ON "
			"F.KOD = S.KOD "
		"LEFT JOIN "
			"EW_OB_DDSL D "
		"ON "
			"S.UID = D.UIDP OR S.UIDSL = D.UIDP "
		"WHERE "
			"S.NAZWA = F.NAZWA "
		"ORDER BY "
			"O.KOD, F.NAZWA, D.OPIS");

	if (Query.exec()) while (Query.next())
	{
		const QString Field = QString("EW_DATA.%1").arg(Query.value(4).toString());
		const QString Table = Query.value(0).toString();
		const bool Dict = !Query.value(7).isNull();

		if (!hasItemByField(List, Table, &TABLE::Name)) List.append(
		{
			Table,
			Query.value(1).toString(),
			Query.value(2).toString(),
			(Query.value(3).toInt() & 356) == 356
		});

		auto& Tabref = getItemByField(List, Table, &TABLE::Name);

		if (!hasItemByField(Tabref.Fields, Field, &FIELD::Name)) Tabref.Fields.append(
		{
			TYPE(Query.value(6).toInt()),
			Field,
			Query.value(5).toString()
		});

		auto& Fieldref = getItemByField(Tabref.Fields, Field, &FIELD::Name);

		if (Dict) Fieldref.Dict.insert(Query.value(7), Query.value(8).toString());

		if (Emit && Step != List.size()) emit onUpdateProgress(Step = List.size());
	}

	QtConcurrent::blockingMap(List, [] (TABLE& Table) -> void
	{
		for (auto& Field : Table.Fields) if (Field.Type == INTEGER && !Field.Dict.isEmpty())
		{
			if (!Field.Dict.contains(0)) Field.Dict.insert(0, tr("Unknown"));
		}
	});

	if (Emit) emit onEndProgress(); return List;
}

QList<DatabaseDriver::FIELD> DatabaseDriver::loadFields(const QString& Table) const
{
	if (!Database.isOpen()) return QList<FIELD>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QList<FIELD> List;

	Query.prepare(
		"SELECT "
			"D.NAZWA, D.TYTUL, D.TYP "
		"FROM "
			"EW_OB_DDSTR D "
		"WHERE "
			"D.KOD = :table");

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

		if (List.last().Type == INTEGER && !List.last().Dict.isEmpty())
		{
			if (!List.last().Dict.contains(0)) List.last().Dict.insert(0, tr("Unknown"));
		}
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
			"L.WARTOSC, L.OPIS "
		"FROM "
			"EW_OB_DDSL L "
		"INNER JOIN "
			"EW_OB_DDSTR R "
		"ON "
			"L.UIDP = R.UID "
		"WHERE "
			"R.NAZWA = :field AND R.KOD = :table "
		"ORDER BY "
			"L.OPIS");

	Query.bindValue(":field", Field);
	Query.bindValue(":table", Table);

	if (Query.exec()) while (Query.next()) List.insert(Query.value(0), Query.value(1).toString());

	Query.prepare(
		"SELECT "
			"L.WARTOSC, L.OPIS "
		"FROM "
			"EW_OB_DDSL L "
		"INNER JOIN "
			"EW_OB_DDSTR R "
		"ON "
			"L.UIDP = R.UIDSL "
		"WHERE "
			"R.NAZWA = :field AND R.KOD = :table "
		"ORDER BY "
			"L.OPIS");

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
			"O.UID, D.KOD, D.DANE_DOD "
		"FROM "
			"EW_OB_OPISY D "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"D.KOD = O.KOD "
		"WHERE "
			"O.STATUS = 0");

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

QHash<int, QHash<int, QVariant>> DatabaseDriver::loadData(const DatabaseDriver::TABLE& Table, const QList<int>& Filter, const QString& Where, bool Dict, bool View)
{
	if (!Database.isOpen()) return QHash<int, QHash<int, QVariant>>();

	QVariant (*GET)(QVariant, const QMap<QVariant, QString>&, TYPE);
	GET = Dict ? getDataFromDict : [] (auto Value, const auto&, auto) { return Value; };

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QHash<int, QVariant>> List; QStringList Attribs;

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

	if (!Where.isEmpty()) Exec.append(QString(" AND (%1)").arg(Where));

	if (Query.exec(Exec)) while (Query.next()) if (Filter.isEmpty() || Filter.contains(Query.value(0).toInt()))
	{
		QHash<int, QVariant> Values; int i = 1;

		const int Index = Query.value(0).toInt();

		for (int j = 0; j < Common.size(); ++j)
		{
			Values.insert(j, GET(Query.value(i++), Common[j].Dict, Common[j].Type));
		}

		if (View) for (int j = 0; j < Table.Headers.size(); ++j)
		{
			Values.insert(Table.Headers[j], GET(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
		}
		else for (int j = 0; j < Table.Indexes.size(); ++j)
		{
			Values.insert(Table.Indexes[j], GET(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
		}

		if (!Values.isEmpty()) List.insert(Index, Values);
	}

	return List;
}

QHash<int, QHash<int, QVariant>> DatabaseDriver::filterData(QHash<int, QHash<int, QVariant>> Data, const QHash<int, QVariant>& Geometry, const QString& Limiter)
{
	if (!Database.isOpen()) Data;

	QFile File(Limiter); QTextStream Stream(&File); QSet<QString> Limit;

	if (!Limiter.isEmpty() && File.open(QFile::ReadOnly | QFile::Text))
	{
		while (!Stream.atEnd()) Limit.insert(Stream.readLine().trimmed());
	}

	if (Geometry.contains(0) || Geometry.contains(1))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		Query.prepare(
			"SELECT "
				"O.UID "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"LEFT JOIN "
				"EW_POLYLINE P "
			"ON "
				"E.IDE = P.ID "
			"WHERE "
				"O.STATUS = 0 "
			"GROUP BY "
				"O.UID "
			"HAVING "
				"COUNT(P.ID) = 0 OR SUM("
					"SQRT(POWER(P.P0_X - P.P1_X, 2) + POWER(P.P0_Y - P.P1_Y, 2))"
				") NOT BETWEEN :min AND :max");

		Query.bindValue(":min", Geometry.contains(0) ? Geometry[0].toDouble() : 0.0);
		Query.bindValue(":max", Geometry.contains(1) ? Geometry[1].toDouble() : 10000.0);

		if (Query.exec()) while (Query.next()) Data.remove(Query.value(0).toInt());
	}

	if (Geometry.contains(2) || Geometry.contains(3) ||
	    Geometry.contains(4) || Geometry.contains(5))
	{
		auto Process = [] (auto i, const auto& Classes, const auto& Objects, auto List, auto Locker) -> void
		{
			for (auto j = Objects.constBegin(); j != Objects.constEnd(); ++j)
			{
				if (i.key() == j.key() ||
				    j.value().second.type() != QVariant::PointF ||
				    !Classes.contains(j.value().first)) continue;

				if (i.value().second.type() == QVariant::PointF)
				{
					if (i.value().second == j.value().second)
					{
						Locker->lock();
						List->insert(i.key());
						Locker->unlock();
					}
				}
				else for (const auto& Point : i.value().second.toList())
				{
					if (Point == j.value().second)
					{
						Locker->lock();
						List->insert(i.key());
						Locker->unlock();
					}
				}
			}
		};

		QSqlQuery Query(Database); Query.setForwardOnly(true);
		QHash<int, QPair<QString, QVariant>> ObjectsA;
		QHash<int, QPair<QString, QVariant>> ObjectsB;

		const bool FilterA = Geometry.contains(2) || Geometry.contains(3);
		const bool FilterB = Geometry.contains(4) || Geometry.contains(5);

		Query.prepare(
			"SELECT "
				"O.UID, O.KOD, T.POS_X, T.POS_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"E.IDE = T.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"E.TYP = 0 AND "
				"T.TYP = 4");

		if (Query.exec()) while (Query.next())
		{
			if (FilterA) ObjectsA.insert(Query.value(0).toInt(), qMakePair(
								    Query.value(1).toString(),
								    QPointF(Query.value(2).toDouble(),
										  Query.value(3).toDouble())));

			if (FilterB) ObjectsB.insert(Query.value(0).toInt(), qMakePair(
								    Query.value(1).toString(),
								    QPointF(Query.value(2).toDouble(),
										  Query.value(3).toDouble())));
		}

		Query.prepare(
			"SELECT "
				"O.UID, O.KOD, P.P0_X, P.P0_Y, P.P1_X, P.P1_Y, O.NUMER "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_POLYLINE P "
			"ON "
				"E.IDE = P.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"E.TYP = 0");

		if (Query.exec()) while (Query.next()) if (Limit.isEmpty() || Limit.contains(Query.value(6).toString()))
		{
			const int Index = Query.value(0).toInt();

			const QPointF PointA = QPointF(Query.value(2).toDouble(),
									 Query.value(3).toDouble());
			const QPointF PointB = QPointF(Query.value(4).toDouble(),
									 Query.value(5).toDouble());

			if (FilterA)
			{
				if (!ObjectsA.contains(Index))
				{
					ObjectsA.insert(Index, qMakePair(
								 Query.value(1).toString(),
								 QVariant(QVariant::List)));
				}

				auto ListA = ObjectsA[Index].second.toList();

				if (!ListA.contains(PointA)) ListA.append(PointA);
				if (!ListA.contains(PointB)) ListA.append(PointB);

				ObjectsA[Index].second.setValue(ListA);
			}

			if (FilterB)
			{
				if (!ObjectsB.contains(Index))
				{
					ObjectsB.insert(Index, qMakePair(
								 Query.value(1).toString(),
								 QVariant(QVariant::List)));
				}

				auto ListB = ObjectsB[Index].second.toList();

				if (!ListB.contains(PointA)) ListB.append(PointA);
				else ListB.removeOne(PointA);
				if (!ListB.contains(PointB)) ListB.append(PointB);
				else ListB.removeOne(PointB);

				ObjectsB[Index].second.setValue(ListB);
			}
		}

		QFutureSynchronizer<void> Synchronizer;
		QMutex LockerA, LockerB;
		QSet<int> ListA, ListB;

		if (Geometry.contains(2)) for (auto i = ObjectsA.constBegin(); i != ObjectsA.constEnd(); ++i)
		{
			Synchronizer.addFuture(QtConcurrent::run(Process, i, Geometry[2].toStringList(), ObjectsA, &ListA, &LockerA));
		}

		if (Geometry.contains(3)) for (auto i = ObjectsA.constBegin(); i != ObjectsA.constEnd(); ++i)
		{
			Synchronizer.addFuture(QtConcurrent::run(Process, i, Geometry[3].toStringList(), ObjectsA, &ListA, &LockerA));
		}

		if (Geometry.contains(4)) for (auto i = ObjectsB.constBegin(); i != ObjectsB.constEnd(); ++i)
		{
			Synchronizer.addFuture(QtConcurrent::run(Process, i, Geometry[4].toStringList(), ObjectsB, &ListB, &LockerB));
		}

		if (Geometry.contains(5)) for (auto i = ObjectsB.constBegin(); i != ObjectsB.constEnd(); ++i)
		{
			Synchronizer.addFuture(QtConcurrent::run(Process, i, Geometry[5].toStringList(), ObjectsB, &ListB, &LockerB));
		}

		Synchronizer.waitForFinished();

		if (Geometry.contains(2))
		{
			for (const auto& Key : Data.keys()) if (!ListA.contains(Key)) Data.remove(Key);
		}

		if (Geometry.contains(3))
		{
			for (const auto& Key : Data.keys()) if (ListA.contains(Key)) Data.remove(Key);
		}

		if (Geometry.contains(4))
		{
			for (const auto& Key : Data.keys()) if (!ListB.contains(Key)) Data.remove(Key);
		}

		if (Geometry.contains(5))
		{
			for (const auto& Key : Data.keys()) if (ListB.contains(Key)) Data.remove(Key);
		}
	}

	if (Geometry.contains(6) || Geometry.contains(7))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		QHash<int, int> CountA, CountB;

		Query.prepare(
			"SELECT "
				"O.UID "
			"FROM "
				"EW_OBIEKTY O "
			"WHERE "
				"O.STATUS = 0");

		if (Query.exec()) while (Query.next())
		{
			if (Geometry.contains(6)) CountA.insert(Query.value(0).toInt(), 0);
			if (Geometry.contains(7)) CountB.insert(Query.value(0).toInt(), 0);
		}

		const QString Select = QString(
			"SELECT "
				"E.UIDO, O.NUMER "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.ID = E.IDE "
			"WHERE "
				"E.TYP = 1 AND "
				"O.STATUS = 0 AND ("
					"%1 = 1 OR "
					"O.KOD IN ('%2')"
				")");

		if (Geometry.contains(6) && Query.exec(Select
			.arg(Geometry[6].toStringList().contains("*"))
			.arg(Geometry[6].toStringList().join("', '"))))
		{
			while (Query.next()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
			{
				const int ID = Query.value(0).toInt();

				if (CountA.contains(ID)) ++CountA[ID];
			}
		}

		if (Geometry.contains(7) && Query.exec(Select
			.arg(Geometry[7].toStringList().contains("*"))
			.arg(Geometry[7].toStringList().join("', '"))))
		{
			while (Query.next()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
			{
				const int ID = Query.value(0).toInt();

				if (CountB.contains(ID)) ++CountB[ID];
			}
		}

		if (Geometry.contains(6)) for (auto i = CountA.constBegin(); i != CountA.constEnd(); ++i)
		{
			if (i.value() == 0) Data.remove(i.key());
		}

		if (Geometry.contains(7)) for (auto i = CountB.constBegin(); i != CountB.constEnd(); ++i)
		{
			if (i.value() != 0) Data.remove(i.key());
		}
	}

	if (Geometry.contains(8) || Geometry.contains(9))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		QHash<int, int> CountA, CountB;

		Query.prepare(
			"SELECT "
				"O.UID "
			"FROM "
				"EW_OBIEKTY O "
			"WHERE "
				"O.STATUS = 0");

		if (Query.exec()) while (Query.next())
		{
			if (Geometry.contains(8)) CountA.insert(Query.value(0).toInt(), 0);
			if (Geometry.contains(9)) CountB.insert(Query.value(0).toInt(), 0);
		}

		const QString Select = QString(
			"SELECT "
				"(SELECT FIRST 1 "
					"B.UID "
				"FROM "
					"EW_OBIEKTY B "
				"WHERE "
					"B.STATUS = 0 AND "
					"B.ID = E.IDE"
				"), "
				"O.NUMER "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"WHERE "
				"E.TYP = 1 AND "
				"O.STATUS = 0 AND ("
					"%1 = 1 OR "
					"O.KOD IN ('%2')"
				")");

		if (Geometry.contains(8) && Query.exec(Select
			.arg(Geometry[8].toStringList().contains("*"))
			.arg(Geometry[8].toStringList().join("', '"))))
		{
			while (Query.next()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
			{
				const int ID = Query.value(0).toInt();

				if (CountA.contains(ID)) ++CountA[ID];
			}
		}

		if (Geometry.contains(9) && Query.exec(Select
			.arg(Geometry[9].toStringList().contains("*"))
			.arg(Geometry[9].toStringList().join("', '"))))
		{
			while (Query.next()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
			{
				const int ID = Query.value(0).toInt();

				if (CountB.contains(ID)) ++CountB[ID];
			}
		}

		if (Geometry.contains(8)) for (auto i = CountA.constBegin(); i != CountA.constEnd(); ++i)
		{
			if (i.value() == 0) Data.remove(i.key());
		}

		if (Geometry.contains(9)) for (auto i = CountB.constBegin(); i != CountB.constEnd(); ++i)
		{
			if (i.value() != 0) Data.remove(i.key());
		}
	}

	if (Geometry.contains(10) || Geometry.contains(11))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		QList<QPair<int, int>> CountA, CountB;
		QHash<QString, QList<QPointF>> Lines;
		QHash<int, QPointF> Points;

		Query.prepare(
			"SELECT "
				"O.UID "
			"FROM "
				"EW_OBIEKTY O "
			"WHERE "
				"O.STATUS = 0");

		if (Query.exec()) while (Query.next())
		{
			if (Geometry.contains(10)) CountA.append(qMakePair(Query.value(0).toInt(), 0));
			if (Geometry.contains(11)) CountB.append(qMakePair(Query.value(0).toInt(), 0));
		}

		Query.prepare(
			"SELECT "
				"O.KOD, P.P0_X, P.P0_Y, P.P1_X, P.P1_Y, O.NUMER "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_POLYLINE P "
			"ON "
				"E.IDE = P.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"E.TYP = 0");

		if (Query.exec()) while (Query.next()) if (Limit.isEmpty() || Limit.contains(Query.value(5).toString()))
		{
			const QString Class = Query.value(0).toString();

			if ((Geometry.contains(10) && (Geometry[10].toStringList().contains("*") || Geometry[10].toStringList().contains(Class))) ||
			    (Geometry.contains(11) && (Geometry[11].toStringList().contains("*") || Geometry[11].toStringList().contains(Class))))
			{
				if (!Lines.contains(Class)) Lines.insert(Class, QList<QPointF>());

				Lines[Class].append(QPointF(Query.value(1).toDouble(), Query.value(2).toDouble()));
				Lines[Class].append(QPointF(Query.value(3).toDouble(), Query.value(4).toDouble()));
			}
		}

		Query.prepare(
			"SELECT "
				"O.UID, T.POS_X, T.POS_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"E.IDE = T.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"E.TYP = 0 AND "
				"T.TYP = 4");

		if (Query.exec()) while (Query.next())
		{
			Points.insert(Query.value(0).toInt(),
					    QPointF(Query.value(1).toDouble(),
							  Query.value(2).toDouble()));
		}

		if (Geometry.contains(10)) QtConcurrent::blockingMap(CountA, [&Points, &Lines, &Geometry] (QPair<int, int>& Value) -> void
		{
			for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
			{
				if (Geometry[10].toStringList().contains("*") || Geometry[10].toStringList().contains(i.key()))
				{
					for (const auto Point : i.value()) if (Points.value(Value.first) == Point) ++Value.second;
				}
			}
		});

		if (Geometry.contains(11)) QtConcurrent::blockingMap(CountB, [&Points, &Lines, &Geometry] (QPair<int, int>& Value) -> void
		{
			for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
			{
				if (Geometry[11].toStringList().contains("*") || Geometry[11].toStringList().contains(i.key()))
				{
					for (const auto Point : i.value()) if (Points.value(Value.first) == Point) ++Value.second;
				}
			}
		});

		if (Geometry.contains(10)) for (const auto& Point : CountA)
		{
			if (Point.second == 0) Data.remove(Point.first);
		}

		if (Geometry.contains(11)) for (const auto& Point : CountB)
		{
			if (Point.second != 0) Data.remove(Point.first);
		}
	}

	if (Geometry.contains(12))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		const QString Select = QString(
			"SELECT "
				"O.UID "
			"FROM "
				"EW_OBIEKTY O "
			"LEFT JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.ID = E.IDE "
			"WHERE "
				"O.STATUS = 0 "
			"GROUP BY "
				"O.UID "
			"HAVING "
				"COUNT("
					"IIF (E.TYP = 1 AND ("
						"SELECT "
							"P.STATUS "
						"FROM "
							"EW_OBIEKTY P "
						"WHERE "
							"P.UID = E.UIDO "
					") = 0, 1, NULL)"
				") < 2");

		if (Query.exec(Select)) while (Query.next()) Data.remove(Query.value(0).toInt());
	}

	return Data;
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

void DatabaseDriver::loadList(const QStringList& Filter)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	const int Index = Headers.indexOf(tr("Object ID"));

	RecordModel* Model = new RecordModel(Headers, this); int Step = 0;

	for (const auto& Table : Tables)
	{
		auto Data = loadData(Table, QList<int>(), QString(), true, true);

		for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
			if (Filter.contains(i.value().value(Index).toString()))
			{
				Model->addItem(i.key(), i.value());
			}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::reloadData(const QString& Filter, QList<int> Used, const QHash<int, QVariant>& Geometry, const QString& Limiter)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	if (Used.isEmpty()) Used = getUsedFields(Filter);

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	RecordModel* Model = new RecordModel(Headers, this); int Step = 0;
	QHash<int, QHash<int, QVariant>> List;

	for (const auto& Table : Tables) if (hasAllIndexes(Table, Used))
	{
		auto Data = loadData(Table, QList<int>(), Filter, true, true);

		if (Geometry.isEmpty()) Model->addItems(Data);
		else for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
		{
			List.insert(i.key(), i.value());
		}

		emit onUpdateProgress(++Step);
	}

	if (!Geometry.isEmpty())
	{
		emit onBeginProgress(tr("Applying geometry filters"));
		emit onSetupProgress(0, 0);

		Model->addItems(filterData(List, Geometry, Limiter));
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::updateData(RecordModel* Model, const QModelIndexList& Items, const QHash<int, QVariant>& Values)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataUpdate(Model); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 1);
	const QList<int> Used = Values.keys(); int Step = 0; QStringList All;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	for (int i = 0; i < Common.size(); ++i) if (Values.contains(i))
	{
		if (Values[i].isNull()) All.append(QString("%1 = NULL").arg(Fields[i].Name));
		else All.append(QString("%1 = '%2'").arg(Fields[i].Name).arg(Values[i].toString()));
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
	emit onSetupProgress(0, Tasks.first().size());

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data); QStringList Updates;

		for (const auto& Index : Used) if (Table.Indexes.contains(Index))
		{
			if (Values[Index].isNull()) Updates.append(QString("%1 = NULL").arg(Fields[Index].Name));
			else Updates.append(QString("%1 = '%2'").arg(Fields[Index].Name).arg(Values[Index].toString()));
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

			emit onUpdateProgress(++Step);
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size() - 1);

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) emit onRowUpdate(j.key(), j.value());

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataUpdate(Model);
}

void DatabaseDriver::removeData(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRemove(Model); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 1);
	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0;

	emit onBeginProgress(tr("Removing data"));
	emit onSetupProgress(0, Items.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		for (const auto& Index : i.value())
		{
			Query.exec(QString(
				"DELETE FROM "
					"EW_TEXT "
				"WHERE "
					"ID IN ("
						"SELECT IDE FROM EW_OB_ELEMENTY WHERE TYP = 0 AND UIDO = '%1'"
					")")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"EW_POLYLINE "
				"WHERE "
					"ID IN ("
						"SELECT IDE FROM EW_OB_ELEMENTY WHERE TYP = 0 AND UIDO = '%1'"
					")")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"EW_OBIEKTY "
				"WHERE "
					"UID = '%1'")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"EW_ELEMENTY "
				"WHERE "
					"UIDO = '%1'")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"%1 "
				"WHERE "
					"UIDO = '%2'")
					 .arg(i.key())
					 .arg(Index));

			emit onUpdateProgress(++Step);
		}
	}

	for (const auto Item : Items) emit onRowRemove(Item);

	emit onEndProgress();
	emit onDataRemove(Model);
}

void DatabaseDriver::splitData(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& From, int Type)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataSplit(0); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);
	QList<int> Points; QHash<int, QList<int>> Objects; int Step = 0; int Count = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	Type = Type == 0 ? 2 : Type == 1 ? 4 : Type == 2 ? 3 : 0;

	if (!Tasks.contains(Point) || !Tasks.contains(From)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size());

	Query.prepare(
		"SELECT "
			"O.UID, O.ID "
		"FROM "
			"EW_OBIEKTY O "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4 AND "
			"O.KOD = :kod");

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
			"E.UIDO, E.IDE "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"WHERE "
			"E.TYP = 1 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = :typ AND "
			"O.KOD = :kod");

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
					"TYP = 1 AND "
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

void DatabaseDriver::joinData(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& Join, bool Override, int Type, double Radius)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataJoin(0); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);
	QList<POINT> Points; QList<int> Joined; QHash<int, QSet<int>> Geometry, Insert;
	QSqlQuery Query(Database); Query.setForwardOnly(true);
	int Step = 0; int Count = 0;

	if (!Tasks.contains(Point) || !Tasks.contains(Join)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Checking used items"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT DISTINCT "
			"E.IDE "
		"FROM "
			"EW_OB_ELEMENTY E "
		"WHERE "
			"E.TYP = 1 AND "
			"UIDO IN ("
				"SELECT "
					"O.UID "
				"FROM "
					"EW_OBIEKTY O "
				"WHERE "
					"O.STATUS = 0"
			")");

	if (Query.exec()) while (Query.next())
	{
		Joined.append(Query.value(0).toInt());
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size());

	Query.prepare(
		"SELECT "
			"O.UID, O.ID, "
			"T.POS_X, T.POS_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER "
			"JOIN EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4 AND "
			"O.KOD = :kod");

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
			"E.UIDO, E.IDE "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"WHERE "
			"E.TYP = 1 AND "
			"O.STATUS = 0 AND "
			"O.KOD = :kod");

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

	switch (Type)
	{
		case 0:
			Insert = joinLines(Geometry, Points, Tasks[Join], Join, Radius);
		break;
		case 1:
			Insert = joinPoints(Geometry, Points, Tasks[Join], Join, Radius);
		break;
		case 2:
			Insert = joinSurfaces(Geometry, Points, Tasks[Join], Join, Radius);
		break;
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
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) emit onRowUpdate(j.key(), j.value());

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataJoin(Count);
}

void DatabaseDriver::mergeData(RecordModel* Model, const QModelIndexList& Items, const QList<int>& Values, const QStringList& Points)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataMerge(0); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 0);
	QHash<int, QList<QPointF>> Geometry; QSet<int> Used; QList<int> Counts; QList<QPointF> Ends;
	QHash<int, QSet<int>> Merges; QList<QPointF> Cuts; int Step = 0; QSet<int> Merged;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	if (!Points.isEmpty())
	{
		Query.prepare(QString(
			"SELECT "
				"T.POS_X, T.POS_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"E.IDE = T.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"O.RODZAJ = 4 AND "
				"E.TYP = 0 AND "
				"T.STAN_ZMIANY = 0 AND "
				"T.TYP = 4 AND "
				"O.KOD IN ('%1')")
				    .arg(Points.join("', '")));

		if (Query.exec()) while (Query.next()) Cuts.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});

		Query.prepare(QString(
			"SELECT "
				"(P.P0_X + P.P1_X) / 2.0, "
				"(P.P0_Y + P.P1_Y) / 2.0 "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_POLYLINE P "
			"ON "
				"E.IDE = P.ID "
			"WHERE "
				"P.STAN_ZMIANY = 0 AND "
				"P.P1_FLAGS = 4 AND "
				"E.TYP = 0 AND "
				"O.STATUS = 0 AND "
				"O.RODZAJ = 3 AND "
				"O.KOD IN ('%1')")
				    .arg(Points.join("', '")));

		if (Query.exec()) while (Query.next()) Cuts.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, "
			"P.P0_X, P.P0_Y, "
			"P.P1_X, P.P1_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_POLYLINE P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 "
		"ORDER BY "
			"O.UID, E.N ASC");

	if (Query.exec()) while (Query.next())
	{
		const int ID = Query.value(0).toInt();

		if (Tasks.first().contains(ID))
		{
			const QPointF PointA(Query.value(1).toDouble(), Query.value(2).toDouble());
			const QPointF PointB(Query.value(3).toDouble(), Query.value(4).toDouble());

			if (!Geometry.contains(ID)) Geometry.insert(ID, QList<QPointF>());

			if (!Geometry[ID].contains(PointA)) Geometry[ID].append(PointA);
			else Geometry[ID].removeOne(PointA);
			if (!Geometry[ID].contains(PointB)) Geometry[ID].append(PointB);
			else Geometry[ID].removeOne(PointB);
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Merging objects"));
	emit onSetupProgress(0, Tasks.first().size());

	for (const auto& Item : Geometry) for (const auto& Point : Item)
	{
		const int Index = Ends.indexOf(Point);

		if (Index == -1)
		{
			Ends.append(Point);
			Counts.append(1);
		}
		else if (++Counts[Index] == 3)
		{
			Cuts.append(Point);
		}
	}

	for (auto k = Tasks.constBegin() + 1; k != Tasks.constEnd(); ++k)
	{
		const auto& Table = getItemByField(Tables, k.key(), &TABLE::Name);

		auto Data = loadData(Table, k.value(), QString(), false, false);

		for (auto i = Data.constBegin(); i != Data.constEnd(); ++i) if (!Used.contains(i.key()) && Geometry[i.key()].size() == 2)
		{
			QPointF& P1 = Geometry[i.key()].first();
			QPointF& P2 = Geometry[i.key()].last();
			QSet<int> Parts; bool Continue = true;
			const auto D1 = Data[i.key()];

			Used.insert(i.key()); while (Continue)
			{
				const int oldSize = Parts.size();

				for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) if (!Used.contains(j.key()) && Geometry[j.key()].size() == 2)
				{
					const QPointF& L1 = Geometry[j.key()].first();
					const QPointF& L2 = Geometry[j.key()].last();
					const auto& D2 = Data[j.key()];

					int T(0); if (P1 == L1 && !Cuts.contains(P1)) T = 1;
					else if (P1 == L2 && !Cuts.contains(P1)) T = 2;
					else if (P2 == L1 && !Cuts.contains(P2)) T = 3;
					else if (P2 == L2 && !Cuts.contains(P2)) T = 4;

					if (T) for (const auto& Field : Values) if (D1[Field] != D2[Field]) T = 0;

					switch (T)
					{
						case 1: P1 = L2; break;
						case 2: P1 = L1; break;
						case 3: P2 = L2; break;
						case 4: P2 = L1; break;
					}

					if (T)
					{
						Parts.insert(j.key());
						Used.insert(j.key());
					}
				}

				Continue = oldSize != Parts.size();
			}

			if (!Parts.isEmpty()) { Parts.insert(i.key()); Merges.insert(i.key(), Parts); }
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating database"));
	emit onSetupProgress(0, Merges.size());

	for (auto k = Tasks.constBegin() + 1; k != Tasks.constEnd(); ++k)
	{
		const auto& Table = getItemByField(Tables, k.key(), &TABLE::Name);

		for (const auto& Index : k.value()) if (Merges.contains(Index))
		{
			struct PART { int ID, Type; double X1, Y1, X2, Y2; bool Text; };

			QList<PART> Parts, Sorted, Labels; QSet<int> Taken; bool Continue = true; int n = 0;

			for (const auto& Part : Merges[Index])
			{
				Query.prepare(QString(
					"SELECT "
						"E.IDE, E.TYP, "
						"ROUND(P.P0_X, 3), ROUND(P.P0_Y, 3), "
						"ROUND(P.P1_X, 3), ROUND(P.P1_Y, 3), "
						"IIF(P.P0_X IS NULL OR P.P0_Y IS NULL, 1, 0) "
					"FROM "
						"EW_OB_ELEMENTY E "
					"LEFT JOIN "
						"EW_POLYLINE P "
					"ON "
						"E.IDE = P.ID AND "
						"E.TYP = 0 AND "
						"P.STAN_ZMIANY = 0 "
					"WHERE "
						"E.UIDO = %1 "
					"ORDER BY "
						"E.UIDO, E.N ASC")
						    .arg(Part));

				if (Query.exec()) while (Query.next()) Parts.append(
				{
					Query.value(0).toInt(),
					Query.value(1).toInt(),
					Query.value(2).toDouble(),
					Query.value(3).toDouble(),
					Query.value(4).toDouble(),
					Query.value(5).toDouble(),
					Query.value(6).toBool()
				});

				Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1").arg(Part));

				if (Part == Index) continue; else Merged.insert(Part);

				Query.exec(QString("DELETE FROM EW_OBIEKTY WHERE UID = %1").arg(Part));
				Query.exec(QString("DELETE FROM %1 WHERE UIDO = %2").arg(Table.Data, Part));
			}

			for (int i = 0; i < Parts.size(); ++i)
			{
				if (Parts[i].Type || Parts[i].Text) Labels.append(Parts.takeAt(i--));
			}

			QPointF P1(Parts.first().X1, Parts.first().Y1);
			QPointF P2(Parts.first().X2, Parts.first().Y2);

			Sorted.append(Parts.first());
			Taken.insert(Parts.first().ID);

			while (Continue)
			{
				const int oldSize = Sorted.size();

				for (const auto& L : Parts) if (!Taken.contains(L.ID))
				{
					QPointF L1(L.X1, L.Y1), L2(L.X2, L.Y2);

					int T(0); if (P1 == L1) T = 1;
					else if (P1 == L2) T = 2;
					else if (P2 == L1) T = 3;
					else if (P2 == L2) T = 4;

					switch (T)
					{
						case 1: P1 = L2; Sorted.push_front(L); break;
						case 2: P1 = L1; Sorted.push_front(L); break;
						case 3: P2 = L2; Sorted.push_back(L); break;
						case 4: P2 = L1; Sorted.push_back(L); break;
					}

					if (T) Taken.insert(L.ID);
				}

				Continue = oldSize != Sorted.size();
			}

			for (const auto& Part: Sorted) Query.exec(QString(
				"INSERT INTO "
					"EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
				"VALUES "
					"(%1, %2, %3, %4)")
				.arg(Index)
				.arg(Part.ID)
				.arg(Part.Type)
				.arg(n++));

			for (const auto& Part: Labels) Query.exec(QString(
				"INSERT INTO "
					"EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
				"VALUES "
					"(%1, %2, %3, %4)")
				.arg(Index)
				.arg(Part.ID)
				.arg(Part.Type)
				.arg(n++));

			emit onUpdateProgress(++Step);
		}
	}

	for (const auto& Item : Merged) emit onRowRemove(Model->index(Item));

	emit onEndProgress();
	emit onDataMerge(Merged.size() + Merges.size());
}

void DatabaseDriver::cutData(RecordModel* Model, const QModelIndexList& Items, const QStringList& Points, bool Endings)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataCut(0); return; }

	struct L_PART { int ID, N; double X1, Y1, X2, Y2; };
	struct P_PART { int ID, LID; double X, Y, L = NAN; };

	struct PARTS { QList<L_PART> Lines; QList<P_PART> Labels; QList<P_PART> Objects; };

	const auto getPairs = [] (PARTS& P) -> void
	{
		static const auto length = [] (double x1, double y1, double x2, double y2)
		{
			const double dx = x1 - x2;
			const double dy = y1 - y2;

			return qSqrt(dx * dx + dy * dy);
		};

		static const auto find = [] (const QList<L_PART>& Lines, P_PART& P)
		{
			for (const auto& L : Lines)
			{
				const double l = length(L.X1, L.Y1, L.X2, L.Y2);
				const double a = length(P.X, P.Y, L.X1, L.Y1);
				const double b = length(P.X, P.Y, L.X2, L.Y2);

				if ((a * a <= l * l + b * b) &&
				    (b * b <= a * a + l * l))
				{
					const double h = (a + b) / l;

					if (qIsNaN(P.L) || h < P.L) { P.L = h; P.LID = L.ID; };
				}
			}
		};

		for (auto& Part : P.Labels) find(P.Lines, Part);
		for (auto& Part : P.Objects) find(P.Lines, Part);
	};

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 0);
	QHash<int, PARTS> Parts; QList<QPointF> Cuts; QHash<int, QSet<int>> Queue; int Step = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true); QMutex Locker;

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	if (!Points.isEmpty())
	{
		Query.prepare(QString(
			"SELECT "
				"T.POS_X, T.POS_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_TEXT T "
			"ON "
				"E.IDE = T.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"O.RODZAJ = 4 AND "
				"E.TYP = 0 AND "
				"T.STAN_ZMIANY = 0 AND "
				"T.TYP = 4 AND "
				"O.KOD IN ('%1')")
				    .arg(Points.join("', '")));

		if (Query.exec()) while (Query.next()) Cuts.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});

		Query.prepare(QString(
			"SELECT "
				"(P.P0_X + P.P1_X) / 2.0, "
				"(P.P0_Y + P.P1_Y) / 2.0 "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_POLYLINE P "
			"ON "
				"E.IDE = P.ID "
			"WHERE "
				"P.STAN_ZMIANY = 0 AND "
				"P.P1_FLAGS = 4 AND "
				"E.TYP = 0 AND "
				"O.STATUS = 0 AND "
				"O.RODZAJ = 3 AND "
				"O.KOD IN ('%1')")
				    .arg(Points.join("', '")));

		if (Query.exec()) while (Query.next()) Cuts.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});
	}

	if (Endings)
	{
		QHash<int, QList<QPointF>> Geometry;

		Query.prepare(
			"SELECT "
				"O.UID, "
				"P.P0_X, P.P0_Y, "
				"P.P1_X, P.P1_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_POLYLINE P "
			"ON "
				"E.IDE = P.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"O.RODZAJ = 2 AND "
				"E.TYP = 0 AND "
				"P.STAN_ZMIANY = 0 "
			"ORDER BY "
				"O.UID, E.N ASC");

		if (Query.exec()) while (Query.next())
		{
			const int ID = Query.value(0).toInt();

			if (Tasks.first().contains(ID))
			{
				const QPointF PointA(Query.value(1).toDouble(), Query.value(2).toDouble());
				const QPointF PointB(Query.value(3).toDouble(), Query.value(4).toDouble());

				if (!Geometry.contains(ID)) Geometry.insert(ID, QList<QPointF>());

				if (!Geometry[ID].contains(PointA)) Geometry[ID].append(PointA);
				else Geometry[ID].removeOne(PointA);
				if (!Geometry[ID].contains(PointB)) Geometry[ID].append(PointB);
				else Geometry[ID].removeOne(PointB);
			}
		}

		for (const auto& Line : Geometry) if (Line.size() == 2)
		{
			Cuts.append(Line.first()); Cuts.append(Line.last());
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, "
			"E.IDE, E.N, "
			"P.P0_X, P.P0_Y, "
			"P.P1_X, P.P1_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_POLYLINE P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 "
		"ORDER BY "
			"O.UID, E.N ASC");

	if (Query.exec()) while (Query.next()) if (Tasks.first().contains(Query.value(0).toInt()))
	{
		const int ID = Query.value(0).toInt();

		if (!Parts.contains(ID)) Parts.insert(ID, PARTS());

		Parts[ID].Lines.append(
		{
			Query.value(1).toInt(),
			Query.value(2).toInt(),
			Query.value(3).toDouble(),
			Query.value(4).toDouble(),
			Query.value(5).toDouble(),
			Query.value(6).toDouble(),
		});
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, 0);

	QtConcurrent::blockingMap(Cuts, [&Parts, &Queue, &Locker] (const QPointF& Point) -> void
	{
		for (auto i = Parts.constBegin(); i != Parts.constEnd(); ++i) for (int j = 1; j < i.value().Lines.size(); ++j)
		{
			QPointF A(i.value().Lines[j - 1].X1, i.value().Lines[j - 1].Y1);
			QPointF B(i.value().Lines[j - 1].X2, i.value().Lines[j - 1].Y2);
			QPointF C(i.value().Lines[j].X1, i.value().Lines[j].Y1);
			QPointF D(i.value().Lines[j].X2, i.value().Lines[j].Y2);

			if ((Point == A && Point == C) || (Point == B && Point == C) ||
			    (Point == A && Point == D) || (Point == B && Point == D))
			{
				Locker.lock();

				if (Queue.contains(i.key())) Queue[i.key()].insert(j);
				else Queue.insert(i.key(), QSet<int>() << j);

				Locker.unlock();
			};
		}
	});

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, E.IDE, "
			"IIF(T.ODN_X IS NULL, T.POS_X, T.POS_X + T.ODN_X), "
			"IIF(T.ODN_Y IS NULL, T.POS_Y, T.POS_Y + T.ODN_Y) "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0");

	if (Query.exec()) while (Query.next()) if (Queue.contains(Query.value(0).toInt()))
	{
		const int ID = Query.value(0).toInt();

		if (Parts.contains(ID)) Parts[ID].Labels.append(
		{
			Query.value(1).toInt(), 0,
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		});
	}

	Query.prepare(
		"SELECT "
			"O.UID, E.IDE, ("
			"SELECT "
				"TE.POS_X "
			"FROM "
				"EW_OB_ELEMENTY EL "
			"INNER JOIN "
				"EW_TEXT TE "
			"ON "
				"EL.IDE = TE.ID "
			"WHERE "
				"EL.TYP = 0 AND "
				"TE.TYP = 4 AND "
				"TE.STAN_ZMIANY = 0 AND "
				"EL.UIDO = ("
					"SELECT FIRST 1 "
						"OB.UID "
					"FROM "
						"EW_OBIEKTY OB "
					"WHERE "
						"OB.ID = E.IDE AND "
						"OB.STATUS = 0)"
			"), ("
			"SELECT "
				"TE.POS_Y "
			"FROM "
				"EW_OB_ELEMENTY EL "
			"INNER JOIN "
				"EW_TEXT TE "
			"ON "
				"EL.IDE = TE.ID "
			"WHERE "
				"EL.TYP = 0 AND "
				"TE.TYP = 4 AND "
				"TE.STAN_ZMIANY = 0 AND "
				"EL.UIDO = ("
					"SELECT FIRST 1 "
						"OB.UID "
					"FROM "
						"EW_OBIEKTY OB "
					"WHERE "
						"OB.ID = E.IDE AND "
						"OB.STATUS = 0)"
			") "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 1");

	if (Query.exec()) while (Query.next()) if (Queue.contains(Query.value(0).toInt()))
	{
		const int ID = Query.value(0).toInt();

		if (Parts.contains(ID)) Parts[ID].Objects.append(
		{
			Query.value(1).toInt(), 0,
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		});
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Inserting objects"));
	emit onSetupProgress(0, Queue.size());

	QtConcurrent::blockingMap(Parts, getPairs);

	for (auto t = Tasks.constBegin() + 1; t != Tasks.constEnd(); ++t)
	{
		const auto& Table = getItemByField(Tables, t.key(), &TABLE::Name);

		QStringList Names; for (const auto& Field : Table.Fields) Names.append(QString(Field.Name).remove("EW_DATA."));

		const QString dataInsert = QString("INSERT INTO %1 (UIDO, %2) "
									"SELECT %3, %2 FROM %1 WHERE UIDO = %4")
							  .arg(Table.Data).arg(Names.join(", "));

		const QString objectInsert = QString("INSERT INTO EW_OBIEKTY (UID, ID, IDKATALOG, KOD, RODZAJ, OSOU, OSOW, DTU, DTW, OPERAT, STATUS) "
									  "SELECT %1, ID, IDKATALOG, KOD, RODZAJ, OSOU, OSOW, DTU, DTW, OPERAT, STATUS FROM EW_OBIEKTY WHERE UID = %2");

		for (auto i = Queue.constBegin(); i != Queue.constEnd(); ++i)
		{
			QList<int> Jobs = i.value().values(); qSort(Jobs); int on = Jobs.first();

			Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1 AND N >= %2")
					 .arg(i.key()).arg(on));

			for (auto j = Jobs.constBegin(); j != Jobs.constEnd(); ++j)
			{
				int Index(0), n(0); const int Stop = (j + 1) == Jobs.constEnd() ? Parts[i.key()].Lines.size() : *(j + 1);

				Query.prepare("SELECT GEN_ID(EW_OBIEKTY_UID_GEN, 1) FROM RDB$DATABASE");

				if (Query.exec() && Query.next()) Index = Query.value(0).toInt();

				Query.exec(objectInsert.arg(Index).arg(i.key()));
				Query.exec(dataInsert.arg(Index).arg(i.key()));

				for (int p = *j; p < Stop; ++p)
				{
					Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
								    "VALUES (%1, %2, 0, %3)")
							 .arg(Index).arg(Parts[i.key()].Lines[p].ID).arg(n++));
				}

				for (int p = *j; p < Stop; ++p) for (const auto& T : Parts[i.key()].Labels) if (T.LID == Parts[i.key()].Lines[p].ID)
				{
					Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
								    "VALUES (%1, %2, 0, %3)")
							 .arg(Index).arg(T.ID).arg(n++));
				}

				for (int p = *j; p < Stop; ++p) for (const auto& T : Parts[i.key()].Objects) if (T.LID == Parts[i.key()].Lines[p].ID)
				{
					Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
								    "VALUES (%1, %2, 1, %3)")
							 .arg(Index).arg(T.ID).arg(n++));
				}
			}

			for (int p = 0; p < Jobs.first(); ++p) for (const auto& T : Parts[i.key()].Labels) if (T.LID == Parts[i.key()].Lines[p].ID)
			{
				Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
							    "VALUES (%1, %2, 0, %3)")
						 .arg(i.key()).arg(T.ID).arg(on++));
			}

			for (int p = 0; p < Jobs.first(); ++p) for (const auto& T : Parts[i.key()].Objects) if (T.LID == Parts[i.key()].Lines[p].ID)
			{
				Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
							    "VALUES (%1, %2, 1, %3)")
						 .arg(i.key()).arg(T.ID).arg(on++));
			}

			emit onUpdateProgress(++Step);
		}
	}

	emit onEndProgress();
	emit onDataCut(Queue.size());
}

void DatabaseDriver::refactorData(RecordModel* Model, const QModelIndexList& Items, const QString& Class, int Line, int Point, int Text)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRefactor(); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 1);
	const auto& Table = getItemByField(Tables, Class, &TABLE::Name); int Step = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	emit onBeginProgress(tr("Updating class"));
	emit onSetupProgress(0, Items.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Tab = getItemByField(Tables, i.key(), &TABLE::Data);
		QStringList Fields;

		for (const auto& Field : Table.Fields)
		{
			if (hasItemByField(Tab.Fields, Field.Name, &FIELD::Name))
			{
				const auto& B = getItemByField(Tab.Fields, Field.Name, &FIELD::Name);

				if (Field == B) Fields.append(Field.Name);
			}
		}

		for (const auto& Item : i.value())
		{
			if (Tab.Name != Class) Query.exec(QString(
				"INSERT INTO "
					"%1 (UIDO, %2) "
				"SELECT "
					"UIDO, %2 "
				"FROM "
					"%3 "
				"WHERE "
					"UIDO = '%4'")
					 .arg(Table.Data)
					 .arg(Fields.replaceInStrings("EW_DATA.", "").join(", "))
					 .arg(i.key())
					 .arg(Item));

			if (Tab.Name != Class) Query.exec(QString(
				"DELETE FROM "
					"%1 "
				"WHERE "
					"UIDO = '%2'")
					 .arg(i.key())
					 .arg(Item));

			Query.exec(QString(
				"UPDATE "
					"EW_POLYLINE "
				"SET "
					"TYP_LINII = ("
						"SELECT "
							"TYP_LINII "
						"FROM "
							"EW_WARSTWA_LINIOWA "
						"WHERE "
							"ID = '%1'"
					"), "
					"ID_WARSTWY = '%1' "
				"WHERE "
					"ID IN ("
						"SELECT "
							"IDE "
						"FROM "
							"EW_OB_ELEMENTY "
						"WHERE "
							"TYP = 0 AND "
							"UIDO = '%2'"
					")")
					 .arg(Line)
					 .arg(Item));

			Query.exec(QString(
				"UPDATE "
					"EW_TEXT "
				"SET "
					"TEXT = ("
						"SELECT "
							"NAZWA "
						"FROM "
							"EW_WARSTWA_TEXTOWA "
						"WHERE "
							"ID = '%1'"
					"), "
					"ID_WARSTWY = '%1' "
				"WHERE "
					"TYP = 4 AND "
					"ID IN ("
						"SELECT "
							"IDE "
						"FROM "
							"EW_OB_ELEMENTY "
						"WHERE "
							"TYP = 0 AND "
							"UIDO = '%2'"
					")")
					 .arg(Point)
					 .arg(Item));

			Query.exec(QString(
				"UPDATE "
					"EW_TEXT "
				"SET "
					"ID_WARSTWY = '%1' "
				"WHERE "
					"TYP = 6 AND "
					"ID IN ("
						"SELECT "
							"IDE "
						"FROM "
							"EW_OB_ELEMENTY "
						"WHERE "
							"TYP = 0 AND "
							"UIDO = '%2'"
					")")
					 .arg(Text)
					 .arg(Item));

			if (Tab.Name != Class) Query.exec(QString(
				"UPDATE "
					"EW_OBIEKTY "
				"SET "
					"KOD = '%1' "
				"WHERE "
					"UID = '%2'")
					 .arg(Class)
					 .arg(Item));

			emit onUpdateProgress(++Step);
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, Class, &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) emit onRowUpdate(j.key(), j.value());

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataRefactor();
}

void DatabaseDriver::restoreJob(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onJobsRestore(0); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 0);
	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0; int Count = 0;

	emit onBeginProgress(tr("Restoring job name"));
	emit onSetupProgress(0, Tasks.first().size());

	for (const auto& ID : Tasks.first())
	{
		int Now = -1; int Last = -1; emit onUpdateProgress(++Step);

		Query.prepare(QString(
			"SELECT "
				"O.OPERAT "
			"FROM "
				"EW_OBIEKTY O "
			"WHERE "
				"O.UID = '%1'")
				    .arg(ID));

		if (Query.exec() && Query.next()) Now = Query.value(0).toInt(); else continue;

		Query.prepare(QString(
			"SELECT FIRST 1 "
				"O.OPERAT "
			"FROM "
				"EW_OBIEKTY O "
			"WHERE "
				"O.STATUS = 3 AND O.ID = ("
					"SELECT U.ID FROM EW_OBIEKTY U WHERE U.UID = '%1'"
				") "
			"ORDER BY O.DTR ASCENDING")
				    .arg(ID));

		if (Query.exec() && Query.next()) Last = Query.value(0).toInt(); else continue;

		if (Last == Now || Last == -1 || Now == -1) continue; else ++Count;

		Query.exec(QString(
			"UPDATE "
				"EW_OBIEKTY O "
			"SET "
				"O.OPERAT = '%1'"
			"WHERE "
				"O.UID = '%2'")
				 .arg(Last)
				 .arg(ID));
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) emit onRowUpdate(j.key(), j.value());

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onJobsRestore(Count);
}

void DatabaseDriver::removeHistory(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onHistoryRemove(0); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 1);
	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0; int Count = 0;

	emit onBeginProgress(tr("Removing history"));
	emit onSetupProgress(0, Tasks.first().size());

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i) for (const auto& ID : i.value())
	{
		QList<int> Indexes;

		Query.prepare(QString(
			"SELECT "
				"O.UID "
			"FROM "
				"EW_OBIEKTY O "
			"WHERE "
				"O.ID = ("
					"SELECT U.ID FROM EW_OBIEKTY U WHERE U.UID = '%1'"
				") AND O.STATUS = 3")
				    .arg(ID));

		if (Query.exec()) while (Query.next()) Indexes.append(Query.value(0).toInt());

		for (const auto& Index : Indexes)
		{
			Query.exec(QString(
				"DELETE FROM "
					"EW_OBIEKTY "
				"WHERE "
					"UID = '%1'")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"EW_ELEMENTY "
				"WHERE "
					"UIDO = '%1'")
					 .arg(Index));

			Query.exec(QString(
				"DELETE FROM "
					"%1 "
				"WHERE "
					"UID = '%2'")
					 .arg(i.key())
					 .arg(Index));
		}

		Count += Indexes.count();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onHistoryRemove(Count);
}

void DatabaseDriver::editText(RecordModel* Model, const QModelIndexList& Items, bool Move, bool Justify, bool Rotate, bool Sort, double Length)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onTextEdit(0); return; }

	struct POINT
	{
		int ID;

		double WX, WY;
		double DX, DY;
		double A;

		unsigned J;

		bool Changed = false;
	};

	struct LINE
	{
		double X1, Y1;
		double X2, Y2;

		double Length;
	};

	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0;
	QMap<int, POINT> Points, Objects; QList<LINE> Lines; int Rejected = 0;
	const QList<int> Tasks = Model->getUids(Items);

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks.size());

	Query.prepare(
		"SELECT "
			"O.UID, T.UID, T.TYP, T.POS_X, T.POS_Y, T.JUSTYFIKACJA "
		"FROM "
			"EW_TEXT T "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"T.ID = E.IDE "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"E.UIDO = O.UID "
		"WHERE "
			"O.STATUS = 0 AND O.RODZAJ = 4 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 "
		"ORDER BY "
			"T.TYP DESC");

	if (Query.exec()) while (Query.next())
	{
		const int ID = Query.value(0).toInt();
		const int T = Query.value(2).toInt();

		if (!Objects.contains(ID)) Objects.insert(ID, POINT());

		POINT& Ref = Objects[ID];

		switch (T)
		{
			case 4:

				Ref.WX = Query.value(3).toDouble();
				Ref.WY = Query.value(4).toDouble();

			break;

			case 6:

				Ref.ID = Query.value(1).toInt();
				Ref.DX = Query.value(3).toDouble();
				Ref.DX = Query.value(4).toDouble();
				Ref.J = Query.value(5).toUInt();

			break;
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"P.P0_X, P.P0_Y, P.P1_X, P.P1_Y "
		"FROM "
			"EW_POLYLINE P "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"P.ID = E.IDE "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"E.UIDO = O.UID "
		"WHERE "
			"O.STATUS = 0 AND O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0");

	if (Query.exec()) while (Query.next())
	{
		Lines.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble(),
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		});
	}

	for (auto i = Objects.constBegin(); i != Objects.constEnd(); ++i)
	{
		if (Tasks.contains(i.key())) Points.insert(i.key(), i.value());
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading circles"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, P.UID, "
			"(P.P0_X + P.P1_X) / 2.0, "
			"(P.P0_Y + P.P1_Y) / 2.0 "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_POLYLINE P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 4 AND "
			"E.TYP = 0 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 3");

	if (Query.exec()) while (Query.next())
	{
		const int ID = Query.value(0).toInt();

		if (!Objects.contains(ID)) Objects.insert(ID,
		{
			Query.value(1).toInt(),
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		});
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Performing edit"));
	emit onSetupProgress(0, 0);

	QtConcurrent::blockingMap(Lines, [] (LINE& Line) -> void
	{
		const double dx = Line.X1 - Line.X2;
		const double dy = Line.Y1 - Line.Y2;

		Line.Length = qSqrt(dx * dx + dy * dy);
	});

	if (Sort) qSort(Lines.begin(), Lines.end(), [] (const LINE& A, const LINE& B) -> bool
	{
		return A.Length > B.Length;
	});

	QtConcurrent::blockingMap(Points, [&Lines, &Objects, Move, Justify, Rotate, Length] (POINT& Point) -> void
	{
		for (const auto& Object : Objects)
			if (Object.ID != Point.ID && (Object.WX == Point.WX && Object.WY == Point.WY)) return;

		if (Move) { Point.DX = Point.WX; Point.DY = Point.WY; Point.Changed = true; }

		if (Justify) for (const auto& Line : Lines)
			if ((Point.WX == Line.X1 && Point.WY == Line.Y1) || (Point.WX == Line.X2 && Point.WY == Line.Y2))
			{
				if (Justify && !Rotate)
				{
					if (Point.J == 1) { Point.J = 4; Point.Changed = true; return; }
				}
				else if (Justify && Rotate && (Line.Length >= Length))
				{
					Point.A = qAtan((Line.Y1 - Line.Y2) / (Line.X1 - Line.X2)) - M_PI / 2.0;

					if (Line.Y1 < Line.Y2)
					{
						if (Point.WX == Line.X1 && Point.WY == Line.Y1) Point.J = 4; else Point.J = 6;
					}
					else
					{
						if (Point.WX == Line.X2 && Point.WY == Line.Y2) Point.J = 4; else Point.J = 6;
					}

					while (Point.A < -(M_PI / 2.0)) Point.A += M_PI;
					while (Point.A > (M_PI / 2.0)) Point.A -= M_PI;

					Point.Changed = true; return;
				}
			}
	});

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Saving changes"));
	emit onSetupProgress(0, Points.size());

	for (const auto& Point : Points)
	{
		if (Point.Changed) Query.exec(QString(
			"UPDATE "
				"EW_TEXT "
			"SET "
				"POS_X = '%2', "
				"POS_Y = '%3', "
				"KAT = '%4', "
				"JUSTYFIKACJA = '%5' "
			"WHERE "
				"UID = '%1'")
				 .arg(Point.ID)
				 .arg(Point.DX, 0, 'f', -1)
				 .arg(Point.DY, 0, 'f', -1)
				 .arg(Point.A, 0, 'f', -1)
				 .arg(Point.J));
		else Rejected += 1;

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onTextEdit(Points.size() - Rejected);
}

QHash<int, QSet<int>> DatabaseDriver::joinSurfaces(const QHash<int, QSet<int>>& Geometry, const QList<DatabaseDriver::POINT>& Points, const QList<int>& Tasks, const QString Class, double Radius)
{
	if (!Database.isOpen()) return QHash<int, QSet<int>>();

	struct PART
	{
		int ID; double X1, Y1, R, X2, Y2;

		bool operator== (const PART& P)
		{
			return
				X1 == P.X1 && Y1 == P.Y1 &&
				X2 == P.X2 && Y2 == P.Y2 &&
				R == P.R;
		}
	};

	const auto SORT = [] (const QList<PART>& List) -> QPolygonF
	{
		QPolygonF Polygon; QSet<int> Used; bool Continue = true;

		if (!List.isEmpty()) while (Continue)
		{
			const int LastSize = Polygon.size();

			for (int i = 0; i < List.size(); ++i) if (List[i].R == 0.0)
			{
				if (Polygon.isEmpty())
				{
					Polygon.append(
					{
						List.first().X1, List.first().Y1
					});

					Used.insert(i);
				}
				else if (!Used.contains(i))
				{
					const QPointF A = { List[i].X1, List[i].Y1 };
					const QPointF B = { List[i].X2, List[i].Y2 };

					if (Polygon.last() == A) Polygon.append(B);
					else if (Polygon.last() == B) Polygon.append(A);
					else if (Polygon.first() == A) Polygon.insert(0, B);
					else if (Polygon.first() == B) Polygon.insert(0, A);

					if (LastSize != Polygon.size()) Used.insert(i);
				}
			}

			Continue = LastSize != Polygon.size();
		}

		return Polygon;
	};

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QSet<int> Used; int Step = 0;
	QMutex Locker; QMap<int, QList<PART>> Parts;

	Query.prepare(
		"SELECT "
			"O.UID, P.P1_FLAGS, "
			"P.P0_X, P.P0_Y, "
			"P.P1_X, P.P1_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_POLYLINE P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 3 AND "
			"O.KOD = :kod "
		"ORDER BY "
			"O.UID, E.N");

	Query.bindValue(":kod", Class);

	if (Query.exec()) while (Query.next())
	{
		const int ID = Query.value(0).toInt();

		if (Tasks.contains(ID))
		{
			if (!Parts.contains(ID)) Parts.insert(ID, QList<PART>());

			if (Query.value(1).toInt() == 4)
			{
				const double x = (Query.value(2).toDouble() + Query.value(4).toDouble()) / 2.0;
				const double y = (Query.value(3).toDouble() + Query.value(5).toDouble()) / 2.0;
				const double r = qAbs(Query.value(2).toDouble() - x);

				Parts[ID].append({ ID, x, y, r, 0.0, 0.0 });
			}
			else
			{
				Parts[ID].append(
				{
					ID,
					Query.value(2).toDouble(),
					Query.value(3).toDouble(),
					0.0,
					Query.value(4).toDouble(),
					Query.value(5).toDouble()
				});
			}
		}

		emit onUpdateProgress(++Step);
	}

	QtConcurrent::blockingMap(Parts, [&Insert, &Used, &Geometry, &Points, &Locker, SORT] (QList<PART>& List) -> void
	{
		const int ID = List.first().ID; const QPolygonF Polygon = SORT(List);

		for (const auto& P : Points) if (!Used.contains(P.ID))
		{
			for (const auto& G : List) if ((G.R != 0.0) && (G.R >= qSqrt(qPow(P.X - G.X1, 2) + qPow(P.Y - G.Y1, 2))))
			{
				Locker.lock();

				if (!Used.contains(P.ID) && !Geometry[G.ID].contains(P.ID))
				{
					Insert[G.ID].insert(P.ID);
					Used.insert(P.ID);
				}

				Locker.unlock();
			}

			if (Polygon.containsPoint(QPointF(P.X, P.Y), Qt::OddEvenFill))
			{
				Locker.lock();

				if (!Used.contains(P.ID) && !Geometry[ID].contains(P.ID))
				{
					Insert[ID].insert(P.ID);
					Used.insert(P.ID);
				}

				Locker.unlock();
			}
		}
	});

	return Insert;
}

QHash<int, QSet<int>> DatabaseDriver::joinLines(const QHash<int, QSet<int>>& Geometry, const QList<POINT>& Points, const QList<int>& Tasks, const QString Class, double Radius)
{
	if (!Database.isOpen()) return QHash<int, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QSet<int> Used; int Step = 0;

	Query.prepare(
		"SELECT "
			"O.UID, "
			"P.P0_X, P.P0_Y, "
			"P.P1_X, P.P1_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_POLYLINE P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"O.KOD = :kod");

	Query.bindValue(":kod", Class);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks.contains(Query.value(0).toInt())) for (const auto P : Points) if (!Used.contains(P.ID))
		{
			if ((qAbs(Query.value(1).toDouble() - P.X) <= Radius && qAbs(Query.value(2).toDouble() - P.Y) <= Radius) ||
			    (qAbs(Query.value(3).toDouble() - P.X) <= Radius && qAbs(Query.value(4).toDouble() - P.Y) <= Radius))
			{
				const int ID = Query.value(0).toInt();

				if (!Geometry[ID].contains(P.ID))
				{
					Insert[ID].insert(P.ID);
					Used.insert(P.ID);
				}
			}
		}

		emit onUpdateProgress(++Step);
	}

	return Insert;
}

QHash<int, QSet<int>> DatabaseDriver::joinPoints(const QHash<int, QSet<int>>& Geometry, const QList<POINT>& Points, const QList<int>& Tasks, const QString Class, double Radius)
{
	if (!Database.isOpen()) return QHash<int, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QSet<int> Used; int Step = 0;

	Query.prepare(
		"SELECT "
			"O.UID, "
			"T.POS_X, T.POS_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4 AND "
			"O.KOD = :kod");

	Query.bindValue(":kod", Class);

	if (Query.exec()) while (Query.next())
	{
		if (Tasks.contains(Query.value(0).toInt())) for (const auto P : Points) if (!Used.contains(P.ID))
		{
			if (qAbs(Query.value(1).toDouble() - P.X) <= Radius &&
			    qAbs(Query.value(2).toDouble() - P.Y) <= Radius)
			{
				const int ID = Query.value(0).toInt();

				if (!Geometry[ID].contains(P.ID))
				{
					Insert[ID].insert(P.ID);
					Used.insert(P.ID);
				}
			}
		}

		emit onUpdateProgress(++Step);
	}

	return Insert;
}

void DatabaseDriver::getCommon(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onCommonReady(QList<int>()); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 1);
	const QList<int> Used = getCommonFields(Tasks.keys());

	emit onEndProgress();
	emit onCommonReady(Used);
}

void DatabaseDriver::getPreset(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPresetReady(QList<QHash<int, QVariant>>(), QList<int>()); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 1);
	const QList<int> Used = getCommonFields(Tasks.keys());
	QList<QHash<int, QVariant>> Values; int Step = 0;

	emit onBeginProgress(tr("Preparing edit data"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		const auto Data = loadData(Table, i.value(), QString(), false, false);

		Values.append(Data.values());

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onPresetReady(Values, Used);
}

void DatabaseDriver::getJoins(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onJoinsReady(QHash<QString, QString>(), QHash<QString, QString>(), QHash<QString, QString>()); return; }

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), true, 0);
	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<QString, QString> Points, Lines, Circles; int Step = 0;

	if (Query.exec("SELECT COUNT(*) FROM EW_OBIEKTY WHERE RODZAJ IN (2, 4)") && Query.next())
	{
		Step = Query.value(0).toInt();
	}

	emit onBeginProgress(tr("Preparing classes"));
	emit onSetupProgress(0, Step); Step = 0;

	Query.prepare(
		"SELECT DISTINCT "
			"O.UID, O.RODZAJ, "
			"D.KOD, D.OPIS "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_OPISY D "
		"ON "
			"O.KOD = D.KOD "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ IN (2, 3, 4)");

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

void DatabaseDriver::getClass(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onClassReady(QHash<QString, QString>(), QHash<QString, QHash<int, QString>>(), QHash<QString, QHash<int, QString>>(), QHash<QString, QHash<int, QString>>()); return; }

	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0;
	QHash<QString, int> Types; QHash<QString, QString> Classes;
	QHash<QString, QHash<int, QString>> Lines, Points, Texts;

	const QMap<QString, QList<int>> Tasks = getClassGroups(Model->getUids(Items), false, 0);

	emit onBeginProgress(tr("Preparing classes"));
	emit onSetupProgress(0, 0); Step = 0;

	if (Query.exec("SELECT D.KOD, D.OPCJE FROM EW_OB_OPISY D")) while (Query.next())
	{
		Types.insert(Query.value(0).toString(), Query.value(1).toInt());
	}

	const int Type = Types[Tasks.firstKey()]; bool Continue = true;

	for (auto i = Tasks.constBegin(); Continue && i != Tasks.constEnd(); ++i)
	{
		for (auto j = Tasks.constBegin(); Continue && j != Tasks.constEnd(); ++j)
		{
			Continue = Types[i.key()] == Types[j.key()];
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Selecting layers data"));
	emit onSetupProgress(0, Types.size());

	if (Continue) for (const auto& Table : Tables) if (Types[Table.Name] == Type)
	{
		Classes.insert(Table.Name, Table.Label);

		QHash<int, QString> L, P, T;

		{
			Query.prepare(QString(
				"SELECT "
					"L.ID, G.NAZWA_L "
				"FROM "
					"EW_WARSTWA_LINIOWA L "
				"INNER JOIN "
					"EW_GRUPY_WARSTW G "
				"ON "
					"L.ID_GRUPY = G.ID "
				"INNER JOIN "
					"EW_OB_KODY_OPISY O "
				"ON "
					"G.ID = O.ID_WARSTWY "
				"WHERE "
					"O.KOD = '%1' AND "
					"L.NAZWA LIKE (O.KOD || '%') "
				"ORDER BY "
					"G.NAZWA_L")
					    .arg(Table.Name));

			if (Query.exec()) while (Query.next())
			{
				L.insert(Query.value(0).toInt(), Query.value(1).toString());
			}
		}

		if (L.isEmpty())
		{
			Query.prepare(QString(
				"SELECT "
					"L.ID, L.DLUGA_NAZWA "
				"FROM "
					"EW_WARSTWA_LINIOWA L "
				"INNER JOIN "
					"EW_GRUPY_WARSTW G "
				"ON "
					"L.ID_GRUPY = G.ID "
				"INNER JOIN "
					"EW_OB_KODY_OPISY O "
				"ON "
					"G.ID = O.ID_WARSTWY "
				"WHERE "
					"O.KOD = '%1' AND "
					"L.NAZWA LIKE (SUBSTRING(O.KOD FROM 1 FOR 4) || '%') "
				"ORDER BY "
					"L.DLUGA_NAZWA")
					    .arg(Table.Name));

			if (Query.exec()) while (Query.next())
			{
				L.insert(Query.value(0).toInt(), Query.value(1).toString());
			}
		}

		{
			Query.prepare(QString(
				"SELECT "
					"T.ID, G.NAZWA_L "
				"FROM "
					"EW_WARSTWA_TEXTOWA T "
				"INNER JOIN "
					"EW_GRUPY_WARSTW G "
				"ON "
					"T.ID_GRUPY = G.ID "
				"INNER JOIN "
					"EW_OB_KODY_OPISY O "
				"ON "
					"G.ID = O.ID_WARSTWY "
				"WHERE "
					"O.KOD = '%1' AND "
					"T.NAZWA LIKE (O.KOD || '_%') "
				"ORDER BY "
					"G.NAZWA_L")
					    .arg(Table.Name));

			if (Query.exec()) while (Query.next())
			{
				P.insert(Query.value(0).toInt(), Query.value(1).toString());
			}
		}

		{
			Query.prepare(QString(
				"SELECT "
					"T.ID, G.NAZWA_L "
				"FROM "
					"EW_WARSTWA_TEXTOWA T "
				"INNER JOIN "
					"EW_GRUPY_WARSTW G "
				"ON "
					"T.ID_GRUPY = G.ID "
				"INNER JOIN "
					"EW_OB_KODY_OPISY O "
				"ON "
					"G.ID = O.ID_WARSTWY "
				"WHERE "
					"O.KOD = '%1' AND "
					"T.NAZWA = O.KOD "
				"ORDER BY "
					"G.NAZWA_L")
					    .arg(Table.Name));

			if (Query.exec()) while (Query.next())
			{
				T.insert(Query.value(0).toInt(), Query.value(1).toString());
			}
		}

		if (T.isEmpty())
		{
			Query.prepare(QString(
				"SELECT "
					"T.ID, T.DLUGA_NAZWA "
				"FROM "
					"EW_WARSTWA_TEXTOWA T "
				"INNER JOIN "
					"EW_GRUPY_WARSTW G "
				"ON "
					"T.ID_GRUPY = G.ID "
				"INNER JOIN "
					"EW_OB_KODY_OPISY O "
				"ON "
					"G.ID = O.ID_WARSTWY "
				"WHERE "
					"O.KOD = '%1' AND "
					"T.NAZWA LIKE (SUBSTRING(O.KOD FROM 1 FOR 4) || '%') "
				"ORDER BY "
					"T.DLUGA_NAZWA")
					    .arg(Table.Name));

			if (Query.exec()) while (Query.next())
			{
				T.insert(Query.value(0).toInt(), Query.value(1).toString());
			}
		}

		if (T.isEmpty() || T.size() != T.values().toSet().size())
		{
			T.clear();

			Query.prepare(QString(
				"SELECT "
					"T.ID, G.NAZWA_L "
				"FROM "
					"EW_WARSTWA_TEXTOWA T "
				"INNER JOIN "
					"EW_GRUPY_WARSTW G "
				"ON "
					"T.ID_GRUPY = G.ID "
				"INNER JOIN "
					"EW_OB_KODY_OPISY O "
				"ON "
					"G.ID = O.ID_WARSTWY "
				"WHERE "
					"O.KOD = '%1' AND "
					"T.NAZWA LIKE (SUBSTRING(O.KOD FROM 1 FOR 4) || '%') "
				"ORDER BY "
					"G.NAZWA_L")
					    .arg(Table.Name));

			if (Query.exec()) while (Query.next())
			{
				T.insert(Query.value(0).toInt(), Query.value(1).toString());
			}
		}

		Lines.insert(Table.Name, L);
		Points.insert(Table.Name, P);
		Texts.insert(Table.Name, T);

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onClassReady(Classes, Lines, Points, Texts);
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
	if (!Value.isValid()) return QVariant();

	if (Type == DatabaseDriver::BOOL && Dict.isEmpty())
	{
		return Value.toBool() ? DatabaseDriver::tr("Yes") : DatabaseDriver::tr("No");
	}

	if (Type == DatabaseDriver::MASK && !Dict.isEmpty())
	{
		QStringList Values; const int Bits = Value.toInt();

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (Bits & (1 << i.key().toInt())) Values.append(i.value());
		}

		return Values.join(", ");
	}

	if (Dict.isEmpty()) return Value;

	if (Dict.contains(Value)) return Dict[Value];
	else return DatabaseDriver::tr("Unknown");
}

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
}

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
}

template<class Type, class Field, template<class> class Container>
bool hasItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return true; return false;
}
