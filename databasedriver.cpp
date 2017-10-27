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
	"=", "<>", ">=", ">", "<=", "<", "BETWEEN",
	"LIKE", "NOT LIKE",
	"IN", "NOT IN",
	"IS NULL", "IS NOT NULL"
};

DatabaseDriver::DatabaseDriver(QObject* Parent)
: Terminated(false), QObject(Parent)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Database = QSqlDatabase::addDatabase(Settings.value("driver", "QIBASE").toString());
	Settings.endGroup();
}

DatabaseDriver::~DatabaseDriver(void) {}

QString DatabaseDriver::getDatabaseName(void) const
{
	return QString("%1:%2@%3:%4")
			.arg(Database.driverName())
			.arg(Database.userName())
			.arg(Database.hostName())
			.arg(Database.databaseName());
}

QString DatabaseDriver::getDatabasePath(void) const
{
	return Database.databaseName();
}

bool DatabaseDriver::isTerminated(void) const
{
	QMutexLocker Locker(&Terminator); return Terminated;
}

QList<DatabaseDriver::FIELD> DatabaseDriver::loadCommon(bool Emit)
{
	if (!Database.isOpen()) return QList<FIELD>();

	QList<FIELD> Fields =
	{
		{ READONLY,	"EW_OBIEKTY.KOD",		tr("Object code")		},
		{ INTEGER,	"EW_OBIEKTY.OPERAT",	tr("Job name")			},
		{ READONLY,	"EW_OBIEKTY.NUMER",		tr("Object ID")		},
		{ READONLY,	"EW_OBIEKTY.IIP",		tr("GML identifier")	},
		{ DATETIME,	"EW_OBIEKTY.DTU",		tr("Creation date")		},
		{ DATETIME,	"EW_OBIEKTY.DTW",		tr("Modification date")	},
		{ INTEGER,	"EW_OBIEKTY.OSOU",		tr("Created by")		},
		{ INTEGER,	"EW_OBIEKTY.OSOW",		tr("Modified by")		}
	};

	QHash<QString, QString> Dict =
	{
		{ "EW_OBIEKTY.KOD",			"SELECT KOD, OPIS FROM EW_OB_OPISY"	},
		{ "EW_OBIEKTY.OPERAT",		"SELECT UID, NUMER FROM EW_OPERATY"	},
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
			"D.WARTOSC, D.OPIS, "
			"BIN_AND(O.OPCJE, 266) "
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
			(Query.value(3).toInt() & 0x100),
			Query.value(9).toInt()
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

QHash<QString, QSet<QString>> DatabaseDriver::loadVariables(void) const
{
	if (!Database.isOpen()) return QHash<QString, QSet<QString>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<QString, QSet<QString>> List;

	Query.prepare("SELECT DISTINCT NAZWA, OPIS FROM EW_OB_ZMIENNE");

	if (Query.exec()) while (Query.next())
	{
		const QString Label = Query.value(0).toString();

		if (!List.contains(Label)) List.insert(Label, QSet<QString>());

		List[Label].insert(Query.value(1).toString());
	}

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

QMap<QString, QSet<int>> DatabaseDriver::getClassGroups(const QSet<int>& Indexes, bool Common, int Index)
{
	if (!Database.isOpen()) return QMap<QString, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMap<QString, QSet<int>> List; int Step = 0;

	emit onBeginProgress(tr("Preparing queries"));
	emit onSetupProgress(0, Indexes.size());

	if (Common) List.insert("EW_OBIEKTY", QSet<int>());

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

	if (Query.exec()) while (Query.next() && !isTerminated()) if (Indexes.contains(Query.value(0).toInt()))
	{
		const QString Table = Query.value(Index + 1).toString();
		const int ID = Query.value(0).toInt();

		if (!List.contains(Table)) List.insert(Table, QSet<int>());

		List[Table].insert(ID);

		if (Common) List["EW_OBIEKTY"].insert(ID);

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); return List;
}

QHash<int, QHash<int, QVariant>> DatabaseDriver::loadData(const DatabaseDriver::TABLE& Table, const QSet<int>& Filter, const QString& Where, bool Dict, bool View)
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
		"LEFT JOIN "
			"%2 EW_DATA "
		"ON "
			"EW_OBIEKTY.UID = EW_DATA.UIDO "
		"WHERE "
			"EW_OBIEKTY.KOD = '%3' AND "
			"EW_OBIEKTY.STATUS = 0")
				.arg(Attribs.join(", "))
				.arg(Table.Data)
				.arg(Table.Name);

	if (!Where.isEmpty()) Exec.append(QString(" AND (%1)").arg(Where));

	if (Query.exec(Exec)) while (Query.next() && !isTerminated()) if (Filter.isEmpty() || Filter.contains(Query.value(0).toInt()))
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

QHash<int, QHash<int, QVariant>> DatabaseDriver::filterData(QHash<int, QHash<int, QVariant>> Data, const QHash<int, QVariant>& Geometry, const QString& Limiter, double Radius)
{
	if (!Database.isOpen()) return Data;

	struct OBJECT
	{
		int ID;

		QString Number;
		QString Class;

		QVariant Geometry;

		bool Filter = true;
	};

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
				"O.STATUS = 0 AND "
				"P.STAN_ZMIANY = 0 AND "
				"E.TYP = 0 "
			"GROUP BY "
				"O.UID "
			"HAVING "
				"COUNT(P.ID) = 0 OR SUM("
					"SQRT(POWER(P.P0_X - P.P1_X, 2) + POWER(P.P0_Y - P.P1_Y, 2))"
				") NOT BETWEEN :min AND :max");

		Query.bindValue(":min", Geometry.contains(0) ? Geometry[0].toDouble() : 0.0);
		Query.bindValue(":max", Geometry.contains(1) ? Geometry[1].toDouble() : 10000.0);

		if (Query.exec()) while (Query.next() && !isTerminated()) Data.remove(Query.value(0).toInt());
	}

	if (Geometry.contains(2) || Geometry.contains(3) ||
	    Geometry.contains(4) || Geometry.contains(5))
	{
		auto Process = [this, Radius] (auto i, const auto& Classes, const auto& Objects, auto List, auto Locker) -> void
		{
			bool End(false); for (auto j = Objects.constBegin(); j != Objects.constEnd(); ++j)
			{
				if (!j.value().Filter || i.key() == j.key() || j.value().Geometry.type() != QVariant::PointF ||
				    (!Classes.contains("*") && !Classes.contains(j.value().Class))) continue;

				if (i.value().Geometry.type() == QVariant::PointF)
				{
					const QLineF Distance(i.value().Geometry.toPointF(), j.value().Geometry.toPointF());

					if (Distance.length() <= Radius)
					{
						Locker->lock();
						List->insert(i.key());
						Locker->unlock();

						End = true;
					}
				}
				else for (const auto& Point : i.value().Geometry.toList())
				{
					const QLineF Distance(Point.toPointF(), j.value().Geometry.toPointF());

					if (Distance.length() <= Radius)
					{
						Locker->lock();
						List->insert(i.key());
						Locker->unlock();

						End = true;
					}
				}

				if (End || this->isTerminated()) return;
			}
		};

		QSqlQuery Query(Database); Query.setForwardOnly(true);
		QHash<int, OBJECT> ObjectsA, ObjectsB;

		const bool FilterA = Geometry.contains(2) || Geometry.contains(3);
		const bool FilterB = Geometry.contains(4) || Geometry.contains(5);

		Query.prepare(
			"SELECT "
				"O.UID, O.KOD, "
				"ROUND(T.POS_X, 3), "
				"ROUND(T.POS_Y, 3), "
				"O.NUMER "
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
				"T.STAN_ZMIANY = 0 AND "
				"E.TYP = 0 AND "
				"T.TYP = 4");

		if (Query.exec()) while (Query.next() && !isTerminated())
		{
			const OBJECT Object =
			{
				Query.value(0).toInt(),
				Query.value(4).toString(),
				Query.value(1).toString(),
				QPointF(Query.value(2).toDouble(), Query.value(3).toDouble()),
				Limit.isEmpty() || Limit.contains(Query.value(4).toString())
			};

			if (FilterA) ObjectsA.insert(Query.value(0).toInt(), Object);
			if (FilterB) ObjectsB.insert(Query.value(0).toInt(), Object);
		}

		Query.prepare(
			"SELECT "
				"O.UID, O.KOD, "
				"ROUND(P.P0_X, 3), ROUND(P.P0_Y, 3), "
				"IIF(P.PN_X IS NULL, ROUND(P.P1_X, 3), ROUND(P.PN_X, 3)), "
				"IIF(P.PN_Y IS NULL, ROUND(P.P1_Y, 3), ROUND(P.PN_Y, 3)), "
				"O.NUMER "
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
				"P.STAN_ZMIANY = 0 AND "
				"P.P1_FLAGS IN (0, 2) AND "
				"E.TYP = 0");

		if (Query.exec()) while (Query.next()  && !isTerminated())
		{
			const int Index = Query.value(0).toInt();

			const QPointF PointA = QPointF(Query.value(2).toDouble(),
									 Query.value(3).toDouble());
			const QPointF PointB = QPointF(Query.value(4).toDouble(),
									 Query.value(5).toDouble());

			if (FilterA)
			{
				if (!ObjectsA.contains(Index)) ObjectsA.insert(Index,
				{
					Query.value(0).toInt(),
					Query.value(6).toString(),
					Query.value(1).toString(),
					QVariant(QVariant::List)
				});

				auto ListA = ObjectsA[Index].Geometry.toList();

				if (!ListA.contains(PointA)) ListA.append(PointA);
				if (!ListA.contains(PointB)) ListA.append(PointB);

				ObjectsA[Index].Geometry.setValue(ListA);
			}

			if (FilterB)
			{
				if (!ObjectsB.contains(Index)) ObjectsB.insert(Index, {
					Query.value(0).toInt(),
					Query.value(6).toString(),
					Query.value(1).toString(),
					QVariant(QVariant::List)
				});

				auto ListB = ObjectsB[Index].Geometry.toList();

				if (!ListB.contains(PointA)) ListB.append(PointA);
				else ListB.removeOne(PointA);
				if (!ListB.contains(PointB)) ListB.append(PointB);
				else ListB.removeOne(PointB);

				ObjectsB[Index].Geometry.setValue(ListB);
			}
		}

		QFutureSynchronizer<void> Synchronizer;
		QMutex LockerA, LockerB; QSet<int> ListA, ListB;

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

		if (Query.exec()) while (Query.next() && !isTerminated())
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
			while (Query.next() && !isTerminated()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
			{
				const int ID = Query.value(0).toInt();

				if (CountA.contains(ID)) ++CountA[ID];
			}
		}

		if (Geometry.contains(7) && Query.exec(Select
			.arg(Geometry[7].toStringList().contains("*"))
			.arg(Geometry[7].toStringList().join("', '"))))
		{
			while (Query.next() && !isTerminated()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
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

		if (Query.exec()) while (Query.next() && !isTerminated())
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
			while (Query.next() && !isTerminated()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
			{
				const int ID = Query.value(0).toInt();

				if (CountA.contains(ID)) ++CountA[ID];
			}
		}

		if (Geometry.contains(9) && Query.exec(Select
			.arg(Geometry[9].toStringList().contains("*"))
			.arg(Geometry[9].toStringList().join("', '"))))
		{
			while (Query.next() && !isTerminated()) if (Limit.isEmpty() || Limit.contains(Query.value(1).toString()))
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

		if (Query.exec()) while (Query.next() && !isTerminated())
		{
			if (Geometry.contains(10)) CountA.append(qMakePair(Query.value(0).toInt(), 0));
			if (Geometry.contains(11)) CountB.append(qMakePair(Query.value(0).toInt(), 0));
		}

		Query.prepare(
			"SELECT "
				"O.KOD, "
				"ROUND(P.P0_X, 3), ROUND(P.P0_Y, 3), "
				"IIF(P.PN_X IS NULL, ROUND(P.P1_X, 3), ROUND(P.PN_X, 3)), "
				"IIF(P.PN_Y IS NULL, ROUND(P.P1_Y, 3), ROUND(P.PN_Y, 3)), "
				"O.NUMER "
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
				"P.STAN_ZMIANY = 0 AND "
				"P.P1_FLAGS IN (0, 2) AND "
				"E.TYP = 0");

		if (Query.exec()) while (Query.next() && !isTerminated()) if (Limit.isEmpty() || Limit.contains(Query.value(5).toString()))
		{
			const QString Class = Query.value(0).toString();

			if (!Lines.contains(Class)) Lines.insert(Class, QList<QPointF>());

			Lines[Class].append(QPointF(Query.value(1).toDouble(), Query.value(2).toDouble()));
			Lines[Class].append(QPointF(Query.value(3).toDouble(), Query.value(4).toDouble()));
		}

		Query.prepare(
			"SELECT "
				"O.UID, ROUND(T.POS_X, 3), ROUND(T.POS_Y, 3), O.NUMER "
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
				"T.STAN_ZMIANY = 0 AND "
				"E.TYP = 0 AND "
				"T.TYP = 4");

		if (Query.exec()) while (Query.next() && !isTerminated())
		{
			Points.insert(Query.value(0).toInt(), QPointF(Query.value(1).toDouble(), Query.value(2).toDouble()));
		}

		if (Geometry.contains(10)) QtConcurrent::blockingMap(CountA, [this, &Points, &Lines, &Geometry, Radius] (QPair<int, int>& Value) -> void
		{
			if (this->isTerminated()) return;

			for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
			{
				if (Geometry[10].toStringList().contains("*") || Geometry[10].toStringList().contains(i.key()))
				{
					for (const auto Point : i.value())
					{
						const QLineF Distance(Points.value(Value.first), Point);

						if (Distance.length() <= Radius) ++Value.second;
					}
				}
			}
		});

		if (Geometry.contains(11)) QtConcurrent::blockingMap(CountB, [this, &Points, &Lines, &Geometry, Radius] (QPair<int, int>& Value) -> void
		{
			if (this->isTerminated()) return;

			for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
			{
				if (Geometry[11].toStringList().contains("*") || Geometry[11].toStringList().contains(i.key()))
				{
					for (const auto Point : i.value())
					{
						const QLineF Distance(Points.value(Value.first), Point);

						if (Distance.length() <= Radius) ++Value.second;
					}
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

		if (Query.exec(Select)) while (Query.next() && !isTerminated()) Data.remove(Query.value(0).toInt());
	}

	if (Geometry.contains(100))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		const QString Select = QString("SELECT O.UID FROM EW_OBIEKTY O "
								 "WHERE O.STATUS = 0 AND O.RODZAJ NOT IN ('%1')")
						   .arg(Geometry[100].toStringList().join("', '"));

		if (Query.exec(Select)) while (Query.next() && !isTerminated()) Data.remove(Query.value(0).toInt());
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

		Variables = loadVariables();

		emit onEndProgress(); emit onConnect(Fields, Tables, Headers, Common.size(), Variables);
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

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	QSet<int> UIDS; UIDS.reserve(Filter.size()); int Step = 0;
	const QSet<QString> Hash = Filter.toSet();

	emit onBeginProgress(tr("Preparing objects list"));
	emit onSetupProgress(0, Hash.size());

	Query.prepare("SELECT UID, NUMER FROM EW_OBIEKTY WHERE STATUS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated()) if (Hash.contains(Query.value(1).toString()))
	{
		UIDS.insert(Query.value(0).toInt()); emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	RecordModel* Model = new RecordModel(Headers, this); Step = 0;

	if (!isTerminated()) for (const auto& Table : Tables)
	{
		if (isTerminated()) break;

		auto Data = loadData(Table, QSet<int>(), QString(), true, true);

		for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
			if (UIDS.contains(i.key()))
			{
				Model->addItem(i.key(), i.value());
			}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::reloadData(const QString& Filter, QList<int> Used, const QHash<int, QVariant>& Geometry, const QString& Limiter, double Radius, int Mode, const RecordModel* Current, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	if (Used.isEmpty()) Used = getUsedFields(Filter);

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	RecordModel* Model = new RecordModel(Headers, this); int Step = 0;
	QHash<int, QHash<int, QVariant>> List; QSet<int> Loaded;

	for (const auto& Table : Tables) if (hasAllIndexes(Table, Used))
	{
		if (isTerminated()) break;

		auto Data = loadData(Table, QSet<int>(), Filter, true, true);

		for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
		{
			List.insert(i.key(), i.value());
		}

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Applying geometry filters"));
	emit onSetupProgress(0, 0);

	if (!Geometry.isEmpty() && !isTerminated())
	{
		List = filterData(List, Geometry, Limiter, Radius);
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataLoad(Model); return; }

	emit onBeginProgress(tr("Creating object list"));
	emit onSetupProgress(0, 0);

	if (!Current) Mode = 0;
	else
	{
		QSet<int> Uids = Current->getUids(Items).toSet(); QSet<int> Exists;

		QSqlQuery Query("SELECT UID FROM EW_OBIEKTY WHERE STATUS = 0", Database);

		while (Query.next()) Exists.insert(Query.value(0).toInt());

		Loaded = Uids.intersect(Exists);
	}

	switch (Mode)
	{
		case 1:
		{
			for (auto i = List.constBegin(); i != List.constEnd(); ++i)
			{
				if (Loaded.contains(i.key())) Model->addItem(i.key(), i.value());
			}
		}
		break;
		case 2:
		{
			const auto Old = Loaded.subtract(List.keys().toSet());

			for (const auto& i : Old)
			{
				Model->addItem(i, Current->fullData(Current->index(i)));
			}

			for (auto i = List.constBegin(); i != List.constEnd(); ++i)
			{
				Model->addItem(i.key(), i.value());
			}
		}
		break;
		case 3:
		{
			const auto Old = Loaded.subtract(List.keys().toSet());

			for (const auto& i : Old)
			{
				Model->addItem(i, Current->fullData(Current->index(i)));
			}
		}
		break;
		default:
		{
			Model->addItems(List);
		}
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::updateData(RecordModel* Model, const QModelIndexList& Items, const QHash<int, QVariant>& Values, bool Emit)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataUpdate(Model); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), true, 1);
	const QSet<int> Used = Values.keys().toSet(); int Step = 0; QStringList All;
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
		if (isTerminated()) break;

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
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);

		QStringList Names, Updates;

		for (const auto& Index : Used) if (Table.Indexes.contains(Index))
		{
			Names.append(QString(Fields[Index].Name).remove("EW_DATA."));

			if (Values[Index].isNull()) Updates.append("NULL");
			else Updates.append(QString("'%1'").arg(Values[Index].toString()));
		}

		if (!Updates.isEmpty()) for (const auto& Index : i.value())
		{
			if (isTerminated()) break;

			Query.exec(QString(
				"UPDATE OR INSERT INTO %1 (UIDO, %2) "
				"VALUES (%3, %4) MATCHING (UIDO)")
					 .arg(Table.Data)
					 .arg(Names.join(", "))
					 .arg(Index)
					 .arg(Updates.join(", ")));

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

	if (!Emit) return;

	emit onEndProgress();
	emit onDataUpdate(Model);
}

void DatabaseDriver::removeData(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRemove(Model); return; }

	const QSet<int> List = Model->getUids(Items).toSet();
	const QMap<QString, QSet<int>> Tasks = getClassGroups(List, false, 1);

	QSqlQuery selectLines(Database), selectTexts(Database), selectUIDS(Database),
			QueryA(Database), QueryB(Database), QueryC(Database),
			QueryD(Database), QueryE(Database), QueryF(Database);

	QSet<int> Lines, Texts; QHash<int, int> UIDS; int Step = 0;
	const QString deleteQuery = QString("DELETE FROM %1 WHERE UIDO = ?");

	selectLines.prepare(
		"SELECT "
			"O.UID, P.ID "
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
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0");

	selectTexts.prepare(
		"SELECT "
			"O.UID, P.ID "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0");

	selectUIDS.prepare("SELECT DISTINCT UID, ID FROM EW_OBIEKTY WHERE STATUS = 0");

	QueryA.prepare("DELETE FROM EW_TEXT WHERE ID = ?");
	QueryB.prepare("DELETE FROM EW_POLYLINE WHERE ID = ?");
	QueryC.prepare("DELETE FROM EW_OB_ELEMENTY WHERE IDE = ? AND TYP = 1");
	QueryD.prepare("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = ?");
	QueryE.prepare("DELETE FROM EW_OBIEKTY WHERE UID = ?");

	emit onBeginProgress(tr("Loading items"));
	emit onSetupProgress(0, List.size());

	if (selectUIDS.exec()) while (selectUIDS.next() && !isTerminated())
	{
		UIDS.insert(selectUIDS.value(0).toInt(),
				  selectUIDS.value(1).toInt());
	}

	for (const auto UID : List)
	{
		if (isTerminated()) break;

		selectLines.addBindValue(UID);

		if (selectLines.exec()) while (selectLines.next())
		{
			Lines.insert(selectLines.value(1).toInt());
		}

		selectTexts.addBindValue(UID);

		if (selectTexts.exec()) while (selectTexts.next())
		{
			Texts.insert(selectTexts.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataRemove(Model); return; }

	emit onBeginProgress(tr("Removing objects"));
	emit onSetupProgress(0, Items.size()); Step = 0;

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		QueryF.prepare(deleteQuery.arg(i.key()));

		for (const auto& Index : i.value())
		{
			QueryC.addBindValue(UIDS[Index]); QueryC.exec();
			QueryD.addBindValue(Index); QueryD.exec();
			QueryE.addBindValue(Index); QueryE.exec();
			QueryF.addBindValue(Index); QueryF.exec();

			emit onUpdateProgress(++Step);
		}
	}

	emit onBeginProgress(tr("Removing geometry")); Step = 0;
	emit onSetupProgress(0, Lines.size() + Texts.size());

	for (const auto& ID : Lines)
	{
		QueryB.addBindValue(ID); QueryB.exec();

		emit onUpdateProgress(++Step);
	}

	for (const auto& ID : Texts)
	{
		QueryA.addBindValue(ID); QueryA.exec();

		emit onUpdateProgress(++Step);
	}

	for (const auto Item : Items) emit onRowRemove(Item);

	emit onEndProgress();
	emit onDataRemove(Model);
}

void DatabaseDriver::execBatch(RecordModel* Model, const QModelIndexList& Items, const QList<QPair<int, BatchWidget::FUNCTION>>& Functions, const QList<QStringList>& Values)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onBatchExec(0); return; } int Changes(0);

	emit onBeginProgress(tr("Executing batch"));
	emit onSetupProgress(0, 0);

	for (const auto& Rules : Values)
	{
		if (isTerminated()) break;

		QModelIndexList Filtered; QHash<int, QVariant> Updates;

		for (const auto& Index : Items)
		{
			const auto& Data = Model->fullData(Index); bool OK = true; int Col = 0;

			for (const auto& F : Functions) if (OK)
			{
				if (F.second == BatchWidget::WHERE) OK = Data[F.first].toString() == Rules[Col]; ++Col;
			}

			if (OK) Filtered.append(Index);
		}

		if (Filtered.size()) for (const auto& Table : Tables)
		{
			int Col(0); for (const auto& F : Functions)
			{
				if (F.second == BatchWidget::UPDATE)
				{
					const int Column = Table.Headers.indexOf(F.first);

					if (Column != -1 && !Updates.contains(Table.Indexes[Column]))
					{
						const int Ufid = Table.Indexes[Column];

						if (Table.Fields[Column].Dict.isEmpty()) Updates.insert(Ufid, Rules[Col]);
						else if (Ufid < Common.size())
						{
							if (Common[Ufid].Type != READONLY) Updates.insert(Ufid, getDataByDict(Rules[Col], Common[Ufid].Dict, Common[Ufid].Type));
						}
						else
						{
							if (Fields[Ufid].Type != READONLY) Updates.insert(Ufid, getDataByDict(Rules[Col], Fields[Ufid].Dict, Fields[Ufid].Type));
						}
					}
				}

				Col += 1;
			}
		}

		if (!isTerminated() && Filtered.size() && Updates.size())
		{
			updateData(Model, Filtered, Updates, false); Changes += Filtered.size();
		}
	}

	emit onEndProgress();
	emit onBatchExec(Changes);
}

void DatabaseDriver::splitData(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& From, int Type)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataSplit(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), false, 0);
	QSet<int> Points; QHash<int, QList<int>> Objects; int Step = 0; int Count = 0;
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

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		if (Tasks[Point].contains(Query.value(0).toInt()))
		{
			Points.insert(Query.value(1).toInt());
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

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		if (Tasks[From].contains(Query.value(0).toInt()))
		{
			const int ID = Query.value(0).toInt();

			if (!Objects.contains(ID)) Objects.insert(ID, QList<int>());

			Objects[ID].append(Query.value(1).toInt());
		}

		emit onUpdateProgress(++Step);
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataSplit(0); return; }

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Splitting data"));
	emit onSetupProgress(0, Objects.size());

	Query.prepare("DELETE FROM EW_OB_ELEMENTY WHERE TYP = 1 AND UIDO = ? AND IDE = ?");

	for (auto i = Objects.constBegin(); i != Objects.constEnd(); ++i)
	{
		for (const auto& Index : i.value()) if (Points.contains(Index))
		{
			Query.addBindValue(i.key());
			Query.addBindValue(Index);

			Query.exec(); Count += 1;
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataSplit(Count);
}

void DatabaseDriver::joinData(RecordModel* Model, const QModelIndexList& Items, const QString& Point, const QString& Join, bool Override, int Type, double Radius)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataJoin(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), false, 0);
	QList<POINT> Points; QSet<int> Joined; QHash<int, QSet<int>> Geometry, Insert;
	QSqlQuery Query(Database), QueryA(Database), QueryB(Database); Query.setForwardOnly(true);
	int Step = 0; int Count = 0;

	if (!Tasks.contains(Point) || !Tasks.contains(Join)) { emit onDataJoin(0); return; }

	emit onBeginProgress(tr("Checking used items"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT DISTINCT "
			"E.IDE "
		"FROM "
			"EW_OB_ELEMENTY E "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"E.UIDO = O.UID "
		"WHERE "
			"E.TYP = 1 AND "
			"O.STATUS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		Joined.insert(Query.value(0).toInt());
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
			"(E.IDE = T.ID AND E.TYP = 0) "
		"WHERE "
			"O.KOD = ? AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4");

	Query.addBindValue(Point);

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();
		const int ID = Query.value(1).toInt();

		if (Tasks[Point].contains(UID))
		{
			if (!Joined.contains(ID)) Points.append(
			{
				Query.value(1).toInt(),
				Query.value(2).toDouble(),
				Query.value(3).toDouble()
			});

			emit onUpdateProgress(++Step);
		}
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
			"O.KOD = ? AND "
			"E.TYP = 1 AND "
			"O.STATUS = 0");

	Query.addBindValue(Join);

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();

		if (Tasks[Join].contains(UID))
		{
			const int ID = Query.value(0).toInt();

			if (!Geometry.contains(ID)) Geometry.insert(ID, QSet<int>());
			if (!Insert.contains(ID)) Insert.insert(ID, QSet<int>());

			Geometry[ID].insert(Query.value(1).toInt());

			emit onUpdateProgress(++Step);
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, 0);

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

	if (isTerminated()) { emit onEndProgress(); emit onDataJoin(0); return; }

	QueryA.prepare(
		"INSERT INTO "
			"EW_OB_ELEMENTY (UIDO, IDE, TYP, N, ATRYBUT) "
		"VALUES "
			"(?, ?, 1, (SELECT MAX(N) FROM EW_OB_ELEMENTY WHERE UIDO = ?) + 1, 0)");

	QueryB.prepare(
		"UPDATE "
			"EW_OBIEKTY "
		"SET "
			"OPERAT = (SELECT OPERAT FROM EW_OBIEKTY WHERE UID = ?) "
		"WHERE "
			"ID = ?");

	for (auto i = Insert.constBegin(); i != Insert.constEnd(); ++i)
	{
		for (const auto& P : i.value())
		{
			QueryA.addBindValue(i.key());
			QueryA.addBindValue(P);
			QueryA.addBindValue(i.key());

			QueryA.exec();

			if (!Override) continue;

			QueryB.addBindValue(i.key());
			QueryB.addBindValue(P);

			QueryB.exec();
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

	struct PART { int ID, Type; double X1, Y1, X2, Y2; bool Text; };

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), true, 0);
	QHash<int, QList<QPointF>> Geometry; QSet<int> Used; QList<int> Counts; QList<QPointF> Ends;
	QHash<int, QSet<int>> Merges; QList<QPointF> Cuts; int Step = 0; QSet<int> Merged;
	QSqlQuery Query(Database); Query.setForwardOnly(true); QHash<int, QList<PART>> Geometries;

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	if (!Points.isEmpty())
	{
		Query.prepare(QString(
			"SELECT "
				"ROUND(T.POS_X, 3), "
				"ROUND(T.POS_Y, 3), "
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});

		Query.prepare(QString(
			"SELECT "
				"ROUND((P.P0_X + P.P1_X) / 2.0, 3), "
				"ROUND((P.P0_Y + P.P1_Y) / 2.0, 3) "
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.append(
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
			"ROUND(P.P0_X, 3), "
			"ROUND(P.P0_Y, 3), "
			"ROUND(P.P1_X, 3), "
			"ROUND(P.P1_Y, 3), "
			"E.IDE, E.TYP, "
			"IIF(P.ID IS NULL, 1, 0) "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"LEFT JOIN "
			"EW_POLYLINE P "
		"ON "
			"("
				"E.IDE = P.ID AND "
				"P.STAN_ZMIANY = 0 AND "
				"E.TYP = 0 "
			")"
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 "
		"ORDER BY "
			"O.UID, E.N ASC");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int ID = Query.value(0).toInt();

		if (Tasks.first().contains(ID))
		{
			const QPointF PointA(Query.value(1).toDouble(), Query.value(2).toDouble());
			const QPointF PointB(Query.value(3).toDouble(), Query.value(4).toDouble());

			if (!Query.value(6).toBool() && !Query.value(7).toBool())
			{
				if (!Geometry.contains(ID)) Geometry.insert(ID, QList<QPointF>());

				if (!Geometry[ID].contains(PointA)) Geometry[ID].append(PointA);
				else Geometry[ID].removeOne(PointA);
				if (!Geometry[ID].contains(PointB)) Geometry[ID].append(PointB);
				else Geometry[ID].removeOne(PointB);
			}

			if (!Geometries.contains(ID)) Geometries.insert(ID, QList<PART>());

			Geometries[ID].append(
			{
				Query.value(5).toInt(),
				Query.value(6).toInt(),
				Query.value(1).toDouble(),
				Query.value(2).toDouble(),
				Query.value(3).toDouble(),
				Query.value(4).toDouble(),
				Query.value(7).toBool()
			});
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
		if (isTerminated()) break;

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

	if (isTerminated()) { emit onEndProgress(); emit onDataMerge(0); return; }

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating database"));
	emit onSetupProgress(0, Merges.size());

	for (auto k = Tasks.constBegin() + 1; k != Tasks.constEnd(); ++k)
	{
		const auto& Table = getItemByField(Tables, k.key(), &TABLE::Name);

		for (const auto& Index : k.value()) if (Merges.contains(Index))
		{
			QList<PART> Parts, Sorted, Labels; QSet<int> Taken; bool Continue = true; int n = 0;

			for (const auto& Part : Merges[Index])
			{
				{
					QVariantList ValuesA, ValuesB, finallValues; QStringList Params;

					Query.exec(QString("SELECT * FROM %1 WHERE UIDO = %2").arg(Table.Data).arg(Index));

					if (Query.next()) for (int i = 0; i < Query.record().count(); ++i) ValuesA.append(Query.value(i));

					Query.exec(QString("SELECT * FROM %1 WHERE UIDO = %2").arg(Table.Data).arg(Part));

					if (Query.next()) for (int i = 0; i < Query.record().count(); ++i) ValuesB.append(Query.value(i));

					if (!ValuesA.isEmpty() && !ValuesB.isEmpty()) for (int i = 0; i < ValuesA.size(); ++i)
					{
						finallValues.append(isVariantEmpty(ValuesA[i]) ? ValuesB[i] : ValuesA[i]);
					}
					else if (!ValuesB.isEmpty()) finallValues = ValuesB;

					if (!finallValues.isEmpty())
					{
						for (int i = 0; i < finallValues.size(); ++i) Params.append("?");

						Query.prepare(QString("UPDATE OR INSERT INTO %1 "
										  "VALUES (%2) MATCHING (UIDO)")
								    .arg(Table.Data)
								    .arg(Params.join(", ")));

						for (const auto& V : finallValues) Query.addBindValue(V); Query.exec();
					}
				}

				{
					QVariantList ValuesA, ValuesB; QStringList Params;

					Query.exec(QString("SELECT * FROM EW_OBIEKTY WHERE UID = %1").arg(Index));

					if (Query.next()) for (int i = 0; i < Query.record().count(); ++i) ValuesA.append(Query.value(i));

					Query.exec(QString("SELECT * FROM EW_OBIEKTY WHERE UID = %1").arg(Part));

					if (Query.next()) for (int i = 0; i < Query.record().count(); ++i) ValuesB.append(Query.value(i));

					for (int i = 8; i < ValuesA.size(); ++i)
					{
						ValuesA[i] = isVariantEmpty(ValuesA[i]) ? ValuesB[i] : ValuesA[i];
					}

					for (int i = 0; i < ValuesA.size(); ++i) Params.append("?");

					Query.prepare(QString("UPDATE OR INSERT INTO EW_OBIEKTY "
									  "VALUES (%1) MATCHING (UID)")
							    .arg(Params.join(", ")));

					for (const auto& V : ValuesA) Query.addBindValue(V); Query.exec();
				}

				Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1").arg(Part));

				if (Part == Index) continue; else Merged.insert(Part);

				Query.exec(QString("DELETE FROM EW_OBIEKTY WHERE UID = %1").arg(Part));
				Query.exec(QString("DELETE FROM %1 WHERE UIDO = %2").arg(Table.Data, Part));

				Parts.append(Geometries[Part]);
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

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j)
		{
			if (!Merged.contains(j.key())) emit onRowUpdate(j.key(), j.value());
		}

		emit onUpdateProgress(++Step);
	}

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
					const double A = P.X - L.X1; const double B = P.Y - L.Y1;
					const double C = L.X2 - L.X1; const double D = L.Y2 - L.Y1;

					const double h = qAbs(A * D - C * B) / qSqrt(C * C + D * D);

					if (qIsNaN(P.L) || h < P.L) { P.L = h; P.LID = L.ID; };
				}
			}
		};

		for (auto& Part : P.Labels) find(P.Lines, Part);
		for (auto& Part : P.Objects) find(P.Lines, Part);
	};

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), true, 0);
	QHash<int, PARTS> Parts; QList<QPointF> Cuts; QHash<int, QSet<int>> Queue; int Step = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true); QMutex Locker;

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	if (!Points.isEmpty())
	{
		Query.prepare(QString(
			"SELECT "
				"ROUND(T.POS_X, 3), "
				"ROUND(T.POS_Y, 3) "
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});

		Query.prepare(QString(
			"SELECT "
				"ROUND((P.P0_X + P.P1_X) / 2.0, 3), "
				"ROUND((P.P0_Y + P.P1_Y) / 2.0, 3) "
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.append(
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
				"ROUND(P.P0_X, 3), "
				"ROUND(P.P0_Y, 3), "
				"ROUND(P.P1_X, 3), "
				"ROUND(P.P1_Y, 3) "
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

		if (Query.exec()) while (Query.next() && !isTerminated())
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
	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, "
			"E.IDE, E.N, "
			"ROUND(P.P0_X, 3), "
			"ROUND(P.P0_Y, 3), "
			"ROUND(P.P1_X, 3), "
			"ROUND(P.P1_Y, 3) "
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

	if (Query.exec()) while (Query.next() && !isTerminated()) if (Tasks.first().contains(Query.value(0).toInt()))
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

	QtConcurrent::blockingMap(Cuts, [this, &Parts, &Queue, &Locker] (const QPointF& Point) -> void
	{
		if (this->isTerminated()) return;

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

	if (Query.exec()) while (Query.next() && !isTerminated()) if (Queue.contains(Query.value(0).toInt()))
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

	if (Query.exec()) while (Query.next() && !isTerminated()) if (Queue.contains(Query.value(0).toInt()))
	{
		const int ID = Query.value(0).toInt();

		if (Parts.contains(ID)) Parts[ID].Objects.append(
		{
			Query.value(1).toInt(), 0,
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		});
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataCut(0); return; }

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

		const QString objectInsert = QString("INSERT INTO EW_OBIEKTY (UID, NUMER, IDKATALOG, KOD, RODZAJ, OSOU, OSOW, DTU, DTW, OPERAT, STATUS) "
									  "SELECT %1, 'OB_ID_' || '%2', IDKATALOG, KOD, RODZAJ, OSOU, OSOW, DTU, DTW, OPERAT, STATUS FROM EW_OBIEKTY WHERE UID = %3");

		for (auto i = Queue.constBegin(); i != Queue.constEnd(); ++i) if (t.value().contains(i.key()))
		{
			QList<int> Jobs = i.value().values(); qSort(Jobs); int on = Jobs.first();

			Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1 AND N >= %2")
					 .arg(i.key()).arg(on));

			for (auto j = Jobs.constBegin(); j != Jobs.constEnd(); ++j)
			{
				int Index(0), n(0); const int Stop = (j + 1) == Jobs.constEnd() ? Parts[i.key()].Lines.size() : *(j + 1);

				Query.prepare("SELECT GEN_ID(EW_OBIEKTY_UID_GEN, 1) FROM RDB$DATABASE");

				if (Query.exec() && Query.next()) Index = Query.value(0).toInt();

				const QString Numer = QString::number(qHash(qMakePair(Index, QDateTime::currentDateTimeUtc())), 16);

				Query.exec(objectInsert.arg(Index).arg(Numer).arg(i.key()));
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

void DatabaseDriver::refactorData(RecordModel* Model, const QModelIndexList& Items, const QString& Class, int Line, int Point, int Text, const QString& Symbol, int Style, const QString& Label)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRefactor(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), false, 1);
	const auto& Table = getItemByField(Tables, Class, &TABLE::Name); QSet<int> Lines, Symbols, Texts;
	QSqlQuery Query(Database); Query.setForwardOnly(true); int LineStyle(0); QString NewSymbol; int Step = 0;
	QSqlQuery LineQuery(Database), SymbolQuery(Database), LabelQuery(Database),
			ClassQuery(Database), selectLines(Database), selectTexts(Database);

	const int Type = Class != "NULL" ? getItemByField(Tables, Class, &TABLE::Name).Type : 0;
	const QSet<int> List = Model->getUids(Items).toSet(); QSet<int> Change;

	if (Type == 0) Change = List;
	else if (Query.exec("SELECT UID, RODZAJ FROM EW_OBIEKTY WHERE STATUS = 0")) while (Query.next())
	{
		const int UID = Query.value(0).toInt();

		if (!List.contains(UID)) continue;

		bool OK(false); switch (Query.value(1).toInt())
		{
			case 2:
				OK = Type & 109;
			break;
			case 3:
				OK = Type & 103;
			break;
			case 4:
				OK = Type & 357;
			break;
		}

		if (OK) Change.insert(UID);
	}

	selectLines.prepare(
		"SELECT "
			"O.UID, P.ID "
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
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0");

	selectTexts.prepare(
		"SELECT "
			"O.UID, P.TYP, P.ID "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0");

	LineQuery.prepare(
		"UPDATE "
			"EW_POLYLINE "
		"SET "
			"TYP_LINII = COALESCE(?, TYP_LINII), "
			"ID_WARSTWY = COALESCE(?, ID_WARSTWY) "
		"WHERE "
			"ID = ?");

	SymbolQuery.prepare(
		"UPDATE "
			"EW_TEXT "
		"SET "
			"TEXT = COALESCE(?, TEXT), "
			"ID_WARSTWY = COALESCE(?, ID_WARSTWY) "
		"WHERE "
			"ID = ?");

	LabelQuery.prepare(
		"UPDATE "
			"EW_TEXT "
		"SET "
			"TEXT = COALESCE(?, TEXT), "
			"ID_WARSTWY = COALESCE(?, ID_WARSTWY) "
		"WHERE "
			"ID = ?");

	ClassQuery.prepare("UPDATE EW_OBIEKTY SET KOD = ? WHERE UID = ?");

	if (!Style && Line != -1)
	{
		Query.prepare("SELECT TYP_LINII FROM EW_WARSTWA_LINIOWA WHERE ID = ?"); Query.addBindValue(Line);

		if (Query.exec() && Query.next()) LineStyle = Query.value(0).toInt();
	}
	else LineStyle = Style;

	if (Symbol.isEmpty() && Point != -1)
	{
		Query.prepare("SELECT NAZWA FROM EW_WARSTWA_TEXTOWA WHERE ID = ?"); Query.addBindValue(Point);

		if (Query.exec() && Query.next()) NewSymbol = Query.value(0).toString();
	}
	else NewSymbol = Symbol;

	const QVariant vClass = Class == "NULL" ? QVariant() : Class;
	const QVariant vSymbol = Symbol == "NULL" ? QVariant() : NewSymbol;
	const QVariant vLabel = Label == "NULL" ? QVariant() : Label;

	const QVariant vStyle = Style == -1 ? QVariant() : LineStyle;
	const QVariant vLine = Line == -1 ? QVariant() : Line;
	const QVariant vPoint = Point == -1 ? QVariant() : Point;
	const QVariant vText = Text == -1 ? QVariant() : Text;

	emit onBeginProgress(tr("Loading elements"));
	emit onSetupProgress(0, List.size());

	for (const auto& UID : Change)
	{
		if (isTerminated()) break;

		selectLines.addBindValue(UID);

		if (selectLines.exec()) while (selectLines.next())
		{
			Lines.insert(selectLines.value(1).toInt());
		}

		selectTexts.addBindValue(UID);

		if (selectTexts.exec()) while (selectTexts.next())
		{
			switch (selectTexts.value(1).toInt())
			{
				case 4:
					Symbols.insert(selectTexts.value(2).toInt());
				break;
				case 6:
					Texts.insert(selectTexts.value(2).toInt());
				break;
			}
		}

		emit onUpdateProgress(++Step);
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataRefactor(0); return; }

	emit onBeginProgress(tr("Updating class"));
	emit onSetupProgress(0, Change.size()); Step = 0;

	if (!vClass.isNull()) for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Tab = getItemByField(Tables, i.key(), &TABLE::Data);

		if (Tab.Name == Class) continue; QStringList Fields;

		for (const auto& Field : Table.Fields)
		{
			if (hasItemByField(Tab.Fields, Field.Name, &FIELD::Name))
			{
				const auto& B = getItemByField(Tab.Fields, Field.Name, &FIELD::Name);

				if (Field == B) Fields.append(Field.Name);
			}
		}

		for (const auto& Item : i.value()) if (Change.contains(Item))
		{
			Query.exec(QString("INSERT INTO %1 (UIDO, %2) "
						    "SELECT UIDO, %2 FROM %3 "
						    "WHERE UIDO = '%4'")
					 .arg(Table.Data)
					 .arg(Fields.replaceInStrings("EW_DATA.", "").join(", "))
					 .arg(i.key())
					 .arg(Item));

			Query.exec(QString("DELETE FROM %1 WHERE UIDO = '%2'")
					 .arg(i.key())
					 .arg(Item));

			ClassQuery.addBindValue(Class);
			ClassQuery.addBindValue(Item);

			ClassQuery.exec();

			emit onUpdateProgress(++Step);
		}
	}

	emit onBeginProgress(tr("Updating lines"));
	emit onSetupProgress(0, Lines.size()); Step = 0;

	if (!vLine.isNull() || !vStyle.isNull()) for (const auto& ID : Lines)
	{
		LineQuery.addBindValue(vStyle);
		LineQuery.addBindValue(vLine);
		LineQuery.addBindValue(ID);

		LineQuery.exec();

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Updating symbols"));
	emit onSetupProgress(0, Symbols.size()); Step = 0;

	if (!vSymbol.isNull() || !vPoint.isNull()) for (const auto& ID : Symbols)
	{
		SymbolQuery.addBindValue(vSymbol);
		SymbolQuery.addBindValue(vPoint);
		SymbolQuery.addBindValue(ID);

		SymbolQuery.exec();

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Updating texts"));
	emit onSetupProgress(0, Texts.size()); Step = 0;

	if (!vText.isNull() || !vLabel.isNull()) for (const auto& ID : Texts)
	{
		LabelQuery.addBindValue(vLabel);
		LabelQuery.addBindValue(vText);
		LabelQuery.addBindValue(ID);

		LabelQuery.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	if (!vClass.isNull()) for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, Class, &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) emit onRowUpdate(j.key(), j.value());

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataRefactor(Change.size());
}

void DatabaseDriver::copyData(RecordModel* Model, const QModelIndexList& Items, const QString& Class, int Line, int Point, int Text, const QString& Symbol, int Style)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataCopy(0); return; }

	struct ELEMENT { int IDE, Type; }; QHash<int, QList<ELEMENT>> Elements;

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), false, 1);
	const auto& Table = getItemByField(Tables, Class, &TABLE::Name); int Step = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true); int LineStyle(0); QString NewSymbol;
	QSqlQuery LineQuery(Database), SymbolQuery(Database), LabelQuery(Database),
			ClassQuery(Database), ElementsQuery(Database), GeometryQuery(Database);

	QSqlQuery getUidQuery(Database), getIdQuery(Database);

	const int Type = getItemByField(Tables, Class, &TABLE::Name).Type;
	const QSet<int> List = Model->getUids(Items).toSet(); QSet<int> Change;

	if (Query.exec("SELECT UID, RODZAJ FROM EW_OBIEKTY WHERE STATUS = 0")) while (Query.next())
	{
		const int UID = Query.value(0).toInt();

		if (!List.contains(UID)) continue;

		bool OK(false); switch (Query.value(1).toInt())
		{
			case 2:
				OK = Type & 109;
			break;
			case 3:
				OK = Type & 103;
			break;
			case 4:
				OK = Type & 357;
			break;
		}

		if (OK) Change.insert(UID);
	}

	LineQuery.prepare(
		"INSERT INTO "
			"EW_POLYLINE (ID, ID_WARSTWY, TYP_LINII, STAN_ZMIANY, OPERAT, POINTCOUNT, P0_X, P0_Y, P1_FLAGS, P1_X, P1_Y, PN_X, PN_Y) "
		"SELECT "
			"?, ?, ?, 0, OPERAT, POINTCOUNT, P0_X, P0_Y, P1_FLAGS, P1_X, P1_Y, PN_X, PN_Y "
		"FROM "
			"EW_POLYLINE "
		"WHERE "
			"ID = ?");

	SymbolQuery.prepare(
		"INSERT INTO "
			"EW_TEXT (ID, ID_WARSTWY, TEXT, STAN_ZMIANY, OPERAT, KAT, POS_X, POS_Y, JUSTYFIKACJA, ODN_X, ODN_Y, TYP) "
		"SELECT "
			"?, ?, ?, 0, OPERAT, KAT, POS_X, POS_Y, JUSTYFIKACJA, ODN_X, ODN_Y, TYP "
		"FROM "
			"EW_TEXT "
		"WHERE "
			"ID = ?");

	LabelQuery.prepare(
		"INSERT INTO "
			"EW_TEXT (ID, ID_WARSTWY, TEXT, STAN_ZMIANY, OPERAT, KAT, POS_X, POS_Y, JUSTYFIKACJA, ODN_X, ODN_Y, TYP) "
		"SELECT "
			"?, ?, TEXT, 0, OPERAT, KAT, POS_X, POS_Y, JUSTYFIKACJA, ODN_X, ODN_Y, TYP "
		"FROM "
			"EW_TEXT "
		"WHERE "
			"ID = ?");

	ClassQuery.prepare(
		"INSERT INTO "
			"EW_OBIEKTY (UID, NUMER, KOD, STATUS, IDKATALOG, RODZAJ, OSOU, OSOW, DTU, DTR, OPERAT) "
		"SELECT "
			"?, ?, ?, 0, IDKATALOG, RODZAJ, OSOU, OSOW, DTU, DTR, OPERAT "
		"FROM "
			"EW_OBIEKTY "
		"WHERE "
			"UIDO = ?");

	ElementsQuery.prepare(
		"INSERT INTO "
			"EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
		"VALUES "
			"(?, ?, 0, ?)");

	GeometryQuery.prepare(
		"SELECT "
			"E.UIDO, E.IDE, T.TYP "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"LEFT JOIN "
			"EW_TEXT T "
		"ON "
			"(E.IDE = T.ID AND T.STAN_ZMIANY = 0) "
		"LEFT JOIN "
			"EW_POLYLINE P "
		"ON "
			"(E.IDE = P.ID AND P.STAN_ZMIANY = 0) "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 "
		"ORDER BY "
			"O.UID ASC, "
			"E.N ASC");

	getUidQuery.prepare("SELECT GEN_ID(EW_OBIEKTY_UID_GEN, 1) FROM RDB$DATABASE");
	getIdQuery.prepare("SELECT GEN_ID(EW_OBIEKTY_ID_GEN, 1) FROM RDB$DATABASE");

	if (!Style)
	{
		Query.prepare("SELECT TYP_LINII FROM EW_WARSTWA_LINIOWA WHERE ID = ?"); Query.addBindValue(Line);

		if (Query.exec() && Query.next()) LineStyle = Query.value(0).toInt();
	}
	else LineStyle = Style;

	if (Symbol.isEmpty())
	{
		Query.prepare("SELECT NAZWA FROM EW_WARSTWA_TEXTOWA WHERE ID = ?"); Query.addBindValue(Point);

		if (Query.exec() && Query.next()) NewSymbol = Query.value(0).toString();
	}
	else NewSymbol = Symbol;

	emit onBeginProgress(tr("Loading elements"));
	emit onSetupProgress(0, Change.size()); Step = 0;

	if (GeometryQuery.exec()) while (GeometryQuery.next() && !isTerminated())
	{
		const int UID = GeometryQuery.value(0).toInt();

		if (Change.contains(UID))
		{
			if (!Elements.contains(UID)) Elements.insert(UID, QList<ELEMENT>());

			Elements[UID].append(
			{
				GeometryQuery.value(1).toInt(),
				GeometryQuery.value(2).toInt()
			});

			emit onUpdateProgress(++Step);
		}
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataCopy(0); return; }

	emit onBeginProgress(tr("Copying objects"));
	emit onSetupProgress(0, Change.size()); Step = 0;

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

		for (const auto& Item : i.value()) if (Change.contains(Item))
		{
			int UIDO(0); if (getUidQuery.exec() && getUidQuery.next())
			{
				UIDO = getUidQuery.value(0).toInt();
			}
			else continue;

			const QString Numer = QString("OB_ID_%1").arg(QString::number(qHash(qMakePair(UIDO, QDateTime::currentDateTimeUtc())), 16));

			Query.exec(QString("INSERT INTO %1 (UIDO, %2) "
						    "SELECT '%3', %2 FROM %4 "
						    "WHERE UIDO = '%5'")
					 .arg(Table.Data)
					 .arg(Fields.replaceInStrings("EW_DATA.", "").join(", "))
					 .arg(UIDO)
					 .arg(i.key())
					 .arg(Item));

			ClassQuery.addBindValue(UIDO);
			ClassQuery.addBindValue(Numer);
			ClassQuery.addBindValue(Class);
			ClassQuery.addBindValue(Item);

			ClassQuery.exec();

			int N(0); for (const auto& E : Elements[Item])
			{
				int ID(0); if (getIdQuery.exec() && getIdQuery.next())
				{
					ID = getIdQuery.value(0).toInt();
				}
				else continue;

				switch (E.Type)
				{
					case 4:

						SymbolQuery.addBindValue(ID);
						SymbolQuery.addBindValue(Point);
						SymbolQuery.addBindValue(NewSymbol);
						SymbolQuery.addBindValue(E.IDE);

						SymbolQuery.exec();

					break;

					case 6:

						LabelQuery.addBindValue(ID);
						LabelQuery.addBindValue(Text);
						LabelQuery.addBindValue(E.IDE);

						LabelQuery.exec();

					break;

					default:

						LineQuery.addBindValue(ID);
						LineQuery.addBindValue(Line);
						LineQuery.addBindValue(LineStyle);
						LineQuery.addBindValue(E.IDE);

						LineQuery.exec();

				}

				ElementsQuery.addBindValue(UIDO);
				ElementsQuery.addBindValue(ID);
				ElementsQuery.addBindValue(N++);

				ElementsQuery.exec();
			}

			emit onUpdateProgress(++Step);
		}
	}

	emit onEndProgress();
	emit onDataCopy(Change.size());
}

void DatabaseDriver::fitData(RecordModel* Model, const QModelIndexList& Items, const QString& Path, bool Points, int X1, int Y1, int X2, int Y2, double Radius, double Length)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataFit(0); return; }

	struct OBJECT { QList<int> Indexes; QList<QLineF> Lines; QList<QPointF> Points; };
	struct LINE { int ID; QLineF Line; QPointF Point; bool Changed = false; };
	struct POINT { int ID; QPointF Point; bool Changed = false; };

	const QSet<int> Tasks = Model->getUids(Items).toSet();
	QHash<int, OBJECT> Objects; QList<LINE> lUpdates; QList<POINT> pUpdates;
	QSqlQuery LoadLines(Database), LoadPoints(Database),
			UpdateLines(Database), UpdatePoints(Database);

	QMutex Synchronizer; int Step = 0; if (Points) X2 = Y2 = 0;

	LoadLines.prepare(
		"SELECT "
			"O.UID, P.ID, P.P1_FLAGS, P.P0_X, P0_Y, "
			"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
			"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y) "
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
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0");

	LoadPoints.prepare(
		"SELECT "
			"O.UID, P.ID, "
			"P.POS_X, P.POS_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4 AND "
			"P.STAN_ZMIANY = 0 AND "
			"P.TYP = 4 AND "
			"E.TYP = 0");

	UpdateLines.prepare("UPDATE EW_POLYLINE SET P0_X = ?, P0_Y = ?, P1_X = ?, P1_Y = ? WHERE ID = ? AND STAN_ZMIANY = 0");
	UpdatePoints.prepare("UPDATE EW_TEXT SET POS_X = ?, POS_Y = ? WHERE ID = ? AND STAN_ZMIANY = 0");

	emit onBeginProgress(tr("Loading file data"));
	emit onSetupProgress(0, 0);

	QList<QPointF> Sources; QList<QLineF> Lines; QFile File(Path); QTextStream Stream(&File);

	File.open(QFile::ReadOnly | QFile::Text);

	const bool CSV = (QFileInfo(File).suffix() == "csv");
	const int Max = qMax(qMax(X1, Y1), qMax(X2, Y2));

	const QRegExp Exp(CSV ? "," : "\\s+");

	while (!Stream.atEnd() && !isTerminated())
	{
		const QStringList Items = Stream.readLine().trimmed().split(Exp, QString::SkipEmptyParts);

		if (Max < Items.size())
		{
			bool OK(true), Current;

			const double pX1 = Items[X1].toDouble(&Current); OK = OK && Current;
			const double pY1 = Items[Y1].toDouble(&Current); OK = OK && Current;
			const double pX2 = Items[X2].toDouble(&Current); OK = OK && Current;
			const double pY2 = Items[Y2].toDouble(&Current); OK = OK && Current;

			if (OK)
			{
				if (!Points) Lines.append({ pX1, pY1, pX2, pY2 });
				else Sources.append({ pX1, pY1 });
			}
		}
		else continue;
	}

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks.size());

	if (LoadLines.exec()) while (LoadLines.next() && !isTerminated())
	{
		const int UID = LoadLines.value(0).toInt();

		if (Tasks.contains(UID))
		{
			if (!Objects.contains(UID)) Objects.insert(UID, OBJECT());

			OBJECT& Object = Objects[UID];

			QPointF P1(LoadLines.value(3).toDouble(), LoadLines.value(4).toDouble());
			QPointF P2(LoadLines.value(5).toDouble(), LoadLines.value(6).toDouble());

			if (Object.Points.contains(P1)) Object.Points.removeOne(P1);
			else Object.Points.append(P1);

			if (Object.Points.contains(P2)) Object.Points.removeOne(P2);
			else Object.Points.append(P2);

			if (LoadLines.value(2).toInt() == 0)
			{
				Object.Indexes.append(LoadLines.value(1).toInt());
				Object.Lines.append({ P1, P2 });
			}

			emit onUpdateProgress(++Step);
		}
	}

	if (LoadPoints.exec()) while (LoadPoints.next() && !isTerminated())
	{
		const int UID = LoadPoints.value(0).toInt();

		if (Tasks.contains(UID))
		{
			pUpdates.append({
				LoadPoints.value(1).toInt(),
				{
					LoadPoints.value(2).toDouble(),
					LoadPoints.value(3).toDouble()
				}
			});

			emit onUpdateProgress(++Step);
		}
	}

	emit onBeginProgress(tr("Computing geometry"));
	emit onSetupProgress(0, 0);

	if (!Lines.isEmpty()) QtConcurrent::blockingMap(Objects, [this, &lUpdates, &Synchronizer] (OBJECT& Object) -> void
	{
		if (this->isTerminated()) return;

		if (Object.Lines.isEmpty() || Object.Points.size() != 2) return; QLineF Start, End; int sIndex(0), eIndex(0);

		const auto& S = Object.Points.first(); const auto& E = Object.Points.last();

		for (int i = 0; i < Object.Indexes.size(); ++i)
		{
			auto& L = Object.Lines[i];

			if (L.p1() == S || L.p2() == S)
			{
				Start = L; sIndex = Object.Indexes[i];
			}

			if (L.p1() == E || L.p2() == E)
			{
				End = L; eIndex = Object.Indexes[i];
			}
		}

		if (sIndex || eIndex)
		{
			Synchronizer.lock();

			if (sIndex) lUpdates.append({ sIndex, Start, S, false });
			if (eIndex) lUpdates.append({ eIndex, End, E, false });

			Synchronizer.unlock();
		}
	});

	if (!Sources.isEmpty()) QtConcurrent::blockingMap(Objects, [this, &Sources, &lUpdates, &Synchronizer, Radius] (OBJECT& Object) -> void
	{
		if (this->isTerminated()) return;

		for (int i = 0; i < Object.Indexes.size(); ++i)
		{
			QPointF Final1, Final2; double h1 = NAN, h2 = NAN;
			const QLineF& L = Object.Lines[i]; QLineF Current = L;

			for (const auto& P : Sources)
			{
				const double Rad1 = QLineF(P, L.p1()).length();

				if (Rad1 <= Radius && (qIsNaN(h1) || Rad1 < h1))
				{
					Final1 = P; h1 = Rad1;
				}

				const double Rad2 = QLineF(P, L.p2()).length();

				if (Rad2 <= Radius && (qIsNaN(h2) || Rad2 < h2))
				{
					Final2 = P; h2 = Rad2;
				}
			}

			if (!qIsNaN(h1) && Final1 != Current.p1())
			{
				Current.setP1(Final1);
			}

			if (!qIsNaN(h2) && Final2 != Current.p2())
			{
				Current.setP2(Final2);
			}

			if (Current != L)
			{
				Synchronizer.lock();
				lUpdates.append({ Object.Indexes[i], Current, QPointF(), true });
				Synchronizer.unlock();
			}
		}
	});

	if (!Lines.isEmpty()) QtConcurrent::blockingMap(lUpdates, [this, &Lines, Radius, Length] (LINE& Part) -> void
	{
		static const auto between = [] (double px, double py, double x1, double y1, double x2, double y2) -> bool
		{
			const double lx1 = qMax(x1, x2); const double lx2 = qMin(x1, x2);
			const double ly1 = qMax(y1, y2); const double ly2 = qMin(y1, y2);

			return (px <= lx1) && (px >= lx2) && (py <= ly1) && (py >= ly2);
		};

		if (this->isTerminated()) return;

		QPointF Final; double h = NAN; const QPointF Second = Part.Point == Part.Line.p1() ? Part.Line.p2() : Part.Line.p1();

		for (const auto& L : Lines) if (Part.Point != L.p1() && Part.Point != L.p2())
		{
			QLineF Normal(Part.Point, Second); QPointF IntersectR, IntersectL;

			Normal.setAngle(L.angle() + 90.0);
			Normal.intersect(L, &IntersectR);
			Part.Line.intersect(L, &IntersectL);

			const double Rad = QLineF(Part.Point, IntersectR).length();
			const double Len = QLineF(Part.Point, IntersectL).length();

			const QLineF Current(Second, Part.Point);
			const QLineF New(Second, IntersectL);

			const bool Int = between(IntersectL.x(), IntersectL.y(), L.x1(), L.y1(), L.x2(), L.y2());
			const bool Ang = qAbs(Current.angle() - New.angle()) < 1.0;

			if (Int && Ang && Rad <= Radius && Len <= Length && (qIsNaN(h) || h > Rad))
			{
				Final = IntersectL; h = Rad;
			}
		}

		if (Part.Changed = (!qIsNaN(h) && Final != Part.Point))
		{
			if (Part.Point == Part.Line.p1()) Part.Line.setP1(Final);
			if (Part.Point == Part.Line.p2()) Part.Line.setP2(Final);
		}
	});

	if (!Lines.isEmpty()) QtConcurrent::blockingMap(pUpdates, [this, &Lines, Radius] (POINT& Symbol) -> void
	{
		if (this->isTerminated()) return;

		QPointF Final; double h = NAN;

		for (const auto& L : Lines)
		{
			const double Rad1 = QLineF(Symbol.Point, L.p1()).length();

			if (Rad1 <= Radius && (qIsNaN(h) || Rad1 < h))
			{
				Final = L.p1(); h = Rad1;
			}

			const double Rad2 = QLineF(Symbol.Point, L.p1()).length();

			if (Rad2 <= Radius && (qIsNaN(h) || Rad2 < h))
			{
				Final = L.p2(); h = Rad2;
			}
		}

		if (Symbol.Changed = (!qIsNaN(h) && Final != Symbol.Point))
		{
			Symbol.Point = Final;
		}
	});

	if (!Sources.isEmpty()) QtConcurrent::blockingMap(pUpdates, [this, &Sources, Radius] (POINT& Symbol) -> void
	{
		if (this->isTerminated()) return;

		QPointF Final; double h = NAN;

		for (const auto& P : Sources)
		{
			const double Rad = QLineF(Symbol.Point, P).length();

			if (Rad <= Radius && (qIsNaN(h) || Rad < h))
			{
				Final = P; h = Rad;
			}
		}

		if (Symbol.Changed = (!qIsNaN(h) && Final != Symbol.Point))
		{
			Symbol.Point = Final;
		}
	});

	if (isTerminated()) { emit onEndProgress(); emit onDataFit(0); return; }

	emit onBeginProgress(tr("Updating geometry")); int Rejected(0);
	emit onSetupProgress(0, lUpdates.size() + pUpdates.size()); Step = 0;

	for (const auto& L : lUpdates)
	{
		if (L.Changed)
		{
			UpdateLines.addBindValue(L.Line.x1());
			UpdateLines.addBindValue(L.Line.y1());
			UpdateLines.addBindValue(L.Line.x2());
			UpdateLines.addBindValue(L.Line.y2());
			UpdateLines.addBindValue(L.ID);

			UpdateLines.exec();
		}
		else ++Rejected;

		emit onUpdateProgress(++Step);
	}

	for (const auto& P : pUpdates)
	{
		if (P.Changed)
		{
			UpdatePoints.addBindValue(P.Point.x());
			UpdatePoints.addBindValue(P.Point.y());
			UpdatePoints.addBindValue(P.ID);

			UpdatePoints.exec();
		}
		else ++Rejected;

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataFit(lUpdates.size() + pUpdates.size() - Rejected);
}

void DatabaseDriver::restoreJob(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onJobsRestore(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), true, 0);
	QSqlQuery QueryA(Database), QueryB(Database), QueryC(Database); int Step = 0; int Count = 0;

	emit onBeginProgress(tr("Restoring job name"));
	emit onSetupProgress(0, Tasks.first().size());

	QueryA.prepare(
		"SELECT "
			"O.OPERAT "
		"FROM "
			"EW_OBIEKTY O "
		"WHERE "
			"O.UID = ?");

	QueryB.prepare(
		"SELECT FIRST 1 "
			"O.OPERAT "
		"FROM "
			"EW_OBIEKTY O "
		"WHERE "
			"O.STATUS = 3 AND O.ID = ("
				"SELECT U.ID FROM EW_OBIEKTY U WHERE U.UID = ?"
			") "
		"ORDER BY O.DTR ASCENDING");

	QueryC.prepare(
		"UPDATE "
			"EW_OBIEKTY O "
		"SET "
			"O.OPERAT = ? "
		"WHERE "
			"O.UID = ?");

	for (const auto& ID : Tasks.first())
	{
		if (isTerminated()) break;

		int Now = -1; int Last = -1;

		emit onUpdateProgress(++Step);

		QueryA.addBindValue(ID);

		if (QueryA.exec() && QueryA.next()) Now = QueryA.value(0).toInt(); else continue;

		QueryB.addBindValue(ID);

		if (QueryB.exec() && QueryB.next()) Last = QueryB.value(0).toInt(); else continue;

		if (Last == Now || Last == -1 || Now == -1) continue; else ++Count;

		QueryC.addBindValue(Last);
		QueryC.addBindValue(ID);

		QueryC.exec();
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

	QSqlQuery QueryA(Database), QueryB(Database), QueryC(Database); int Step = 0;
	const QSet<int> Tasks = Model->getUids(Items).toSet(); int Count = 0;

	emit onBeginProgress(tr("Removing history"));
	emit onSetupProgress(0, Tasks.size());

	QueryA.prepare(
		"SELECT "
			"O.UID "
		"FROM "
			"EW_OBIEKTY O "
		"WHERE "
			"O.ID = ("
				"SELECT U.ID FROM EW_OBIEKTY U WHERE U.UID = ?"
			") AND O.STATUS = 3");

	QueryB.prepare("DELETE FROM EW_OBIEKTY WHERE UID = ?");
	QueryC.prepare("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = ?");

	for (const auto& ID : Tasks)
	{
		if (isTerminated()) break;

		QueryA.addBindValue(ID);

		if (QueryA.exec()) while (QueryA.next())
		{
			const int Index = QueryA.value(0).toInt();

			QueryB.addBindValue(Index); QueryB.exec();
			QueryC.addBindValue(Index); QueryC.exec();

			Count += 1;
		}

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
		int UIDO, IDE;

		double WX, WY;
		double DX, DY;
		double A;

		unsigned J;

		bool Changed = false;
	};

	struct LINE
	{
		int UIDO, IDE;

		double X1, Y1;
		double X2, Y2;

		double Length;
	};

	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0;
	QMap<int, POINT> Points, Texts, Objects; QList<LINE> Lines;
	const QSet<int> Tasks = Model->getUids(Items).toSet(); QList<const POINT*> Union;

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, T.UID, T.TYP, "
			"ROUND(T.POS_X, 3), "
			"ROUND(T.POS_Y, 3), "
			"T.JUSTYFIKACJA "
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

	if (Query.exec()) while (Query.next() && !isTerminated())
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

				Ref.IDE = Query.value(1).toInt();
				Ref.DX = Query.value(3).toDouble();
				Ref.DY = Query.value(4).toDouble();
				Ref.J = Query.value(5).toUInt();

			break;
		}
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"E.UIDO, E.IDE, "
			"ROUND(P.P0_X, 3), "
			"ROUND(P.P0_Y, 3), "
			"ROUND(P.P1_X, 3), "
			"ROUND(P.P1_Y, 3) "
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
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		Lines.append(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble(),
			Query.value(2).toDouble(),
			Query.value(3).toDouble(),
			Query.value(4).toDouble(),
			Query.value(5).toDouble()
		});
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Loading texts"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"O.UID, T.UID, "
			"ROUND(T.POS_X, 3), "
			"ROUND(T.POS_Y, 3), "
			"T.JUSTYFIKACJA "
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
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 6 "
		"ORDER BY "
			"O.UID ASC");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();
		const int IDE = Query.value(1).toInt();

		if (Tasks.contains(UID)) Texts.insert(IDE,
		{
			Query.value(0).toInt(),
			Query.value(1).toInt(),
			0.0, 0.0,
			Query.value(2).toDouble(),
			Query.value(3).toDouble(),
			0.0,
			Query.value(4).toDouble()
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
			"ROUND((P.P0_X + P.P1_X) / 2.0, 3), "
			"ROUND((P.P0_Y + P.P1_Y) / 2.0, 3) "
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

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int ID = Query.value(0).toInt();

		if (!Objects.contains(ID)) Objects.insert(ID,
		{
			Query.value(0).toInt(),
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

	QtConcurrent::blockingMap(Points, [this, &Lines, &Objects, Move, Justify, Rotate, Length] (POINT& Point) -> void
	{
		static const auto distance = [] (const QLineF& L, const QPointF& P) -> double
		{
				const double a = QLineF(P.x(), P.y(), L.x1(), L.y1()).length();
				const double b = QLineF(P.x(), P.y(), L.x2(), L.y2()).length();
				const double l = L.length();

				if ((a * a <= l * l + b * b) &&
				    (b * b <= a * a + l * l))
				{
					const double A = P.x() - L.x1(); const double B = P.y() - L.y1();
					const double C = L.x2() - L.x1(); const double D = L.y2() - L.y1();

					return qAbs(A * D - C * B) / qSqrt(C * C + D * D);
				}
				else return INFINITY;
		};

		if (this->isTerminated()) return;

		bool Found = false; LINE Match;

		for (const auto& Object : Objects)
			if (Object.IDE != Point.IDE && (Object.WX == Point.WX && Object.WY == Point.WY)) return;

		if (Move) { Point.DX = Point.WX; Point.DY = Point.WY; Point.J &= 0b1011111; Point.Changed = true; }

		if (Justify && !Found) for (const auto& Line : Lines) if (!Found)
			if ((Point.WX == Line.X1 && Point.WY == Line.Y1) || (Point.WX == Line.X2 && Point.WY == Line.Y2))
			{
				Match = Line; Found = true;
			}

		if (Justify && !Found) for (const auto& Line : Lines) if (!Found)
			if (distance(QLineF(Line.X1, Line.Y1, Line.X2, Line.Y2), QPointF(Point.WX, Point.WY)) <= 0.05)
			{
				Match = Line; Found = true;
			}

		if (Found)
		{
			if (Justify && !Rotate)
			{
				if (Point.J == 1) { Point.J = 4; Point.Changed = true; return; }
			}
			else if (Justify && Rotate && (Match.Length >= Length))
			{
				Point.A = qAtan((Match.Y1 - Match.Y2) / (Match.X1 - Match.X2)) - M_PI / 2.0;

				const double lengthA = QLineF(Point.WX, Point.WY, Match.X1, Match.Y1).length();
				const double lengthB = QLineF(Point.WX, Point.WY, Match.X2, Match.Y2).length();

				if (Match.Y1 < Match.Y2)
				{
					if (lengthA < lengthB) Point.J = 4; else Point.J = 6;
				}
				else
				{
					if (lengthB < lengthA) Point.J = 4; else Point.J = 6;
				}

				while (Point.A < -(M_PI / 2.0)) Point.A += M_PI;
				while (Point.A > (M_PI / 2.0)) Point.A -= M_PI;

				Point.Changed = true;
			}
		}
	});

	QtConcurrent::blockingMap(Texts, [this, &Lines, Move, Justify, Rotate, Length] (POINT& Point) -> void
	{
		static const auto length = [] (double x1, double y1, double x2, double y2)
		{
			const double dx = x1 - x2;
			const double dy = y1 - y2;

			return qSqrt(dx * dx + dy * dy);
		};

		if (this->isTerminated()) return;

		const LINE* Match = nullptr; double Distance = NAN;

		if (Justify) { Point.J = 5; Point.Changed = true; }

		if (Move || Rotate) for (const auto& L : Lines) if (Point.UIDO == L.UIDO)
		{
			const double a = length(Point.DX, Point.DY, L.X1, L.Y1);
			const double b = length(Point.DX, Point.DY, L.X2, L.Y2);
			const double l = length(L.X1, L.Y1, L.X2, L.Y2);

			if ((a * a <= l * l + b * b) && (b * b <= l * l + a * a))
			{
				const double A = Point.DX - L.X1; const double B = Point.DY - L.Y1;
				const double C = L.X2 - L.X1; const double D = L.Y2 - L.Y1;

				const double h = qAbs(A * D - C * B) / qSqrt(C * C + D * D);

				if (qIsNaN(Distance) || h < Distance)
				{
					Distance = h; Match = &L;
				}
			}
		}

		if (Match)
		{
			double dtg = qAtan2(Match->Y1 - Match->Y2, Match->X1 - Match->X2) + (M_PI / 2.0);

			while (dtg >= M_PI) dtg -= M_PI; while (dtg < 0.0) dtg += M_PI;

			if ((dtg > (M_PI / 2.0)) && (dtg < M_PI)) dtg += M_PI;
			if ((dtg > M_PI) && (dtg < (M_PI * 1.5))) dtg -= M_PI;

			const QLineF Line(Match->X1, Match->Y1, Match->X2, Match->Y2);
			QLineF Offset(Point.DX, Point.DY, 0, 0);
			Offset.setAngle(Line.angle() + 90.0);

			QPointF Int; Offset.intersect(Line, &Int);

			if (Move) { Point.DX = Int.x(); Point.DY = Int.y(); }
			if (Rotate) { Point.A = dtg; }

			Point.Changed = true;
		}
	});

	if (isTerminated()) { emit onEndProgress(); emit onTextEdit(0); return; }

	for (const auto& Point : Points) if (Point.Changed) Union.append(&Point);
	for (const auto& Point : Texts) if (Point.Changed) Union.append(&Point);

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Saving changes"));
	emit onSetupProgress(0, Union.size());

	Query.prepare(
		"UPDATE "
			"EW_TEXT "
		"SET "
			"POS_X = ?, "
			"POS_Y = ?, "
			"KAT = ?, "
			"JUSTYFIKACJA = ? "
		"WHERE "
			"UID = ?");

	for (const auto& Point : Union)
	{
		if (isTerminated()) break;

		Query.addBindValue(Point->DX);
		Query.addBindValue(Point->DY);
		Query.addBindValue(Point->A);
		Query.addBindValue(Point->J);
		Query.addBindValue(Point->IDE);

		Query.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onTextEdit(Step);
}

void DatabaseDriver::insertLabel(RecordModel* Model, const QModelIndexList& Items, const QString& Label, int J, double X, double Y, bool P, double L, double R)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onLabelInsert(0); return; }

	struct LABEL { int ID; double X, Y, R; int Layer; };

	struct LINE { int ID; QList<QLineF> Lines; int Layer; };

	struct SURFACE { int ID; QPolygonF Surface; int Layer; };

	QSqlQuery Symbols(Database), Lines(Database), Surfaces(Database), Select(Database), Text(Database), Element(Database);
	QList<LINE> LineList; QList<SURFACE> SurfaceList; QList<LABEL> Insertions; QSet<int> Used;
	const QSet<int> Tasks = Model->getUids(Items).toSet(); QMutex Synchronizer; int Step = 0, Count = 0;

	if (!J)
	{
		if (Y > 0) J += 1; else if (Y < 0) J += 3; else J += 2;
		if (X < 0) J += 6; else if (X == 0) J += 3;
	}

	if (P) J |= 0b110000;

	Symbols.prepare(
		"SELECT "
			"O.UID, T.POS_X, T.POS_Y, IIF(("
				"SELECT FIRST 1 P.ID FROM EW_WARSTWA_TEXTOWA P "
				"INNER JOIN EW_GRUPY_WARSTW G ON P.ID_GRUPY = G.ID "
				"WHERE G.NAZWA = ("
					"SELECT H.NAZWA FROM EW_GRUPY_WARSTW H "
					"INNER JOIN EW_WARSTWA_TEXTOWA J ON H.ID = J.ID_GRUPY "
					"WHERE J.ID = T.ID_WARSTWY "
				") || '_E' AND P.NAZWA LIKE O.KOD || '%' "
				") IS NOT NULL, ("
				"SELECT FIRST 1 P.ID FROM EW_WARSTWA_TEXTOWA P  "
				"INNER JOIN EW_GRUPY_WARSTW G ON P.ID_GRUPY = G.ID "
				"WHERE G.NAZWA = ("
					"SELECT H.NAZWA FROM EW_GRUPY_WARSTW H "
					"INNER JOIN EW_WARSTWA_TEXTOWA J ON H.ID = J.ID_GRUPY "
					"WHERE J.ID = T.ID_WARSTWY "
				") || '_E' AND P.NAZWA LIKE O.KOD || '%' "
				"), T.ID_WARSTWY) "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN  "
			"EW_OB_ELEMENTY E "
		"ON "
			"E.UIDO = O.UID "
		"INNER JOIN  "
			"EW_TEXT T "
		"ON "
			"(T.ID = E.IDE AND E.TYP = 0) "
		"WHERE "
			"O.UID = ? AND "
			"O.RODZAJ = 4 AND "
			"O.STATUS = 0 AND "
			"0 = ("
				"SELECT COUNT(L.ID) FROM EW_TEXT L "
				"INNER JOIN EW_OB_ELEMENTY Q "
				"ON (L.ID = Q.IDE AND Q.TYP = 0) "
				"WHERE Q.UIDO = O.UID AND L.TYP = 6 AND L.STAN_ZMIANY = 0 "
			")");

	Lines.prepare(
		"SELECT "
			"O.UID, T.P0_X, T.P0_Y, "
			"IIF(T.PN_X IS NULL, T.P1_X, T.PN_X), "
			"IIF(T.PN_Y IS NULL, T.P1_Y, T.PN_Y), "
			"("
				"SELECT FIRST 1 P.ID FROM EW_WARSTWA_TEXTOWA P "
				"WHERE P.NAZWA = ("
					"SELECT H.NAZWA FROM EW_WARSTWA_LINIOWA H "
					"WHERE H.ID = T.ID_WARSTWY "
				") "
			") "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"E.UIDO = O.UID "
		"INNER JOIN "
			"EW_POLYLINE T "
		"ON "
			"(T.ID = E.IDE AND E.TYP = 0) "
		"WHERE "
			"O.UID = ? AND "
			"O.RODZAJ = 2 AND "
			"O.STATUS = 0 AND "
			"0 = ("
				"SELECT COUNT(L.ID) FROM EW_TEXT L "
				"INNER JOIN EW_OB_ELEMENTY Q "
				"ON (L.ID = Q.IDE AND Q.TYP = 0) "
				"WHERE Q.UIDO = O.UID AND L.TYP = 6 AND L.STAN_ZMIANY = 0 "
			") "
		"ORDER BY "
			"E.N ASCENDING");

	Surfaces.prepare(
		"SELECT "
			"O.UID, T.P1_FLAGS, T.P0_X, T.P0_Y, "
			"IIF(T.PN_X IS NULL, T.P1_X, T.PN_X), "
			"IIF(T.PN_Y IS NULL, T.P1_Y, T.PN_Y), "
			"("
				"SELECT FIRST 1 P.ID FROM EW_WARSTWA_TEXTOWA P "
				"WHERE P.NAZWA = LEFT(("
					"SELECT H.NAZWA FROM EW_WARSTWA_LINIOWA H "
					"WHERE H.ID = T.ID_WARSTWY "
				"), 6) "
			") "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"E.UIDO = O.UID "
		"INNER JOIN "
			"EW_POLYLINE T "
		"ON "
			"(T.ID = E.IDE AND E.TYP = 0) "
		"WHERE "
			"O.UID = ? AND "
			"O.RODZAJ = 3 AND "
			"O.STATUS = 0 AND "
			"0 = ("
				"SELECT COUNT(L.ID) FROM EW_TEXT L "
				"INNER JOIN EW_OB_ELEMENTY Q "
				"ON (L.ID = Q.IDE AND Q.TYP = 0) "
				"WHERE Q.UIDO = O.UID AND L.TYP = 6 AND L.STAN_ZMIANY = 0 "
			") "
		"ORDER BY "
			"E.N ASCENDING");

	Select.prepare("SELECT GEN_ID(EW_ELEMENT_ID_GEN, 1) FROM RDB$DATABASE");

	Text.prepare(QString("INSERT INTO EW_TEXT (ID, STAN_ZMIANY, CREATE_TS, MODIFY_TS, TYP, TEXT, POS_X, POS_Y, KAT, ID_WARSTWY, JUSTYFIKACJA, ODN_X, ODN_Y) "
					 "VALUES (?, 0, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, 6, '%1', ?, ?, ?, ?, %2, %3, %4)")
			   .arg(Label).arg(J).arg(-X).arg(-Y));

	Element.prepare("INSERT INTO EW_OB_ELEMENTY (UIDO, TYP, IDE, N) "
				 "VALUES (?, 0, ?, 1 + ("
					"SELECT MAX(N) FROM EW_OB_ELEMENTY WHERE UIDO = ?)"
				 ")");

	emit onBeginProgress(tr("Generating tasklist for symbols"));
	emit onSetupProgress(0, Tasks.size() - Used.size()); Step = 0;

	for (const auto& UID : Tasks) if (!Used.contains(UID))
	{
		if (isTerminated()) break;

		Symbols.addBindValue(UID);

		if (Symbols.exec() && Symbols.next())
		{
			Used.insert(UID);

			Insertions.append(
			{
				Symbols.value(0).toInt(),
				Symbols.value(1).toDouble(),
				Symbols.value(2).toDouble(),
				0.0, Symbols.value(3).toInt()
			});
		}

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Generating tasklist for lines"));
	emit onSetupProgress(0, Tasks.size() - Used.size()); Step = 0;

	for (const auto& UID : Tasks) if (!Used.contains(UID))
	{
		if (isTerminated()) break;

		Lines.addBindValue(UID); int Layer(0);
		QPointF Start, End; QList<QLineF> Line;

		if (Lines.exec()) while (Lines.next())
		{
			const QPointF First =
			{
				Lines.value(1).toDouble(),
				Lines.value(2).toDouble()
			};

			const QPointF Last =
			{
				Lines.value(3).toDouble(),
				Lines.value(4).toDouble()
			};

			if (!Line.isEmpty())
			{
				if (End == First) Line.push_back({ First, End = Last });
				else if (End == Last) Line.push_back({ Last, End = First });
				else if (Start == First) Line.push_front({ Start = Last, First });
				else if (Start == Last) Line.push_front({ Start = First, Last });
			}
			else
			{
				Line.append({ Start = First, End = Last });
				Layer = Lines.value(5).toInt();
			}
		}

		if (!Line.isEmpty()) { LineList.append({ UID, Line, Layer }); Used.insert(UID); }

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Generating tasklist for surfaces"));
	emit onSetupProgress(0, Tasks.size() - Used.size()); Step = 0;

	for (const auto& UID : Tasks) if (!Used.contains(UID))
	{
		if (isTerminated()) break;

		Surfaces.addBindValue(UID); int Layer(0); QPolygonF Surface;

		if (Surfaces.exec()) while (Surfaces.next())
		{
			const QPointF First =
			{
				Surfaces.value(2).toDouble(),
				Surfaces.value(3).toDouble()
			};

			const QPointF Last =
			{
				Surfaces.value(4).toDouble(),
				Surfaces.value(5).toDouble()
			};

			if (Surfaces.value(1).toInt() == 4)
			{
				const double X = (First.x() + Last.x()) / 2.0;
				const double Y = (First.y() + Last.y()) / 2.0;

				Insertions.append({ UID, X, Y, 0.0, Surfaces.value(6).toInt() });
			}
			else
			{
				if (!Surface.isEmpty() && (!Surface.contains(First) || !Surface.contains(Last)))
				{
					if (Surface.last() == First) Surface.push_back(Last);
					else if (Surface.last() == Last) Surface.push_back(First);
					else if (Surface.first() == First) Surface.push_front(Last);
					else if (Surface.first() == Last) Surface.push_front(First);
				}
				else
				{
					Surface.append(First); Surface.append(Last);

					Layer = Surfaces.value(6).toInt();
				}
			}
		}

		if (!Surface.isEmpty()) { SurfaceList.append({ UID, Surface, Layer }); Used.insert(UID); }

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Computing labels"));
	emit onSetupProgress(0, 0); Step = 0;

	QtConcurrent::blockingMap(LineList, [this, &Insertions, &Synchronizer, L, R, P] (LINE& Line) -> void
	{
		if (this->isTerminated()) return;

		double Length(0.0); for (const auto& Segment : Line.Lines) Length += Segment.length(); if (Length < L) return;

		double Step((R != 0.0) ? ((Length - (int(Length / R) * R)) / 2.0) : (Length / 2.0)), Now(0.0);

		for (const auto& Segment : Line.Lines) if (const double Len = Segment.length())
		{
			const double Last = Now; Now += Len; while (Now >= Step)
			{
				double A = qAtan((Segment.y1() - Segment.y2()) / (Segment.x1() - Segment.x2())) - M_PI / 2.0;
				while (A < -(M_PI / 2.0)) A += M_PI; while (A > (M_PI / 2.0)) A -= M_PI;

				const QPointF Point = Segment.pointAt((Step - Last) / Len);

				Synchronizer.lock();
				Insertions.append({ Line.ID, Point.x(), Point.y(), P ? 0.0 : A, Line.Layer });
				Synchronizer.unlock();

				if (R != 0.0) Step += R; else return;
			}
		}
	});

	QtConcurrent::blockingMap(SurfaceList, [this, &Insertions, &Synchronizer] (SURFACE& Surface) -> void
	{
		if (this->isTerminated()) return;

		if (Surface.Surface.isEmpty()) return;

		const double Mul = 1.0 / Surface.Surface.size(); double X(0.0), Y(0.0);
		for (const auto& P : Surface.Surface) { X += Mul * P.x(); Y += Mul * P.y(); }

		Synchronizer.lock();
		Insertions.append({ Surface.ID, X, Y, 0.0, Surface.Layer });
		Synchronizer.unlock();
	});

	QtConcurrent::blockingMap(Insertions, [X, Y] (LABEL& Point) -> void
	{
		Point.X += X; Point.Y += Y;
	});

	if (isTerminated()) { emit onEndProgress(); emit onLabelInsert(0); return; }

	emit onBeginProgress(tr("Inserting labels"));
	emit onSetupProgress(0, Insertions.size()); Step = 0;

	for (const auto& Item : Insertions)
	{
		if (isTerminated()) break;

		if (Select.exec() && Select.next())
		{
			const int ID = Select.value(0).toInt();

			Text.addBindValue(ID);
			Text.addBindValue(Item.X);
			Text.addBindValue(Item.Y);
			Text.addBindValue(Item.R);
			Text.addBindValue(Item.Layer);

			if (!Text.exec()) continue;

			Element.addBindValue(Item.ID);
			Element.addBindValue(ID);
			Element.addBindValue(Item.ID);

			if (!Element.exec()) continue;

			Count += 1;
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onLabelInsert(Count);
}

void DatabaseDriver::removeLabel(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onLabelDelete(0); return; }

	const QSet<int> Tasks = Model->getUids(Items).toSet(); QSet<int> Deletes; int Step = 0;
	QSqlQuery Select(Database), DeleteA(Database), DeleteB(Database); Select.setForwardOnly(true);

	Select.prepare(
		"SELECT "
			"O.UID, "
			"P.ID "
		"FROM "
			"EW_TEXT P "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"P.ID = E.IDE "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"O.UID = E.UIDO "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 AND "
			"P.TYP <> 4");

	DeleteA.prepare("DELETE FROM EW_OB_ELEMENTY WHERE TYP = 0 AND IDE = ?");
	DeleteB.prepare("DELETE FROM EW_TEXT WHERE ID = ?");

	emit onBeginProgress(tr("Loading labels"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	if (Select.exec()) while (Select.next() && !isTerminated())
	{
		const int UID = Select.value(0).toInt();

		if (Tasks.contains(UID))
		{
			Deletes.insert(Select.value(1).toInt());

			emit onUpdateProgress(++Step);
		}
	}

	if (isTerminated()) { emit onEndProgress(); emit onLabelDelete(0); return; }

	emit onBeginProgress(tr("Deleting labels"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	for (const auto& IDE : Deletes)
	{
		if (isTerminated()) break;

		DeleteA.addBindValue(IDE); DeleteA.exec();
		DeleteB.addBindValue(IDE); DeleteB.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onLabelDelete(Step);
}

void DatabaseDriver::editLabel(RecordModel* Model, const QModelIndexList& Items, const QString& Label)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onLabelEdit(0); return; }

	const QSet<int> Tasks = Model->getUids(Items).toSet();
	QSqlQuery selectQuery(Database), updateQuery(Database);
	QSet<int> Labels;  int Step = 0;

	selectQuery.prepare(
		"SELECT "
			"O.UID, P.ID "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT P "
		"ON "
			"E.IDE = P.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 AND "
			"P.TYP = 6");

	updateQuery.prepare(
		"UPDATE "
			"EW_TEXT "
		"SET "
			"TEXT = ? "
		"WHERE "
			"ID = ?");

	emit onBeginProgress(tr("Loading texts"));
	emit onSetupProgress(0, 0);

	if (selectQuery.exec()) while (selectQuery.next() && !isTerminated())
		if (Tasks.contains(selectQuery.value(0).toInt()))
		{
			Labels.insert(selectQuery.value(1).toInt());
		}

	if (isTerminated()) { emit onEndProgress(); emit onLabelEdit(0); return; }

	emit onBeginProgress(tr("Updating texts"));
	emit onSetupProgress(0, Labels.size());

	for (const auto& ID : Labels)
	{
		if (isTerminated()) break;

		updateQuery.addBindValue(Label);
		updateQuery.addBindValue(ID);

		updateQuery.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onLabelEdit(Step);
}

void DatabaseDriver::insertPoints(RecordModel* Model, const QModelIndexList& Items, int Mode, double Radius, bool Recursive)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPointInsert(0); return; }

	const QSet<int> Tasks = Model->getUids(Items).toSet(); int Count(0), Current(0);

	if (!isTerminated()) do
	{
		Current = insertBreakpoints(Tasks, Mode, Radius); Count += Current;
	}
	while (Recursive && Current && !isTerminated());

	emit onEndProgress();
	emit onPointInsert(Count);
}

QHash<int, QSet<int>> DatabaseDriver::joinSurfaces(const QHash<int, QSet<int>>& Geometry, const QList<DatabaseDriver::POINT>& Points, const QSet<int>& Tasks, const QString& Class, double Radius)
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

					const int ThisSize = Polygon.size();

					if (Polygon.last() == A) Polygon.append(B);
					else if (Polygon.last() == B) Polygon.append(A);
					else if (Polygon.first() == A) Polygon.insert(0, B);
					else if (Polygon.first() == B) Polygon.insert(0, A);

					if (ThisSize != Polygon.size()) Used.insert(i);
				}
			}

			Continue = LastSize != Polygon.size();
		}

		return Polygon;
	};

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QSet<int> Used;
	QMutex Locker; QMap<int, QList<PART>> Parts;

	Query.prepare(
		"SELECT "
			"O.UID, P.P1_FLAGS, "
			"P.P0_X, P.P0_Y, "
			"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
			"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y) "
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
			"O.KOD = ? AND "
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 3 "
		"ORDER BY "
			"E.N ASCENDING");

	Query.addBindValue(Class);

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();

		if (Tasks.contains(UID))
		{
			if (!Parts.contains(UID)) Parts.insert(UID, QList<PART>());

			if (Query.value(1).toInt() == 4)
			{
				const double x = (Query.value(2).toDouble() + Query.value(4).toDouble()) / 2.0;
				const double y = (Query.value(3).toDouble() + Query.value(5).toDouble()) / 2.0;
				const double r = qAbs(Query.value(2).toDouble() - x);

				Parts[UID].append({ UID, x, y, r, 0.0, 0.0 });
			}
			else
			{
				Parts[UID].append(
				{
					UID,
					Query.value(2).toDouble(),
					Query.value(3).toDouble(),
					0.0,
					Query.value(4).toDouble(),
					Query.value(5).toDouble()
				});
			}
		}
	}

	if (!isTerminated()) QtConcurrent::blockingMap(Parts, [this, &Insert, &Used, &Geometry, &Points, &Locker, SORT] (QList<PART>& List) -> void
	{
		if (this->isTerminated()) return;

		const int ID = List.first().ID; const QPolygonF Polygon = SORT(List);

		for (const auto& P : Points)
		{
			Locker.lock(); const bool Calc = !Used.contains(P.IDE); Locker.unlock();

			if (Calc)
			{
				for (const auto& G : List) if ((G.R != 0.0) && (G.R >= qSqrt(qPow(P.X - G.X1, 2) + qPow(P.Y - G.Y1, 2))))
				{
					Locker.lock();

					if (!Used.contains(P.IDE) && !Geometry[G.ID].contains(P.IDE))
					{
						Insert[G.ID].insert(P.IDE);
						Used.insert(P.IDE);
					}

					Locker.unlock();
				}

				bool OK(false); for (const auto& E : Polygon) if (QLineF(E, QPointF(P.X, P.Y)).length() < 0.05) OK = true;

				if (OK || Polygon.containsPoint(QPointF(P.X, P.Y), Qt::OddEvenFill))
				{
					Locker.lock();

					if (!Used.contains(P.IDE) && !Geometry[ID].contains(P.IDE))
					{
						Insert[ID].insert(P.IDE);
						Used.insert(P.IDE);
					}

					Locker.unlock();
				}
			}
		}
	});

	return Insert;
}

QHash<int, QSet<int>> DatabaseDriver::joinLines(const QHash<int, QSet<int>>& Geometry, const QList<POINT>& Points, const QSet<int>& Tasks, const QString& Class, double Radius)
{
	if (!Database.isOpen()) return QHash<int, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QSet<int> Used;

	static const auto distance = [] (const QLineF& L, const QPointF& P) -> double
	{
		const double a = QLineF(P.x(), P.y(), L.x1(), L.y1()).length();
		const double b = QLineF(P.x(), P.y(), L.x2(), L.y2()).length();
		const double l = L.length();

		if ((a * a <= l * l + b * b) &&
		    (b * b <= a * a + l * l))
		{
			const double A = P.x() - L.x1(); const double B = P.y() - L.y1();
			const double C = L.x2() - L.x1(); const double D = L.y2() - L.y1();

			return qAbs(A * D - C * B) / qSqrt(C * C + D * D);
		}
		else return INFINITY;
	};

	Query.prepare(
		"SELECT "
			"O.UID, "
			"P.P0_X, P.P0_Y, "
			"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
			"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y) "
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
			"O.KOD = ? AND "
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2");

	Query.addBindValue(Class);

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();

		if (Tasks.contains(UID))
		{

			for (const auto P : Points) if (!Used.contains(P.IDE))
			{
				const double X1 = Query.value(1).toDouble();
				const double Y1 = Query.value(2).toDouble();

				const double X2 = Query.value(3).toDouble();
				const double Y2 = Query.value(4).toDouble();

				bool Continue = false;

				if (!Continue) Continue = QLineF(X1, Y1, P.X, P.Y).length() <= Radius;
				if (!Continue) Continue = QLineF(X2, Y2, P.X, P.Y).length() <= Radius;
				if (!Continue) Continue = distance(QLineF(X1, Y1, X2, Y2), QPointF(P.X, P.Y)) <= Radius;

				if (Continue)
				{
					const int ID = Query.value(0).toInt();

					if (!Geometry[ID].contains(P.IDE))
					{
						Insert[ID].insert(P.IDE);
						Used.insert(P.IDE);
					}
				}
			}
		}
	}

	return Insert;
}

QHash<int, QSet<int>> DatabaseDriver::joinPoints(const QHash<int, QSet<int>>& Geometry, const QList<POINT>& Points, const QSet<int>& Tasks, const QString& Class, double Radius)
{
	if (!Database.isOpen()) return QHash<int, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QSet<int> Used;

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
			"O.KOD = ? AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4 AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4");

	Query.addBindValue(Class);

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();

		if (Tasks.contains(UID))
		{
			for (const auto P : Points) if (!Used.contains(P.IDE))
			{
				if (qAbs(Query.value(1).toDouble() - P.X) <= Radius &&
				    qAbs(Query.value(2).toDouble() - P.Y) <= Radius)
				{
					const int ID = Query.value(0).toInt();

					if (!Geometry[ID].contains(P.IDE))
					{
						Insert[ID].insert(P.IDE);
						Used.insert(P.IDE);
					}
				}
			}
		}
	}

	return Insert;
}

int DatabaseDriver::insertBreakpoints(const QSet<int> Tasks, int Mode, double Radius)
{
	struct LINE { int ID; QLineF Line; int Type; bool Changed = false; };

	struct INSERT { int ID, Base; QLineF Line; };

	struct ELEMENT { int IDE, Typ; };

	QSqlQuery Symbols(Database), Objects(Database), Elements(Database), getIndex(Database),
			deleteElement(Database), insertElement(Database),
			updateSegment(Database), insertSegment(Database);

	QMutex Synchronizer; int Currentrun(0), Step(0);

	Symbols.prepare(
		"SELECT "
			"O.UID,"
			"T.POS_X, "
			"T.POS_Y "
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
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4 AND "
			"E.TYP = 0");

	Objects.prepare(
		"SELECT "
			"O.UID, P.ID, "
			"P.P0_X, P.P0_Y, "
			"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
			"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y), "
			"P.P1_FLAGS "
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
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 0 AND "
			"E.TYP = 0");

	Elements.prepare(
		"SELECT "
			"E.UIDO, "
			"E.IDE, "
			"E.TYP "
		"FROM "
			"EW_OB_ELEMENTY E "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"O.UID = E.UIDO "
		"WHERE "
			"O.STATUS = 0 "
		"ORDER BY "
			"E.UIDO ASC, "
			"E.N ASC");

	insertSegment.prepare(
		"INSERT INTO EW_POLYLINE (ID, P0_X, P0_Y, P1_X, P1_Y, P1_FLAGS, STAN_ZMIANY, ID_WARSTWY, OPERAT, TYP_LINII, MNOZNIK, POINTCOUNT) "
		"SELECT ?, ?, ?, ?, ?, 0, 0, ID_WARSTWY, OPERAT, TYP_LINII, MNOZNIK, POINTCOUNT FROM EW_POLYLINE WHERE ID = ? AND STAN_ZMIANY = 0");

	updateSegment.prepare("UPDATE EW_POLYLINE SET P0_X = ?, P0_Y = ?, P1_X = ?, P1_Y = ? WHERE ID = ?");

	insertElement.prepare("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) VALUES (?, ?, ?, ?)");

	deleteElement.prepare("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = ?");

	getIndex.prepare("SELECT GEN_ID(EW_ELEMENT_ID_GEN, 1) FROM RDB$DATABASE");

	QHash<int, QList<ELEMENT>> Geometry; QHash<int, INSERT> Inserts; QSet<int> Changed;
	QHash<int, QList<QPointF>> Unique;	QHash<int, LINE> Lines, Origins;
	QList<QPointF> Points, Ends, Breaks, Intersect, pointCuts;

	emit onBeginProgress(tr("Loading symbols"));
	emit onSetupProgress(0, 0);

	if (Mode & 0x8) if (Symbols.exec()) while (Symbols.next() && !isTerminated())
	{
		if (Tasks.contains(Symbols.value(0).toInt()))
		{
			Points.append(
			{
				Symbols.value(1).toDouble(),
				Symbols.value(2).toDouble()
			});
		}
	}

	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, Tasks.size());

	if (Objects.exec()) while (Objects.next() && !isTerminated())
	{
		const int UID = Objects.value(0).toInt();

		const QPointF A(Objects.value(2).toDouble(),
					 Objects.value(3).toDouble());

		const QPointF B(Objects.value(4).toDouble(),
					 Objects.value(5).toDouble());

		if (Tasks.contains(UID))
		{
			if (Mode & 0x3)
			{
				if (!Unique.contains(UID)) Unique.insert(UID, QList<QPointF>());

				if (!Unique[UID].contains(A)) Unique[UID].append(A);
				else Unique[UID].removeAll(A);

				if (!Unique[UID].contains(B)) Unique[UID].append(B);
				else Unique[UID].removeAll(B);

				if ((Mode & 0x2) && !Breaks.contains(A)) Breaks.append(A);
				if ((Mode & 0x2) && !Breaks.contains(B)) Breaks.append(B);
			}

			Lines.insert(Objects.value(1).toInt(),
			{
				Objects.value(1).toInt(), QLineF(A, B), Objects.value(6).toInt()
			});

			Origins.insert(Objects.value(1).toInt(),
			{
				Objects.value(1).toInt(), QLineF(A, B), Objects.value(6).toInt()
			});

			emit onUpdateProgress(++Step);
		}
	}

	if (Mode & 0x1) for (const auto& E : Unique) Ends.append(E);
	if (Mode & 0x2) for (const auto& E : Ends) Breaks.removeAll(E);

	emit onBeginProgress(tr("Loading elements"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	if (Elements.exec()) while (Elements.next() && !isTerminated())
	{
		const int UID = Elements.value(0).toInt();

		if (Tasks.contains(UID))
		{
			if (!Geometry.contains(UID)) Geometry.insert(UID, QList<ELEMENT>());

			Geometry[UID].append(
			{
				Elements.value(1).toInt(),
				Elements.value(2).toInt()
			});

			emit onUpdateProgress(++Step);
		}
	}

	emit onBeginProgress(tr("Computing geometry"));
	emit onSetupProgress(0, 0);

	if (Mode & 0x4) QtConcurrent::blockingMap(Lines, [this, &Lines, &Intersect, &Synchronizer] (LINE& Part) -> void
	{
		if (this->isTerminated()) return;

		for (auto& L : Lines) if (!Part.Type && !L.Type && L.ID != Part.ID)
		{
			if (!pointComp(L.Line.p1(), Part.Line.p1()) && !pointComp(L.Line.p1(), Part.Line.p2()) &&
			    !pointComp(L.Line.p2(), Part.Line.p1()) && !pointComp(L.Line.p2(), Part.Line.p2()));
			else continue;

			QPointF Int; const auto Type = Part.Line.intersect(L.Line, &Int);

			if (Type == QLineF::BoundedIntersection)
			{
				Synchronizer.lock();

				if (!Intersect.contains(Int)) Intersect.append(Int);

				Synchronizer.unlock();
			}
		}
	});

	if (Mode) pointCuts.setSharable(false);

	if (Mode & 0x1) pointCuts.append(Ends);
	if (Mode & 0x2) pointCuts.append(Breaks);
	if (Mode & 0x4) pointCuts.append(Intersect);
	if (Mode & 0x8) pointCuts.append(Points);

	QtConcurrent::blockingMap(Lines, [this, &pointCuts, &Inserts, &Origins, &Synchronizer, Radius] (LINE& Part) -> void
	{
		if (this->isTerminated()) return;

		if (!Part.Type) for (const auto& P : pointCuts) if (P != Part.Line.p1() && P != Part.Line.p2())
		{
			QPointF Int; QLineF Normal(P, QPointF());

			Normal.setAngle(Part.Line.angle() + 90.0);
			Part.Line.intersect(Normal, &Int);

			const double a = QLineF(Int, Part.Line.p1()).length();
			const double b = QLineF(Int, Part.Line.p2()).length();
			const double c = Part.Line.length();

			if (QLineF(Int, P).length() <= Radius &&
			    Int != Part.Line.p1() &&
			    Int != Part.Line.p2() &&
			    a * a <= c * c + b * b &&
			    b * b <= c * c + a * a)
			{
				const QLineF NewA(Part.Line.p1(), P);
				const QLineF NewB(P, Part.Line.p2());

				if (NewA.length() >= Radius &&
				    NewB.length() >= Radius)
				{
					const INSERT Insert = { 0, Part.ID, NewA };

					Synchronizer.lock();

					Inserts.insert(Part.ID, Insert);

					Part.Changed = true;
					Part.Line = NewB;

					Synchronizer.unlock();
				}

				if (Part.Changed) return;
			}
		}
	});

	if (isTerminated()) return 0;

	for (const auto& L : Lines) if (L.Changed) Changed.insert(L.ID);

	emit onBeginProgress(tr("Inserting segments"));
	emit onSetupProgress(0, Inserts.size()); Step = 0;

	for (auto& I : Inserts)
	{
		emit onUpdateProgress(++Step);

		if (getIndex.exec() && getIndex.next())
		{
			I.ID = getIndex.value(0).toInt();

			insertSegment.addBindValue(I.ID);

			insertSegment.addBindValue(I.Line.x1());
			insertSegment.addBindValue(I.Line.y1());
			insertSegment.addBindValue(I.Line.x2());
			insertSegment.addBindValue(I.Line.y2());

			insertSegment.addBindValue(I.Base);

			insertSegment.exec();
		}
		else Changed.remove(I.Base);
	}

	emit onBeginProgress(tr("Updating segments"));
	emit onSetupProgress(0, Changed.size()); Step = 0;

	for (const auto& L : Lines) if (L.Changed && Changed.contains(L.ID))
	{
		emit onUpdateProgress(++Step);

		updateSegment.addBindValue(L.Line.x1());
		updateSegment.addBindValue(L.Line.y1());
		updateSegment.addBindValue(L.Line.x2());
		updateSegment.addBindValue(L.Line.y2());

		updateSegment.addBindValue(L.ID);

		updateSegment.exec();
	}

	emit onBeginProgress(tr("Updating geometry"));
	emit onSetupProgress(0, Geometry.size()); Step = 0;

	for (auto i = Geometry.constBegin(); i != Geometry.constEnd(); ++i)
	{
		QList<ELEMENT> Inserted, Rest;
		QList<LINE> Sorted; int N(0);

		for (const auto& E : i.value())
		{
			if (E.Typ == 1 || !Origins.contains(E.IDE))
			{
				Rest.append(E);
			}
			else Sorted.append(Origins[E.IDE]);
		}

		for (int n = 0; n < Sorted.size(); ++n)
		{
			const int IDE = Sorted[n].ID;

			if (Inserts.contains(IDE) && Changed.contains(IDE))
			{
				const int IID = Inserts[IDE].ID;

				const QLineF& A = Inserts[IDE].Line;
				const QLineF& B = Lines[IDE].Line;

				const QLineF Next = (n < Sorted.size() - 1) ? Sorted[n + 1].Line : QLineF();
				const QLineF Prev = (n > 0) ? Sorted[n - 1].Line : QLineF();

				if (pointComp(A.p1(), Next.p1()) || pointComp(A.p1(), Next.p2()) ||
				    pointComp(A.p2(), Next.p1()) || pointComp(A.p2(), Next.p2()))
				{
					Inserted.append({ IDE, 0 }); Inserted.append({ IID, 0 });
				}
				else if (pointComp(A.p1(), Prev.p1()) || pointComp(A.p1(), Prev.p2()) ||
					    pointComp(A.p2(), Prev.p1()) || pointComp(A.p2(), Prev.p2()))
				{
					Inserted.append({ IID, 0 }); Inserted.append({ IDE, 0 });
				}
				else if (pointComp(B.p1(), Next.p1()) || pointComp(B.p1(), Next.p2()) ||
					    pointComp(B.p2(), Next.p1()) || pointComp(B.p2(), Next.p2()))
				{
					Inserted.append({ IID, 0 }); Inserted.append({ IDE, 0 });
				}
				else if (pointComp(B.p1(), Prev.p1()) || pointComp(B.p1(), Prev.p2()) ||
					    pointComp(B.p2(), Prev.p1()) || pointComp(B.p2(), Prev.p2()))
				{

					Inserted.append({ IDE, 0 }); Inserted.append({ IID, 0 });
				}
				else
				{
					Inserted.append({ IDE, 0 }); Inserted.append({ IID, 0 });
				}
			}
			else
			{
				Inserted.append({ IDE, 0 });
			}
		}

		for (const auto& R : Rest) Inserted.append(R);

		if (i.value().size() != Inserted.size())
		{
			deleteElement.addBindValue(i.key());
			deleteElement.exec();

			for (const auto& E : Inserted)
			{
				insertElement.addBindValue(i.key());
				insertElement.addBindValue(E.IDE);
				insertElement.addBindValue(E.Typ);
				insertElement.addBindValue(N++);

				insertElement.exec();
			}

			Currentrun += Inserted.size() - i.value().size();
		}

		emit onUpdateProgress(++Step);
	}

	return Currentrun;
}

void DatabaseDriver::getCommon(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onCommonReady(QList<int>()); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), false, 1);
	const QList<int> Used = getCommonFields(Tasks.keys());

	emit onEndProgress();
	emit onCommonReady(Used);
}

void DatabaseDriver::getPreset(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPresetReady(QList<QHash<int, QVariant>>(), QList<int>()); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Model->getUids(Items).toSet(), false, 1);
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

	const QSet<int> Tasks = Model->getUids(Items).toSet();
	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<QString, QString> Points, Lines, Circles; int Step = 0;

	emit onBeginProgress(tr("Preparing classes"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	Query.prepare(
		"SELECT DISTINCT "
			"O.RODZAJ, D.KOD, D.OPIS "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_OPISY D "
		"ON "
			"O.KOD = D.KOD "
		"WHERE "
			"O.UID = ? AND "
			"O.STATUS = 0");

	for (const auto& UID : Tasks)
	{
		Query.addBindValue(UID);

		if (Query.exec()) while (Query.next())
		{
			switch (Query.value(0).toInt())
			{
				case 2:
					Lines.insert(Query.value(1).toString(), Query.value(2).toString());
				break;
				case 3:
					Circles.insert(Query.value(1).toString(), Query.value(2).toString());
				break;
				case 4:
					Points.insert(Query.value(1).toString(), Query.value(2).toString());
				break;
			}
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onJoinsReady(Points, Lines, Circles);
}

void DatabaseDriver::getClass(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onClassReady(QHash<QString, QString>(), QHash<QString, QHash<int, QString>>(), QHash<QString, QHash<int, QString>>(), QHash<QString, QHash<int, QString>>()); return; }

	QSqlQuery Query(Database), QueryA(Database), QueryB(Database), QueryC(Database), QueryD(Database),
			QueryE(Database), QueryF(Database), QueryG(Database), QueryH(Database);

	QHash<QString, QHash<int, QString>> Lines, Points, Texts;
	QHash<QString, QString> Classes; int Step = 0;

	const QSet<int> Tasks = Model->getUids(Items).toSet();

	const int Mask = 8 | 2 | 256; int Type = 0;

	Query.prepare("SELECT O.RODZAJ FROM EW_OBIEKTY O WHERE O.UID = ? AND O.STATUS = 0");

	QueryA.prepare(
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
			"O.KOD = ? AND "
			"L.NAZWA LIKE (O.KOD || '%') "
		"ORDER BY "
			"G.NAZWA_L");

	QueryB.prepare(
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
			"O.KOD = ? AND "
			"L.NAZWA LIKE (SUBSTRING(O.KOD FROM 1 FOR 4) || '%') "
		"ORDER BY "
			"L.DLUGA_NAZWA");

	QueryC.prepare(
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
			"O.KOD = ? AND "
			"T.NAZWA = O.KOD AND "
			"G.NAZWA NOT LIKE '%#_E' "
		"ESCAPE "
			"'#' "
		"ORDER BY "
			"G.NAZWA_L");

	QueryD.prepare(
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
			"O.KOD = ? AND "
			"T.NAZWA LIKE (O.KOD || '#_%') "
		"ESCAPE "
			"'#' "
		"ORDER BY "
			"G.NAZWA_L");

	QueryE.prepare(
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
			"O.KOD = ? AND "
			"T.NAZWA = O.KOD AND "
			"G.NAZWA LIKE '%#_E' "
		"ESCAPE "
			"'#' "
		"ORDER BY "
			"G.NAZWA_L");

	QueryF.prepare(
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
			"O.KOD = ? AND "
			"T.NAZWA = O.KOD "
		"ORDER BY "
			"G.NAZWA_L");

	QueryG.prepare(
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
			"O.KOD = ? AND "
			"T.NAZWA LIKE (SUBSTRING(O.KOD FROM 1 FOR 4) || '%') "
		"ORDER BY "
			"T.DLUGA_NAZWA");

	QueryH.prepare(
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
			"O.KOD = ? AND "
			"T.NAZWA LIKE (SUBSTRING(O.KOD FROM 1 FOR 4) || '%') "
		"ORDER BY "
			"G.NAZWA_L");

	emit onBeginProgress(tr("Preparing classes"));
	emit onSetupProgress(0, 0); Step = 0;

	for (const auto& UID : Tasks)
	{
		Query.addBindValue(UID);

		if (Query.exec() && Query.next()) switch (Query.value(0).toInt())
		{
			case 2:
				Type |= 8;
			break;
			case 3:
				Type |= 2;
			break;
			case 4:
				Type |= 256;
			break;
		}

		if (Type == Mask) break;
	}

	emit onEndProgress(); Step = 0;
	emit onBeginProgress(tr("Selecting layers data"));
	emit onSetupProgress(0, Tables.size());

	if (Type) for (const auto& Table : Tables)
	{
		if (Table.Type & Type)
		{
			Classes.insert(Table.Name, Table.Label);

			QHash<int, QString> L, P, T;

			{
				QueryA.addBindValue(Table.Name);

				if (QueryA.exec()) while (QueryA.next())
				{
					L.insert(QueryA.value(0).toInt(), QueryA.value(1).toString());
				}
			}

			if (L.isEmpty())
			{
				QueryB.addBindValue(Table.Name);

				if (QueryB.exec()) while (QueryB.next())
				{
					L.insert(QueryB.value(0).toInt(), QueryB.value(1).toString());
				}
			}

			{
				QueryC.addBindValue(Table.Name);

				if (QueryC.exec()) while (QueryC.next())
				{
					P.insert(QueryC.value(0).toInt(), QueryC.value(1).toString());
				}
			}

			{
				QueryD.addBindValue(Table.Name);

				if (QueryD.exec()) while (QueryD.next())
				{
					P.insert(QueryD.value(0).toInt(), QueryD.value(1).toString());
				}
			}

			{
				QueryE.addBindValue(Table.Name);

				if (QueryE.exec()) while (QueryE.next())
				{
					T.insert(QueryE.value(0).toInt(), QueryE.value(1).toString());
				}
			}

			if (T.isEmpty())
			{
				QueryF.addBindValue(Table.Name);

				if (QueryF.exec()) while (QueryF.next())
				{
					T.insert(QueryF.value(0).toInt(), QueryF.value(1).toString());
				}
			}

			if (T.isEmpty())
			{
				QueryG.addBindValue(Table.Name);

				if (QueryG.exec()) while (QueryG.next())
				{
					T.insert(QueryG.value(0).toInt(), QueryG.value(1).toString());
				}
			}

			if (T.isEmpty() || T.size() != T.values().toSet().size())
			{
				T.clear(); QueryH.addBindValue(Table.Name);

				if (QueryH.exec()) while (QueryH.next())
				{
					T.insert(QueryH.value(0).toInt(), QueryH.value(1).toString());
				}
			}

			Lines.insert(Table.Name, L);
			Points.insert(Table.Name, P);
			Texts.insert(Table.Name, T);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onClassReady(Classes, Lines, Points, Texts);
}

bool DatabaseDriver::addInterface(const QString& Path, int Type, bool Modal)
{
	if (!Database.isOpen() || !QFile::exists(Path)) return false;

	QSqlQuery Query(Database);

	Query.prepare(
		"UPDATE OR INSERT INTO "
			"EW_OB_INTERFEJSY (NAZWA, PROGRAM, TYP, MODALNY, IDKATALOG) "
		"VALUES "
			"('EW-Database', ?, ?, ?, 1) "
		"MATCHING "
			"(PROGRAM)");

	Query.addBindValue(Path);
	Query.addBindValue(Type);
	Query.addBindValue(Modal);

	return Query.exec();
}

void DatabaseDriver::unterminate(void)
{
	QMutexLocker Locker(&Terminator); Terminated = false;
}

void DatabaseDriver::terminate(void)
{
	QMutexLocker Locker(&Terminator); Terminated = true;
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

QVariant getDataByDict(QVariant Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type)
{
	if (!Value.isValid() || Value.toString() == "NULL") return QVariant();

	if (Type == DatabaseDriver::BOOL && Dict.isEmpty())
	{
		if (Value.toString() == DatabaseDriver::tr("Yes")) return true;
		else if (Value.toString() == DatabaseDriver::tr("No")) return false;
		else return Value.toBool();
	}

	if (Type == DatabaseDriver::MASK && !Dict.isEmpty())
	{
		QString List = Value.toString(); int Mask = 0;

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (List.contains(i.value())) Mask |= (1 << i.key().toInt());
		}

		return Mask;
	}

	if (Dict.isEmpty()) return Value;

	for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		if (i.value() == Value) return i.key();
	}

	return 0;
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

bool isVariantEmpty(const QVariant& Value)
{
	switch (Value.type())
	{
		case QVariant::Int: return Value == QVariant(int(0));
		case QVariant::Double: return Value == QVariant(double(0.0));
		case QVariant::Date: return Value == QVariant(QDate());
		case QVariant::DateTime: return Value == QVariant(QDateTime());
		case QVariant::String: return Value == QVariant(QString());
		case QVariant::List: return Value == QVariantList();

		default: return false;
	}
}

bool pointComp(const QPointF& A, const QPointF& B, double d)
{
	return (qAbs(A.x() - B.x()) <= d) && (qAbs(A.y() - B.y()) <= d);
}
