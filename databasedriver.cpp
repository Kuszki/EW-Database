/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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
: QObject(Parent), Terminated(false), makeHistory(false), Dateupdate(false)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Database = QSqlDatabase::addDatabase(Settings.value("driver", "QIBASE").toString());
	maxBindedSize = Settings.value("binded", 2500).toUInt();
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

QSqlDatabase& DatabaseDriver::getDatabase(void)
{
	return Database;
}

bool DatabaseDriver::isTerminated(void) const
{
	QMutexLocker Locker(&Terminator); return Terminated;
}

QStringList DatabaseDriver::nullReasons(void)
{
	return
	{
		tr("Default", "nilreason"),
		tr("Inapplicable", "nilreason"),
		tr("Missing", "nilreason"),
		tr("Template", "nilreason"),
		tr("Unknown", "nilreason"),
		tr("Withheld", "nilreason")
	};
}

QList<DatabaseDriver::FIELD> DatabaseDriver::loadCommon(bool Emit)
{
	if (!Database.isOpen()) return QList<FIELD>();

	QList<FIELD> Fieldlst =
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
		{ "EW_OBIEKTY.KOD",			"SELECT KOD, OPIS FROM EW_OB_OPISY"								},
		{ "EW_OBIEKTY.OPERAT",		"SELECT UID, COALESCE(NUMER || ':' || OPERACJA, NUMER) FROM EW_OPERATY"	},
		{ "EW_OBIEKTY.OSOU",		"SELECT ID, NAME FROM EW_USERS"									},
		{ "EW_OBIEKTY.OSOW",		"SELECT ID, NAME FROM EW_USERS"									}
	};

	if (Emit) emit onSetupProgress(0, Dict.size());

	int j = 0; for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		auto& Field = getItemByField(Fieldlst, i.key(), &FIELD::Name);

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
			"S.WYPELNIENIE "
		"FROM "
			"EW_OB_OPISY O "
		"LEFT JOIN "
			"EW_OB_DDSTR F "
		"ON "
			"O.KOD = F.KOD "
		"LEFT JOIN "
			"EW_OB_DDSTR S "
		"ON "
			"F.KOD = S.KOD "
		"LEFT JOIN "
			"EW_OB_DDSL D "
		"ON "
			"S.UID = D.UIDP OR S.UIDSL = D.UIDP "
		"WHERE "
			"S.NAZWA = F.NAZWA OR (S.NAZWA IS NULL AND F.NAZWA IS NULL) "
		"ORDER BY "
			"O.KOD, F.NAZWA, D.OPIS");

	if (Query.exec()) while (Query.next())
	{
		const QString Fname = Query.value(4).toString();
		const QString Field = QString("EW_DATA.%1").arg(Fname);
		const QString Table = Query.value(0).toString();
		const bool Dict = !Query.value(7).isNull();

		if (!Table.isEmpty() && !hasItemByField(List, Table, &TABLE::Name)) List.append(
		{
			Table,
			Query.value(1).toString(),
			Query.value(2).toString(),
			bool(Query.value(3).toInt() & 0x100),
			Query.value(3).toInt() & 266
		});

		auto& Tabref = getItemByField(List, Table, &TABLE::Name);

		if (!Fname.isEmpty() && !hasItemByField(Tabref.Fields, Field, &FIELD::Name)) Tabref.Fields.append(
		{
			TYPE(Query.value(6).toInt()),
			Field,
			Query.value(5).toString(),
			Query.value(9).toInt() == 2
		});

		if (!Fname.isEmpty() && Dict)
		{
			auto& Fieldref = getItemByField(Tabref.Fields, Field, &FIELD::Name);
			Fieldref.Dict.insert(Query.value(7), Query.value(8).toString());
		}

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
			"D.NAZWA, D.TYTUL, D.TYP, D.WYPELNIENIE "
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
			Query.value(3).toInt() == 2,
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
		for (const auto& Field : Tab.Fields) Tab.Indexes.append(List.indexOf(Field));
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
		for (const auto& Field : Tab.Fields) Tab.Headers.append(List.indexOf(Field.Label));
	});

	return List;
}

QMap<QString, QSet<int>> DatabaseDriver::getClassGroups(const QSet<int>& Indexes, bool Common, int Index)
{
	if (!Database.isOpen()) return QMap<QString, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMap<QString, QSet<int>> List; int Step = 0;

	const auto append = [&] (const QString& Table, int ID) -> void
	{
		if (!List.contains(Table)) List.insert(Table, QSet<int>());

		if (Common) List[QString()].insert(ID); List[Table].insert(ID);
	};

	emit onBeginProgress(tr("Preparing queries"));
	emit onSetupProgress(0, Indexes.size());

	if (Common) List.insert(QString(), QSet<int>());

	const bool isBinded = Indexes.size() < maxBindedSize;

	if (isBinded) Query.prepare(
		"SELECT "
			"D.KOD, D.DANE_DOD "
		"FROM "
			"EW_OB_OPISY D "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"D.KOD = O.KOD "
		"WHERE "
			"O.STATUS = 0 AND "
			"O.UID = ?");
	else Query.prepare(
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

	if (isBinded) for (const auto& ID : Indexes)
	{
		if (isTerminated()) break;

		Query.addBindValue(ID); Query.exec();

		if (Query.next())
		{
			append(Query.value(Index).toString(), ID);
		}

		emit onUpdateProgress(++Step);
	}
	else if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int ID = Query.value(0).toInt();

		if (Indexes.contains(ID))
		{
			append(Query.value(Index + 1).toString(), ID);

			emit onUpdateProgress(++Step);
		}

		if (Step == Indexes.size()) break;
	}

	emit onEndProgress(); return List;
}

QMap<QString, QSet<int>> DatabaseDriver::createHistory(const QMap<QString, QSet<int>>& Tasks, QHash<int, int>* Updates)
{
	if (!Database.isOpen()) return QMap<QString, QSet<int>>();

	QSqlQuery Query(Database); QHash<int, int> Uids; int Step(0);

	emit onBeginProgress(tr("Creating history"));
	emit onSetupProgress(0, Tasks.first().size() * 5);

	Query.prepare("SELECT GEN_ID(EW_OBIEKTY_UID_GEN, 1) FROM RDB$DATABASE");

	for (const auto& UID : Tasks[QString()])
	{
		if (Query.exec() && Query.next())
		{
			Uids.insert(UID, Query.value(0).toInt());
		}
		else return Tasks;

		emit onUpdateProgress(++Step);
	}

	Query.prepare("UPDATE EW_OBIEKTY SET "
				    "DTR = CURRENT_TIMESTAMP, "
				    "OSOR = 0, "
				    "STATUS = 3, "
				    "OPERATR = OPERATR "
			    "WHERE "
				    "UID = ?");

	for (const auto& UID : Tasks[QString()])
	{
		Query.addBindValue(UID);
		Query.exec();

		emit onUpdateProgress(++Step);
	}

	Query.prepare("INSERT INTO EW_OBIEKTY "
			    "("
				    "UID, ID, IDKATALOG, KOD, TYP, NUMER, POZYSKANIE, RODZAJ, "
				    "ATRYBUT_TYP, ATRYBUT_KUBATURA, ATRYBUT_N1, ATRYBUT_N2, "
				    "ATRYBUT_N3, ATRYBUT_S, OSOU, OSOW, OSOR, DTU, DTW, DTR, "
				    "OPERAT, OPERATR, STATUS, OPERATW, IIP, TERYT, DOD_OPERATY"
			    ") "
			    "SELECT "
				    "?, ID, IDKATALOG, KOD, TYP, NUMER, POZYSKANIE, RODZAJ, "
				    "ATRYBUT_TYP, ATRYBUT_KUBATURA, ATRYBUT_N1, ATRYBUT_N2, "
				    "ATRYBUT_N3, ATRYBUT_S, OSOU, OSOW, NULL, DTU, CURRENT_TIMESTAMP, "
				    "NULL, OPERAT, NULL, 0, OPERATW, IIP, TERYT, DOD_OPERATY "
			    "FROM EW_OBIEKTY WHERE UID = ?");

	for (const auto& UID : Tasks[QString()])
	{
		Query.addBindValue(Uids[UID]);
		Query.addBindValue(UID);
		Query.exec();

		emit onUpdateProgress(++Step);
	}

	Query.prepare("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, N, TYP, ATRYBUT) "
			    "SELECT ?, IDE, N, TYP, ATRYBUT FROM EW_OB_ELEMENTY "
			    "WHERE UIDO = ?");

	for (const auto& UID : Tasks[QString()])
	{
		Query.addBindValue(Uids[UID]);
		Query.addBindValue(UID);
		Query.exec();

		emit onUpdateProgress(++Step);
	}

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		QStringList Fieldlst;

		for (const auto& F : Table.Fields)
		{
			const QString Name = QString(F.Name).remove("EW_DATA.");

			Fieldlst.append(Name);

			if (F.Missing) Fieldlst.append(Name + "_V");
		}

		const QString List = Fieldlst.join(", ");

		Query.prepare(QString("INSERT INTO %1 (UIDO, %2) "
						  "SELECT ?, %2 FROM %1 "
						  "WHERE UIDO = ?")
				    .arg(Table.Data, List));

		for (const auto& UID : Tasks[QString()])
		{
			Query.addBindValue(Uids[UID]);
			Query.addBindValue(UID);
			Query.exec();

			emit onUpdateProgress(++Step);
		}
	}

	QMap<QString, QSet<int>> Res;

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		QSet<int> Class;

		for (const auto& UID : i.value())
		{
			Class.insert(Uids[UID]);
		}

		Res.insert(i.key(), Class);
	}

	if (Updates) *Updates = Uids;

	return Res;
}

QHash<int, QHash<int, QVariant>> DatabaseDriver::loadData(const DatabaseDriver::TABLE& Table, const QSet<int>& Filter, const QString& Where, bool Dict, bool View)
{
	if (!Database.isOpen()) return QHash<int, QHash<int, QVariant>>();

	const auto GET = Dict ? getDataFromDict :
	[] (const QVariant& Value, const QMap<QVariant, QString>&, TYPE) -> QVariant
	{
		return Value;
	};

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QHash<int, QVariant>> List; QStringList Attribs;

	for (const auto& Field : qAsConst(Common)) Attribs.append(Field.Name);
	for (const auto& Field : Table.Fields) Attribs.append(Field.Name);

	const auto append = [&] (const QSqlQuery& Query, int Index) -> void
	{
		const int Size = Common.size() + qMax(Table.Headers.size(), Table.Indexes.size());
		static QHash<int, QVariant> Values; int i = 1; Values.reserve(Size);

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

		if (!Values.isEmpty()) List.insert(Index, Values); Values.clear();
	};

	const QString ExecA = Table.Fields.isEmpty() ?
	QString(
		"SELECT "
			"EW_OBIEKTY.UID, %1 "
		"FROM "
			"EW_OBIEKTY "
		"WHERE "
			"EW_OBIEKTY.KOD = '%3' AND "
			"EW_OBIEKTY.STATUS = 0")
	.arg(Attribs.join(", "), Table.Name)
	:
	QString(
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
	.arg(Attribs.join(", "), Table.Data, Table.Name);

	const QString ExecB = Table.Fields.isEmpty() ?
	QString(
		"SELECT "
			"EW_OBIEKTY.UID, %1 "
		"FROM "
			"EW_OBIEKTY "
		"WHERE "
			"EW_OBIEKTY.KOD = '%3' AND "
			"EW_OBIEKTY.UID = ? AND "
			"EW_OBIEKTY.STATUS = 0")
	.arg(Attribs.join(", "), Table.Name)
	:
	QString(
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
			"EW_OBIEKTY.UID = ? AND "
			"EW_OBIEKTY.STATUS = 0")
	.arg(Attribs.join(", "), Table.Data, Table.Name);

	const bool isBinded = Filter.size() < maxBindedSize;
	const bool isEmpty = Filter.isEmpty();

	QString Exec = (isBinded && !isEmpty) ? ExecB : ExecA;

	if (!Where.isEmpty()) Exec.append(QString(" AND (%1)").arg(Where));

	if (!Query.prepare(Exec)) return QHash<int, QHash<int, QVariant>>();

	if (isBinded && !isEmpty) for (const auto& Index : Filter)
	{
		if (isTerminated()) break;

		Query.addBindValue(Index); Query.exec();

		if (Query.next()) append(Query, Index);
	}
	else if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int Index = Query.value(0).toInt();
		const bool Insert = isEmpty || Filter.contains(Index);

		if (Insert) append(Query, Index);
	}

	return List;
}

void DatabaseDriver::filterData(QHash<int, QHash<int, QVariant>>& Data, const QHash<int, QVariant>& Geometry, const QHash<int, QVariant>& Redaction, const QString& Limiter, double Radius)
{
	if (!Database.isOpen()) return; const QSet<int> Before = Data.keys().toSet(); QSet<int> Filtered = Before;

	const auto createSubsListIs = [] (QSet<int>* Current, const SUBOBJECTSTABLE* All, const QStringList& Classes, const QSet<int>& Limit) -> void
	{
		for (const auto& Object : *All) if (Classes.contains("*") || Classes.contains(Object.first.second))
		{
			if (Limit.contains(Object.first.first)) Current->insert(Object.first.first);
		}
	};

	const auto createSubsListHas = [] (QSet<int>* Current, const SUBOBJECTSTABLE* All, const QStringList& Classes, const QSet<int>& Limit) -> void
	{
		for (const auto& Object : *All) if (Classes.contains("*") || Classes.contains(Object.second.second))
		{
			if (Limit.contains(Object.second.first)) Current->insert(Object.second.first);
		}
	};

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0);

	QFile File(Limiter); QTextStream Stream(&File); QSet<int> Limit;

	if (!Limiter.isEmpty() && File.open(QFile::ReadOnly | QFile::Text))
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		const bool ok = Query.prepare("SELECT UID FROM EW_OBIEKTY WHERE STATUS = 0 AND NUMER = ?");

		while (ok && !Stream.atEnd() && !isTerminated())
		{
			Query.addBindValue(Stream.readLine().trimmed());

			if (Query.exec() && Query.next()) Limit.insert(Query.value(0).toInt());
		}
	}
	else
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true); QStringList All;

		for (int i = 4; i <= 15; ++i) if (Geometry.contains(i)) All.append(Geometry[i].toStringList());

		const int loadAll = All.contains("*");
		const QString Classes = All.toSet().toList().join("', '");

		const QString Str = "SELECT UID FROM EW_OBIEKTY WHERE STATUS = 0 AND (1 = '%1' OR KOD IN ('%2'))";

		if (Query.exec(Str.arg(loadAll).arg(Classes))) while (Query.next())
		{
			Limit.insert(Query.value(0).toInt());
		}
	}

	bool loadG(false); for (int i = 0; i <= 11; ++i) loadG = loadG || Geometry.contains(i);
	bool loadR(false); for (int i = 12; i <= 15; ++i) loadR = loadR || Geometry.contains(i);

	auto Redact = Redaction.isEmpty() ? QList<REDACTION>() : loadRedaction(Filtered);
	auto Geom = loadG ? loadGeometry(Filtered | Limit) : QList<OBJECT>(); QSet<int> Types;
	const auto Subs = loadR ? loadSubobjects() : SUBOBJECTSTABLE(); QSet<int> Subsl[4];
	const int originSize = Filtered.size(); QFutureSynchronizer<void> Synch;

	if (Geometry.contains(12)) Synch.addFuture(QtConcurrent::run(createSubsListHas, &Subsl[0], &Subs, Geometry[12].toStringList(), Limit));
	if (Geometry.contains(13)) Synch.addFuture(QtConcurrent::run(createSubsListHas, &Subsl[1], &Subs, Geometry[13].toStringList(), Limit));
	if (Geometry.contains(14)) Synch.addFuture(QtConcurrent::run(createSubsListIs, &Subsl[2], &Subs, Geometry[14].toStringList(), Limit));
	if (Geometry.contains(15)) Synch.addFuture(QtConcurrent::run(createSubsListIs, &Subsl[3], &Subs, Geometry[15].toStringList(), Limit));

	if (loadG) Synch.addFuture(QtConcurrent::map(Geom, [&Geometry, &Filtered, &Limit] (OBJECT& Obj) -> void
	{
		const auto hasClass = [] (const QVariant& V, const QString& C) -> bool
		{
			const auto L = V.toStringList(); return L.contains(C) || L.contains("*");
		};

		if (Filtered.contains(Obj.UID)) Obj.Mask |= 0b01;
		if (Limit.contains(Obj.UID)) Obj.Mask |= 0b10;

		for (int i = 4; i <= 11; ++i) if (Geometry.contains(i))
		{
			if (hasClass(Geometry[i], Obj.Class))
			{
				Obj.Mask |= (1 << i);
			}
		}
	}));

	if (Geometry.contains(100)) for (const auto& ID : Geometry[100].toList()) Types.insert(ID.toInt());

	Synch.waitForFinished();

	emit onBeginProgress(tr("Applying geometry filters"));
	emit onSetupProgress(0, 0);

	if (Geometry.contains(0) || Geometry.contains(1))
	{
		const double Min = Geometry.contains(0) ? Geometry[0].toDouble() : 0.0;
		const double Max = Geometry.contains(1) ? Geometry[1].toDouble() : INFINITY;

		Filtered = Filtered.intersect(filterDataByLength(Geom, Min, Max, originSize));
	}

	if (Geometry.contains(2) || Geometry.contains(3))
	{
		const double Min = Geometry.contains(2) ? Geometry[2].toDouble() : 0.0;
		const double Max = Geometry.contains(3) ? Geometry[3].toDouble() : INFINITY;

		Filtered = Filtered.intersect(filterDataBySurface(Geom, Min, Max, originSize));
	}

	if (Geometry.contains(4)) Filtered = Filtered.intersect(filterDataByIspartof(Geom, Radius, false, originSize));
	if (Geometry.contains(5)) Filtered = Filtered.intersect(filterDataByIspartof(Geom, Radius, true, originSize));

	if (Geometry.contains(6)) Filtered = Filtered.intersect(filterDataByContaining(Geom, Radius, false, originSize));
	if (Geometry.contains(7)) Filtered = Filtered.intersect(filterDataByContaining(Geom, Radius, true, originSize));

	if (Geometry.contains(8)) Filtered = Filtered.intersect(filterDataByEndswith(Geom, Radius, false, originSize));
	if (Geometry.contains(9)) Filtered = Filtered.intersect(filterDataByEndswith(Geom, Radius, true, originSize));

	if (Geometry.contains(10)) Filtered = Filtered.intersect(filterDataByIsnear(Geom, Radius, false, originSize));
	if (Geometry.contains(11)) Filtered = Filtered.intersect(filterDataByIsnear(Geom, Radius, true, originSize));

	if (Geometry.contains(12)) Filtered = Filtered.intersect(filterDataByHasSubobject(Filtered, Subsl[0], Subs, false));
	if (Geometry.contains(13)) Filtered = Filtered.intersect(filterDataByHasSubobject(Filtered, Subsl[1], Subs, true));

	if (Geometry.contains(14)) Filtered = Filtered.intersect(filterDataByIsSubobject(Filtered, Subsl[2], Subs, false));
	if (Geometry.contains(15)) Filtered = Filtered.intersect(filterDataByIsSubobject(Filtered, Subsl[3], Subs, true));

	if (Geometry.contains(20)) Filtered = Filtered.intersect(filterDataByHasMulrel(Filtered));

	if (Geometry.contains(100)) Filtered = Filtered.intersect(filterDataByHasGeoemetry(Filtered, Types));

	if (Redaction.contains(0) || Redaction.contains(1))
	{
		const double Min = Redaction.contains(0) ? Redaction[0].toDouble() : -INFINITY;
		const double Max = Redaction.contains(1) ? Redaction[1].toDouble() : INFINITY;

		Filtered = Filtered.intersect(filterDataBySymbolAngle(Redact, Min, Max));
	}

	if (Redaction.contains(2) || Redaction.contains(3))
	{
		const double Min = Redaction.contains(2) ? Redaction[2].toDouble() : -INFINITY;
		const double Max = Redaction.contains(3) ? Redaction[3].toDouble() : INFINITY;

		Filtered = Filtered.intersect(filterDataByLabelAngle(Redact, Min, Max));
	}

	if (Redaction.contains(4)) Filtered = Filtered.intersect(filterDataByLabelStyle(Redact, Redaction[4].toInt(), false));
	if (Redaction.contains(5)) Filtered = Filtered.intersect(filterDataByLabelStyle(Redact, Redaction[5].toInt(), true));

	if (Redaction.contains(6)) Filtered = Filtered.intersect(filterDataBySymbolText(Redact, Redaction[6].toStringList(), false));
	if (Redaction.contains(7)) Filtered = Filtered.intersect(filterDataBySymbolText(Redact, Redaction[7].toStringList(), true));

	if (Redaction.contains(8)) Filtered = Filtered.intersect(filterDataByLineStyle(Redact, Redaction[8].toStringList(), false));
	if (Redaction.contains(9)) Filtered = Filtered.intersect(filterDataByLineStyle(Redact, Redaction[9].toStringList(), true));

	if (Redaction.contains(10)) Filtered = Filtered.intersect(filterDataByLabelText(Redact, Redaction[10].toStringList(), false));
	if (Redaction.contains(11)) Filtered = Filtered.intersect(filterDataByLabelText(Redact, Redaction[11].toStringList(), true));

	for (const auto& UID : (Before - Filtered)) Data.remove(UID);
}

void DatabaseDriver::filterData(QHash<int, QHash<int, QVariant>>& Data, const QString& Expression)
{
	if (!Database.isOpen()) return; QSet<int> Deletes;

	const QSet<int> All = Data.keys().toSet(); QMutex Synchronizer;
	const QStringList Props = QStringList(Headers).replaceInStrings(QRegExp("\\W+"), " ")
										 .replaceInStrings(QRegExp("\\s+"), "_");

	emit onBeginProgress(tr("Performing advanced filters"));
	emit onSetupProgress(0, All.size()); int Step = 0;

	QtConcurrent::blockingMap(All, [this, &Data, &Synchronizer, &Step, &Expression, &Props, &Deletes] (int UID) -> void
	{
		if (isTerminated()) return;

		const auto& Row = Data[UID]; QJSEngine Engine;

		for (int i = 0; i < Props.size(); ++i)
		{
			const auto Val = Engine.toScriptValue(Row[i]);

			Engine.globalObject().setProperty(Props[i], Val);
		}

		const auto Res = Engine.evaluate(Expression);

		if (Res.isError() || !Res.toBool())
		{
			Synchronizer.lock();
			Deletes.insert(UID); if (!(++Step % 100)) emit onUpdateProgress(Step);
			Synchronizer.unlock();
		}
		else
		{
			Synchronizer.lock();
			if (!(++Step % 100)) emit onUpdateProgress(Step);
			Synchronizer.unlock();
		}
	});

	for (const auto& ID : Deletes) Data.remove(ID);
}

void DatabaseDriver::performDataUpdates(QMap<QString, QSet<int>>& Tasks, const QHash<int, QVariant>& Values, const QHash<int, int>& Reasons, bool Emit)
{
	const QSet<int> Used = Values.keys().toSet();
	const QSet<int> Nills = Reasons.keys().toSet();

	const bool Signals = signalsBlocked();
	if (!Emit) blockSignals(true);
	bool ok = true;

	if (makeHistory)
	{
		QHash<int, int> Newuids;

		Tasks = createHistory(Tasks, &Newuids);

		if (!Emit) blockSignals(Signals);
		emit onUidsUpdate(Newuids);
		if (!Emit) blockSignals(true);
	}

	QSqlQuery commonQuery(Database); commonQuery.setForwardOnly(true);
	QVariantList commonBinds; int Step = 0; QStringList All;

	emit onBeginProgress(tr("Updating common data"));
	emit onSetupProgress(0, Tasks.first().size());

	for (int i = 0; i < Common.size(); ++i) if (Values.contains(i))
	{
		All.append(QString("%1 = ?").arg(Fields[i].Name));
		commonBinds.append(Values[i]);
	}

	ok = commonQuery.prepare(QString("UPDATE EW_OBIEKTY SET %1 WHERE UID = ?").arg(All.join(", ")));

	if (ok && !All.isEmpty()) for (const auto& Index : Tasks.first())
	{
		if (isTerminated()) break;

		for (const auto& V : commonBinds)
		{
			commonQuery.addBindValue(V);
		}

		commonQuery.addBindValue(Index);
		commonQuery.exec();

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Updating special data"));
	emit onSetupProgress(0, Tasks.first().size()); Step = 0;

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i) if (!i.value().isEmpty())
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);

		QSqlQuery attribQuery(Database); attribQuery.setForwardOnly(true);
		QStringList fieldsNames; QVariantList attribBinds;

		for (const auto& Index : Used) if (Table.Indexes.contains(Index))
		{
			const auto Name = QString(Fields[Index].Name).remove("EW_DATA.");

			fieldsNames.append(Name);
			attribBinds.append(Values[Index]);
		}

		for (const auto& Index : Nills) if (Table.Indexes.contains(Index))
		{
			const auto Name = QString(Fields[Index].Name).remove("EW_DATA.").append("_V");

			fieldsNames.append(Name);
			attribBinds.append(Reasons[Index]);
		}

		ok = attribQuery.prepare(QString("UPDATE OR INSERT INTO %1 (%2, UIDO) "
							   "VALUES (%3?) MATCHING (UIDO)")
							.arg(Table.Data)
							.arg(fieldsNames.join(", "))
							.arg(QString("?, ").repeated(fieldsNames.size())));

		if (ok && !fieldsNames.isEmpty()) for (const auto& Index : i.value())
		{
			if (isTerminated()) break;

			for (const auto& V : attribBinds)
			{
				attribQuery.addBindValue(V);
			}

			attribQuery.addBindValue(Index);
			attribQuery.exec();

			emit onUpdateProgress(++Step);
		}
		else emit onUpdateProgress(Step += i.value().size());
	}

	if (Dateupdate) updateModDate(Tasks.first(), 0);

	if (!Emit) blockSignals(Signals);
}

QSet<int> DatabaseDriver::performBatchUpdates(const QMap<QString, QSet<int>> Tasks, const QList<BatchWidget::RECORD>& Functions, const QList<QVariantList>& Values)
{
	QHash<int, QHash<int, QVariant>> List; List.reserve(Tasks.first().size());

	emit onBeginProgress(tr("Executing batch"));
	emit onSetupProgress(0, 0); QSet<int> Changes;

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i) if (!isTerminated())
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j)
		{
			List.insert(j.key(), j.value());
		}
	}

	emit onSetupProgress(0, Values.size()); int Step = 0;

	for (const auto& Rules : Values) if (!isTerminated())
	{
		QSet<int> Filtered; QHash<int, QVariant> Updates;

		for (const auto& Index : Tasks.first())
		{
			const auto Data = List.value(Index);
			bool OK = true; int Col = 0;

			for (const auto& F : Functions) if (OK)
			{
				if (F.second == BatchWidget::WHERE) OK = Data[F.first] == Rules[Col]; ++Col;
			}

			if (OK) Filtered.insert(Index);
		}

		if (Filtered.size()) for (const auto& Table : Tables)
		{
			int Col(0); for (const auto& F : Functions)
			{
				if (F.second == BatchWidget::UPDATE)
				{
					if (F.first < Common.size())
					{
						if (F.first >= 0 && !Updates.contains(F.first) && Common[F.first].Type != READONLY)
						{
							const QVariant Val = castVariantTo(Rules[Col], Common[F.first].Type);

							if (Common[F.first].Dict.isEmpty()) Updates.insert(F.first, Val);
							else Updates.insert(F.first, getDataByDict(Rules[Col], Common[F.first].Dict, Common[F.first].Type));
						}
					}
					else
					{
						const int Column = Table.Headers.indexOf(F.first);
						const int Ufid = Table.Indexes.value(Column);

						if (Column != -1 && !Updates.contains(Ufid) && Fields[Ufid].Type != READONLY)
						{
							const QVariant Val = castVariantTo(Rules[Col], Fields[Ufid].Type);

							if (Table.Fields[Column].Dict.isEmpty()) Updates.insert(Ufid, Val);
							else Updates.insert(Ufid, getDataByDict(Rules[Col], Fields[Ufid].Dict, Fields[Ufid].Type));
						}
					}
				}

				Col += 1;
			}
		}

		if (Filtered.size() && Updates.size())
		{
			QMap<QString, QSet<int>> Update(Tasks); Changes += Filtered;

			for (auto& Task : Update) Task.intersect(Filtered);

			performDataUpdates(Update, Updates, QHash<int, int>(), false);
		}

		emit onUpdateProgress(++Step);
	}

	return Changes;
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

	All.subtract(Disabled).values();

	for (int i = 0; i < Common.size(); ++i) All.insert(i);

	return All.values();
}

bool DatabaseDriver::hasAllIndexes(const DatabaseDriver::TABLE& Tab, const QList<int>& Used)
{
	for (const auto& Index : Used) if (Index >= Common.size())
	{
		if (!Tab.Indexes.contains(Index)) return false;
	}

	return true;
}

void DatabaseDriver::updateModDate(const QSet<int>& Objects, int Type)
{
	if (!Database.isOpen()) return; QSqlQuery Query(Database);

	emit onBeginProgress(tr("Updating mod date"));
	emit onSetupProgress(0, Objects.size());
	int Step = 0; bool ok = true;

	switch (Type)
	{
		case 0:
			ok = Query.prepare("UPDATE EW_OBIEKTY SET DTW = CURRENT_TIMESTAMP WHERE UID = ?");
		break;
		case 1:
			ok = Query.prepare("UPDATE EW_POLYLINE SET MODIFY_TS = CURRENT_TIMESTAMP WHERE ID = ? AND STAN_ZMIANY = 0");
		break;
		case 2:
			ok = Query.prepare("UPDATE EW_TEXT SET MODIFY_TS = CURRENT_TIMESTAMP WHERE ID = ? AND STAN_ZMIANY = 0");
		break;
		default: return;
	}

	if (ok) for (const auto& ID : Objects)
	{
		Query.addBindValue(ID);
		Query.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
}

void DatabaseDriver::appendLog(const QString& Title, const QSet<int>& Items)
{
	QMutexLocker Locker(&Terminator); QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	const QString Logdir = Settings.value("logdir").toString();
	Settings.endGroup();

	const QString Path = QString("%1/%2_%3.txt").arg(Logdir).arg(Title)
					 .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));

	QFile File(Path); QTextStream Stream(&File);

	if (!File.open(QFile::WriteOnly | QFile::Text)) return;

	emit onBeginProgress(tr("Creating log file"));
	emit onSetupProgress(0, Items.size()); int Step(0);

	QSqlQuery Query("SELECT UID, NUMER FROM EW_OBIEKTY WHERE STATUS = 0", Database);

	if (Items.size()) while (Query.next()) if (Items.contains(Query.value(0).toInt()))
	{
		Stream << Query.value(1).toString() << Qt::endl; emit onUpdateProgress(++Step);
	}
}

bool DatabaseDriver::isLogEnabled(void) const
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	const bool Log = Settings.value("logen", false).toBool();
	Settings.endGroup();

	return Log;
}

bool DatabaseDriver::openDatabase(const QString& Server, const QString& Base, const QString& User, const QString& Pass)
{
	static int defaultPort = 0;

	if (!defaultPort) defaultPort = Database.port();
	if (Database.isOpen()) Database.close();

	Database.setHostName(Server.section(':', 0, 0));
	Database.setDatabaseName(Base);
	Database.setUserName(User);
	Database.setPassword(Pass);

	if (Server.contains(':')) Database.setPort(Server.section(':', 1).toInt());
	else Database.setPort(defaultPort);

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

void DatabaseDriver::loadList(const QStringList& Filter, int Index, int Action, const RecordModel* Current, const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	const QSet<QString> Hash = Filter.toSet(); QSet<int> Load;
	QSet<int> UIDS; UIDS.reserve(Hash.size()); int Step = 0;
	RecordModel* Model = new RecordModel(Headers);

	emit onBeginProgress(tr("Preparing objects list"));
	emit onSetupProgress(0, Hash.size());

	Query.prepare("SELECT O.UID, O.NUMER, O.IIP, K.NUMER FROM EW_OBIEKTY O "
			    "LEFT JOIN EW_OPERATY K ON O.OPERAT = K.UID WHERE O.STATUS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
		if (!Query.value(Index).isNull() && Hash.contains(Query.value(Index).toString()))
		{
			UIDS.insert(Query.value(0).toInt()); emit onUpdateProgress(++Step);
		}

	if (Action == 0) Load = UIDS;
	else if (Action == 1) Load = UIDS & Items;
	else if (Action == 2) Load = UIDS + Items;
	else if (Action == 3) Load = Items - UIDS;

	if (isTerminated() || Load.isEmpty()) { emit onEndProgress(); emit onDataLoad(Model); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Load, false, 0);

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i) if (!isTerminated())
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j)
		{
			Model->addItem(j.key(), j.value());
		}

		emit onUpdateProgress(++Step);
	}

	if (sender()) Model->moveToThread(sender()->thread());

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::reloadData(const QString& Filter, const QString& Script, QList<int> Used, const QHash<int, QVariant>& Geometry, const QHash<int, QVariant>& Redaction, const QString& Limiter, double Radius, int Mode, const RecordModel* Current, const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataLoad(nullptr); return; }

	if (Used.isEmpty()) Used = getUsedFields(Filter);

	emit onBeginProgress(tr("Querying database"));
	emit onSetupProgress(0, Tables.size());

	RecordModel* Model = new RecordModel(Headers); int Step = 0;
	QHash<int, QHash<int, QVariant>> List; QSet<int> Loaded;

	const QSet<int> Preload = (Mode == 1 || Mode == 3) ? Items : QSet<int>();
	const QMap<QString, QSet<int>> Tasks = Preload.isEmpty() ? QMap<QString, QSet<int>>() : getClassGroups(Preload, false, 0);

	for (const auto& Table : Tables) if (!isTerminated() && hasAllIndexes(Table, Used))
	{
		if (Tasks.isEmpty() || Tasks.contains(Table.Name))
		{
			const auto Set = Tasks.value(Table.Name, QSet<int>());
			auto Data = loadData(Table, Set, Filter, true, true);

			for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
			{
				List.insert(i.key(), i.value());
			}
		}

		emit onUpdateProgress(++Step);
	}

	if (!isTerminated() && (!Geometry.isEmpty() || !Redaction.isEmpty()))
	{
		filterData(List, Geometry, Redaction, Limiter, Radius);
	}

	if (!isTerminated() && !Script.isEmpty()) filterData(List, Script);

	if (isTerminated()) { emit onEndProgress(); emit onDataLoad(Model); return; }

	emit onBeginProgress(tr("Creating object list"));
	emit onSetupProgress(0, 0);

	if (!Current) Mode = 0;
	else
	{
		QSet<int> Exists; QSqlQuery Query("SELECT UID FROM EW_OBIEKTY WHERE STATUS = 0", Database);

		while (Query.next()) Exists.insert(Query.value(0).toInt()); Loaded = Items & Exists;
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

	if (sender()) Model->moveToThread(sender()->thread());

	emit onEndProgress();
	emit onDataLoad(Model);
}

void DatabaseDriver::updateData(const QSet<int>& Items, const QHash<int, QVariant>& Values, const QHash<int, int>& Reasons)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataUpdate(); return; }

	QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 1);

	performDataUpdates(Tasks, Values, Reasons, true); int Step = 0;

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size() - 1);

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataUpdate();
}

void DatabaseDriver::removeData(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRemove(); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, false, 1);

	QSqlQuery selectLines(Database), selectTexts(Database), selectUIDS(Database), selectCommon(Database),
			QueryA(Database), QueryB(Database), QueryC(Database),
			QueryD(Database), QueryE(Database), QueryF(Database);

	QSet<int> Lines, Texts, Commonlst; QHash<int, int> UIDS; int Step = 0;
	const QString deleteQuery = QString("DELETE FROM %1 WHERE UIDO = ?");

	selectCommon.prepare(
		"SELECT "
			"E.UIDO, E.IDE "
		"FROM "
			"EW_OB_ELEMENTY E "
		"INNER JOIN "
			"EW_OBIEKTY O "
		"ON "
			"O.UID = E.UIDO "
		"WHERE "
			"E.TYP = 0 AND O.STATUS = 0");

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
	emit onSetupProgress(0, 0);

	if (selectCommon.exec()) while (selectCommon.next() && !isTerminated())
	{
		if (!Items.contains(selectCommon.value(0).toInt()))
		{
			Commonlst.insert(selectCommon.value(1).toInt());
		}
	}

	if (selectUIDS.exec()) while (selectUIDS.next() && !isTerminated())
	{
		UIDS.insert(selectUIDS.value(0).toInt(),
				  selectUIDS.value(1).toInt());
	}

	emit onBeginProgress(tr("Loading items"));
	emit onSetupProgress(0, Items.size());

	for (const auto UID : Items) if (!isTerminated())
	{
		selectLines.addBindValue(UID);

		if (selectLines.exec()) while (selectLines.next())
		{
			const int IDE = selectLines.value(1).toInt();
			if (!Commonlst.contains(IDE)) Lines.insert(IDE);
		}

		selectTexts.addBindValue(UID);

		if (selectTexts.exec()) while (selectTexts.next())
		{
			const int IDE = selectTexts.value(1).toInt();
			if (!Commonlst.contains(IDE)) Texts.insert(IDE);
		}

		emit onUpdateProgress(++Step);
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataRemove(); return; }

	emit onBeginProgress(tr("Removing objects"));
	emit onSetupProgress(0, Items.size()); Step = 0;

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const bool ok = QueryF.prepare(deleteQuery.arg(i.key()));

		if (ok) for (const auto& Index : i.value())
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

	emit onRowsRemove(Items);

	emit onEndProgress();
	emit onDataRemove();
}

void DatabaseDriver::execBatch(const QSet<int>& Items, const QList<BatchWidget::RECORD>& Functions, const QList<QStringList>& Values)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onBatchExec(0); return; }

	QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 1); int Step = 0;

	QList<QVariantList> Args; for (const auto& A : Values)
	{
		QVariantList Row;

		for (const auto& V : A) Row.append(QVariant::fromValue(V));

		Args.append(Row);
	}

	const QSet<int> Changes = performBatchUpdates(Tasks, Functions, Args);
	for (auto& Group : Tasks) Group.intersect(Changes);

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size() - 1);

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onBatchExec(Changes.count());
}

void DatabaseDriver::execFieldcopy(const QSet<int>& Items, const QList<CopyfieldsWidget::RECORD>& Functions, bool Nulls)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onCopyExec(0); return; }

	QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 1);
	QHash<int, QHash<int, QVariant>> List; List.reserve(Items.size());

	emit onBeginProgress(tr("Loading items"));
	emit onSetupProgress(0, 0); int Step = 0;

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i) if (!isTerminated())
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j)
		{
			List.insert(j.key(), j.value());
		}
	}

	QList<BatchWidget::RECORD> uFunctions; QList<QVariantList> uValues;

	for (const auto& R : List)
	{
		QVariantList Row; Row.reserve(Functions.size());

		for (const auto& F : Functions)
			if (F.first == CopyfieldsWidget::WHERE)
			{
				Row.append(R.value(F.second.second));
			}
			else if (Nulls || !isVariantEmpty(R.value(F.second.second)))
			{
				Row.append(R.value(F.second.second));
			}

		if (Row.size() == Functions.size() && !uValues.contains(Row))
		{
			uValues.append(Row);
		}
	}

	for (const auto& F : Functions) switch (F.first)
	{
		case CopyfieldsWidget::WHERE:
			uFunctions.append(qMakePair(F.second.first, BatchWidget::WHERE));
		break;
		case CopyfieldsWidget::UPDATE:
			uFunctions.append(qMakePair(F.second.first, BatchWidget::UPDATE));
		break;
	}

	const QSet<int> Changes = performBatchUpdates(Tasks, uFunctions, uValues);
	for (auto& Group : Tasks) Group.intersect(Changes);

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size() - 1);

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onCopyExec(Changes.size());
}

void DatabaseDriver::execScript(const QSet<int>& Items, const QString& Script)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onScriptExec(0); return; }

	QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 1);
	QHash<int, QHash<int, QVariant>> List; List.reserve(Items.size());

	emit onBeginProgress(tr("Loading items"));
	emit onSetupProgress(0, 0); int Step = 0;

	const QStringList Props = QStringList(Headers).replaceInStrings(QRegExp("\\W+"), " ")
										 .replaceInStrings(QRegExp("\\s+"), "_");

	QMutex Synchronizer; const int Size = Props.size();

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i) if (!isTerminated())
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		auto Data = loadData(Table, i.value(), QString(), true, true);

		for (auto j = Data.constBegin(); j != Data.constEnd(); ++j)
		{
			List.insert(j.key(), j.value());
		}
	}

	QList<BatchWidget::RECORD> Functions; QList<QVariantList> Values;

	for (int i = 0; i < Size; ++i) if (i != 2)
	{
		Functions.append({ i, BatchWidget::UPDATE });
	}
	else Functions.append({ i, BatchWidget::WHERE });

	const auto Loaded = List.keys().toSet();

	QtConcurrent::blockingMap(Loaded,
	[&List, &Script, &Values, &Props, &Size, &Synchronizer]
	(const int& UID) -> void
	{
		const auto& Item = List[UID];
		QHash<QString, QJSValue> Before;
		QJSEngine Engine; bool OK(false);

		for (int i = 0; i < Size; ++i)
		{
			const QJSValue Value = Engine.toScriptValue(Item.value(i));
			const QString Name = Props[i];

			Engine.globalObject().setProperty(Name, Value);
			Before.insert(Name, Value);
		}

		const auto V = Engine.evaluate(Script);

		if (!V.isError()) for (int i = 0; i < Size; ++i) if (!OK)
		{
			OK = Before[Props[i]].equals(Engine.globalObject().property(Props[i]));
		}

		if (OK)
		{
			QVariantList Updates;

			for (int i = 0; i < Size; ++i) if (i != 2)
			{
				Updates.append(Engine.globalObject().property(Props[i]).toVariant());
			}
			else Updates.append(Item[2].toString());

			Synchronizer.lock();
			Values.append(Updates);
			Synchronizer.unlock();
		}
	});

	const QSet<int> Changes = performBatchUpdates(Tasks, Functions, Values);
	for (auto& Group : Tasks) Group.intersect(Changes);

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onScriptExec(Changes.size());
}

void DatabaseDriver::splitData(const QSet<int>& Items, const QString& Point, const QString& From, int Type)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataSplit(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 0);
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

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks[From].size()); Step = 0;

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
			"O.STATUS = 0");

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

	emit onBeginProgress(tr("Splitting data"));
	emit onSetupProgress(0, Objects.size()); Step = 0;

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

void DatabaseDriver::joinData(const QSet<int>& Items, const QString& Point, const QString& Join, bool Override, int Type, double Radius)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataJoin(0); return; }

	QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 0); QHash<int, int> Newuids;
	QList<POINT> Points; QSet<int> Joined, Skip; QHash<int, QSet<int>> Geometry, Insert;
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

	Query.prepare("SELECT UID, ID FROM EW_OBIEKTY WHERE STATUS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		if (Joined.contains(Query.value(1).toInt()))
		{
			Skip.insert(Query.value(0).toInt());
		}
	}

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, Tasks[Point].size()); Step = 0;

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

	if (Type != 3 && Query.exec()) while (Query.next() && !isTerminated())
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

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Tasks[Join].size()); Step = 0;

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

	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, 0); Step = 0;

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
		case 3:
			Insert = joinMixed(Geometry, Tasks[Join], Tasks[Point] - Skip, Radius);
		break;
	}

	emit onBeginProgress(tr("Creating history"));
	emit onSetupProgress(0, 0);

	if (makeHistory)
	{
		QSet<int> Uids; for (auto i = Insert.constBegin(); i != Insert.constEnd(); ++i)
		{
			Uids.insert(i.key());
		}

		createHistory(getClassGroups(Uids, true, 1), &Newuids);

		QtConcurrent::blockingMap(Tasks, [&Newuids] (QSet<int>& Set)
		{
			for (auto i = Newuids.constBegin(); i != Newuids.constEnd(); ++i)
			{
				if (Set.contains(i.key()))
				{
					Set.remove(i.key());
					Set.insert(i.value());
				}
			}
		});

		emit onUidsUpdate(Newuids);
	}

	if (Dateupdate) updateModDate(Newuids.values().toSet(), 0);

	emit onBeginProgress(tr("Joining data"));
	emit onSetupProgress(0, Insert.size()); Step = 0;

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
		const int UIDO = Newuids.value(i.key(), i.key());

		for (const auto& P : i.value())
		{
			QueryA.addBindValue(UIDO);
			QueryA.addBindValue(P);
			QueryA.addBindValue(UIDO);

			QueryA.exec();

			if (!Override) continue;

			QueryB.addBindValue(UIDO);
			QueryB.addBindValue(P);

			QueryB.exec();
		}

		Count += i.value().size();

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	if (!Insert.isEmpty() && (Override || Dateupdate))
		for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
		{
			const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
			const auto Data = loadData(Table, i.value(), QString(), true, true);

			emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
		}

	emit onEndProgress();
	emit onDataJoin(Count);
}

void DatabaseDriver::mergeData(const QSet<int>& Items, const QList<int>& Values, const QStringList& Points, double Diff)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataMerge(0); return; }

	struct PART { int ID; QLineF Line; };

	const auto appendCount = [] (const QPointF& P, QSet<QPair<double, double>>& Cuts, QHash<QPair<double, double>, int>& Counts)
	{
		if (++Counts[qMakePair(P.x(), P.y())] == 3) Cuts.insert(qMakePair(P.x(), P.y()));
	};

	const auto isTouching = [] (const QLineF& A, const QLineF& B, QPointF* P = nullptr) -> bool
	{
		QPointF Touch = QPointF(); int N = 0;

		if (A.p1() == B.p1() || A.p1() == B.p2()) { Touch = A.p1(); ++N; }
		if (A.p2() == B.p1() || A.p2() == B.p2()) { Touch = A.p2(); ++N; }

		if (N == 1 && P) *P = Touch; return N == 1;
	};

	const auto isClosed = [isTouching] (const QList<PART>& List) -> bool
	{
		if (List.size() < 3) return false;
		else return isTouching(List.first().Line, List.last().Line);
	};

	const auto isAngleOK = [Diff, isTouching] (const QLineF& A, const QLineF& B) -> bool
	{
		QPointF T; QLineF New; const double Angle = A.angleTo(B);

		if (!isTouching(A, B, &T)) return false;

		if (T == A.p1()) New.setP1(A.p2()); else New.setP1(A.p1());
		if (T == B.p1()) New.setP2(B.p2()); else New.setP2(B.p1());

		if (New.length() < A.length() || New.length() < B.length()) return false;

		return (qAbs(Angle) < Diff || qAbs(qAbs(Angle) - 180) < Diff || qAbs(qAbs(Angle) - 360) < Diff);
	};

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 0);

	QHash<QPair<double, double>, int> Counts; QSet<QPair<double, double>> Cuts;
	QSet<int> Used; QHash<int, QSet<int>> Merges; int Step = 0;

	QHash<int, QList<PART>> Geometries; QSet<int> Merged;
	QHash<int, QList<QPair<int, int>>> Additions;

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	if (!Points.isEmpty())
	{
		Query.prepare(QString(
			"SELECT "
				"ROUND(T.POS_X, 5), "
				"ROUND(T.POS_Y, 5), "
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.insert(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});

		Query.prepare(QString(
			"SELECT "
				"ROUND((P.P0_X + P.P1_X) / 2.0, 5), "
				"ROUND((P.P0_Y + P.P1_Y) / 2.0, 5) "
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.insert(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});
	}

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Items.size()); Step = 0;

	Query.prepare(
		"SELECT "
			"O.UID, "
			"ROUND(P.P0_X, 5), ROUND(P.P0_Y, 5), "
			"ROUND(IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), 5), "
			"ROUND(IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y), 5), "
			"E.IDE, E.TYP, IIF(P.ID IS NULL, 1, 0) "
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

		if (Items.contains(ID))
		{
			if (!Geometries.contains(ID)) emit onUpdateProgress(++Step);

			const QPointF PointA(Query.value(1).toDouble(), Query.value(2).toDouble());
			const QPointF PointB(Query.value(3).toDouble(), Query.value(4).toDouble());

			if (Query.value(7).toBool()) Additions[ID].append(
			{
				{
					Query.value(5).toInt(),
					Query.value(6).toInt()
				}
			});
			else Geometries[ID].append(
			{
				Query.value(5).toInt(),
				{
					PointA, PointB
				}
			});

			appendCount(PointA, Cuts, Counts);
			appendCount(PointB, Cuts, Counts);
		}
	}

	emit onBeginProgress(tr("Merging objects"));
	emit onSetupProgress(0, Items.size()); Step = 0;

	for (auto k = Tasks.constBegin() + 1; k != Tasks.constEnd(); ++k) if (!isTerminated())
	{
		const auto& Table = getItemByField(Tables, k.key(), &TABLE::Name);
		const auto Data = loadData(Table, k.value(), QString(), false, false);

		for (auto i = Data.constBegin(); i != Data.constEnd(); ++i) if (!isTerminated())
		{
			auto& Geometry = Geometries[i.key()]; const auto& D1 = i.value();

			if (!Used.contains(i.key()) && !Geometry.isEmpty() && !isClosed(Geometry))
			{
				bool Continue(true); Used.insert(i.key());

				while (Continue && !isTerminated())
				{
					const int oldSize = Geometry.size();

					for (auto j = Data.constBegin(); j != Data.constEnd(); ++j) if (!Geometries[j.key()].isEmpty())
					{
						auto& othGeometry = Geometries[j.key()]; QPointF Touch;
						const auto& D2 = j.value(); bool OK(true), Added(false);

						if (Used.contains(j.key()) || isClosed(othGeometry)) continue;

						for (const auto& Field : Values) OK = OK && (D1[Field] == D2[Field]); if (!OK) continue;

						const auto& L1f = Geometry.first().Line; const auto& L1l = Geometry.last().Line;
						const auto& L2f = othGeometry.first().Line; const auto& L2l = othGeometry.last().Line;

						if (isTouching(L1f, L2f, &Touch))
						{
							if ((Diff != 0.0 && isAngleOK(L1f, L2f)) || !Cuts.contains({ Touch.x(), Touch.y() }))
							{
								std::reverse(Geometry.begin(), Geometry.end());
								Geometry = Geometry + othGeometry; Added = true;
							}
						}
						else if (isTouching(L1f, L2l, &Touch))
						{
							if ((Diff != 0.0 && isAngleOK(L1f, L2l)) || !Cuts.contains({ Touch.x(), Touch.y() }))
							{
								Geometry = othGeometry + Geometry; Added = true;
							}
						}
						else if (isTouching(L1l, L2f, &Touch))
						{
							if ((Diff != 0.0 && isAngleOK(L1l, L2f)) || !Cuts.contains({ Touch.x(), Touch.y() }))
							{
								Geometry = Geometry + othGeometry; Added = true;
							}
						}
						else if (isTouching(L1l, L2l, &Touch))
						{
							if ((Diff != 0.0 && isAngleOK(L1l, L2f)) || !Cuts.contains({ Touch.x(), Touch.y() }))
							{
								std::reverse(Geometry.begin(), Geometry.end());
								Geometry = othGeometry + Geometry; Added = true;
							}
						}

						if (Added)
						{
							Used.insert(j.key());
							Merges[i.key()].insert(j.key());

							Additions[i.key()].append(Additions[j.key()]);
						}

						if (Added && isClosed(Geometry)) break;
					}

					Continue = oldSize != Geometry.size() && !isClosed(Geometry);
				}
			}

			emit onUpdateProgress(++Step);
		}
	}

	if (isTerminated()) { emit onEndProgress(); emit onDataMerge(0); return; }

	emit onBeginProgress(tr("Updating database")); int Count(0);
	emit onSetupProgress(0, Merges.size()); Step = 0;

	for (auto k = Tasks.constBegin() + 1; k != Tasks.constEnd(); ++k)
	{
		const auto& Table = getItemByField(Tables, k.key(), &TABLE::Name);

		for (const auto& Index : k.value()) if (Merges.contains(Index))
		{
			QVariantList obValues, ddValues;

			Count += Merges[Index].size() + 1;
			Merged += Merges[Index];

			Query.exec(QString("SELECT * FROM EW_OBIEKTY WHERE UID = %1").arg(Index));
			if (Query.next()) for (int i = 1; i < Query.record().count(); ++i) obValues.append(Query.value(i));

			Query.exec(QString("SELECT * FROM %1 WHERE UIDO = %2").arg(Table.Data).arg(Index));
			if (Query.next()) for (int i = 1; i < Query.record().count(); ++i) ddValues.append(Query.value(i));

			for (const auto& Part : Merges[Index])
			{
				QVariantList mObValues, mDdValues;

				Query.exec(QString("SELECT * FROM EW_OBIEKTY WHERE UID = %1").arg(Part));
				if (Query.next()) for (int i = 1; i < Query.record().count(); ++i) mObValues.append(Query.value(i));

				Query.exec(QString("SELECT * FROM %1 WHERE UIDO = %2").arg(Table.Data).arg(Part));
				if (Query.next()) for (int i = 1; i < Query.record().count(); ++i) mDdValues.append(Query.value(i));

				if (!obValues.isEmpty() && !mObValues.isEmpty()) for (int i = 0; i < obValues.size(); ++i)
				{
					obValues[i] = isVariantEmpty(obValues[i]) ? mObValues[i] : obValues[i];
				}
				else if (!mObValues.isEmpty()) obValues = mObValues;

				if (!ddValues.isEmpty() && !mDdValues.isEmpty()) for (int i = 0; i < ddValues.size(); ++i)
				{
					ddValues[i] = qMax(ddValues[i], mDdValues[i]);
				}
				else if (!mDdValues.isEmpty()) ddValues = mDdValues;

				Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1").arg(Part));
				Query.exec(QString("DELETE FROM EW_OBIEKTY WHERE UID = %1").arg(Part));
				Query.exec(QString("DELETE FROM %1 WHERE UIDO = %2").arg(Table.Data, Part));
			}

			if (!obValues.isEmpty())
			{
				Query.prepare(QString("UPDATE OR INSERT INTO EW_OBIEKTY VALUES (%1%2) MATCHING (UID)")
						    .arg(Index).arg(QString(",?").repeated(obValues.size())));

				for (const auto& V : obValues) Query.addBindValue(V); Query.exec();
			}

			if (!ddValues.isEmpty())
			{
				Query.prepare(QString("UPDATE OR INSERT INTO %1 VALUES (%2%3) MATCHING (UIDO)")
						    .arg(Table.Data).arg(Index).arg(QString(",?").repeated(ddValues.size())));

				for (const auto& V : ddValues) Query.addBindValue(V); Query.exec();
			}

			Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1").arg(Index)); int N(0);
			Query.prepare("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) VALUES (?, ?, ?, ?)");

			for (const auto& P: Geometries[Index])
			{
				Query.addBindValue(Index);
				Query.addBindValue(P.ID);
				Query.addBindValue(0);
				Query.addBindValue(++N);

				Query.exec();
			}

			for (const auto& P: Additions[Index]) if (P.second == 0)
			{
				Query.addBindValue(Index);
				Query.addBindValue(P.first);
				Query.addBindValue(0);
				Query.addBindValue(++N);

				Query.exec();
			}

			for (const auto& P: Additions[Index]) if (P.second != 0)
			{
				Query.addBindValue(Index);
				Query.addBindValue(P.first);
				Query.addBindValue(P.second);
				Query.addBindValue(++N);

				Query.exec();
			}

			emit onUpdateProgress(++Step);
		}
	}

	emit onRowsRemove(Merged);

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Data = loadData(Table, i.value() - Merged, QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataMerge(Count);
}

void DatabaseDriver::cutData(const QSet<int>& Items, const QStringList& Points, int Endings)
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

					if (qIsNaN(P.L) || h < P.L) { P.L = h; P.LID = L.ID; }
				}
				else
				{
					const double h = qMin(a, b) * 2.0;

					if (qIsNaN(P.L) || h < P.L) { P.L = h; P.LID = L.ID; }
				}
			}
		};

		for (auto& Part : P.Labels) find(P.Lines, Part);
		for (auto& Part : P.Objects) find(P.Lines, Part);
	};

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 0); int Step = 0;
	QHash<int, PARTS> Parts; QSet<QPair<double, double>> Cuts; QHash<int, QList<int>> Queue;
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.insert(
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

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.insert(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});
	}

	if (Endings)
	{
		QHash<QPair<double, double>, int> Counts;
		QHash<int, QSet<QPair<double, double>>> Unique;

		Query.prepare(
			"SELECT "
				"O.UID, ROUND(P.P0_X, 3), ROUND(P.P0_Y, 3), "
				"ROUND(IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), 3), "
				"ROUND(IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y), 3) "
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
				"P.STAN_ZMIANY = 0");

		if (Query.exec()) while (Query.next() && !isTerminated())
		{
			const int UID = Query.value(0).toInt();

			if (Items.contains(UID))
			{
				const QPair<double, double> PA(Query.value(1).toDouble(), Query.value(2).toDouble());
				const QPair<double, double> PB(Query.value(3).toDouble(), Query.value(4).toDouble());

				if (Endings == 1)
				{
					if (++Counts[PA] == 3) Cuts.insert(PA);
					if (++Counts[PB] == 3) Cuts.insert(PB);
				}
				else if (Endings == 2)
				{
					if (!Unique[UID].contains(PA))
					{
						Unique[UID].insert(PA);
					}
					else Unique[UID].remove(PA);

					if (!Unique[UID].contains(PB))
					{
						Unique[UID].insert(PB);
					}
					else Unique[UID].remove(PB);
				}
			}
		}

		if (Endings == 2) for (const auto& Set : Unique) Cuts += Set;
	}

	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0); Step = 0;

	Query.prepare(
		"SELECT "
			"O.UID, E.IDE, E.N, "
			"ROUND(P.P0_X, 3), ROUND(P.P0_Y, 3), "
			"ROUND(IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), 3), "
			"ROUND(IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y), 3) "
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

	emit onBeginProgress(tr("Generating tasklist"));
	emit onSetupProgress(0, 0); Step = 0;

	QtConcurrent::blockingMap(Cuts, [this, &Parts, &Queue, &Locker] (const QPair<double, double>& Pair) -> void
	{
		if (this->isTerminated()) return; const QPointF Point(Pair.first, Pair.second);

		for (auto i = Parts.constBegin(); i != Parts.constEnd(); ++i) for (int j = 1; j < i.value().Lines.size(); ++j)
		{
			const auto& L1 = i.value().Lines[j - 1];
			const auto& L2 = i.value().Lines[j];

			const QPointF A(L1.X1, L1.Y1); const QPointF B(L1.X2, L1.Y2);
			const QPointF C(L2.X1, L2.Y1); const QPointF D(L2.X2, L2.Y2);

			if ((Point == A && Point == C) || (Point == B && Point == C) ||
			    (Point == A && Point == D) || (Point == B && Point == D))
			{
				Locker.lock();

				if (Queue.contains(i.key())) Queue[i.key()].append(L1.N);
				else Queue.insert(i.key(), QList<int>() << L1.N);

				Locker.unlock();
			}
		}
	});

	QtConcurrent::blockingMap(Queue, [] (QList<int>& List) -> void { std::stable_sort(List.begin(), List.end()); });

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0); Step = 0;

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
			"T.TYP = 6 AND "
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

	emit onBeginProgress(tr("Inserting objects"));
	emit onSetupProgress(0, Queue.size()); Step = 0;

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
			const QList<int>& Jobs = i.value(); int on = Jobs.first();

			Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1 AND N > %2").arg(i.key()).arg(on));

			for (int j = 1; j <= Jobs.size(); ++j)
			{
				int Index(0), n(-1), From(Jobs[j - 1]), To(j == Jobs.size() ? INT_MAX : Jobs[j]);

				Query.prepare("SELECT GEN_ID(EW_OBIEKTY_UID_GEN, 1) FROM RDB$DATABASE");

				if (Query.exec() && Query.next()) Index = Query.value(0).toInt();
				else
				{
					Index = qHash(QDateTime::currentMSecsSinceEpoch()); thread()->usleep(1500);
				}

				const QString Numer = QString("%1%2").arg(qHash(QDateTime::currentMSecsSinceEpoch()), 0, 16).arg(qHash(Index), 0, 16);

				Query.exec(objectInsert.arg(Index).arg(Numer).arg(i.key()));
				Query.exec(dataInsert.arg(Index).arg(i.key()));

				for (const auto& Line : Parts[i.key()].Lines) if (Line.N > From && Line.N <= To)
				{
					Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
								    "VALUES (%1, %2, 0, %3)")
							 .arg(Index).arg(Line.ID).arg(++n));
				}

				for (const auto& Line : Parts[i.key()].Lines) if (Line.N > From && Line.N <= To)
					for (const auto& T : Parts[i.key()].Labels) if (T.LID == Line.ID)
					{
						Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
									    "VALUES (%1, %2, 0, %3)")
								 .arg(Index).arg(T.ID).arg(++n));
					}

				for (const auto& Line : Parts[i.key()].Lines) if (Line.N > From && Line.N <= To)
					for (const auto& T : Parts[i.key()].Objects) if (T.LID == Line.ID)
					{
						Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
									    "VALUES (%1, %2, 1, %3)")
								 .arg(Index).arg(T.ID).arg(++n));
					}
			}

			for (const auto& Line : Parts[i.key()].Lines) if (Line.N <= Jobs.first())
				for (const auto& T : Parts[i.key()].Labels) if (T.LID == Line.ID)
				{
					Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
								    "VALUES (%1, %2, 0, %3)")
							 .arg(i.key()).arg(T.ID).arg(++on));
				}

			for (const auto& Line : Parts[i.key()].Lines) if (Line.N <= Jobs.first())
				for (const auto& T : Parts[i.key()].Objects) if (T.LID == Line.ID)
				{
					Query.exec(QString("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) "
								    "VALUES (%1, %2, 1, %3)")
							 .arg(i.key()).arg(T.ID).arg(++on));
				}

			emit onUpdateProgress(++Step);
		}
	}

	emit onEndProgress();
	emit onDataCut(Queue.size());
}

void DatabaseDriver::refactorData(const QSet<int>& Items, const QString& Class, int Line, int Point, int Text, const QString& Symbol, int Style, const QString& Label, int Actions, double Radius)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRefactor(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, false, 1); QSet<int> Change;
	const auto& Table = getItemByField(Tables, Class, &TABLE::Name); QSet<int> Lines, Symbols, Texts;
	QSqlQuery Query(Database); Query.setForwardOnly(true); int LineStyle(0); QString NewSymbol; int Step = 0;
	QSqlQuery LineQuery(Database), SymbolQuery(Database), LabelQuery(Database),
			ClassQuery(Database), selectLines(Database), selectTexts(Database);

	const int Type = Class != "NULL" ? getItemByField(Tables, Class, &TABLE::Name).Type : 0;

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

	emit onBeginProgress(tr("Converting geometry"));
	emit onSetupProgress(0, 0);

	if (Actions & 0b0001 && Type & 256 && !vPoint.isNull())
	{
		convertSurfaceToPoint(Items, NewSymbol, vPoint.toInt());
	}

	if (Actions & 0b0010 && Type & 8)
	{
		convertSurfaceToLine(Items);
	}

	if (Actions & 0b0100 && Type & 2)
	{
		convertLineToSurface(Items);
	}

	if (Actions & 0b1000 && Type & 2 && !vLine.isNull() && !vStyle.isNull())
	{
		convertPointToSurface(Items, vStyle.toInt(), vLine.toInt(), Radius > 0.0 ? Radius : 0.8);
	}

	emit onBeginProgress(tr("Loading elements"));
	emit onSetupProgress(0, Items.size());

	if (Type == 0) Change = Items;
	else if (Query.exec("SELECT UID, RODZAJ FROM EW_OBIEKTY WHERE STATUS = 0")) while (Query.next())
	{
		const int UID = Query.value(0).toInt();

		if (!Items.contains(UID)) continue;

		bool OK(false); switch (Query.value(1).toInt())
		{
			case 2:
				OK = (Type & 8);
			break;
			case 3:
				OK = (Type & 2);
			break;
			case 4:
				OK = (Type & 256);
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

		if (Tab.Name == Class) continue; QStringList Fieldlst;

		for (const auto& Field : Table.Fields)
		{
			if (hasItemByField(Tab.Fields, Field.Name, &FIELD::Name))
			{
				const auto& B = getItemByField(Tab.Fields, Field.Name, &FIELD::Name);

				if (Field.Type == B.Type && Field.Name == B.Name && Field.Dict == B.Dict)
				{
					Fieldlst.append(Field.Name);
				}
			}
		}

		for (const auto& Item : i.value()) if (Change.contains(Item))
		{
			Query.exec(QString("INSERT INTO %1 (UIDO, %2) "
						    "SELECT UIDO, %2 FROM %3 "
						    "WHERE UIDO = '%4'")
					 .arg(Table.Data)
					 .arg(Fieldlst.replaceInStrings("EW_DATA.", "").join(", "))
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

	if (Dateupdate) { ClassQuery.finish(); updateModDate(Tasks.first(), 0); }

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

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	if (!vClass.isNull() || Dateupdate) for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Tab = getItemByField(Tables, Class, &TABLE::Name);
		const auto Data = loadData(Tab, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataRefactor(Change.size());
}

void DatabaseDriver::copyData(const QSet<int>& Items, const QString& Class, int Line, int Point, int Text, const QString& Symbol, int Style)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataCopy(0); return; }

	struct ELEMENT { int IDE, Type; }; QHash<int, QList<ELEMENT>> Elements;

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, false, 1);
	const auto& Table = getItemByField(Tables, Class, &TABLE::Name); int Step = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true); int LineStyle(0); QString NewSymbol;
	QSqlQuery LineQuery(Database), SymbolQuery(Database), LabelQuery(Database),
			ClassQuery(Database), ElementsQuery(Database), GeometryQuery(Database);

	QSqlQuery getUidQuery(Database), getIdQuery(Database);

	const int Type = getItemByField(Tables, Class, &TABLE::Name).Type; QSet<int> Change;

	if (Query.exec("SELECT UID, RODZAJ FROM EW_OBIEKTY WHERE STATUS = 0")) while (Query.next())
	{
		const int UID = Query.value(0).toInt();

		if (!Items.contains(UID)) continue;

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
		QStringList Fieldlst;

		for (const auto& Field : Table.Fields)
		{
			if (hasItemByField(Tab.Fields, Field.Name, &FIELD::Name))
			{
				const auto& B = getItemByField(Tab.Fields, Field.Name, &FIELD::Name);

				if (Field == B) Fieldlst.append(Field.Name);
			}
		}

		for (const auto& Item : i.value()) if (Change.contains(Item))
		{
			int UIDO(0); if (getUidQuery.exec() && getUidQuery.next())
			{
				UIDO = getUidQuery.value(0).toInt();
			}
			else continue;

			const QString Numer = QString("OB_ID_%1%2").arg(qHash(QDateTime::currentMSecsSinceEpoch()), 0, 16).arg(qHash(UIDO), 0, 16);

			Query.exec(QString("INSERT INTO %1 (UIDO, %2) "
						    "SELECT '%3', %2 FROM %4 "
						    "WHERE UIDO = '%5'")
					 .arg(Table.Data)
					 .arg(Fieldlst.replaceInStrings("EW_DATA.", "").join(", "))
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

void DatabaseDriver::fitData(const QSet<int>& Items, const QString& Path, int Jobtype, int X1, int Y1, int X2, int Y2, double Radius, double Length, bool Endings)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataFit(0); return; }

	struct DATA { QList<int> Indexes; QList<QLineF> Lines; QList<QPointF> Points; };
	struct LINE { int ID; QLineF Line; QPointF Point; bool Changed = false; };
	struct POINT { int ID; QPointF Point; bool Changed = false; };

	QHash<int, DATA> Objects; QList<LINE> lUpdates; QList<POINT> pUpdates;
	QSqlQuery LoadLines(Database), LoadPoints(Database),
			UpdateLines(Database), UpdatePoints(Database);

	QMutex Synchronizer; int Step = 0; if (Jobtype == 0) X2 = Y2 = 0;

	QSettings Settings("EW-Database");

	Settings.beginGroup("Locale");
	const auto csvSep = Settings.value("csv", ",").toString();
	const auto txtSep = Settings.value("txt", "\\s+").toString();
	Settings.endGroup();

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
			"O.RODZAJ IN (2, 3) AND "
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

	const QRegExp Exp(CSV ? QString("\\s*%1\\s*").arg(csvSep) : txtSep);

	while (!Stream.atEnd() && !isTerminated())
	{
		const QStringList Lists = Stream.readLine().trimmed().split(Exp, Qt::SkipEmptyParts);

		if (Max < Lists.size())
		{
			bool OK(true), Current;

			const double pX1 = Lists[X1].toDouble(&Current); OK = OK && Current;
			const double pY1 = Lists[Y1].toDouble(&Current); OK = OK && Current;
			const double pX2 = Lists[X2].toDouble(&Current); OK = OK && Current;
			const double pY2 = Lists[Y2].toDouble(&Current); OK = OK && Current;

			if (OK)
			{
				if (Jobtype == 0) Sources.append({ pX1, pY1 });
				else Lines.append({ pX1, pY1, pX2, pY2 });
			}
		}
		else continue;
	}

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, Items.size());

	if (LoadLines.exec()) while (LoadLines.next() && !isTerminated())
	{
		const int UID = LoadLines.value(0).toInt();

		if (Items.contains(UID))
		{
			if (!Objects.contains(UID)) Objects.insert(UID, DATA());

			DATA& Object = Objects[UID];

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

		if (Items.contains(UID))
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
	emit onSetupProgress(0, Objects.size()); Step = 0;

	if (Jobtype == 2) QtConcurrent::blockingMap(Objects, [this, &lUpdates, &Synchronizer, &Step] (DATA& Object) -> void
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

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
	});

	emit onSetupProgress(0, Objects.size()); Step = 0;

	if (Jobtype == 0) QtConcurrent::blockingMap(Objects, [this, &Sources, &lUpdates, &Synchronizer, &Step, Radius, Endings] (DATA& Object) -> void
	{
		if (this->isTerminated()) return;

		for (int i = 0; i < Object.Indexes.size(); ++i)
		{
			QPointF Final1, Final2; double h1 = NAN, h2 = NAN;
			const QLineF& L = Object.Lines[i]; QLineF Current = L;

			const bool isEnd = Object.Points.contains(L.p1()) ||
						    Object.Points.contains(L.p2());

			if (Endings && !isEnd) continue;

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

			if (!qIsNaN(h1) && !qIsNaN(h2) && Final1 == Final2)
			{
				if (h1 < h2) Final2 = Current.p2();
				else Final1 = Current.p1();
			}

			if (!qIsNaN(h1) && Final1 != Current.p1())
			{
				Current.setP1(Final1);
			}

			if (!qIsNaN(h2) && Final2 != Current.p2())
			{
				Current.setP2(Final2);
			}

			if (Current != L && Current.p1() != Current.p2())
			{
				Synchronizer.lock();
				lUpdates.append({ Object.Indexes[i], Current, QPointF(), true });
				Synchronizer.unlock();
			}
		}

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
	});

	emit onSetupProgress(0, Objects.size()); Step = 0;

	if (Jobtype == 1) QtConcurrent::blockingMap(Objects, [this, &Lines, &lUpdates, &Synchronizer, &Step, Radius, Endings] (DATA& Object) -> void
	{
		static const auto pdistance = [Radius] (const QLineF& L, const QPointF& P) -> bool
		{
			const double a = QLineF(P.x(), P.y(), L.x1(), L.y1()).length();
			const double b = QLineF(P.x(), P.y(), L.x2(), L.y2()).length();
			const double l = L.length();

			if ((a * a <= l * l + b * b) &&
			    (b * b <= a * a + l * l))
			{
				const double A = P.x() - L.x1(); const double B = P.y() - L.y1();
				const double C = L.x2() - L.x1(); const double D = L.y2() - L.y1();

				return (qAbs(A * D - C * B) / qSqrt(C * C + D * D)) <= Radius;
			}
			else return false;
		};

		if (this->isTerminated()) return;

		for (int i = 0; i < Object.Indexes.size(); ++i)
		{
			QPointF FinalA, FinalB; double hA = NAN, hB = NAN;
			const QLineF& L = Object.Lines[i]; QLineF Current = L;

			const bool isEnd = Object.Points.contains(L.p1()) ||
						    Object.Points.contains(L.p2());

			if (Endings && !isEnd) continue;

			for (const auto& P : Lines)
			{
				QLineF DummyA(L.p1(), { 0, 0 });
				DummyA.setAngle(P.angle() + 90);

				QLineF DummyB(L.p2(), { 0, 0 });
				DummyB.setAngle(P.angle() + 90);

				QPointF IntersectA, IntersectB;
				DummyA.intersects(P, &IntersectA);
				DummyB.intersects(P, &IntersectB);

				const double Rad1A = QLineF(L.p1(), P.p1()).length();

				if (Rad1A <= Radius && (qIsNaN(hA) || Rad1A < hA))
				{
					FinalA = P.p1(); hA = Rad1A;
				}

				const double Rad2A = QLineF(L.p1(), P.p2()).length();

				if (Rad2A <= Radius && (qIsNaN(hA) || Rad2A < hA))
				{
					FinalA = P.p2(); hA = Rad2A;
				}

				const double Rad3A = QLineF(L.p1(), IntersectA).length();

				if (Rad3A <= Radius && (qIsNaN(hA) || 2*Rad3A < hA) && pdistance(P, L.p1()))
				{
					FinalA = IntersectA; hA = 2*Rad3A;
				}

				const double Rad1B = QLineF(L.p2(), P.p1()).length();

				if (Rad1B <= Radius && (qIsNaN(hB) || Rad1B < hB))
				{
					FinalB = P.p1(); hB = Rad1B;
				}

				const double Rad2B = QLineF(L.p2(), P.p2()).length();

				if (Rad2B <= Radius && (qIsNaN(hB) || Rad2B < hB))
				{
					FinalB = P.p2(); hB = Rad2B;
				}

				const double Rad3B = QLineF(L.p2(), IntersectB).length();

				if (Rad3B <= Radius && (qIsNaN(hB) || 2*Rad3B < hB) && pdistance(P, L.p2()))
				{
					FinalB = IntersectB; hB = 2*Rad3B;
				}
			}

			if (!qIsNaN(hA) && FinalA != Current.p1())
			{
				Current.setP1(FinalA);
			}

			if (!qIsNaN(hB) && FinalB != Current.p2())
			{
				Current.setP2(FinalB);
			}

			if (Current != L && Current.p1() != Current.p2())
			{
				Synchronizer.lock();
				lUpdates.append({ Object.Indexes[i], Current, QPointF(), true });
				Synchronizer.unlock();
			}
		}

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
	});

	emit onSetupProgress(0, lUpdates.size()); Step = 0;

	if (Jobtype == 2) QtConcurrent::blockingMap(lUpdates, [this, &Lines, &Sources, &Synchronizer, &Step, Radius, Length] (LINE& Part) -> void
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
			Normal.intersects(L, &IntersectR);
			Part.Line.intersects(L, &IntersectL);

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

		if ((Part.Changed = (!qIsNaN(h) && Final != Part.Point)))
		{
			if (Part.Point == Part.Line.p1()) Part.Line.setP1(Final);
			if (Part.Point == Part.Line.p2()) Part.Line.setP2(Final);

			Synchronizer.lock();
			Sources.append(Final);
			Synchronizer.unlock();
		}

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
	});

	emit onSetupProgress(0, pUpdates.size()); Step = 0;

	if (Jobtype == 1) QtConcurrent::blockingMap(pUpdates, [this, &Lines, &Synchronizer, &Step, Radius] (POINT& Symbol) -> void
	{
		static const auto pdistance = [Radius] (const QLineF& L, const QPointF& P) -> bool
		{
			const double a = QLineF(P.x(), P.y(), L.x1(), L.y1()).length();
			const double b = QLineF(P.x(), P.y(), L.x2(), L.y2()).length();
			const double l = L.length();

			if ((a * a <= l * l + b * b) &&
			    (b * b <= a * a + l * l))
			{
				const double A = P.x() - L.x1(); const double B = P.y() - L.y1();
				const double C = L.x2() - L.x1(); const double D = L.y2() - L.y1();

				return (qAbs(A * D - C * B) / qSqrt(C * C + D * D)) <= Radius;
			}
			else return false;
		};

		if (this->isTerminated()) return;

		QPointF Final; double h = NAN;

		for (const auto& L : Lines)
		{
			QLineF Dummy(Symbol.Point, { 0, 0 });
			Dummy.setAngle(L.angle() + 90);

			QPointF Intersect;
			Dummy.intersects(L, &Intersect);

			const double Rad1 = QLineF(Symbol.Point, L.p1()).length();

			if (Rad1 <= Radius && (qIsNaN(h) || Rad1 < h))
			{
				Final = L.p1(); h = Rad1;
			}

			const double Rad2 = QLineF(Symbol.Point, L.p2()).length();

			if (Rad2 <= Radius && (qIsNaN(h) || Rad2 < h))
			{
				Final = L.p2(); h = Rad2;
			}

			const double Rad3 = QLineF(Symbol.Point, Intersect).length();

			if (Rad3 <= Radius && (qIsNaN(h) || 2*Rad3 < h) && pdistance(L, Symbol.Point))
			{
				Final = Intersect; h = 2*Rad3;
			}
		}

		if ((Symbol.Changed = (!qIsNaN(h) && Final != Symbol.Point)))
		{
			Symbol.Point = Final;
		}

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
	});

	emit onSetupProgress(0, pUpdates.size()); Step = 0;

	if (Jobtype == 0 || Jobtype == 2) QtConcurrent::blockingMap(pUpdates, [this, &Sources, &Synchronizer, &Step, Radius] (POINT& Symbol) -> void
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

		if ((Symbol.Changed = (!qIsNaN(h) && Final != Symbol.Point)))
		{
			Symbol.Point = Final;
		}

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
	});

	if (isTerminated()) { emit onEndProgress(); emit onDataFit(0); return; }

	emit onBeginProgress(tr("Updating geometry")); int Rejected(0);
	emit onSetupProgress(0, lUpdates.size() + pUpdates.size()); Step = 0;

	for (const auto& L : lUpdates)
	{
		if (!L.Changed) ++Rejected;
		else
		{
			UpdateLines.addBindValue(L.Line.x1());
			UpdateLines.addBindValue(L.Line.y1());
			UpdateLines.addBindValue(L.Line.x2());
			UpdateLines.addBindValue(L.Line.y2());
			UpdateLines.addBindValue(L.ID);

			UpdateLines.exec();
		}

		emit onUpdateProgress(++Step);
	}

	for (const auto& P : pUpdates)
	{
		if (!P.Changed) ++Rejected;
		else
		{
			UpdatePoints.addBindValue(P.Point.x());
			UpdatePoints.addBindValue(P.Point.y());
			UpdatePoints.addBindValue(P.ID);

			UpdatePoints.exec();
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataFit(lUpdates.size() + pUpdates.size() - Rejected);
}

void DatabaseDriver::unifyData(const QSet<int>& Items, double Radius)
{
	if (!Database.open()) { emit onDataUnify(0); return; }

	QList<QPointF> Points; int Step(0);

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	Terminator.lock(); Terminated = false; Terminator.unlock();

	QSqlQuery Query(Database); QList<QPointF> Base; QMutex Locker;
	QList<QPair<QPointF*, QSet<QPointF*>>> Pools; QSet<QPointF*> Rem;

	Query.prepare(
				"SELECT "
				"T.POS_X, T.POS_Y, O.UID "
				"FROM "
				"EW_TEXT T "
				"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = T.ID "
				"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
				"WHERE "
				"T.STAN_ZMIANY = 0 AND "
				"T.TYP = 4 AND "
				"O.STATUS = 0 "

				"UNION "
				"SELECT "
				"F.P0_X, F.P0_Y, O.UID "
				"FROM "
				"EW_POLYLINE F "
				"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = F.ID "
				"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
				"WHERE "
				"F.STAN_ZMIANY = 0 AND "
				"F.P1_FLAGS <> 4 AND "
				"O.STATUS = 0 "

				"UNION "
				"SELECT "
				"L.P1_X, L.P1_Y, O.UID "
				"FROM "
				"EW_POLYLINE L "
				"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = L.ID "
				"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
				"WHERE "
				"L.STAN_ZMIANY = 0 AND "
				"L.P1_FLAGS = 0 AND "
				"O.STATUS = 0 "

				"UNION "
				"SELECT "
				"A.PN_X, A.PN_Y, O.UID "
				"FROM "
				"EW_POLYLINE A "
				"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = A.ID "
				"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
				"WHERE "
				"A.STAN_ZMIANY = 0 AND "
				"A.P1_FLAGS = 2 AND "
				"O.STATUS = 0 "

				"UNION "
				"SELECT "
				"(C.P0_X + C.P1_X) / 2.0, "
				"(C.P0_Y + C.P1_Y) / 2.0, "
				"O.UID "
				"FROM "
				"EW_POLYLINE C "
				"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = C.ID "
				"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
				"WHERE "
				"C.STAN_ZMIANY = 0 AND "
				"C.P1_FLAGS = 4 AND "
				"O.STATUS = 0 "

				);

	if (Query.exec()) while (Query.next())
		if (Items.contains(Query.value(2).toInt()))
			Base.append(
			{
				Query.value(0).toDouble(),
				Query.value(1).toDouble()
			});

	emit onSetupProgress(0, Base.size() * 2);

	QtConcurrent::blockingMap(Base, [this, &Pools, &Base, &Locker, &Step, Radius] (QPointF& I) -> void
	{
		QSet<QPointF*> List; for (auto& P : Base) if (&P != &I)
		{
			const double l = QLineF(I, P).length();

			if (l <= Radius) List.insert(&P);
		}

		QMutexLocker Synchronizer(&Locker); emit onUpdateProgress(++Step);

		for (int i = 0; i < Pools.size(); ++i)
		{
			if (List.size() >= Pools[i].second.size())
			{
				Pools.insert(i, qMakePair(&I, List)); return;
			}
		}

		Pools.append(qMakePair(&I, List));
	});

	for (const auto& P : qAsConst(Pools)) if (!Rem.contains(P.first))
	{
		double X(0.0), Y(0.0); unsigned Count(0); Rem.insert(P.first);

		for (const auto& O : qAsConst(P.second)) if (!Rem.contains(O))
		{
			X += O->x(); Y += O->y(); ++Count; Rem.insert(O);
		}

		if (Count)
		{
			X += P.first->x(); Y += P.first->y(); ++Count;

			Points.append({ X / Count, Y / Count });
		}

		emit onUpdateProgress(++Step);
	}

	emit onBeginProgress(tr("Fitting geometry"));
	emit onSetupProgress(0, 0);

	QList<QPair<int, QPointF>> Symbols;
	QList<QPair<int, QLineF>> Circles;
	QList<QPair<int, QLineF>> Lines;

	QHash<int, QVariant> Updates;

	Query.prepare(
		"SELECT "
			"P.ID, P.POS_X, P.POS_Y, O.UID "
		"FROM "
			"EW_TEXT P "
		"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = P.ID "
		"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
		"WHERE "
			"P.STAN_ZMIANY = 0 AND "
			"P.TYP = 4 AND "
			"O.STATUS = 0");

	if (Query.exec()) while (Query.next())
		if (Items.contains(Query.value(3).toInt()))
			Symbols.append(
			{
				Query.value(0).toInt(),
				{
					Query.value(1).toDouble(),
					Query.value(2).toDouble()
				}
			});

	Query.prepare(
		"SELECT "
			"P.ID, P.P0_X, P.P0_Y, P.P1_X, P.P1_Y, O.UID "
		"FROM "
			"EW_POLYLINE P "
		"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = P.ID "
		"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
		"WHERE "
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 4 AND "
			"O.STATUS = 0");

	if (Query.exec()) while (Query.next())
		if (Items.contains(Query.value(5).toInt()))
			Circles.append(
			{
				Query.value(0).toInt(),
				{
					{
						Query.value(1).toDouble(),
						Query.value(2).toDouble()
					},
					{
						Query.value(3).toDouble(),
						Query.value(4).toDouble()
					}
				}
			});

	Query.prepare(
		"SELECT "
			"P.ID, P.P0_X, P.P0_Y, "
			"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
			"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y),"
			"O.UID "
		"FROM "
			"EW_POLYLINE P "
		"INNER JOIN EW_OB_ELEMENTY E ON E.IDE = P.ID "
		"INNER JOIN EW_OBIEKTY O ON O.UID = E.UIDO "
		"WHERE "
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS IN (0, 2) AND "
			"O.STATUS = 0");

	if (Query.exec()) while (Query.next())
		if (Items.contains(Query.value(5).toInt()))
			Lines.append(
			{
				Query.value(0).toInt(),
				{
					{
						Query.value(1).toDouble(),
						Query.value(2).toDouble()
					},
					{
						Query.value(3).toDouble(),
						Query.value(4).toDouble()
					}
				}
			});

	emit onSetupProgress(0, Symbols.size() + Circles.size() + Lines.size());

	QtConcurrent::blockingMap(Symbols, [this, &Points, &Updates, &Locker, &Step, Radius] (QPair<int, QPointF>& S) -> void
	{
		double L = NAN; QPointF Found;

		for (const auto& P : Points) if (P != S.second)
		{
			const double l = QLineF(S.second, P).length();

			if (l <= Radius && (qIsNaN(L) || L > l))
			{
				L = l; Found = P;
			}
		}

		if (Found != QPointF())
		{
			Locker.lock();
			Updates.insert(S.first, Found);
			Locker.unlock();
		}

		Locker.lock();
		emit onUpdateProgress(++Step);
		Locker.unlock();
	});

	QtConcurrent::blockingMap(Circles, [this, &Points, &Updates, &Locker, &Step, Radius] (QPair<int, QLineF>& S) -> void
	{
		double L = NAN; QPointF Found; QPointF C =
		{
			(S.second.x1() + S.second.x2()) / 2.0,
			(S.second.y1() + S.second.y2()) / 2.0
		};

		const double R = qAbs(S.second.x1() - S.second.x2()) / 2.0;

		for (const auto& P : Points) if (P != C)
		{
			const double l = QLineF(C, P).length();

			if (l <= Radius && (qIsNaN(L) || L > l))
			{
				L = l; Found = P;
			}
		}

		if (Found != QPointF())
		{
			QLineF Updated(Found.x() - R, Found.y(),
						Found.x() + R, Found.y());

			Locker.lock();
			Updates.insert(S.first, Updated);
			Locker.unlock();
		}

		Locker.lock();
		emit onUpdateProgress(++Step);
		Locker.unlock();
	});

	QtConcurrent::blockingMap(Lines, [this, &Points, &Updates, &Locker, &Step, Radius] (QPair<int, QLineF>& S) -> void
	{
		double L1 = NAN, L2 = NAN; QPointF Found1, Found2;

		for (const auto& P : Points) if (P != S.second.p1() || P != S.second.p2())
		{
			const double l1 = QLineF(S.second.p1(), P).length();
			const double l2 = QLineF(S.second.p2(), P).length();

			if (l1 <= Radius && (qIsNaN(L1) || L1 > l1))
			{
				L1 = l1; Found1 = P;
			}

			if (l2 <= Radius && (qIsNaN(L2) || L2 > l2))
			{
				L2 = l2; Found2 = P;
			}
		}

		if (Found1 == Found2) return;
		else if (Found1 != QPointF() || Found2 != QPointF())
		{
			QLineF Updated = S.second;

			if (Found1 != QPointF()) Updated.setP1(Found1);
			if (Found2 != QPointF()) Updated.setP2(Found2);

			Locker.lock();
			Updates.insert(S.first, Updated);
			Locker.unlock();
		}

		Locker.lock();
		emit onUpdateProgress(++Step);
		Locker.unlock();
	});

	emit onBeginProgress(tr("Updating database"));
	emit onSetupProgress(0, Updates.size());

	QSqlQuery lineQuery(Database), roundQuery(Database), pointQuery(Database);

	lineQuery.prepare("UPDATE EW_POLYLINE SET P0_X = ?, P0_Y = ?, P1_X = ?, P1_Y = ? WHERE ID = ? AND P1_FLAGS IN (0, 4)");
	roundQuery.prepare("UPDATE EW_POLYLINE SET P0_X = ?, P0_Y = ?, PN_X = ?, PN_Y = ? WHERE ID = ? AND P1_FLAGS IN (2)");
	pointQuery.prepare("UPDATE EW_TEXT SET POS_X = ?, POS_Y = ? WHERE ID = ?");

	for (auto i = Updates.constBegin(); i != Updates.constEnd(); ++i) if (!isTerminated())
	{
		if (i.value().type() == QVariant::PointF)
		{
			const auto P = i.value().toPointF();

			pointQuery.addBindValue(P.x());
			pointQuery.addBindValue(P.y());
			pointQuery.addBindValue(i.key());

			pointQuery.exec();
		}
		else if (i.value().type() == QVariant::LineF)
		{
			const auto L = i.value().toLineF();

			lineQuery.addBindValue(L.x1());
			lineQuery.addBindValue(L.y1());
			lineQuery.addBindValue(L.x2());
			lineQuery.addBindValue(L.y2());
			lineQuery.addBindValue(i.key());

			lineQuery.exec();

			roundQuery.addBindValue(L.x1());
			roundQuery.addBindValue(L.y1());
			roundQuery.addBindValue(L.x2());
			roundQuery.addBindValue(L.y2());
			roundQuery.addBindValue(i.key());

			roundQuery.exec();
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onDataUnify(Updates.size());
}

void DatabaseDriver::restoreJob(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onJobsRestore(0); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, true, 0);
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

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Tasks.size()); Step = 0;

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onJobsRestore(Count);
}

void DatabaseDriver::removeHistory(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onHistoryRemove(0); return; }

	QSqlQuery QueryA(Database), QueryB(Database), QueryC(Database); int Step = 0; int Count = 0;

	emit onBeginProgress(tr("Removing history"));
	emit onSetupProgress(0, Items.size());

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

	for (const auto& ID : Items)
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

void DatabaseDriver::editText(const QSet<int>& Items, bool Move, int Justify, bool Rotate, bool Sort, double Length, bool Ignrel)
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

	struct MATCH
	{
		const LINE* Line;
		double Distance;
	};

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMap<int, POINT> Points, Texts, Objects; int Step = 0;
	QList<LINE> Lines, Surfaces; QList<const POINT*> Union;
	QHash<int, QSet<int>> Subobjects; QSet<int> SetA, SetB;

	emit onBeginProgress(tr("Loading subobjects"));
	emit onSetupProgress(0, 0);

	Query.prepare(
		"SELECT "
			"S.UID, D.UID "
		"FROM "
			"EW_OB_ELEMENTY E "
		"INNER JOIN "
			"EW_OBIEKTY S "
		"ON "
			"S.UID = E.UIDO "
		"INNER JOIN "
			"EW_OBIEKTY D "
		"ON "
			"D.ID = E.IDE "
		"WHERE "
			"E.TYP = 1 AND "
			"S.STATUS = 0 AND "
			"D.STATUS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();
		const int SID = Query.value(1).toInt();

		if (!Subobjects.contains(UID)) Subobjects.insert(UID, QSet<int>());

		Subobjects[UID].insert(SID);
	}

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
			"O.STATUS = 0 AND "
			"O.RODZAJ = 4 AND "
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

				Ref.UIDO = Query.value(0).toInt();
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

	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0); Step = 0;

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
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		Lines.append(
		{
			Query.value(0).toInt(),
			Query.value(1).toInt(),
			Query.value(2).toDouble(),
			Query.value(3).toDouble(),
			Query.value(4).toDouble(),
			Query.value(5).toDouble()
		});
	}

	emit onBeginProgress(tr("Loading surfaces"));
	emit onSetupProgress(0, 0); Step = 0;

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
			"O.RODZAJ = 3 AND "
			"E.TYP = 0 AND "
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 0");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		Surfaces.append(
		{
			Query.value(0).toInt(),
			Query.value(1).toInt(),
			Query.value(2).toDouble(),
			Query.value(3).toDouble(),
			Query.value(4).toDouble(),
			Query.value(5).toDouble()
		});
	}

	emit onBeginProgress(tr("Loading texts"));
	emit onSetupProgress(0, 0); Step = 0;

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
			"O.RODZAJ IN (2, 3) AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 6 "
		"ORDER BY "
			"O.UID ASC");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();
		const int IDE = Query.value(1).toInt();

		if (Items.contains(UID)) Texts.insert(IDE,
		{
			Query.value(0).toInt(),
			Query.value(1).toInt(),
			0.0, 0.0,
			Query.value(2).toDouble(),
			Query.value(3).toDouble(),
			0.0,
			Query.value(4).toUInt()
		});
	}

	for (auto i = Objects.constBegin(); i != Objects.constEnd(); ++i)
	{
		if (Items.contains(i.key())) Points.insert(i.key(), i.value());
	}

	emit onBeginProgress(tr("Loading circles"));
	emit onSetupProgress(0, 0);  Step = 0;

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

	emit onBeginProgress(tr("Performing edit"));
	emit onSetupProgress(0, 0); Step = 0;

	QtConcurrent::blockingMap(Lines, [] (LINE& Line) -> void
	{
		const double dx = Line.X1 - Line.X2;
		const double dy = Line.Y1 - Line.Y2;

		Line.Length = qSqrt(dx * dx + dy * dy);
	});

	if (Sort) std::stable_sort(Lines.begin(), Lines.end(),
	[] (const LINE& A, const LINE& B) -> bool
	{
		return A.Length > B.Length;
	});

	const auto distance = [] (const QLineF& L, const QPointF& P) -> double
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

	const auto length = [] (double x1, double y1, double x2, double y2)
	{
		const double dx = x1 - x2;
		const double dy = y1 - y2;

		return qSqrt(dx * dx + dy * dy);
	};

	const auto cmp = [] (double a, double b) { return diffComp(a, b, 0.01); };

	QtConcurrent::blockingMap(Points, [this, &Lines, &Subobjects, Move, Justify, Rotate, Length, Ignrel, distance, cmp] (POINT& Point) -> void
	{
		if (this->isTerminated()) return;

		bool Found = false; const LINE* Match = nullptr;

		const QVector<unsigned> jL = { 7, 4, 1};
		const QVector<unsigned> jM = { 8, 5, 2};
		const QVector<unsigned> jR = { 9, 6, 3};

		if (Move)
		{
			Point.DX = Point.WX; Point.DY = Point.WY;
			Point.J &= 0b1011111; Point.Changed = true;
		}

		if (Justify && !Rotate)
		{
			const unsigned mask = 0b1111;
			unsigned Just = Point.J & mask;

			if (jL.contains(Just)) Just = 4;
			else if (jM.contains(Just)) Just = 5;
			else if (jR.contains(Just)) Just = 6;

			if (Justify == 1) Just += 3;
			if (Justify == 3) Just -= 3;

			Point.J = (Point.J & ~mask) | (Just & mask);
			Point.Changed = true;
		}

		if (Rotate && !Found) for (const auto& Line : qAsConst(Lines))
			if (!Found && (Ignrel || Subobjects.value(Line.UIDO).contains(Point.UIDO)))
				if ((cmp(Point.WX, Line.X1) && cmp(Point.WY, Line.Y1)) ||
				    (cmp(Point.WX, Line.X2) && cmp(Point.WY, Line.Y2)))
				{
					Match = &Line; Found = true;
				}

		if (Rotate && !Found) for (const auto& Line : qAsConst(Lines))
			if (!Found && (Ignrel || Subobjects.value(Line.UIDO).contains(Point.UIDO)))
				if (distance(QLineF(Line.X1, Line.Y1, Line.X2, Line.Y2),
						   QPointF(Point.WX, Point.WY)) <= 0.05)
				{
					Match = &Line; Found = true;
				}

		if (Found && Match && Rotate && (Match->Length >= Length))
		{
			const unsigned mask = 0b1111;
			unsigned Just = Point.J & mask;
			unsigned oldJust = (12 - Just) / 3;

			Point.A = qAtan((Match->Y1 - Match->Y2) / (Match->X1 - Match->X2)) - M_PI / 2.0;

			const double lengthA = QLineF(Point.WX, Point.WY, Match->X1, Match->Y1).length();
			const double lengthB = QLineF(Point.WX, Point.WY, Match->X2, Match->Y2).length();

			if (Match->Y1 < Match->Y2)
			{
				if (lengthA < lengthB) Just = 4; else Just = 6;
			}
			else
			{
				if (lengthB < lengthA) Just = 4; else Just = 6;
			}

			if (Justify) oldJust = unsigned(Justify);

			if (oldJust == 1) Just += 3;
			if (oldJust == 3) Just -= 3;

			while (Point.A < -(M_PI / 2.0)) Point.A += M_PI;
			while (Point.A > (M_PI / 2.0)) Point.A -= M_PI;

			Point.J = (Point.J & ~mask) | (Just & mask);
			Point.Changed = true;
		}
	});

	QtConcurrent::blockingMap(Points, [this, &Surfaces, &Subobjects, Move, Justify, Rotate, Ignrel, distance, length, cmp] (POINT& Point) -> void
	{
		if (this->isTerminated()) return; QList<MATCH> Distances;

		if (Move)
		{
			Point.DX = Point.WX; Point.DY = Point.WY;
			Point.J &= 0b1011111; Point.Changed = true;
		}

		if (Justify)
		{
			const unsigned mask = 0b1111;
			unsigned Just = Point.J & mask;

			const QVector<unsigned> jL = { 7, 4, 1};
			const QVector<unsigned> jM = { 8, 5, 2};
			const QVector<unsigned> jR = { 9, 6, 3};

			if (jL.contains(Just)) Just = 4;
			else if (jM.contains(Just)) Just = 5;
			else if (jR.contains(Just)) Just = 6;

			if (Justify == 1) Just += 3;
			if (Justify == 3) Just -= 3;

			Point.J = (Point.J & ~mask) | (Just & mask);
			Point.Changed = true;
		}

		if (Rotate) for (const auto& L : Surfaces)
		{
			if (Ignrel || Subobjects.value(L.UIDO).contains(Point.UIDO))
			{
				const auto F = QLineF(L.X1, L.Y1, L.X2, L.Y2);
				const auto P = QPointF(Point.DX, Point.DY);

				const double nd = distance(F, P);
				const double d1 = length(Point.DX, Point.DY, L.X1, L.Y1);
				const double d2 = length(Point.DX, Point.DY, L.X2, L.Y2);

				const double dist = qMin(nd, qMin(d1, d2));

				Distances.append({ &L, dist});
			}
		}

		std::stable_sort(Distances.begin(), Distances.end(),
		[] (const MATCH& A, const MATCH& B) -> bool
		{
			return A.Distance < B.Distance;
		});

		bool Delete = true; while (Delete && Distances.length() >= 2)
		{
			const auto& a = Distances[0].Line;
			const auto& b = Distances[1].Line;

			Delete = (cmp(a->X1, b->X1) && cmp(a->Y1, b->Y1)) ||
				    (cmp(a->X1, b->X2) && cmp(a->Y1, b->Y2)) ||
				    (cmp(a->X2, b->X1) && cmp(a->Y2, b->Y1)) ||
				    (cmp(a->X2, b->X2) && cmp(a->Y2, b->Y2));

			if (Delete) Distances.removeAt(1);
		}

		if (Rotate && Distances.length() >= 2)
		{
			const auto m1 = Distances[0].Line;
			const auto m2 = Distances[1].Line;

			const auto pp1 = QPointF(m1->X1, m1->Y1);
			const auto pp2 = QPointF(m1->X2, m1->Y2);
			const auto pp3 = QPointF(m2->X1, m2->Y1);
			const auto pp4 = QPointF(m2->X2, m2->Y2);

			const double dd23 = QLineF(pp2, pp3).length();
			const double dd24 = QLineF(pp2, pp4).length();

			const QPolygonF Polly = dd23 < dd24 ?
				QVector<QPointF>({ pp1, pp2, pp3, pp4, pp1 }) :
				QVector<QPointF>({ pp1, pp2, pp4, pp3, pp1 });

			for (const auto& p : Polly) qDebug() << Qt::fixed << qSetRealNumberPrecision(2) << p.x() << p.y();

			if (!Polly.containsPoint({ Point.DX, Point.DY }, Qt::OddEvenFill)) return;

			const double w1 = 1.0 / Distances[0].Distance;
			const double w2 = 1.0 / Distances[1].Distance;

			double dtg1 = qAtan2(m1->Y1 - m1->Y2, m1->X1 - m1->X2) + (M_PI / 2.0);
			double dtg2 = qAtan2(m2->Y1 - m2->Y2, m2->X1 - m2->X2) + (M_PI / 2.0);

			while (dtg1 >= M_PI) dtg1 -= M_PI; while (dtg1 < 0.0) dtg1 += M_PI;

			if ((dtg1 > (M_PI / 2.0)) && (dtg1 < M_PI)) dtg1 += M_PI;
			if ((dtg1 > M_PI) && (dtg1 < (M_PI * 1.5))) dtg1 -= M_PI;

			while (dtg2 >= M_PI) dtg2 -= M_PI; while (dtg2 < 0.0) dtg2 += M_PI;

			if ((dtg2 > (M_PI / 2.0)) && (dtg2 < M_PI)) dtg2 += M_PI;
			if ((dtg2 > M_PI) && (dtg2 < (M_PI * 1.5))) dtg2 -= M_PI;

			double rot = (w1*dtg1 + w2*dtg2) / (w1 + w2);

			if (!qIsNaN(rot))
			{
				while (rot >= M_PI) rot -= M_PI; while (rot < 0.0) rot += M_PI;

				if ((rot > (M_PI / 2.0)) && (rot < M_PI)) rot += M_PI;
				if ((rot > M_PI) && (rot < (M_PI * 1.5))) rot -= M_PI;

				Point.A = rot;
				Point.Changed = true;
			}
		}
	});

	QtConcurrent::blockingMap(Texts, [this, &Lines, Move, Justify, Rotate, Ignrel, distance] (POINT& Point) -> void
	{
		if (this->isTerminated()) return;
		if (!(Move || Rotate)) return;

		const LINE* Match = nullptr; double Distance = NAN;

		for (const auto& L : Lines) if (Point.UIDO == L.UIDO)
		{
			const auto LN = QLineF(L.X1, L.Y1, L.X2, L.Y2);
			const auto PN = QPointF(Point.DX, Point.DY);

			const double h = distance(LN, PN);

			if (qIsNaN(Distance) || h < Distance)
			{
				Distance = h; Match = &L;
			}
		}

		if (!Match || (Ignrel && Distance > 0.1)) return;

		if (Move)
		{
			const QLineF Line(Match->X1, Match->Y1, Match->X2, Match->Y2);
			QLineF Offset(Point.DX, Point.DY, 0, 0);
			Offset.setAngle(Line.angle() + 90.0);

			QPointF Int; Offset.intersects(Line, &Int);

			Point.DX = Int.x();
			Point.DY = Int.y();
			Point.Changed = true;
		}

		if (Rotate)
		{
			double dtg = qAtan2(Match->Y1 - Match->Y2, Match->X1 - Match->X2) + (M_PI / 2.0);

			while (dtg >= M_PI) dtg -= M_PI; while (dtg < 0.0) dtg += M_PI;

			if ((dtg > (M_PI / 2.0)) && (dtg < M_PI)) dtg += M_PI;
			if ((dtg > M_PI) && (dtg < (M_PI * 1.5))) dtg -= M_PI;

			Point.A = dtg;
			Point.Changed = true;
		}

		if (Justify)
		{
			const unsigned mask = 0b1111;
			unsigned Just = 5;

			if (Justify == 1) Just += 3;
			if (Justify == 3) Just -= 3;

			Point.J = (Point.J & ~mask) | (Just & mask);
			Point.Changed = true;
		}
	});

	QtConcurrent::blockingMap(Texts, [this, &Surfaces, Move, Justify, Rotate, distance, cmp] (POINT& Point) -> void
	{
		if (this->isTerminated()) return; QList<MATCH> Distances;

		if (Justify)
		{
			const unsigned mask = 0b1111;
			unsigned Just = Point.J & mask;

			const QVector<unsigned> jL = { 7, 4, 1};
			const QVector<unsigned> jM = { 8, 5, 2};
			const QVector<unsigned> jR = { 9, 6, 3};

			if (jL.contains(Just)) Just = 4;
			else if (jM.contains(Just)) Just = 5;
			else if (jR.contains(Just)) Just = 6;

			if (Justify == 1) Just += 3;
			if (Justify == 3) Just -= 3;

			Point.J = (Point.J & ~mask) | (Just & mask);
			Point.Changed = true;
		}

		if (Move || Rotate) for (const auto& L : Surfaces)
		{
			if (Point.UIDO == L.UIDO)
			{
				const auto F = QLineF(L.X1, L.Y1, L.X2, L.Y2);
				const auto P = QPointF(Point.DX, Point.DY);

				Distances.append({ &L, distance(F, P)});
			}
		}

		std::stable_sort(Distances.begin(), Distances.end(),
		[] (const MATCH& A, const MATCH& B) -> bool
		{
			return A.Distance < B.Distance;
		});

		bool Delete = true; while (Delete && Distances.length() >= 2)
		{
			const auto& a = Distances[0].Line;
			const auto& b = Distances[1].Line;

			Delete = (cmp(a->X1, b->X1) && cmp(a->Y1, b->Y1)) ||
				    (cmp(a->X1, b->X2) && cmp(a->Y1, b->Y2)) ||
				    (cmp(a->X2, b->X1) && cmp(a->Y2, b->Y1)) ||
				    (cmp(a->X2, b->X2) && cmp(a->Y2, b->Y2));

			if (Delete) Distances.removeAt(1);
		}

		if ((Move || Rotate) && Distances.length() >= 2)
		{
			const auto m1 = Distances[0].Line;
			const auto m2 = Distances[1].Line;

			const auto pp1 = QPointF(m1->X1, m1->Y1);
			const auto pp2 = QPointF(m1->X2, m1->Y2);
			const auto pp3 = QPointF(m2->X1, m2->Y1);
			const auto pp4 = QPointF(m2->X2, m2->Y2);

			const double dd23 = QLineF(pp2, pp3).length();
			const double dd24 = QLineF(pp2, pp4).length();

			const QPolygonF Polly = dd23 < dd24 ?
				QVector<QPointF>({ pp1, pp2, pp3, pp4, pp1 }) :
				QVector<QPointF>({ pp1, pp2, pp4, pp3, pp1 });

			if (!Polly.containsPoint({ Point.DX, Point.DY }, Qt::OddEvenFill)) return;

			if (Move)
			{
				const QLineF Axis((m1->X1 + m2->X1) / 2.0, (m1->Y1 + m2->Y1) / 2.0,
							   (m1->X2 + m2->X2) / 2.0, (m1->Y2 + m2->Y2) / 2.0);

				QLineF Offset(Point.DX, Point.DY, 0, 0);
				Offset.setAngle(Axis.angle() + 90.0);

				QPointF Int; Offset.intersects(Axis, &Int);

				Point.DX = Int.x();
				Point.DY = Int.y();
				Point.Changed = true;
			}

			if (Rotate)
			{
				const double w1 = 1.0 / Distances[0].Distance;
				const double w2 = 1.0 / Distances[1].Distance;

				double dtg1 = qAtan2(m1->Y1 - m1->Y2, m1->X1 - m1->X2) + (M_PI / 2.0);
				double dtg2 = qAtan2(m2->Y1 - m2->Y2, m2->X1 - m2->X2) + (M_PI / 2.0);

				while (dtg1 >= M_PI) dtg1 -= M_PI; while (dtg1 < 0.0) dtg1 += M_PI;

				if ((dtg1 > (M_PI / 2.0)) && (dtg1 < M_PI)) dtg1 += M_PI;
				if ((dtg1 > M_PI) && (dtg1 < (M_PI * 1.5))) dtg1 -= M_PI;

				while (dtg2 >= M_PI) dtg2 -= M_PI; while (dtg2 < 0.0) dtg2 += M_PI;

				if ((dtg2 > (M_PI / 2.0)) && (dtg2 < M_PI)) dtg2 += M_PI;
				if ((dtg2 > M_PI) && (dtg2 < (M_PI * 1.5))) dtg2 -= M_PI;

				double rot = (w1*dtg1 + w2*dtg2) / (w1 + w2);

				if (!qIsNaN(rot))
				{
					while (rot >= M_PI) rot -= M_PI; while (rot < 0.0) rot += M_PI;

					if ((rot > (M_PI / 2.0)) && (rot < M_PI)) rot += M_PI;
					if ((rot > M_PI) && (rot < (M_PI * 1.5))) rot -= M_PI;

					Point.A = rot;
					Point.Changed = true;
				}
			}
		}
	});

	if (isTerminated()) { emit onEndProgress(); emit onTextEdit(0); return; }

	for (const auto& Point : qAsConst(Points)) if (Point.Changed) Union.append(&Point);
	for (const auto& Point : qAsConst(Texts)) if (Point.Changed) Union.append(&Point);

	emit onBeginProgress(tr("Saving changes"));
	emit onSetupProgress(0, Union.size()); Step = 0;

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

		if (Query.exec())
		{
			SetA.insert(Point->UIDO);
			SetB.insert(Point->IDE);
		}

		emit onUpdateProgress(++Step);
	}

	if (Dateupdate)
	{
		updateModDate(SetA, 0);
		updateModDate(SetB, 2);
	}

	emit onEndProgress();
	emit onTextEdit(Step);
}

void DatabaseDriver::insertLabel(const QSet<int>& Items, const QString& Label, int J, double X, double Y, bool P, double L, double R)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onLabelInsert(0); return; }

	struct LABEL { int ID; double X, Y, R; int Layer; };

	struct LINE { int ID; QList<QLineF> Lines; int Layer; };

	struct SURFACE { int ID; QPolygonF Surface; int Layer; };

	QSqlQuery Symbols(Database), Lines(Database), Surfaces(Database), Select(Database), Text(Database), Element(Database);
	QList<LINE> LineList; QList<SURFACE> SurfaceList; QList<LABEL> Insertions; QSet<int> Used, Set;
	QMutex Synchronizer; int Step = 0, Count = 0;

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
			"T.STAN_ZMIANY = 0 AND "
			"0 = ("
				"SELECT COUNT(L.ID) FROM EW_TEXT L "
				"INNER JOIN EW_OB_ELEMENTY Q "
				"ON (L.ID = Q.IDE AND Q.TYP = 0) "
				"WHERE Q.UIDO = O.UID AND L.TYP = 6 AND L.STAN_ZMIANY = 0 "
			")");

	Lines.prepare(
		"SELECT "
			"O.UID, ROUND(T.P0_X, 5), ROUND(T.P0_Y, 5), "
			"ROUND(IIF(T.PN_X IS NULL, T.P1_X, T.PN_X), 5), "
			"ROUND(IIF(T.PN_Y IS NULL, T.P1_Y, T.PN_Y), 5), "
			"("
				"SELECT FIRST 1 P.ID FROM EW_WARSTWA_TEXTOWA P "
				"INNER JOIN EW_GRUPY_WARSTW M ON P.ID_GRUPY = M.ID "
				"WHERE P.NAZWA = ("
					"SELECT H.NAZWA FROM EW_WARSTWA_LINIOWA H "
					"WHERE H.ID = T.ID_WARSTWY"
				") AND M.NAZWA LIKE '%#_E' ESCAPE '#'"
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
			"T.STAN_ZMIANY = 0 AND "
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
			"O.UID, T.P1_FLAGS, ROUND(T.P0_X, 5), ROUND(T.P0_Y, 5), "
			"ROUND(IIF(T.PN_X IS NULL, T.P1_X, T.PN_X), 5), "
			"ROUND(IIF(T.PN_Y IS NULL, T.P1_Y, T.PN_Y), 5), "
			"("
				"SELECT FIRST 1 P.ID FROM EW_WARSTWA_TEXTOWA P "
				"WHERE P.NAZWA = LEFT(("
					"SELECT H.NAZWA FROM EW_WARSTWA_LINIOWA H "
					"WHERE H.ID = T.ID_WARSTWY "
				"), 6) OR P.NAZWA = ("
				"SELECT H.NAZWA FROM EW_WARSTWA_LINIOWA H "
				"WHERE H.ID = T.ID_WARSTWY "
				") || '_E'"
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
			"T.STAN_ZMIANY = 0 AND "
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
	emit onSetupProgress(0, Items.size() - Used.size()); Step = 0;

	for (const auto& UID : Items) if (!Used.contains(UID))
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
	emit onSetupProgress(0, Items.size() - Used.size()); Step = 0;

	for (const auto& UID : Items) if (!Used.contains(UID))
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
	emit onSetupProgress(0, Items.size() - Used.size()); Step = 0;

	for (const auto& UID : Items) if (!Used.contains(UID))
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
				const double sX = (First.x() + Last.x()) / 2.0;
				const double sY = (First.y() + Last.y()) / 2.0;

				Insertions.append({ UID, sX, sY, 0.0, Surfaces.value(6).toInt() });
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

		double Length(0.0);
		for (const auto& Segment : qAsConst(Line.Lines)) Length += Segment.length();
		if (Length < L) return;

		double Step((R != 0.0) ? ((Length - (int(Length / R) * R)) / 2.0) : (Length / 2.0)), Now(0.0);

		for (const auto& Segment : qAsConst(Line.Lines)) if (const double Len = Segment.length())
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

	QtConcurrent::blockingMap(SurfaceList, [this, &Insertions, &Synchronizer, L, R] (SURFACE& Surface) -> void
	{
		if (this->isTerminated()) return;

		if (Surface.Surface.isEmpty()) return;

		QLineF Radius(INFINITY, INFINITY, -INFINITY, -INFINITY);

		for (const auto& P : qAsConst(Surface.Surface))
		{
			if (P.x() < Radius.x1()) Radius.setP1({ P.x(), Radius.y1() });
			if (P.x() > Radius.x2()) Radius.setP2({ P.x(), Radius.y2() });
			if (P.y() < Radius.y1()) Radius.setP1({ Radius.x1(), P.y() });
			if (P.y() > Radius.y2()) Radius.setP2({ Radius.x2(), P.y() });
		}

		const double Length = qMax(Radius.dx(), Radius.dy());

		if (R == 0.0 && Length >= L)
		{
			const double Mul = 1.0 / (Surface.Surface.size() - 1);
			double X(0.0), Y(0.0); bool Skip(true);

			for (const auto& P : qAsConst(Surface.Surface))
				if (Skip) Skip = false;
				else { X += Mul * P.x(); Y += Mul * P.y(); }

			Synchronizer.lock();
			Insertions.append({ Surface.ID, X, Y, 0.0, Surface.Layer });
			Synchronizer.unlock();
		}
		else if (R != 0.0 && Length >= L)
		{
			auto normalizeAngle = [] (double rad) -> double
			{
				while (rad > 2*M_PI) rad -= 2*M_PI;
				while (rad < 0) rad += 2*M_PI;

				if (rad > 1*M_PI_2 && rad <= 2*M_PI_2) rad += M_PI;
				if (rad < 3*M_PI_2 && rad >= 2*M_PI_2) rad -= M_PI;

				return rad;
			};

			auto getAngle = [] (const QList<QPair<QPointF, double>>& L, const QPointF& P) -> double
			{
				for (const auto& I : L) if (I.first == P) return I.second; return NAN;
			};

			double Start = (Length - (int(Length / R) * R)) / 2.0;

			if (Radius.dx() > Radius.dy())
			{
				double Stop(Radius.x2()); Start += Radius.x1();

				QLineF Step(Start, Radius.y1(), Start, Radius.y2());

				while (Start < Stop)
				{
					QList<QPointF> Intersections; QPointF Int;
					QList<QPair<QPointF, double>> Angles;

					for (int i = 1; i < Surface.Surface.size(); ++i)
					{
						const QLineF vLine(Surface.Surface[i - 1],
									    Surface.Surface[i]);

						auto Type = Step.intersects(vLine, &Int);

						if (Type == QLineF::BoundedIntersection)
						{
							const double ang = qAtan((vLine.y1() - vLine.y2()) /
												(vLine.x1() - vLine.x2()));

							Angles.append({ Int, normalizeAngle(ang - M_PI_2) });
							Intersections.append(Int);
						}
					}

					std::sort(Intersections.begin(), Intersections.end(),
					[] (const QPointF& A, const QPointF& B) -> bool
					{
						return A.y() < B.y();
					});

					while (Intersections.size() >= 2)
					{
						QLineF vLine(Intersections.takeFirst(),
								   Intersections.takeFirst());

						const double vLength = vLine.length();
						const double a1 = getAngle(Angles, vLine.p1());
						const double a2 = getAngle(Angles, vLine.p2());

						if (vLength >= R)
						{
							double vStart = (vLength - (int(vLength / R) * R)) / 2.0;
							vStart += qMin(vLine.y1(), vLine.y2());

							while (vStart < qMax(vLine.y1(), vLine.y2()))
							{
								const QPointF C(Start, vStart); vStart += R;

								const double w1 = 1/QLineF(C, vLine.p1()).length();
								const double w2 = 1/QLineF(C, vLine.p2()).length();

								const double ang = normalizeAngle((w1*a1 + w2*a2) / (w1 + w2));

								if (Surface.Surface.containsPoint(C, Qt::OddEvenFill))
								{
									Synchronizer.lock();
									Insertions.append({ Surface.ID, C.x(), C.y(), ang, Surface.Layer });
									Synchronizer.unlock();
								}
							}
						}
						else
						{
							const auto C = vLine.center();
							const double ang = normalizeAngle((a1 + a2) / 2);

							if (Surface.Surface.containsPoint(C, Qt::OddEvenFill))
							{
								Synchronizer.lock();
								Insertions.append({ Surface.ID, C.x(), C.y(), ang, Surface.Layer });
								Synchronizer.unlock();
							}
						}
					}

					Start += R; Step.setLine(Start, Radius.y1(), Start, Radius.y2());
				}
			}
			else
			{
				double Stop(Radius.y2()); Start += Radius.y1();

				QLineF Step(Radius.x1(), Start, Radius.x2(), Start);

				while (Start < Stop)
				{
					QList<QPointF> Intersections; QPointF Int;
					QList<QPair<QPointF, double>> Angles;

					for (int i = 1; i < Surface.Surface.size(); ++i)
					{
						const QLineF hLine(Surface.Surface[i - 1],
									    Surface.Surface[i]);

						auto Type = Step.intersects(hLine, &Int);

						if (Type == QLineF::BoundedIntersection)
						{
							const double ang = qAtan((hLine.y1() - hLine.y2()) /
												(hLine.x1() - hLine.x2()));

							Angles.append({ Int, normalizeAngle(ang - M_PI_2) });
							Intersections.append(Int);
						}
					}

					std::sort(Intersections.begin(), Intersections.end(),
					[] (const QPointF& A, const QPointF& B) -> bool
					{
						return A.x() < B.x();
					});

					while (Intersections.size() >= 2)
					{
						QLineF hLine(Intersections.takeFirst(),
								   Intersections.takeFirst());

						const double hLength = hLine.length();
						const double a1 = getAngle(Angles, hLine.p1());
						const double a2 = getAngle(Angles, hLine.p2());

						if (hLength >= R)
						{
							double hStart = (hLength - (int(hLength / R) * R)) / 2.0;
							hStart += qMin(hLine.x1(), hLine.x2());

							while (hStart < qMax(hLine.x1(), hLine.x2()))
							{
								const QPointF C(Start, hStart); hStart += R;

								const double w1 = 1/QLineF(C, hLine.p1()).length();
								const double w2 = 1/QLineF(C, hLine.p2()).length();

								const double ang = normalizeAngle((w1*a1 + w2*a2) / (w1 + w2));

								if (Surface.Surface.containsPoint(C, Qt::OddEvenFill))
								{
									Synchronizer.lock();
									Insertions.append({ Surface.ID, C.x(), C.y(), ang, Surface.Layer });
									Synchronizer.unlock();
								}
							}
						}
						else
						{
							const auto C = hLine.center();
							const double ang = normalizeAngle((a1 + a2) / 2);

							if (Surface.Surface.containsPoint(C, Qt::OddEvenFill))
							{
								Synchronizer.lock();
								Insertions.append({ Surface.ID, C.x(), C.y(), ang, Surface.Layer });
								Synchronizer.unlock();
							}
						}
					}

					Start += R; Step.setLine(Radius.x1(), Start, Radius.x2(), Start);
				}
			}
		}
	});

	QtConcurrent::blockingMap(Insertions, [X, Y] (LABEL& Point) -> void
	{
		Point.X += X; Point.Y += Y;
	});

	if (isTerminated()) { emit onEndProgress(); emit onLabelInsert(0); return; }

	emit onBeginProgress(tr("Inserting labels"));
	emit onSetupProgress(0, Insertions.size()); Step = 0;

	for (const auto& Item : qAsConst(Insertions)) if (!isTerminated())
	{
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
			else Set.insert(Item.ID);

			Count += 1;
		}

		emit onUpdateProgress(++Step);
	}

	if (Dateupdate) updateModDate(Set);

	emit onEndProgress();
	emit onLabelInsert(Count);
}

void DatabaseDriver::removeLabel(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onLabelDelete(0); return; }

	QSet<int> Deletes, Set; int Step = 0; QSqlQuery Select(Database), DeleteA(Database), DeleteB(Database);

	Select.setForwardOnly(true);

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
	emit onSetupProgress(0, Items.size()); Step = 0;

	if (Select.exec()) while (Select.next() && !isTerminated())
	{
		const int UID = Select.value(0).toInt();

		if (Items.contains(UID))
		{
			Deletes.insert(Select.value(1).toInt());
			Set.insert(UID);

			emit onUpdateProgress(++Step);
		}
	}

	if (isTerminated()) { emit onEndProgress(); emit onLabelDelete(0); return; }

	emit onBeginProgress(tr("Deleting labels"));
	emit onSetupProgress(0, Items.size()); Step = 0;

	for (const auto& IDE : Deletes)
	{
		if (isTerminated()) break;

		DeleteA.addBindValue(IDE); DeleteA.exec();
		DeleteB.addBindValue(IDE); DeleteB.exec();

		emit onUpdateProgress(++Step);
	}

	if (Dateupdate) updateModDate(Set);

	emit onEndProgress();
	emit onLabelDelete(Step);
}

void DatabaseDriver::editLabel(const QSet<int>& Items, const QString& Label, int Underline, int Pointer, double Rotation, int Posset, double setX, double setY, int Just)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onLabelEdit(0); return; }

	struct POINT
	{
		int IDE, UIDO;

		double WX, WY;
		double DX, DY;

		unsigned J;

		QVariant OX, OY;
	};

	QHash<int, POINT> Labels; QSet<int> SetA, SetB; int Step = 0;
	QSqlQuery selectQuery(Database), updateQuery(Database), pointsQuery(Database);

	pointsQuery.prepare(
		"SELECT "
			"O.UID, T.POS_X, T.POS_Y "
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
			"O.RODZAJ = 4 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4");

	selectQuery.prepare(
		"SELECT "
			"O.UID, P.ID, P.POS_X, P.POS_Y, P.JUSTYFIKACJA, P.ODN_X, P.ODN_Y "
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
			"TEXT = COALESCE(?, TEXT), "
			"JUSTYFIKACJA = ?, "
			"POS_X = ?, POS_Y = ?, "
			"ODN_X = ?, ODN_Y = ?, "
			"KAT = COALESCE(?, KAT) "
		"WHERE "
			"ID = ?");

	emit onBeginProgress(tr("Loading texts"));
	emit onSetupProgress(0, 0);

	if (selectQuery.exec()) while (selectQuery.next() && !isTerminated())
	{
		const int UIDO = selectQuery.value(0).toInt();

		if (Items.contains(UIDO)) Labels.insert(UIDO,
		{
			selectQuery.value(1).toInt(),
			UIDO, NAN, NAN,
			selectQuery.value(2).toDouble(),
			selectQuery.value(3).toDouble(),
			selectQuery.value(4).toUInt(),
			selectQuery.value(5),
			selectQuery.value(6),
		});
	}

	if (pointsQuery.exec()) while (pointsQuery.next() && !isTerminated())
	{
		const int UIDO = pointsQuery.value(0).toInt();

		if (Labels.contains(UIDO))
		{
			auto& Labelr = Labels[UIDO];

			Labelr.WX = pointsQuery.value(1).toDouble();
			Labelr.WY = pointsQuery.value(2).toDouble();
		}
	}

	if (isTerminated()) { emit onEndProgress(); emit onLabelEdit(0); return; }

	emit onBeginProgress(tr("Updating texts"));
	emit onSetupProgress(0, 0);

	if (Posset) QtConcurrent::blockingMap(Labels, [Posset,setX, setY] (POINT& Point) -> void
	{
		if (Posset == 1)
		{
			Point.DX += setX;
			Point.DY += setY;
		}
		else if (!qIsNaN(Point.WX) && !qIsNaN(Point.WY))
		{
			Point.DX = Point.WX + setX;
			Point.DY = Point.WY + setY;
		}
	});

	if (Pointer) QtConcurrent::blockingMap(Labels, [Pointer] (POINT& Point) -> void
	{
		if (Pointer == 1 && !qIsNaN(Point.WX) && !qIsNaN(Point.WY))
		{
			Point.OX = Point.WX - Point.DX;
			Point.OY = Point.WY - Point.DY;

			Point.J |= 32;
		}
		else if (Pointer == 2) Point.J &= ~32;
	});

	if (Underline) QtConcurrent::blockingMap(Labels, [Underline] (POINT& Point) -> void
	{
		if (Underline == 1) Point.J |= 16;
		if (Underline == 2) Point.J &= ~16;
	});

	if (Just) QtConcurrent::blockingMap(Labels, [Just] (POINT& Point) -> void
	{
		Point.J = (Point.J & ~0b1111) | (Just & 0b1111);
	});

	emit onBeginProgress(tr("Updating texts"));
	emit onSetupProgress(0, Labels.size());

	const QVariant vLabel = Label.isEmpty() ? QVariant() : Label;
	const QVariant vRotation = qIsNaN(Rotation) ? QVariant() : Rotation;

	for (const auto& Item : qAsConst(Labels))
	{
		if (isTerminated()) break;

		updateQuery.addBindValue(vLabel);
		updateQuery.addBindValue(Item.J);
		updateQuery.addBindValue(Item.DX);
		updateQuery.addBindValue(Item.DY);
		updateQuery.addBindValue(Item.OX);
		updateQuery.addBindValue(Item.OY);
		updateQuery.addBindValue(vRotation);
		updateQuery.addBindValue(Item.IDE);

		qDebug() << updateQuery.boundValues();

		if (updateQuery.exec())
		{
			SetA.insert(Item.UIDO);
			SetB.insert(Item.IDE);
		}

		emit onUpdateProgress(++Step);
	}

	if (Dateupdate)
	{
		updateModDate(SetA, 0);
		updateModDate(SetB, 2);
	}

	emit onEndProgress();
	emit onLabelEdit(SetA.size());
}

void DatabaseDriver::insertPoints(const QSet<int>& Items, int Mode, double Radius, const QString& Path)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPointInsert(0); return; }

	QSet<QPair<double, double>> Predef;
	int Count(0), Current(0);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Locale");
	const auto csvSep = Settings.value("csv", ",").toString();
	const auto txtSep = Settings.value("txt", "\\s+").toString();
	Settings.endGroup();

	if (Mode & 0x8 && !Path.isEmpty())
	{
		QFile File(Path); File.open(QFile::ReadOnly | QFile::Text);

		if (File.isOpen())
		{
			const QString Extension = QFileInfo(Path).suffix(); QRegExp Separator;

			if (Extension != "csv") Separator = QRegExp(txtSep);
			else Separator = QRegExp(QString("\\s*%1\\s*").arg(csvSep));

			while (!File.atEnd())
			{
				const QString Line = File.readLine().trimmed(); if (Line.isEmpty()) continue;
				const QStringList Data = Line.split(Separator, Qt::KeepEmptyParts);

				if (Data.size() == 2)
				{
					bool ok = true;

					const double a = Data[0].toDouble(&ok);
					const double b = Data[1].toDouble(&ok);

					if (ok) Predef.insert({ a, b });
				}
			}
		}
	}

	if (Mode & 0x10) Count += insertSurfsegments(Items, Radius, Mode);

	if ((Mode & 0x0F) && !isTerminated()) do
	{
		Current = insertBreakpoints(Items, Mode, Radius, Predef); Count += Current;
	}
	while (Current && !isTerminated());

	emit onEndProgress();
	emit onPointInsert(Count);
}

void DatabaseDriver::mergeSegments(const QSet<int>& Items, int Flags, double Diff, double Length, bool Mode)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onSegmentReduce(0); return; }

	const auto appendCount = [] (const QPointF& P, QSet<QPair<double, double>>& Cuts, QHash<QPair<double, double>, int>& Counts)
	{
		if (++Counts[qMakePair(P.x(), P.y())] == 3) Cuts.insert(qMakePair(P.x(), P.y()));
	};

	const auto isCut = [] (const QPointF& P, const QSet<QPair<double, double>>& Cuts) -> bool
	{
		for (const auto& C : Cuts) if (pointComp(P, QPointF(C.first, C.second))) return true; return false;
	};

	struct LINE { int ID; QLineF Line; };

	QSet<QPair<double, double>> Cuts; QHash<QPair<double, double>, int> Counts;

	emit onBeginProgress(tr("Loading points"));
	emit onSetupProgress(0, 0);

	if (Flags & 0x01)
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		Query.prepare(
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
				"T.TYP = 4");

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.insert(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});

		Query.prepare(
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
				"O.RODZAJ = 3");

		if (Query.exec()) while (Query.next() && !isTerminated()) Cuts.insert(
		{
			Query.value(0).toDouble(),
			Query.value(1).toDouble()
		});
	}

	if (Flags & 0x02)
	{
		QSqlQuery Query(Database); Query.setForwardOnly(true);

		Query.prepare(
			"SELECT "
				"P.P0_X, P.P0_Y, "
				"P.P1_X, P.P1_Y "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"INNER JOIN "
				"EW_POLYLINE T "
			"ON "
				"E.IDE = T.ID "
			"WHERE "
				"O.STATUS = 0 AND "
				"O.RODZAJ = 4 AND "
				"E.TYP = 0 AND "
				"T.STAN_ZMIANY = 0 AND "
				"T.P1_FLAGS = 0");

		if (Query.exec()) while (Query.next() && !isTerminated())
		{
			const QPointF PointA(Query.value(0).toDouble(), Query.value(1).toDouble());
			const QPointF PointB(Query.value(2).toDouble(), Query.value(3).toDouble());

			appendCount(PointA, Cuts, Counts); appendCount(PointB, Cuts, Counts);
		}
	}

	QSqlQuery Objects(Database), deleteGeometry(Database),
			updateSegment(Database), deleteSegment(Database);

	QMutex Synchronizer; int Step(0);

	Objects.prepare(
		"SELECT "
			"O.UID, P.ID, P.P0_X, P.P0_Y, "
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
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS <> 4 AND "
			"E.TYP = 0 "
		"ORDER BY "
			"O.UID ASC, "
			"E.N ASC");

	deleteGeometry.prepare("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = ? AND IDE = ? AND TYP = 0");
	deleteSegment.prepare("DELETE FROM EW_POLYLINE WHERE ID = ?");
	updateSegment.prepare("UPDATE EW_POLYLINE SET P0_X = ?, P0_Y = ?, P1_X = ?, P1_Y = ?,"
					  "PN_X = NULL, PN_Y = NULL, P1_FLAGS = 0 WHERE ID = ? AND STAN_ZMIANY = 0");

	QHash<int, QList<LINE>> Geometry;
	QSet<QPair<int, int>> Deletes;
	QHash<int, QLineF> Updates;

	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0);

	if (Objects.exec()) while (Objects.next() && !isTerminated())
	{
		const int UID = Objects.value(0).toInt();

		if (!Items.contains(UID)) continue;

		Geometry[UID].append(
		{
			Objects.value(1).toInt(),
			{
				Objects.value(2).toDouble(),
				Objects.value(3).toDouble(),
				Objects.value(4).toDouble(),
				Objects.value(5).toDouble()
			}
		});
	}

	if (isTerminated()) { emit onEndProgress(); emit onSegmentReduce(0); return; }

	emit onBeginProgress(tr("Computing geometry"));
	emit onSetupProgress(0, 0); Step = 0;

	if (Diff == 0.0) Diff = INFINITY;
	if (Length == 0.0) Length = INFINITY;

	QtConcurrent::blockingMap(Items,
	[&Geometry, &Deletes, &Updates, &Cuts, &Synchronizer, Diff, Length, Mode, isCut]
	(const int& UID) -> void
	{
		if (!Geometry.contains(UID)) return;
		auto& List = Geometry[UID];

		for (int i = 1; i < List.size(); ++i)
		{
			auto& L1 = List[i - 1];
			auto& L2 = List[i];

			auto L3 = (i == List.size() - 1) ? QLineF() : List[i + 1].Line;
			auto New = L2.Line;
			bool OK = true;

			const double ang = L1.Line.angleTo(L2.Line);

			const bool isNextOk = (Mode == 0) && (L3 != QLineF()) && (L1.Line.length() < Length || L2.Line.length() < Length) && L3.length() > Length;
			const bool isLastOk = (Mode == 0) && (i == List.size() - 1) && (L1.Line.length() < Length || L2.Line.length() < Length);
			const bool isLenOk = isNextOk || isLastOk || (L1.Line.length() < Length && L2.Line.length() < Length);
			const bool isAngOk = qAbs(ang) < Diff || qAbs(qAbs(ang) - 180) < Diff || qAbs(qAbs(ang) - 360) < Diff;

			if (!isLenOk || !isAngOk) continue;

			if (pointComp(L2.Line.p1(), L1.Line.p1()) && !isCut(L2.Line.p1(), Cuts)) New.setP1(L1.Line.p2());
			else if (pointComp(L2.Line.p1(), L1.Line.p2()) && !isCut(L2.Line.p1(), Cuts)) New.setP1(L1.Line.p1());
			else if (pointComp(L2.Line.p2(), L1.Line.p1()) && !isCut(L2.Line.p2(), Cuts)) New.setP2(L1.Line.p2());
			else if (pointComp(L2.Line.p2(), L1.Line.p2()) && !isCut(L2.Line.p2(), Cuts)) New.setP2(L1.Line.p1());
			else OK = false;

			if (New.length() < L1.Line.length() || New.length() < L2.Line.length()) continue;
			else if (OK)
			{
				L2.Line = New;

				Synchronizer.lock();
				Deletes.insert({ UID, L1.ID });
				Updates.remove(L1.ID);
				Updates.insert(L2.ID, L2.Line);
				Synchronizer.unlock();
			}
		}
	});

	if (isTerminated()) { emit onEndProgress(); emit onSegmentReduce(0); return; }

	emit onBeginProgress(tr("Updating geometry")); Step = 0;
	emit onSetupProgress(0, Deletes.size() + Updates.size());

	for (auto i = Updates.constBegin(); i != Updates.constEnd(); ++i)
	{
		updateSegment.addBindValue(i.value().x1());
		updateSegment.addBindValue(i.value().y1());
		updateSegment.addBindValue(i.value().x2());
		updateSegment.addBindValue(i.value().y2());

		updateSegment.addBindValue(i.key());

		updateSegment.exec();

		emit onUpdateProgress(++Step);
	}

	for (auto i = Deletes.constBegin(); i != Deletes.constEnd(); ++i)
	{
		deleteGeometry.addBindValue((*i).first);
		deleteGeometry.addBindValue((*i).second);

		deleteGeometry.exec();

		deleteSegment.addBindValue((*i).second);

		deleteSegment.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onSegmentReduce(Deletes.size());
}

void DatabaseDriver::updateKergs(const QSet<int>& Items, const QString& Path, int Action, int Elements)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onKergUpdate(0); return; }

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	QHash<QString, int> Mapping; QHash<int, QSet<int>> Kergs;
	QHash<int, QDate> Dates; QHash<int, int> Updates;
	QMutex Synchronizer; QSet<int> Uids; int Step(0);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Locale");
	const auto csvSep = Settings.value("csv", ",").toString();
	const auto txtSep = Settings.value("txt", "\\s+").toString();
	Settings.endGroup();

	emit onBeginProgress(tr("Loading jobs"));
	emit onSetupProgress(0, 0);

	Query.prepare("SELECT UID, NUMER FROM EW_OPERATY");

	if (Query.exec()) while (Query.next()) Mapping.insert
	(
		Query.value(1).toString(),
		Query.value(0).toInt()
	);

	if (Action == 1)
	{
		QFile File(Path); File.open(QFile::ReadOnly | QFile::Text);

		if (File.isOpen())
		{
			const QString Extension = QFileInfo(Path).suffix(); QRegExp Separator;

			if (Extension != "csv") Separator = QRegExp(txtSep);
			else Separator = QRegExp(QString("\\s*%1\\s*").arg(csvSep));

			while (!File.atEnd())
			{
				const QString Line = File.readLine().trimmed(); if (Line.isEmpty()) continue;
				const QStringList Data = Line.split(Separator, Qt::KeepEmptyParts);

				if (Data.size() < 2 || !Mapping.contains(Data[0])) continue;

				const QDate Date = castStrToDate(Data[1]);

				if (Date.isValid()) Dates[Mapping[Data[0]]] = Date;
			}
		}
	}

	emit onBeginProgress(tr("Loading elements"));
	emit onSetupProgress(0, 0);

	if (Action == 1)
	{
		Query.prepare(
			"SELECT DISTINCT "
				"O.UID, COALESCE(P.OPERAT, T.OPERAT, 0), COALESCE(T.TYP, 0) "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"LEFT JOIN "
				"EW_POLYLINE P "
			"ON "
				"(E.IDE = P.ID AND P.STAN_ZMIANY = 0) "
			"LEFT JOIN "
				"EW_TEXT T "
			"ON "
				"(E.IDE = T.ID AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"COALESCE(P.OPERAT, T.OPERAT, 0) <> 0 AND "
				"O.STATUS = 0 AND E.TYP = 0");

		if (Query.exec()) while (Query.next() && !isTerminated())
		{
			const int UID = Query.value(0).toInt();
			const int OP = Query.value(1).toInt();

			if (!Items.contains(UID)) continue;

			const int Type = Query.value(2).toInt();
			bool OK = false;

			OK = OK || (Type == 0 && (Elements & 0x1));
			OK = OK || (Type == 4 && (Elements & 0x2));
			OK = OK || (Type == 6 && (Elements & 0x4));

			if (OK) Kergs[UID].insert(OP);
		}

		for (const auto& UID : Kergs.keys()) Uids.insert(UID);
	}

	QtConcurrent::blockingMap(Uids, [&Updates, &Dates, &Kergs, &Synchronizer] (const int& UID) -> void
	{
		int Finall(0); QDate Current;

		for (const auto& OP : qAsConst(Kergs[UID]))
		{
			const QDate Date = Dates.value(OP);

			if (Current.isNull() || Date > Current)
			{
				Current = Date; Finall = OP;
			}
		}

		if (Finall)
		{
			Synchronizer.lock();
			Updates.insert(UID, Finall);
			Synchronizer.unlock();
		}
	});

	emit onBeginProgress(tr("Updating elements")); int Count(0);
	emit onSetupProgress(0, Action ? Updates.size() : 0);

	Query.prepare("UPDATE EW_OBIEKTY SET OPERAT = ? WHERE UID = ?");

	if (Action == 0)
	{
		QMap<int, int> Lines, Texts; QSqlQuery selectQuery(Database);
		QSqlQuery lineQuery(Database), textQuery(Database);

		selectQuery.prepare(
			"SELECT "
				"O.UID, O.OPERAT, COALESCE(P.UID, T.UID), COALESCE(T.TYP, 0) "
			"FROM "
				"EW_OBIEKTY O "
			"INNER JOIN "
				"EW_OB_ELEMENTY E "
			"ON "
				"O.UID = E.UIDO "
			"LEFT JOIN "
				"EW_POLYLINE P "
			"ON "
				"(E.IDE = P.ID AND P.STAN_ZMIANY = 0) "
			"LEFT JOIN "
				"EW_TEXT T "
			"ON "
				"(E.IDE = T.ID AND T.STAN_ZMIANY = 0) "
			"WHERE "
				"O.STATUS = 0 AND E.TYP = 0");

		if (selectQuery.exec()) while (selectQuery.next() && !isTerminated())
		{
			const int UID = selectQuery.value(0).toInt();
			const int Typ = selectQuery.value(3).toInt();

			if (Items.contains(UID)) switch (Typ)
			{
				case 0:
					if (Elements & 0x1) Lines.insert(selectQuery.value(2).toInt(),
											   selectQuery.value(1).toInt());
				break;
				case 4:
					if (Elements & 0x2) Texts.insert(selectQuery.value(2).toInt(),
											   selectQuery.value(1).toInt());
				break;
				case 6:
					if (Elements & 0x4) Texts.insert(selectQuery.value(2).toInt(),
											   selectQuery.value(1).toInt());
				break;
			}
		}

		lineQuery.prepare("UPDATE EW_POLYLINE P SET P.OPERAT = ? WHERE P.UID = ?");
		textQuery.prepare("UPDATE EW_TEXT P SET P.OPERAT = ? WHERE P.UID = ?");

		emit onSetupProgress(0, Count = Lines.size() + Texts.size());

		if (!isTerminated())
		{
			for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
			{
				lineQuery.addBindValue(i.value());
				lineQuery.addBindValue(i.key());
				lineQuery.exec();

				emit onUpdateProgress(++Step);
			}

			for (auto i = Texts.constBegin(); i != Texts.constEnd(); ++i)
			{
				textQuery.addBindValue(i.value());
				textQuery.addBindValue(i.key());
				textQuery.exec();

				emit onUpdateProgress(++Step);
			}
		}
	}
	else if (!isTerminated())
	{
		for (auto i = Updates.constBegin(); i != Updates.constEnd(); ++i)
		{
			Query.addBindValue(i.value());
			Query.addBindValue(i.key());

			Query.exec();

			emit onUpdateProgress(++Step);

		}
	}

	const QMap<QString, QSet<int>> Views = Action ? getClassGroups(Items, true, 0) :
										   QMap<QString, QSet<int>>();

	emit onBeginProgress(tr("Updating view"));
	emit onSetupProgress(0, Views.size());

	if (Action) for (auto i = Views.constBegin() + 1; i != Views.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Data = loadData(Table, i.value(), QString(), true, true);

		emit onRowsUpdate(Data); emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onKergUpdate(Action ? Updates.size() : Count);
}

void DatabaseDriver::hideEdges(const QSet<int>& Items, const QList<int>& Values)
{
	if (!Database.open()) { emit onError(tr("Database is not opened")); emit onEdgesHide(0); return; }

	struct SEGMENT { int OID, LID, ID; QPointF A, B; }; int Step(0);

	QList<SEGMENT> Lines; QSet<int> Hides; QMutex Synchronizer;

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, false, 0);
	QHash<int, QHash<int, QVariant>> Data;

	emit onBeginProgress(tr("Loading data"));
	emit onSetupProgress(0, Tasks.size());

	for (auto i = Tasks.constBegin(); i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Name);
		const auto Rows = loadData(Table, i.value(), QString(), false, false);

		for (auto j = Rows.constBegin(); j != Rows.constEnd(); ++j)
		{
			Data.insert(j.key(), j.value());
		}
	}

	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0);

	QSqlQuery selectQuery(Database), updateQuery(Database);

	selectQuery.prepare(
		"SELECT "
			"O.UID, P.ID, P.UID, "
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
			"P.STAN_ZMIANY = 0 AND "
			"P.P1_FLAGS = 0 AND "
			"O.STATUS = 0 AND "
			"E.TYP = 0");

	if (selectQuery.exec()) while (selectQuery.next())
	{
		if (Items.contains(selectQuery.value(0).toInt())) Lines.append(
		{
			selectQuery.value(0).toInt(),
			selectQuery.value(1).toInt(),
			selectQuery.value(2).toInt(),
			{
				selectQuery.value(3).toDouble(),
				selectQuery.value(4).toDouble()
			},
			{
				selectQuery.value(5).toDouble(),
				selectQuery.value(6).toDouble()
			}
		});
	}

	emit onBeginProgress(tr("Computing geometry"));
	emit onSetupProgress(0, 0);

	QtConcurrent::blockingMap(Lines, [&Lines, &Hides, &Data, &Values, &Synchronizer] (SEGMENT& Segment) -> void
	{
		const auto check = [&Data, &Values] (int IDA, int IDB)
		{
			for (int ID : Values) if (Data[IDA][ID] != Data[IDB][ID]) return false; return true;
		};

		const auto compare = [] (const SEGMENT& A, const SEGMENT& B) -> bool
		{
			return (A.LID == B.LID) || (A.A == B.A && A.B == B.B) || (A.B == B.A && A.A == B.B);
		};

		for (const auto& Other : qAsConst(Lines)) if (Other.OID != Segment.OID)
		{
			if (compare(Segment, Other) && check(Segment.OID, Other.OID))
			{
				Synchronizer.lock();
				Hides.insert(Segment.ID);
				Synchronizer.unlock();

				return;
			}
		}
	});

	emit onBeginProgress(tr("Updating lines"));
	emit onSetupProgress(0, Hides.size());

	updateQuery.prepare("UPDATE EW_POLYLINE SET TYP_LINII = 14 WHERE UID = ?");

	for (const auto& Line : qAsConst(Lines))
	{
		if (Hides.contains(Line.ID))
		{
			updateQuery.addBindValue(Line.ID);
			updateQuery.exec();
		}
		else Hides.remove(Line.ID);

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onEdgesHide(Hides.size());
}

void DatabaseDriver::saveGeometry(const QSet<int>& Items, const QString& Path)
{
	QList<QLineF> Set;

	const auto Geometry = loadGeometry(Items); emit onEndProgress();

	for (const auto& G : Geometry) switch (G.Geometry.type())
	{
		case QVariant::PointF:
		{
			Set.append({ G.Geometry.toPointF(), G.Geometry.toPointF() });
		}
		break;
		case QVariant::LineF:
		{
			const QLineF C = G.Geometry.toLineF();
			const QPointF P = { (C.x1() + C.x2()) / 2.0, (C.y1() + C.y2()) / 2.0 };

			Set.append({ P, P });
		}
		break;
		case QVariant::PolygonF:
		{
			const QPolygonF P = G.Geometry.value<QPolygonF>();
			const int Size = P.count();

			for (int i = 1; i < Size; ++i)
			{
				Set.append({ P[i - 1], P[i] });
			}
		}
		break;
		case QVariant::List:
		{
			for (const auto& L : G.Geometry.toList())
			{
				Set.append(L.toLineF());
			}
		}
		break;
		default: break;
	}

	QFile File(Path); QTextStream Stream(&File);

	File.open(QFile::Text | QFile::WriteOnly);
	Stream.setRealNumberNotation(QTextStream::FixedNotation);
	Stream.setRealNumberPrecision(5);

	for (const auto& L : Set)
	{
		Stream << L.x1() << '\t' << L.y1() << '\t' << L.x2() << '\t' << L.y2() << Qt::endl;
	}
}

void DatabaseDriver::fixGeometry(const QSet<int>& Items)
{
	if (!Database.open()) { emit onError(tr("Database is not opened")); emit onGeometryFix(0); return; }

	struct SEGMENT { int OID, LID; QLineF Line; int N1 = 0, N2 = 0; };

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMutex Locker; QList<QPair<int, int>> Deletes;
	QHash<int, QList<SEGMENT>> Objects;

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0); int Step(0);

	Query.prepare(
		"SELECT "
			"O.UID, P.ID, "
			"ROUND(P.P0_X, 5), ROUND(P.P0_Y, 5), "
			"ROUND(P.P1_X, 5), ROUND(P.P1_Y, 5) "
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
			"E.TYP = 0 AND "
			"P.P1_FLAGS = 0 "
		"ORDER BY "
			"E.UIDO ASCENDING,"
			"E.N ASCENDING");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int UID = Query.value(0).toInt();

		if (Items.isEmpty() || Items.contains(UID))
		{
			Objects[UID].append(
			{
				UID, Query.value(1).toInt(),
				QLineF(Query.value(2).toDouble(),
					  Query.value(3).toDouble(),
					  Query.value(4).toDouble(),
					  Query.value(5).toDouble())
			});
		}
	}
	else qDebug() << Query.lastError().text();

	emit onBeginProgress(tr("Calculating geometry"));
	emit onSetupProgress(0, Objects.size()); Step = 0;

	QtConcurrent::blockingMap(Objects, [this, &Deletes, &Step, &Locker] (QList<SEGMENT>& Obj) -> void
	{
		QSet<int> Rem; const int UID = Obj.first().OID;

		for (SEGMENT& S : Obj) for (const SEGMENT& O : Obj) if (S.LID != O.LID)
		{
			if (S.Line.p1() == O.Line.p1()) ++S.N1;
			if (S.Line.p1() == O.Line.p2()) ++S.N1;
			if (S.Line.p2() == O.Line.p1()) ++S.N2;
			if (S.Line.p2() == O.Line.p2()) ++S.N2;
		}

		for (const SEGMENT& S : Obj) if (S.N1 > 1)
			for (SEGMENT& O : Obj) if (S.LID != O.LID)
			{
				if (S.Line.p1() == O.Line.p1())
				{
					if (O.N2 > S.N2 || O.Line.length() > S.Line.length())
					{
						Rem.insert(S.LID); --O.N1;
					}
				}
				else if (S.Line.p1() == O.Line.p2())
				{
					if (O.N1 > S.N2 || O.Line.length() > S.Line.length())
					{
						Rem.insert(S.LID); --O.N2;
					}
				}
			}

		for (const SEGMENT& S : Obj) if (S.N2 > 1)
			for (SEGMENT& O : Obj) if (S.LID != O.LID)
			{
				if (S.Line.p2() == O.Line.p1())
				{
					if (O.N2 > S.N1 || O.Line.length() > S.Line.length())
					{
						Rem.insert(S.LID); --O.N1;
					}
				}
				else if (S.Line.p2() == O.Line.p2())
				{
					if (O.N1 > S.N1 || O.Line.length() > S.Line.length())
					{
						Rem.insert(S.LID); --O.N2;
					}
				}
			}

		if (Obj.size() > 1) for (const SEGMENT& S : Obj) if (S.N1 == 0 && S.N2 == 0) Rem.insert(S.LID);
		for (const SEGMENT& S : Obj) if (S.Line.length() < 0.01) Rem.insert(S.LID);

		QMutexLocker ML(&Locker);

		for (const int& ID : Rem) Deletes.append({ UID, ID });

		emit onUpdateProgress(++Step);
	});

	emit onBeginProgress(tr("Updating geometry"));
	emit onSetupProgress(0, Deletes.size()*2); Step = 0;

	Query.prepare(
		"DELETE FROM EW_OB_ELEMENTY E "
		"WHERE E.UIDO = ? AND E.IDE = ? AND E.TYP = 0");

	for (const auto& P : qAsConst(Deletes))
	{
		Query.addBindValue(P.first);
		Query.addBindValue(P.second);

		Query.exec();

		emit onUpdateProgress(++Step);
	}

	Query.prepare("DELETE FROM EW_POLYLINE P WHERE P.ID = ?");

	for (const auto& P : qAsConst(Deletes))
	{
		Query.addBindValue(P.second);

		Query.exec();

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onGeometryFix(Deletes.size());
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

	if (!isTerminated()) QtConcurrent::blockingMap(Parts, [this, &Insert, &Used, &Geometry, &Points, &Locker, Radius] (QList<PART>& List) -> void
	{
		const auto polygon = [] (const QList<PART>& List) -> QPolygonF
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

		const int ID = List.first().ID;
		const QPolygonF Polygon = polygon(List);

		for (const auto& P : Points)
		{
			bool OK(false); const QPointF Point = QPointF(P.X, P.Y);

			for (const auto& G : List) if (G.R != 0.0)
			{
				OK = OK || (G.R + Radius) >= QLineF(Point, QPointF(G.X1, G.Y1)).length();
			}

			if (!OK) for (int k = 1; k < Polygon.size(); ++k)
			{
				const QLineF Part(Polygon[k - 1], Polygon[k]);

				OK = OK || (distance(Part, Point) <= Radius);
			}

			if (!OK) for (const auto& E : Polygon)
			{
				OK = OK || (QLineF(E, Point).length() <= Radius);
			}

			if (OK || Polygon.containsPoint(Point, Qt::OddEvenFill))
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

				Continue = Continue || (QLineF(X1, Y1, P.X, P.Y).length() <= Radius);
				Continue = Continue || (QLineF(X2, Y2, P.X, P.Y).length() <= Radius);
				Continue = Continue || (distance(QLineF(X1, Y1, X2, Y2), QPointF(P.X, P.Y)) <= Radius);

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

QHash<int, QSet<int>> DatabaseDriver::joinMixed(const QHash<int, QSet<int>>& Geometry, const QSet<int>& Obj, const QSet<int>& Sub, double Radius)
{
	if (!Database.isOpen()) return QHash<int, QSet<int>>();
	if (Obj.isEmpty() || Sub.isEmpty()) return QHash<int, QSet<int>>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QSet<int>> Insert; QMutex Synchronizer;

	const QList<OBJECT> Objects = loadGeometry(Obj - Sub);
	const QList<OBJECT> Subobj = loadGeometry(Sub);
	const QList<REDACTION> Redaction = loadRedaction(Sub, { 6 });

	QtConcurrent::blockingMap(Subobj, [this, &Synchronizer, &Objects, &Insert, &Geometry, &Redaction, Radius] (const OBJECT& Object) -> void
	{
		static const auto pdistance = [] (const QLineF& L, const QPointF& P) -> double
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

		static const auto ldistance = [] (const QLineF& A, const QLineF& B) -> double
		{
			if (A.intersects(B, nullptr) == QLineF::BoundedIntersection) return 0.0;

			return qMin
			(
				qMin(pdistance(A, B.p1()), pdistance(A, B.p2())),
				qMin(pdistance(B, A.p1()), pdistance(B, A.p2()))
			);
		};

		double fR = INFINITY; int fID = 0; int fN = 0;

		if (!this->isTerminated()) for (const auto& Other : Objects)
		{
			if (Object.UID == Other.UID) continue;
			double OK(INFINITY); int ON(0);

			if (Object.Geometry.type() == QVariant::PointF)
			{
				const QPointF ThisPoint = Object.Geometry.toPointF();

				if (Other.Geometry.type() == QVariant::PointF)
				{
					OK = QLineF(ThisPoint, Other.Geometry.toPointF()).length();
					if (OK <= Radius) ON += 100;
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF Circle = Other.Geometry.toLineF();
					const QPointF& P = ThisPoint;

					const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
					const double X = (Circle.x1() + Circle.x2()) / 2.0;
					const double Y = (Circle.y1() + Circle.y2()) / 2.0;

					OK = qSqrt(qPow(P.x() - X, 2) + qPow(P.y() - Y, 2)) - R;
					if (OK <= Radius) ON += 75;
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF P = Other.Geometry.value<QPolygonF>();

					if (P.containsPoint(ThisPoint, Qt::OddEvenFill)) { OK = 0.0; ON += 75; }
					else for (int i = 1; i < P.size(); ++i)
					{
						const double Now = pdistance(QLineF(P[i - 1], P[i]), ThisPoint);

						if (Now <= Radius) ON += 1;
						OK = qMin(OK, Now);
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					const QLineF Lin = Part.toLineF();

					double Angle = qAtan2(Lin.y1() - Lin.y2(), Lin.x1() - Lin.x2()) - M_PI / 2.0;

					auto Red = hasItemByField(Redaction, Object.UID, &REDACTION::UID) ?
								 getItemByField(Redaction, Object.UID, &REDACTION::UID) :
								 REDACTION({ 0, 0, 0 });

					const double Now = pdistance(Part.toLineF(), ThisPoint);

					while (Angle < -(M_PI / 2.0)) Angle += M_PI;
					while (Angle > (M_PI / 2.0)) Angle -= M_PI;

					while (Red.Angle < -(M_PI / 2.0)) Red.Angle += M_PI;
					while (Red.Angle > (M_PI / 2.0)) Red.Angle -= M_PI;

					if (Now <= Radius) ON += 1;
					OK = qMin(OK, Now);

					if (Red.UID && Now <= Radius)
					{
						const auto ABS = qAbs(Angle - Red.Angle);

						for (int i = 0; i < 25; ++i)
						{
							if (ABS < (M_PI / qExp(i / 2.0))) ON += 1;
						}
					}
				}
			}
			else if (Object.Geometry.type() == QVariant::LineF)
			{
				const QLineF Circle = Object.Geometry.toLineF();

				const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
				const double X = (Circle.x1() + Circle.x2()) / 2.0;
				const double Y = (Circle.y1() + Circle.y2()) / 2.0;

				const QPointF ThisPoint = QPointF(X, Y);

				if (Other.Geometry.type() == QVariant::PointF)
				{
					const QPointF P = Other.Geometry.toPointF();

					OK = qSqrt(qPow(P.x() - X, 2) + qPow(P.y() - Y, 2)) - R;
					if (OK <= Radius) ++ON;
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF OtherCircle = Other.Geometry.toLineF();

					const double OR = qAbs(OtherCircle.x1() - OtherCircle.x2()) / 2.0;
					const double OX = (OtherCircle.x1() + OtherCircle.x2()) / 2.0;
					const double OY = (OtherCircle.y1() + OtherCircle.y2()) / 2.0;

					OK = QLineF(X, Y, OX, OY).length() - OR - R;
					if (OK <= Radius) ++ON;
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF P = Other.Geometry.value<QPolygonF>();

					if (P.containsPoint(ThisPoint, Qt::OddEvenFill)) { OK = 0.0; ++ON; }

					for (int i = 1; i < P.size(); ++i)
					{
						const double Now = pdistance(QLineF(P[i - 1], P[i]), ThisPoint) - R;

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					const double Now = pdistance(Part.toLineF(), ThisPoint) - R;

					if (Now <= Radius) ++ON;
					OK = qMin(OK, Now);
				}
			}
			else if (Object.Geometry.type() == QVariant::PolygonF)
			{
				const QPolygonF Polygon = Object.Geometry.value<QPolygonF>();

				if (Other.Geometry.type() == QVariant::PointF)
				{
					const QPointF P = Other.Geometry.toPointF();

					if (Polygon.containsPoint(P, Qt::OddEvenFill)) { OK = 0.0; ++ON; }

					for (int i = 1; i < Polygon.size(); ++i)
					{
						const double Now = pdistance(QLineF(Polygon[i - 1], Polygon[i]), P);

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF Circle = Other.Geometry.toLineF();

					const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
					const double X = (Circle.x1() + Circle.x2()) / 2.0;
					const double Y = (Circle.y1() + Circle.y2()) / 2.0;

					const QPointF Point = QPointF(X, Y);

					if (Polygon.containsPoint(Point, Qt::OddEvenFill)) { OK = 0.0; ++ON; }

					for (int i = 1; i < Polygon.size(); ++i)
					{
						const double Now = pdistance(QLineF(Polygon[i - 1], Polygon[i]), Point) - R;

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF OP = Other.Geometry.value<QPolygonF>();

					for (const auto& C : OP) if (Polygon.containsPoint(C, Qt::OddEvenFill)) { OK = 0.0; ++ON; }
					for (const auto& C : Polygon) if (OP.containsPoint(C, Qt::OddEvenFill)) { OK = 0.0; ++ON; }

					for (int i = 1; i < Polygon.size(); ++i) for (int j = 1; j < OP.size(); ++j)
					{
						const double Now = ldistance(QLineF(Polygon[i - 1], Polygon[i]), QLineF(OP[j - 1], OP[j]));

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					const QLineF Line = Part.toLineF();

					if (Polygon.containsPoint(Line.p1(), Qt::OddEvenFill) ||
					    Polygon.containsPoint(Line.p2(), Qt::OddEvenFill)) { OK = 0.0; ++ON; }

					for (int i = 1; i < Polygon.size(); ++i)
					{
						const double Now = ldistance(QLineF(Polygon[i - 1], Polygon[i]), Line);

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
			}
			else if (Object.Geometry.type() == QVariant::List)
			{
				if (Other.Geometry.type() == QVariant::PointF)
				{
					const QPointF Point = Other.Geometry.toPointF();

					for (const auto& Part : Object.Geometry.toList())
					{
						const double Now = pdistance(Part.toLineF(), Point);

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF Circle = Other.Geometry.toLineF();

					const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
					const double X = (Circle.x1() + Circle.x2()) / 2.0;
					const double Y = (Circle.y1() + Circle.y2()) / 2.0;

					const QPointF Point = QPointF(X, Y);

					for (const auto& Part : Object.Geometry.toList())
					{
						const QLineF Line = Part.toLineF(); double Now(INFINITY);

						Now = qMin(Now, pdistance(Line, Point) - R);
						Now = qMin(Now, QLineF(Line.p1(), Point).length() - R);
						Now = qMin(Now, QLineF(Line.p2(), Point).length() - R);

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF OP = Other.Geometry.value<QPolygonF>();

					for (const auto& Part : Object.Geometry.toList()) for (int j = 1; j < OP.size(); ++j)
					{
						const double Now = ldistance(Part.toLineF(), QLineF(OP[j - 1], OP[j]));

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}

					for (const auto& Part : Object.Geometry.toList())
					{
						const QLineF Line = Part.toLineF();

						if (OP.containsPoint(Line.p1(), Qt::OddEvenFill) ||
						    OP.containsPoint(Line.p2(), Qt::OddEvenFill)) { OK = 0.0; ++ON; }
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					for (const auto& This : Object.Geometry.toList())
					{
						const double Now = ldistance(This.toLineF(), Part.toLineF());

						if (Now <= Radius) ++ON;
						OK = qMin(OK, Now);
					}
				}
			}

			if (OK <= Radius && OK <= fR && ON > fN) { fID = Other.UID; fR = OK; fN = ON; }
		}

		if (fID && !Geometry[fID].contains(Object.ID))
		{
			QMutexLocker Locker(&Synchronizer);
			Insert[fID].insert(Object.ID);
		}
	});

	return Insert;
}

void DatabaseDriver::convertSurfaceToPoint(const QSet<int>& Objects, const QString& Symbol, int Layer)
{
	if (!Database.isOpen()) return; QSet<int> Used;

	QSqlQuery selectCirclesQuery(Database), selectLinesQuery(Database),
			insertQuery(Database), deleteQuery(Database),
			updateQuery(Database), elementsQuery(Database);

	selectCirclesQuery.prepare(
		"SELECT "
			"E.IDE, P.OPERAT, "
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
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 3 AND "
			"E.TYP = 0 AND "
			"P.P1_FLAGS = 4 AND "
			"P.STAN_ZMIANY = 0");

	selectLinesQuery.prepare(
		"SELECT "
			"E.IDE, P.OPERAT, P.P0_X, P.P0_Y, "
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
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 3 AND "
			"E.TYP = 0 AND "
			"P.P1_FLAGS <> 4 AND "
			"P.STAN_ZMIANY = 0");

	insertQuery.prepare(
		"INSERT INTO EW_TEXT "
			"(ID, ID_WARSTWY, OPERAT, TEXT, POS_X, POS_Y, TYP, KAT, JUSTYFIKACJA, STAN_ZMIANY) "
		"VALUES "
			"(?, ?, ?, ?, ?, ?, 4, 0.0, 5, 0)");

	deleteQuery.prepare("DELETE FROM EW_POLYLINE WHERE ID = ?");
	updateQuery.prepare("UPDATE EW_OBIEKTY SET RODZAJ = 4 WHERE UID = ?");
	elementsQuery.prepare("DELETE FROM EW_OB_ELEMENTY WHERE TYP = 0 AND UIDO = ? AND IDE = ?");

	emit onSetupProgress(0, Objects.size()); int Step(0);

	for (const auto UID : Objects)
	{
		selectCirclesQuery.addBindValue(UID);

		bool OK(false); if (selectCirclesQuery.exec()) while (selectCirclesQuery.next())
		{
			const int ID = selectCirclesQuery.value(0).toInt();

			insertQuery.addBindValue(ID);
			insertQuery.addBindValue(Layer);
			insertQuery.addBindValue(selectCirclesQuery.value(1));
			insertQuery.addBindValue(Symbol);
			insertQuery.addBindValue(selectCirclesQuery.value(2));
			insertQuery.addBindValue(selectCirclesQuery.value(3));

			if (!insertQuery.exec()) continue; else OK = true;

			deleteQuery.addBindValue(ID); deleteQuery.exec();
		}

		if (OK) { updateQuery.addBindValue(UID); updateQuery.exec(); }

		emit onUpdateProgress(++Step); if (OK) Used.insert(UID);
	}

	for (const auto UID : Objects) if (!Used.contains(UID))
	{
		QList<int> IDES; QList<QPointF> Polygon; int Opr(0); double X(0.0), Y(0.0);

		selectLinesQuery.addBindValue(UID);

		if (selectLinesQuery.exec()) while (selectLinesQuery.next())
		{
			const QPointF A = QPointF(selectLinesQuery.value(2).toDouble(),
								 selectLinesQuery.value(3).toDouble());

			const QPointF B = QPointF(selectLinesQuery.value(4).toDouble(),
								 selectLinesQuery.value(5).toDouble());

			IDES.append(selectLinesQuery.value(0).toInt());

			if (!Polygon.contains(A)) Polygon.append(A);
			if (!Polygon.contains(B)) Polygon.append(B);

			if (!Opr) Opr = selectLinesQuery.value(1).toInt();
		}

		emit onUpdateProgress(++Step);

		if (!IDES.size() || !Polygon.size()) continue;

		for (const auto& P : Polygon)
		{
			X += P.x() / Polygon.size();
			Y += P.y() / Polygon.size();
		}

		insertQuery.addBindValue(IDES.first());
		insertQuery.addBindValue(Layer);
		insertQuery.addBindValue(Opr);
		insertQuery.addBindValue(Symbol);
		insertQuery.addBindValue(X);
		insertQuery.addBindValue(Y);

		if (insertQuery.exec())
		{
			updateQuery.addBindValue(UID); updateQuery.exec();
			deleteQuery.addBindValue(IDES.first()); deleteQuery.exec();

			for (int i = 1; i < IDES.size(); ++i)
			{
				elementsQuery.addBindValue(UID);
				elementsQuery.addBindValue(IDES[i]);

				elementsQuery.exec();

				deleteQuery.addBindValue(IDES[i]);

				deleteQuery.exec();
			}
		}
	}
}

void DatabaseDriver::convertPointToSurface(const QSet<int>& Objects, int Style, int Layer, double Radius)
{
	if (!Database.isOpen()) return;

	QSqlQuery selectQuery(Database), insertQuery(Database),
			deleteQuery(Database), updateQuery(Database);

	selectQuery.prepare(
		"SELECT "
			"E.IDE, P.OPERAT, P.POS_X, P.POS_Y "
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
			"O.RODZAJ = 4 AND "
			"E.TYP = 0 AND "
			"P.TYP = 4 AND "
			"P.STAN_ZMIANY = 0");

	insertQuery.prepare(
		"INSERT INTO EW_POLYLINE "
			"(ID, ID_WARSTWY, OPERAT, TYP_LINII, P0_X, P1_X, P0_Y, P1_Y, POINTCOUNT, P1_FLAGS, STAN_ZMIANY) "
		"VALUES "
			"(?, ?, ?, ?, ?, ?, ?, ?, 2, 4, 0)");

	deleteQuery.prepare("DELETE FROM EW_TEXT WHERE ID = ?");
	updateQuery.prepare("UPDATE EW_OBIEKTY SET RODZAJ = 3 WHERE UID = ?");

	emit onSetupProgress(0, Objects.size()); int Step(0);

	for (const auto UID : Objects)
	{
		selectQuery.addBindValue(UID);

		bool OK(false); if (selectQuery.exec()) while (selectQuery.next())
		{
			const int ID = selectQuery.value(0).toInt();

			insertQuery.addBindValue(ID);
			insertQuery.addBindValue(Layer);
			insertQuery.addBindValue(selectQuery.value(1));
			insertQuery.addBindValue(Style);
			insertQuery.addBindValue(selectQuery.value(2).toDouble() - Radius / 2.0);
			insertQuery.addBindValue(selectQuery.value(2).toDouble() + Radius / 2.0);
			insertQuery.addBindValue(selectQuery.value(3));
			insertQuery.addBindValue(selectQuery.value(3));

			if (!insertQuery.exec()) continue; else OK = true;

			deleteQuery.addBindValue(ID); deleteQuery.exec();
		}

		if (OK) { updateQuery.addBindValue(UID); updateQuery.exec(); }

		emit onUpdateProgress(++Step);
	}
}

void DatabaseDriver::convertSurfaceToLine(const QSet<int>& Objects)
{
	if (!Database.isOpen()) return; auto Tasks = Objects;

	QSqlQuery selectQuery(Database), updateQuery(Database);

	emit onSetupProgress(0, 0);

	selectQuery.prepare(
		"SELECT DISTINCT "
			"O.UID "
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
			"O.RODZAJ = 3 AND "
			"E.TYP = 0 AND "
			"P.P1_FLAGS = 4 AND "
			"P.STAN_ZMIANY = 0");

	if (selectQuery.exec()) while (selectQuery.next()) Tasks.remove(selectQuery.value(0).toInt());

	emit onSetupProgress(0, Tasks.size()); int Step(0);

	updateQuery.prepare("UPDATE EW_OBIEKTY SET RODZAJ = 2 WHERE UID = ? AND RODZAJ = 3");

	for (const auto UID : Tasks) if (!isTerminated())
	{
		updateQuery.addBindValue(UID);
		updateQuery.exec();

		emit onUpdateProgress(++Step);
	}
}

void DatabaseDriver::convertLineToSurface(const QSet<int>& Objects)
{
	if (!Database.isOpen()) return; QSet<int> Updates; int Step(0);

	QSqlQuery selectQuery(Database), updateQuery(Database);

	selectQuery.prepare(
		"SELECT "
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
			"O.UID = ? AND "
			"O.STATUS = 0 AND "
			"O.RODZAJ = 2 AND "
			"E.TYP = 0 AND "
			"P.P1_FLAGS <> 4 AND "
			"P.STAN_ZMIANY = 0");

	emit onSetupProgress(0, Objects.size()); Step = 0;

	for (const auto& UID : Objects) if (!isTerminated())
	{
		selectQuery.addBindValue(UID); QList<QPointF> Points;

		int Size(0); if (selectQuery.exec()) while (selectQuery.next())
		{
			const QPointF A = QPointF(selectQuery.value(0).toDouble(),
								 selectQuery.value(1).toDouble());

			const QPointF B = QPointF(selectQuery.value(2).toDouble(),
								 selectQuery.value(3).toDouble());

			if (Points.contains(A)) Points.removeOne(A);
			else Points.append(A);

			if (Points.contains(B)) Points.removeOne(B);
			else Points.append(B);

			Size += 1;
		}

		if (Size && Points.isEmpty()) Updates.insert(UID);

		emit onUpdateProgress(++Step);
	}

	emit onSetupProgress(0, Updates.size()); Step = 0;

	updateQuery.prepare("UPDATE EW_OBIEKTY SET RODZAJ = 3 WHERE UID = ?");

	for (const auto& UID : Updates) if (!isTerminated())
	{
		updateQuery.addBindValue(UID);
		updateQuery.exec();

		emit onUpdateProgress(++Step);
	}
}

QList<DatabaseDriver::OBJECT> DatabaseDriver::loadGeometry(const QSet<int>& Limiter)
{
	if (!Database.isOpen()) return QList<OBJECT>(); QHash<int, OBJECT> Objects;

	QSqlQuery selectPoint(Database), selectLine(Database);

	selectPoint.setForwardOnly(true); selectLine.setForwardOnly(true);

	emit onSetupProgress(0, Limiter.size()); int Step(0);

	selectPoint.prepare(
		"SELECT "
			"O.UID, O.ID, O.KOD, P.POS_X, P.POS_Y "
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
			"P.STAN_ZMIANY = 0 AND "
			"P.TYP = 4 AND "
			"E.TYP = 0");

	selectLine.prepare(
		"SELECT "
			"O.UID, O.ID, O.KOD, O.RODZAJ, "
			"P.P1_FLAGS, P.P0_X, P.P0_Y, "
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
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0 "
		"ORDER BY "
			"E.UIDO ASCENDING,"
			"E.N ASCENDING");

	if (selectPoint.exec()) while (selectPoint.next() && !isTerminated())
	{
		const int UID = selectPoint.value(0).toInt();

		if (Limiter.isEmpty() || Limiter.contains(UID))
		{
			Objects.insert(UID,
			{
				UID, selectPoint.value(1).toInt(),
				selectPoint.value(2).toString(), 4,
				QPointF(selectPoint.value(3).toDouble(),
					   selectPoint.value(4).toDouble())
			});

			emit onUpdateProgress(++Step);
		}
	}

	if (selectLine.exec()) while (selectLine.next() && !isTerminated())
	{
		const int UID = selectLine.value(0).toInt();

		if (Limiter.isEmpty() || Limiter.contains(UID))
		{
			const int Type = selectLine.value(3).toInt();

			if (!Objects.contains(UID))
			{
				Objects.insert(UID,
				{
					UID, selectLine.value(1).toInt(),
					selectLine.value(2).toString(), Type,
					(Type == 2) ? QVariant(QVariant::List) :
					(Type == 3) ? QVariant(QVariant::PolygonF) :
					QVariant()
				});

				emit onUpdateProgress(++Step);
			}

			OBJECT& Obj = Objects[UID];

			switch (Type)
			{
				case 2:
				{
					if (selectLine.value(4).toInt() != 4)
					{
						QVariantList List = Obj.Geometry.toList();

						List.append(QLineF(selectLine.value(5).toDouble(),
									    selectLine.value(6).toDouble(),
									    selectLine.value(7).toDouble(),
									    selectLine.value(8).toDouble()));

						Obj.Geometry = List;
					}
				}
				break;
				case 3:
				{
					QPolygonF Polygon = Obj.Geometry.value<QPolygonF>();

					const QPointF A(selectLine.value(5).toDouble(),
								 selectLine.value(6).toDouble());

					const QPointF B(selectLine.value(7).toDouble(),
								 selectLine.value(8).toDouble());

					if (selectLine.value(4).toInt() == 4)
					{
						Obj.Geometry = QLineF(A, B);
					}
					else
					{
						if (Polygon.isEmpty())
						{
							Polygon.append(A); Polygon.append(B);
						}
						else if (Polygon.last() == A)
						{
							Polygon.push_back(B);
						}
						else if (Polygon.last() == B)
						{
							Polygon.push_back(A);
						}
						else if (Polygon.first() == A)
						{
							Polygon.push_front(B);
						}
						else if (Polygon.first() == B)
						{
							Polygon.push_front(A);
						}
					}

					if (!Polygon.isEmpty()) Obj.Geometry = Polygon;
				}
				break;
			}
		}
	}

	return Objects.values();
}

QList<DatabaseDriver::REDACTION> DatabaseDriver::loadRedaction(const QSet<int>& Limiter, const QSet<int>& Types)
{
	if (!Database.isOpen()) return QList<REDACTION>(); QList<REDACTION> List;

	QSqlQuery selectPoint(Database), selectLine(Database);

	selectPoint.setForwardOnly(true); selectLine.setForwardOnly(true);

	emit onSetupProgress(0, Limiter.size()); int Step(0);

	selectPoint.prepare(
		"SELECT "
			"O.UID, O.ID, P.TYP, P.TEXT, P.KAT, P.JUSTYFIKACJA "
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
			"P.STAN_ZMIANY = 0 AND "
			"E.TYP = 0");

	selectLine.prepare(
		"SELECT "
			"O.UID, O.ID, P.TYP_LINII "
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
			"E.TYP = 0");

	if (selectPoint.exec()) while (selectPoint.next() && !isTerminated())
	{
		const int UID = selectPoint.value(0).toInt();
		const int Type = selectPoint.value(2).toInt();

		if ((Limiter.isEmpty() || Limiter.contains(UID)) &&
		    (Types.isEmpty() || Types.contains(Type)))
		{
			List.append(
			{
				UID, selectPoint.value(1).toInt(),
				selectPoint.value(2).toInt(),
				selectPoint.value(3).toString(),
				selectPoint.value(4).toDouble(),
				selectPoint.value(5).toInt()
			});

			emit onUpdateProgress(++Step);
		}
	}

	if (selectLine.exec()) while (selectLine.next() && !isTerminated())
	{
		const int UID = selectLine.value(0).toInt();

		if (Limiter.isEmpty() || Limiter.contains(UID))
		{
			List.append(
			{
				UID, selectLine.value(1).toInt(), 0,
				selectLine.value(2).toInt(), 0.0, 0
			});

			emit onUpdateProgress(++Step);
		}
	}

	return List;
}

DatabaseDriver::SUBOBJECTSTABLE DatabaseDriver::loadSubobjects(void)
{
	if (!Database.isOpen()) return SUBOBJECTSTABLE(); SUBOBJECTSTABLE List;

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	Query.prepare(
		"SELECT "
			"S.UID, S.KOD, D.UID, D.KOD "
		"FROM "
			"EW_OB_ELEMENTY E "
		"INNER JOIN "
			"EW_OBIEKTY S "
		"ON "
			"S.UID = E.UIDO "
		"INNER JOIN "
			"EW_OBIEKTY D "
		"ON "
			"D.ID = E.IDE "
		"WHERE "
			"E.TYP = 1 AND "
			"S.STATUS = 0 AND "
			"D.STATUS = 0");

	if (Query.exec()) while (Query.next()) List.append(
	{
		{
			Query.value(0).toInt(),
			Query.value(1).toString()
		},
		{
			Query.value(2).toInt(),
			Query.value(3).toString()
		}
	});

	return List;
}

QSet<int> DatabaseDriver::filterDataByLength(const QList<OBJECT>& Data, double Minimum, double Maximum, int Count)
{
	QSet<int> Filtered; QMutex Synchronizer; int Step(0); emit onSetupProgress(0, Count ? Count : Data.size());

	const auto vA = [Minimum, Maximum] (double L) -> bool { return L > Minimum && L < Maximum; };
	const auto vB = [Minimum, Maximum] (double L) -> bool { return !(L > Maximum && L < Minimum); };

	const auto isOK = [Minimum, Maximum, vA, vB] (double L) -> bool
	{
		if (Minimum < Maximum) return vA(L); else return vB(L);
	};

	QtConcurrent::blockingMap(Data, [this, &Synchronizer, &Step, &Filtered, isOK] (const OBJECT& Object) -> void
	{
		if (!(Object.Mask & 0b1)) return; double L(0.0);

		if (Object.Geometry.type() == QVariant::PolygonF)
		{
			const auto P = Object.Geometry.value<QPolygonF>();

			for (int i = 1; i < P.size(); ++i)
			{
				L += QLineF(P[i - 1], P[i]).length();
			}
		}
		else if (Object.Geometry.type() == QVariant::List)
		{
			for (const auto& P : Object.Geometry.toList())
			{
				L += P.toLineF().length();
			}
		}

		if (isOK(L))
		{
			Synchronizer.lock();
			Filtered.insert(Object.UID);
			Synchronizer.unlock();
		}

		QMutexLocker Locker(&Synchronizer);
		emit onUpdateProgress(++Step);
	});

	return Filtered;
}

QSet<int> DatabaseDriver::filterDataBySurface(const QList<OBJECT>& Data, double Minimum, double Maximum, int Count)
{
	QSet<int> Filtered; QMutex Synchronizer; int Step(0); emit onSetupProgress(0, Count ? Count : Data.size());

	const auto vA = [Minimum, Maximum] (double L) -> bool { return L > Minimum && L < Maximum; };
	const auto vB = [Minimum, Maximum] (double L) -> bool { return !(L > Maximum && L < Minimum); };

	const auto isOK = [Minimum, Maximum, vA, vB] (double L) -> bool
	{
		if (Minimum < Maximum) return vA(L); else return vB(L);
	};

	QtConcurrent::blockingMap(Data, [this, &Synchronizer, &Step, &Filtered, isOK] (const OBJECT& Object) -> void
	{
		if (!(Object.Mask & 0b1)) return; double L(0.0);

		if (Object.Geometry.type() == QVariant::PolygonF)
		{
			L += getSurface(Object.Geometry.value<QPolygonF>());
		}

		if (Object.Geometry.type() == QVariant::LineF)
		{
			const QLineF Circle = Object.Geometry.toLineF();

			const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;

			L += M_PI * R * R;
		}

		if (isOK(L))
		{
			Synchronizer.lock();
			Filtered.insert(Object.UID);
			Synchronizer.unlock();
		}

		QMutexLocker Locker(&Synchronizer);
		emit onUpdateProgress(++Step);
	});

	return Filtered;
}

QSet<int> DatabaseDriver::filterDataByIspartof(const QList<DatabaseDriver::OBJECT>& Data, double Radius, bool Not, int Count)
{
	QSet<int> Filtered; QMutex Synchronizer; int Step(0); emit onSetupProgress(0, Count ? Count : Data.size());

	const unsigned Mask = (Not ? (1 << 5) : (1 << 4)) | 0b10;

	QtConcurrent::blockingMap(Data, [this, &Synchronizer, &Data, &Step, &Filtered, Radius, Mask] (const OBJECT& Object) -> void
	{
		if (!(Object.Mask & 0b1)) return; const QPointF OtherPoint = Object.Geometry.toPointF();

		if (!this->isTerminated()) if (Object.Geometry.type() == QVariant::PointF) for (const auto& Other : Data)
		{
			if (Object.UID == Other.UID || (Other.Mask & Mask) != Mask) continue;

			if (Other.Geometry.type() == QVariant::PointF)
			{
				const QLineF Distance(OtherPoint, Other.Geometry.toPointF());

				if (Distance.length() <= Radius)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
			else if (Other.Geometry.type() == QVariant::LineF)
			{
				const QLineF Circle = Other.Geometry.toLineF();
				const QPointF& P = OtherPoint;

				const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
				const double X = (Circle.x1() + Circle.x2()) / 2.0;
				const double Y = (Circle.y1() + Circle.y2()) / 2.0;

				if ((R + Radius) >= qSqrt(qPow(P.x() - X, 2) + qPow(P.y() - Y, 2)))
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
			else if (Other.Geometry.type() == QVariant::PolygonF)
			{
				const QPolygonF Polygon = Other.Geometry.value<QPolygonF>(); bool OK(false);

				if (Polygon.containsPoint(OtherPoint, Qt::OddEvenFill)) OK = true;
				else for (const auto& ThisPoint : Polygon)
				{
					const QLineF Distance(ThisPoint, OtherPoint);

					OK = OK || (Distance.length() <= Radius);
				}

				if (OK)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
			else for (const auto& Part : Other.Geometry.toList())
			{
				const QLineF Segment = Part.toLineF();

				const QLineF DistanceA(OtherPoint, Segment.p1());
				const QLineF DistanceB(OtherPoint, Segment.p2());

				if (DistanceA.length() <= Radius || DistanceB.length() <= Radius)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
		}

		QMutexLocker Locker(&Synchronizer); emit onUpdateProgress(++Step);
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data) if (Obj.Mask & 0b1)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByContaining(const QList<DatabaseDriver::OBJECT>& Data, double Radius, bool Not, int Count)
{
	QSet<int> Filtered; QMutex Synchronizer; int Step(0); emit onSetupProgress(0, Count ? Count : Data.size());

	const unsigned Mask = (Not ? (1 << 7) : (1 << 6)) | 0b10;

	QtConcurrent::blockingMap(Data, [this, &Synchronizer, &Data, &Step, &Filtered, Radius, Mask] (const OBJECT& Object) -> void
	{
		if (!(Object.Mask & 0b1)) return;

		if (!this->isTerminated()) for (const auto& Other : Data) if ((Other.Mask & Mask) == Mask)
		{
			if (Object.UID == Other.UID || Other.Geometry.type() != QVariant::PointF) continue;

			if (Object.Geometry.type() == QVariant::PointF)
			{
				const QLineF Distance(Object.Geometry.toPointF(),
								  Other.Geometry.toPointF());

				if (Distance.length() <= Radius)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
			else if (Object.Geometry.type() == QVariant::LineF)
			{
				const QLineF Circle = Object.Geometry.toLineF();
				const QPointF P = Other.Geometry.toPointF();

				const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
				const double X = (Circle.x1() + Circle.x2()) / 2.0;
				const double Y = (Circle.y1() + Circle.y2()) / 2.0;

				if ((R + Radius) >= qSqrt(qPow(P.x() - X, 2) + qPow(P.y() - Y, 2)))
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
			else if (Object.Geometry.type() == QVariant::PolygonF)
			{
				const QPolygonF Polygon = Object.Geometry.value<QPolygonF>();
				const QPointF OtherPoint = Other.Geometry.toPointF(); bool OK(false);

				if (Polygon.containsPoint(OtherPoint, Qt::OddEvenFill)) OK = true;
				else for (const auto& ThisPoint : Polygon)
				{
					const QLineF Distance(ThisPoint, OtherPoint);

					OK = OK || (Distance.length() <= Radius);
				}

				if (OK)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
			else for (const auto& Part : Object.Geometry.toList())
			{
				const QPointF OtherPoint = Other.Geometry.toPointF();
				const QLineF Segment = Part.toLineF();

				const QLineF DistanceA(OtherPoint, Segment.p1());
				const QLineF DistanceB(OtherPoint, Segment.p2());

				if (DistanceA.length() <= Radius || DistanceB.length() <= Radius)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
		}

		QMutexLocker Locker(&Synchronizer); emit onUpdateProgress(++Step);
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data) if (Obj.Mask & 0b1)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByEndswith(const QList<DatabaseDriver::OBJECT>& Data, double Radius, bool Not, int Count)
{
	QSet<int> Filtered; QMutex Synchronizer; int Step(0); emit onSetupProgress(0, Count ? Count : Data.size());

	const unsigned Mask = (Not ? (1 << 9) : (1 << 8)) | 0b10;

	QtConcurrent::blockingMap(Data, [this, &Synchronizer, &Data, &Step, &Filtered, Radius, Mask] (const OBJECT& Object) -> void
	{
		if (!(Object.Mask & 0b1)) return; QList<QPointF> Endings;

		for (const auto& Part : Object.Geometry.toList())
		{
			const QLineF P = Part.toLineF();

			if (Endings.contains(P.p1())) Endings.removeOne(P.p1());
			else Endings.append(P.p1());

			if (Endings.contains(P.p2())) Endings.removeOne(P.p2());
			else Endings.append(P.p2());
		}

		if (!this->isTerminated()) if (Object.Geometry.type() == QVariant::List) for (const auto& Other : Data) if ((Other.Mask & Mask) == Mask)
		{
			if (Object.UID == Other.UID || Other.Geometry.type() != QVariant::PointF) continue;

			for (const auto& Point : Endings)
			{
				const QLineF Distance(Point, Other.Geometry.toPointF());

				if (Distance.length() <= Radius)
				{
					QMutexLocker Locker(&Synchronizer);

					emit onUpdateProgress(++Step);
					Filtered.insert(Object.UID); return;
				}
			}
		}

		QMutexLocker Locker(&Synchronizer); emit onUpdateProgress(++Step);
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data) if (Obj.Mask & 0b1)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByIsnear(const QList<DatabaseDriver::OBJECT>& Data, double Radius, bool Not, int Count)
{
	QSet<int> Filtered; QMutex Synchronizer; int Step(0); emit onSetupProgress(0, Count ? Count : Data.size());

	const unsigned Mask = (Not ? (1 << 11) : (1 << 10)) | 0b10;

	QtConcurrent::blockingMap(Data, [this, &Synchronizer, &Data, &Step, &Filtered, Radius, Mask] (const OBJECT& Object) -> void
	{
		static const auto pdistance = [] (const QLineF& L, const QPointF& P) -> double
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

		static const auto ldistance = [] (const QLineF& A, const QLineF& B) -> double
		{
			if (A.intersects(B, nullptr) == QLineF::BoundedIntersection) return 0.0;

			return qMin
			(
				qMin(pdistance(A, B.p1()), pdistance(A, B.p2())),
				qMin(pdistance(B, A.p1()), pdistance(B, A.p2()))
			);
		};

		if (!(Object.Mask & 0b1)) return;

		if (!this->isTerminated()) for (const auto& Other : Data) if ((Other.Mask & Mask) == Mask)
		{
			if (Object.UID == Other.UID) continue; bool OK(false);

			if (Object.Geometry.type() == QVariant::PointF)
			{
				const QPointF ThisPoint = Object.Geometry.toPointF();

				if (Other.Geometry.type() == QVariant::PointF)
				{
					OK = QLineF(ThisPoint, Other.Geometry.toPointF()).length() <= Radius;
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF Circle = Other.Geometry.toLineF();
					const QPointF& P = ThisPoint;

					const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
					const double X = (Circle.x1() + Circle.x2()) / 2.0;
					const double Y = (Circle.y1() + Circle.y2()) / 2.0;

					OK = (R + Radius) >= qSqrt(qPow(P.x() - X, 2) + qPow(P.y() - Y, 2));
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF P = Other.Geometry.value<QPolygonF>();

					OK = P.containsPoint(ThisPoint, Qt::OddEvenFill);

					if (!OK) for (int i = 1; i < P.size(); ++i)
					{
						OK = OK || (pdistance(QLineF(P[i - 1], P[i]), ThisPoint) <= Radius);
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					OK = OK || (pdistance(Part.toLineF(), ThisPoint) <= Radius);
				}
			}
			else if (Object.Geometry.type() == QVariant::LineF)
			{
				const QLineF Circle = Object.Geometry.toLineF();

				const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
				const double X = (Circle.x1() + Circle.x2()) / 2.0;
				const double Y = (Circle.y1() + Circle.y2()) / 2.0;

				const QPointF ThisPoint = QPointF(X, Y);

				if (Other.Geometry.type() == QVariant::PointF)
				{
					const QPointF P = Other.Geometry.toPointF();

					OK = (R + Radius) >= qSqrt(qPow(P.x() - X, 2) + qPow(P.y() - Y, 2));
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF OtherCircle = Other.Geometry.toLineF();

					const double OR = qAbs(OtherCircle.x1() - OtherCircle.x2()) / 2.0;
					const double OX = (OtherCircle.x1() + OtherCircle.x2()) / 2.0;
					const double OY = (OtherCircle.y1() + OtherCircle.y2()) / 2.0;

					OK = (R + OR + Radius) >= QLineF(X, Y, OX, OY).length();
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF P = Other.Geometry.value<QPolygonF>();

					OK = P.containsPoint(ThisPoint, Qt::OddEvenFill);

					if (!OK) for (int i = 1; i < P.size(); ++i)
					{
						OK = OK || (pdistance(QLineF(P[i - 1], P[i]), ThisPoint) <= (Radius + R));
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					OK = OK || (pdistance(Part.toLineF(), ThisPoint) <= (Radius + R));
				}
			}
			else if (Object.Geometry.type() == QVariant::PolygonF)
			{
				const QPolygonF Polygon = Object.Geometry.value<QPolygonF>();

				if (Other.Geometry.type() == QVariant::PointF)
				{
					const QPointF P = Other.Geometry.toPointF();

					OK = Polygon.containsPoint(P, Qt::OddEvenFill);

					if (!OK) for (int i = 1; i < Polygon.size(); ++i)
					{
						OK = OK || (pdistance(QLineF(Polygon[i - 1], Polygon[i]), P) <= Radius);
					}
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF Circle = Other.Geometry.toLineF();

					const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
					const double X = (Circle.x1() + Circle.x2()) / 2.0;
					const double Y = (Circle.y1() + Circle.y2()) / 2.0;

					const QPointF Point = QPointF(X, Y);

					OK = Polygon.containsPoint(Point, Qt::OddEvenFill);

					if (!OK) for (int i = 1; i < Polygon.size(); ++i)
					{
						OK = OK || (pdistance(QLineF(Polygon[i - 1], Polygon[i]), Point) <= (Radius + R));
					}
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF OP = Other.Geometry.value<QPolygonF>();

					for (const auto& C : OP) OK = OK || Polygon.containsPoint(C, Qt::OddEvenFill);
					for (const auto& C : Polygon) OK = OK || OP.containsPoint(C, Qt::OddEvenFill);

					if (!OK) for (int i = 1; i < Polygon.size(); ++i) for (int j = 1; j < OP.size(); ++j)
					{
						OK = OK || (ldistance(QLineF(Polygon[i - 1], Polygon[i]), QLineF(OP[j - 1], OP[j])) <= Radius);
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					const QLineF Line = Part.toLineF();

					OK = OK || Polygon.containsPoint(Line.p1(), Qt::OddEvenFill);
					OK = OK || Polygon.containsPoint(Line.p2(), Qt::OddEvenFill);

					if (!OK) for (int i = 1; i < Polygon.size(); ++i)
					{
						OK = OK || (ldistance(QLineF(Polygon[i - 1], Polygon[i]), Line) <= Radius);
					}
				}
			}
			else if (Object.Geometry.type() == QVariant::List)
			{
				if (Other.Geometry.type() == QVariant::PointF)
				{
					const QPointF Point = Other.Geometry.toPointF();

					for (const auto& Part : Object.Geometry.toList())
					{
						OK = OK || (pdistance(Part.toLineF(), Point) <= Radius);
					}
				}
				else if (Other.Geometry.type() == QVariant::LineF)
				{
					const QLineF Circle = Other.Geometry.toLineF();

					const double R = qAbs(Circle.x1() - Circle.x2()) / 2.0;
					const double X = (Circle.x1() + Circle.x2()) / 2.0;
					const double Y = (Circle.y1() + Circle.y2()) / 2.0;

					const QPointF Point = QPointF(X, Y);

					for (const auto& Part : Object.Geometry.toList())
					{
						const QLineF Line = Part.toLineF();

						OK = OK || (pdistance(Line, Point) <= (Radius + R));
						OK = OK || (QLineF(Line.p1(), Point).length() <= (Radius + R));
						OK = OK || (QLineF(Line.p2(), Point).length() <= (Radius + R));
					}
				}
				else if (Other.Geometry.type() == QVariant::PolygonF)
				{
					const QPolygonF OP = Other.Geometry.value<QPolygonF>();

					for (const auto& Part : Object.Geometry.toList()) for (int j = 1; j < OP.size(); ++j)
					{
						OK = OK || (ldistance(Part.toLineF(), QLineF(OP[j - 1], OP[j])) <= Radius);
					}

					if (!OK) for (const auto& Part : Object.Geometry.toList())
					{
						const QLineF Line = Part.toLineF();

						OK = OK || OP.containsPoint(Line.p1(), Qt::OddEvenFill);
						OK = OK || OP.containsPoint(Line.p2(), Qt::OddEvenFill);
					}
				}
				else for (const auto& Part : Other.Geometry.toList())
				{
					for (const auto& This : Object.Geometry.toList())
					{
						OK = OK || (ldistance(This.toLineF(), Part.toLineF()) <= Radius);
					}
				}
			}

			if (OK)
			{
				QMutexLocker Locker(&Synchronizer);

				emit onUpdateProgress(++Step);
				Filtered.insert(Object.UID); return;
			}
		}

		QMutexLocker Locker(&Synchronizer); emit onUpdateProgress(++Step);
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data) if (Obj.Mask & 0b1)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByIsSubobject(const QSet<int>& Data, const QSet<int>& Objects, const SUBOBJECTSTABLE& Table, bool Not)
{
	emit onSetupProgress(0, Table.size()); int Step(0); QSet<int> Filtered;

	for (const auto& R : Table) if (Objects.contains(R.first.first))
	{
		Filtered.insert(R.second.first);

		if (!(++Step % 1000))
		{
			emit onUpdateProgress(Step);
		}
	}

	if (Not) return QSet<int>(Data).subtract(Filtered);
	else return Filtered.intersect(Data);
}

QSet<int> DatabaseDriver::filterDataByHasSubobject(const QSet<int>& Data, const QSet<int>& Objects, const DatabaseDriver::SUBOBJECTSTABLE& Table, bool Not)
{
	emit onSetupProgress(0, Table.size()); int Step(0); QSet<int> Filtered;

	for (const auto& R : Table) if (Objects.contains(R.second.first))
	{
		Filtered.insert(R.first.first);

		if (!(++Step % 1000))
		{
			emit onUpdateProgress(Step);
		}
	}

	if (Not) return QSet<int>(Data).subtract(Filtered);
	else return Filtered.intersect(Data);
}

QSet<int> DatabaseDriver::filterDataBySymbolAngle(const QList<DatabaseDriver::REDACTION>& Data, double Minimum, double Maximum)
{
	QSet<int> Filtered; QMutex Synchronizer; emit onSetupProgress(0, 0);

	const auto vA = [Minimum, Maximum] (double L) -> bool { return L > Minimum && L < Maximum; };
	const auto vB = [Minimum, Maximum] (double L) -> bool { return !(L > Maximum && L < Minimum); };

	const auto isOK = [Minimum, Maximum, vA, vB] (double L) -> bool
	{
		if (Minimum < Maximum) return vA(L); else return vB(L);
	};

	QtConcurrent::blockingMap(Data, [&Synchronizer, &Filtered, isOK] (const REDACTION& Object) -> void
	{
		if (Object.Type == 4 && isOK(Object.Angle))
		{
			Synchronizer.lock(); Filtered.insert(Object.UID); Synchronizer.unlock();
		}
	});

	return Filtered;
}

QSet<int> DatabaseDriver::filterDataByLabelAngle(const QList<DatabaseDriver::REDACTION>& Data, double Minimum, double Maximum)
{
	QSet<int> Filtered; QMutex Synchronizer; emit onSetupProgress(0, 0);

	const auto vA = [Minimum, Maximum] (double L) -> bool { return L > Minimum && L < Maximum; };
	const auto vB = [Minimum, Maximum] (double L) -> bool { return !(L > Maximum && L < Minimum); };

	const auto isOK = [Minimum, Maximum, vA, vB] (double L) -> bool
	{
		if (Minimum < Maximum) return vA(L); else return vB(L);
	};

	QtConcurrent::blockingMap(Data, [&Synchronizer, &Filtered, isOK] (const REDACTION& Object) -> void
	{
		if (Object.Type == 6 && isOK(Object.Angle))
		{
			Synchronizer.lock(); Filtered.insert(Object.UID); Synchronizer.unlock();
		}
	});

	return Filtered;
}

QSet<int> DatabaseDriver::filterDataBySymbolText(const QList<DatabaseDriver::REDACTION>& Data, const QStringList& Text, bool Not)
{
	QSet<int> Filtered; QMutex Synchronizer; emit onSetupProgress(0, 0);

	const bool Any = Text.isEmpty() || Text.contains(QString());

	QtConcurrent::blockingMap(Data, [&Synchronizer, &Filtered, &Text, Any] (const REDACTION& Object) -> void
	{
		if (Object.Type != 4) return; bool OK = Any;

		const QString Current = Object.Format.toString();

		for (const auto& Txt : Text) if (!OK)
		{
			OK = OK || !Current.compare(Txt, Qt::CaseInsensitive);
		}

		if (OK)
		{
			Synchronizer.lock();
			Filtered.insert(Object.UID);
			Synchronizer.unlock();
		}
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByLabelText(const QList<DatabaseDriver::REDACTION>& Data, const QStringList& Text, bool Not)
{
	QSet<int> Filtered; QMutex Synchronizer; emit onSetupProgress(0, 0);

	const bool Any = Text.isEmpty() || Text.contains(QString());

	QtConcurrent::blockingMap(Data, [&Synchronizer, &Filtered, &Text, Any] (const REDACTION& Object) -> void
	{
		if (Object.Type != 6) return; bool OK = Any;

		const QString Current = Object.Format.toString();

		for (const auto& Txt : Text) if (!OK)
		{
			OK = OK || !Current.compare(Txt, Qt::CaseInsensitive);
		}

		if (OK)
		{
			Synchronizer.lock();
			Filtered.insert(Object.UID);
			Synchronizer.unlock();
		}
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByLineStyle(const QList<DatabaseDriver::REDACTION>& Data, const QStringList& Style, bool Not)
{
	QSet<int> Filtered; QMutex Synchronizer; emit onSetupProgress(0, 0);

	const bool Any = Style.isEmpty() || Style.contains(QString()); QSet<int> List;

	bool Number(false); for (const auto& Txt : Style)
	{
		const int n = Txt.toInt(&Number); if (Number) List.insert(n);
	}

	QtConcurrent::blockingMap(Data, [&Synchronizer, &Filtered, &List, Any] (const REDACTION& Object) -> void
	{
		if (Object.Type != 0) return; const int Current = Object.Format.toInt();

		if (Any || List.contains(Current))
		{
			Synchronizer.lock();
			Filtered.insert(Object.UID);
			Synchronizer.unlock();
		}
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByLabelStyle(const QList<DatabaseDriver::REDACTION>& Data, int Style, bool Not)
{
	QSet<int> Filtered; QMutex Synchronizer; emit onSetupProgress(0, 0);

	QtConcurrent::blockingMap(Data, [&Synchronizer, &Filtered, Style] (const REDACTION& Object) -> void
	{
		if (Object.Type == 6 && (Object.Just & Style) == Style)
		{
			Synchronizer.lock(); Filtered.insert(Object.UID); Synchronizer.unlock();
		}
	});

	if (Not)
	{
		QSet<int> Return;

		for (const auto& Obj : Data)
			if (!Filtered.contains(Obj.UID))
			{
				Return.insert(Obj.UID);
			}

		return Return;
	}
	else return Filtered;
}

QSet<int> DatabaseDriver::filterDataByHasGeoemetry(const QSet<int>& Data, const QSet<int>& Types)
{
	static const QSet<int> Commonlst = { 2, 3, 4 }; QSet<int> Filtered;

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	emit onSetupProgress(0, 0);

	Query.prepare("SELECT UID, RODZAJ FROM EW_OBIEKTY WHERE STATUS = 0");

	if (Commonlst.intersects(Types)) if (Query.exec()) while (Query.next())
	{
		const int UID = Query.value(0).toInt();
		const int Type = Query.value(1).toInt();

		if (Types.contains(Type)) Filtered.insert(UID);
	}

	if (!Types.contains(100)) return QSet<int>(Data).intersect(Filtered);

	Query.prepare("SELECT DISTINCT O.UID FROM EW_OBIEKTY O "
			    "INNER JOIN EW_OB_ELEMENTY E ON O.UID = E.UIDO "
			    "INNER JOIN EW_POLYLINE P ON E.IDE = P.ID "
			    "WHERE "
			    "O.STATUS = 0 AND E.TYP = 0 AND "
			    "P.STAN_ZMIANY = 0 AND P.P1_FLAGS = 4");

	if (Query.exec()) while (Query.next())
	{
		Filtered.insert(Query.value(0).toInt());
	}

	return QSet<int>(Data).intersect(Filtered);
}

QSet<int> DatabaseDriver::filterDataByHasMulrel(const QSet<int>& Data)
{
	QSqlQuery Query(Database); Query.setForwardOnly(true);

	emit onSetupProgress(0, 0); QSet<int> Filtered;

	Query.prepare(
		"SELECT "
			"S.UID "
		"FROM "
			"EW_OBIEKTY S "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"S.ID = E.IDE "
		"INNER JOIN "
			"EW_OBIEKTY D "
		"ON "
			"D.UID = E.UIDO "
		"WHERE "
			"S.STATUS = 0 AND "
			"E.TYP = 1 AND "
			"D.STATUS = 0 "
		"GROUP BY "
			"S.UID "
		"HAVING "
			"COUNT(D.UID) > 1");

	if (Query.exec()) while (Query.next())
	{
		Filtered.insert(Query.value(0).toInt());
	}

	return QSet<int>(Data).intersect(Filtered);
}

int DatabaseDriver::insertBreakpoints(const QSet<int>& Tasks, int Mode, double Radius, const QSet<QPair<double, double>>& Predef)
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
			"P.P1_FLAGS <> 4 AND "
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
		"INSERT INTO EW_POLYLINE (ID, P0_X, P0_Y, P1_X, P1_Y, P1_FLAGS, STAN_ZMIANY, ID_WARSTWY, OPERAT, TYP_LINII, MNOZNIK, POINTCOUNT, CREATE_TS, MODIFY_TS) "
		"SELECT ?, ?, ?, ?, ?, 0, 0, ID_WARSTWY, OPERAT, TYP_LINII, MNOZNIK, POINTCOUNT, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP "
		"FROM EW_POLYLINE WHERE ID = ? AND STAN_ZMIANY = 0");

	updateSegment.prepare("UPDATE EW_POLYLINE SET P0_X = ?, P0_Y = ?, P1_X = ?, P1_Y = ?, PN_X = NULL, PN_Y = NULL, P1_FLAGS = 0 WHERE ID = ?");

	insertElement.prepare("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) VALUES (?, ?, ?, ?)");

	deleteElement.prepare("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = ?");

	getIndex.prepare("SELECT GEN_ID(EW_ELEMENT_ID_GEN, 1) FROM RDB$DATABASE");

	QHash<int, QList<ELEMENT>> Geometry; QHash<int, INSERT> Inserts; QSet<int> Changed;
	QHash<int, QList<QPointF>> Unique;	QHash<int, LINE> Lines, Origins;
	QSet<QPair<double, double>> Points, Ends, Breaks, Intersect, pointCuts;

	emit onBeginProgress(tr("Loading symbols"));
	emit onSetupProgress(0, 0);

	if (Mode & 0x8) if (Symbols.exec()) while (Symbols.next() && !isTerminated())
	{
		if (Tasks.contains(Symbols.value(0).toInt()))
		{
			Points.insert(
			{
				Symbols.value(1).toDouble(),
				Symbols.value(2).toDouble()
			});
		}
	}

	if (Mode & 0x8) Points += Predef;

	emit onBeginProgress(tr("Loading lines"));
	emit onSetupProgress(0, 0);

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

				if (Mode & 0x2)
				{
					Breaks.insert({ A.x(), A.y() });
					Breaks.insert({ B.x(), B.y() });
				}
			}

			Lines.insert(Objects.value(1).toInt(),
			{
				Objects.value(1).toInt(), QLineF(A, B), Objects.value(6).toInt()
			});

			Origins.insert(Objects.value(1).toInt(),
			{
				Objects.value(1).toInt(), QLineF(A, B), Objects.value(6).toInt()
			});
		}
	}

	if (Mode & 0x1) for (const auto& E : Unique) for (const auto& B : E) Ends.insert({ B.x(), B.y() });
	if (Mode & 0x2) for (const auto& E : Ends) Breaks.remove(E);

	emit onBeginProgress(tr("Loading elements"));
	emit onSetupProgress(0, 0); Step = 0;

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
		}
	}

	if (Mode & 0x4) QtConcurrent::blockingMap(Lines, [this, &Lines, &Intersect, &Synchronizer] (LINE& Part) -> void
	{
		if (this->isTerminated()) return;

		for (auto& L : Lines) if (!Part.Type && !L.Type && L.ID != Part.ID)
		{
			if (!pointComp(L.Line.p1(), Part.Line.p1()) && !pointComp(L.Line.p1(), Part.Line.p2()) &&
			    !pointComp(L.Line.p2(), Part.Line.p1()) && !pointComp(L.Line.p2(), Part.Line.p2()));
			else continue;

			QPointF Int; const auto Type = Part.Line.intersects(L.Line, &Int);

			if (Type == QLineF::BoundedIntersection)
			{
				Synchronizer.lock();
				Intersect.insert({ Int.x(), Int.y() });
				Synchronizer.unlock();
			}
		}
	});

	if (Mode) pointCuts.setSharable(false);

	if (Mode & 0x1) pointCuts += Ends;
	if (Mode & 0x2) pointCuts += Breaks;
	if (Mode & 0x4) pointCuts += Intersect;
	if (Mode & 0x8) pointCuts += Points;

	emit onBeginProgress(tr("Computing geometry"));
	emit onSetupProgress(0, Lines.size()); Step = 0;

	QtConcurrent::blockingMap(Lines, [this, &pointCuts, &Inserts, &Synchronizer, &Step, Radius] (LINE& Part) -> void
	{
		if (this->isTerminated()) return;

		if (!Part.Type) for (const auto& Pair : pointCuts)
		{
			const QPointF P(Pair.first, Pair.second);

			if (P.x() >= qMax(Part.Line.x1(), Part.Line.x2()) ||
			    P.x() <= qMin(Part.Line.x1(), Part.Line.x2()) ||
			    P.y() >= qMax(Part.Line.y1(), Part.Line.y2()) ||
			    P.y() <= qMin(Part.Line.y1(), Part.Line.y2())) continue;

			QPointF Int; QLineF Normal(P, QPointF());

			Normal.setAngle(Part.Line.angle() + 90.0);
			Part.Line.intersects(Normal, &Int);

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

		Synchronizer.lock();
		emit onUpdateProgress(++Step);
		Synchronizer.unlock();
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

int DatabaseDriver::insertSurfsegments(const QSet<int>& Tasks, double Radius, int Mode)
{
	const auto append_pl = [] (QPolygonF& Pol, const QPointF& A, const QPointF& B) -> void
	{
		if (Pol.isEmpty())
		{
			Pol.append(A); Pol.append(B);
		}
		else if (Pol.last() == A)
		{
			Pol.push_back(B);
		}
		else if (Pol.last() == B)
		{
			Pol.push_back(A);
		}
		else if (Pol.first() == A)
		{
			Pol.push_front(B);
		}
		else if (Pol.first() == B)
		{
			Pol.push_front(A);
		}
	};

	QHash<int, QList<QPair<int, QLineF>>> Segments;
	QHash<int, QPair<QPointF, double>> Circles;
	QHash<int, QList<QPair<int, int>>> Addons;
	QHash<int, QPolygonF> Polygons;
	QHash<int, QPointF> Centers;
	QHash<int, QPair<int, int>> Styles;

	QSet<int> Mods; QMutex Synchronizer;
	int Step = 0, Count = 0;

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QSqlQuery Index(Database); Index.setForwardOnly(true);

	emit onBeginProgress(tr("Loading geometry"));
	emit onSetupProgress(0, 0);

	Index.prepare("SELECT GEN_ID(EW_ELEMENT_ID_GEN, 1) FROM RDB$DATABASE");

	Query.prepare(
		"SELECT "
			"O.UID, O.RODZAJ, P.P0_X, P.P0_Y, "
			"IIF(P.PN_X IS NULL, P.P1_X, P.PN_X), "
			"IIF(P.PN_Y IS NULL, P.P1_Y, P.PN_Y), "
			"E.IDE, E.TYP, IIF(P.ID IS NULL, 1, 0),"
			"P.TYP_LINII, P.ID_WARSTWY, P.P1_FLAGS "
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
			"O.RODZAJ IN (2, 3) "
		"ORDER BY "
			"O.UID, E.N ASC");

	if (Query.exec()) while (Query.next() && !isTerminated())
	{
		const int ID = Query.value(0).toInt();
		const int Ot = Query.value(1).toInt();
		const bool Gt = Query.value(8).toInt();
		const bool Cr = Query.value(11).toInt() == 4;

		if (!Tasks.contains(ID)) continue;

		if (Ot == 2 && Gt) Addons[ID].append(
		{
			Query.value(6).toInt(),
			Query.value(7).toInt()
		});
		else if (Ot == 2 && !Gt) Segments[ID].append(
		{
			Query.value(6).toInt(),
			{
				Query.value(2).toDouble(),
				Query.value(3).toDouble(),
				Query.value(4).toDouble(),
				Query.value(5).toDouble()
			}
		});
		else if (Ot == 3 && !Gt && !Cr) append_pl(Polygons[ID],
		{
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		},
		{
			Query.value(4).toDouble(),
			Query.value(5).toDouble()
		});
		else if (Ot == 3 && !Gt && Cr) Circles[ID] =
		{
			{
				(Query.value(2).toDouble() + Query.value(4).toDouble()) / 2.0,
				(Query.value(3).toDouble() + Query.value(5).toDouble()) / 2.0,
			},
			qAbs(Query.value(2).toDouble() - ((Query.value(2).toDouble() + Query.value(4).toDouble()) / 2.0))
		};

		const int IW = Query.value(10).toInt();
		const int SL = Query.value(9).toInt();

		if (Ot == 2 && !Gt && IW) Styles[ID] = { IW, SL };
	}

	emit onBeginProgress(tr("Computing geometry"));
	emit onSetupProgress(0, 0);

	QtConcurrent::blockingMap(Tasks,
	[&Polygons, &Centers, &Synchronizer]
	(int UID) -> void
	{
		if (!Polygons.contains(UID)) return;

		const auto& Pol = Polygons[UID];
		double X(0.0), Y(0.0), Div(Pol.size());

		for (int i = 1; i < Div; ++i)
		{
			X += Pol[i].x() / (Div - 1);
			Y += Pol[i].y() / (Div - 1);
		}

		Synchronizer.lock();
		Centers.insert(UID, { X, Y });
		Synchronizer.unlock();
	});

	QtConcurrent::blockingMap(Tasks,
	[&Segments, &Polygons, &Circles, &Centers, &Mods, &Synchronizer, Radius, Mode]
	(int UID) -> void
	{
		static const auto between = [] (double px, double py, double x1, double y1, double x2, double y2) -> bool
		{
			const double lx1 = qMax(x1, x2); const double lx2 = qMin(x1, x2);
			const double ly1 = qMax(y1, y2); const double ly2 = qMin(y1, y2);

			return (px <= lx1) && (px >= lx2) && (py <= ly1) && (py >= ly2);
		};

		if (!Segments.contains(UID)) return;

		QPointF sMatch, eMatch;
		double sRad(NAN), eRad(NAN);
		QPointF Start, Stop;

		auto& Geom = Segments[UID];

		if (Geom.size() < 2)
		{
			Start = Geom.first().second.p1();
			Stop = Geom.first().second.p2();
		}
		else
		{
			const int L = Geom.size() - 1; const int P = L - 1;

			if (Geom[0].second.p1() == Geom[1].second.p1()) Start = Geom[0].second.p2();
			else if (Geom[0].second.p1() == Geom[1].second.p2()) Start = Geom[0].second.p2();
			else if (Geom[0].second.p2() == Geom[1].second.p1()) Start = Geom[0].second.p1();
			else if (Geom[0].second.p2() == Geom[1].second.p2()) Start = Geom[0].second.p1();

			if (Geom[L].second.p1() == Geom[P].second.p1()) Stop = Geom[L].second.p2();
			else if (Geom[L].second.p1() == Geom[P].second.p2()) Stop = Geom[L].second.p2();
			else if (Geom[L].second.p2() == Geom[P].second.p1()) Stop = Geom[L].second.p1();
			else if (Geom[L].second.p2() == Geom[P].second.p2()) Stop = Geom[L].second.p1();
		}

		for (auto i = Polygons.constBegin(); i != Polygons.constEnd(); ++i)
		{
			if (!(Mode & 0x20) && i.value().containsPoint(Start, Qt::OddEvenFill))
			{
				sMatch = Centers[i.key()]; sRad = 0.0;
			}

			if (!(Mode & 0x20) && i.value().containsPoint(Stop, Qt::OddEvenFill))
			{
				sMatch = Centers[i.key()]; sRad = 0.0;
			}

			for (int j = 1; j < i.value().size(); ++j)
			{
				QPointF IntA, IntB; const QLineF L(i.value()[j - 1], i.value()[j]);

				L.intersects(Geom.first().second, &IntA);
				L.intersects(Geom.last().second, &IntB);

				const double RadA = QLineF(IntA, Start).length();
				const double RadB = QLineF(IntB, Stop).length();

				const bool b1 = between(IntA.x(), IntA.y(), L.x1(), L.y1(), L.x2(), L.y2());
				const bool b2 = between(IntB.x(), IntB.y(), L.x1(), L.y1(), L.x2(), L.y2());

				if (b1 && RadA <= Radius && (qIsNaN(sRad) || RadA < sRad))
				{
					sMatch = Centers[i.key()]; sRad = RadA;
				}

				if (b2 && RadB <= Radius && (qIsNaN(eRad) || RadB < eRad))
				{
					eMatch = Centers[i.key()]; eRad = RadB;
				}
			}
		}

		for (auto i = Circles.constBegin(); i != Circles.constEnd(); ++i)
		{
			double RadA = QLineF(i.value().first, Start).length() - i.value().second;
			double RadB = QLineF(i.value().first, Stop).length() - i.value().second;

			if (Mode & 0x20) { RadA = qAbs(RadA); RadB = qAbs(RadB); }

			if (RadA <= Radius && (qIsNaN(sRad) || RadA < sRad))
			{
				sMatch = i.value().first; sRad = RadA;
			}

			if (RadB <= Radius && (qIsNaN(eRad) || RadB < eRad))
			{
				eMatch = i.value().first; eRad = RadB;
			}
		}

		if (sMatch != QPointF() && sMatch != Start)
		{
			Synchronizer.lock();
			Geom.push_front({ 0, { Start, sMatch } });
			Mods.insert(UID);
			Synchronizer.unlock();
		}

		if (eMatch != QPointF() && eMatch != Stop)
		{
			Synchronizer.lock();
			Geom.push_back({ 0, { Stop, eMatch } });
			Mods.insert(UID);
			Synchronizer.unlock();
		}
	});

	QSqlQuery Insert(Database); Insert.setForwardOnly(true);
	QSqlQuery Line(Database); Line.setForwardOnly(true);

	Insert.prepare("INSERT INTO EW_OB_ELEMENTY (UIDO, IDE, TYP, N) VALUES (?, ?, ?, ?)");

	Line.prepare(
		"INSERT INTO EW_POLYLINE "
			"(ID, P0_X, P0_Y, P1_X, P1_Y, P1_FLAGS, STAN_ZMIANY, ID_WARSTWY, TYP_LINII, MNOZNIK, POINTCOUNT, CREATE_TS, MODIFY_TS) "
		"VALUES (?, ?, ?, ?, ?, 0, 0, ?, ?, 0.0, 2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)");

	emit onBeginProgress(tr("Inserting segments"));
	emit onSetupProgress(0, Mods.size());

	for (const auto& UID : Mods)
	{
		Query.exec(QString("DELETE FROM EW_OB_ELEMENTY WHERE UIDO = %1").arg(UID)); int N(0);

		for (auto& L : Segments[UID])
		{
			if (L.first == 0 && Index.exec() && Index.next())
			{
				L.first = Index.value(0).toInt();

				Line.addBindValue(L.first);
				Line.addBindValue(L.second.x1());
				Line.addBindValue(L.second.y1());
				Line.addBindValue(L.second.x2());
				Line.addBindValue(L.second.y2());
				Line.addBindValue(Styles[UID].first);
				Line.addBindValue((Mode & 0x40) ? 14 : Styles[UID].second);

				Line.exec(); ++Count;
			}

			Insert.addBindValue(UID);
			Insert.addBindValue(L.first);
			Insert.addBindValue(0);
			Insert.addBindValue(N++);

			Insert.exec();
		}

		for (const auto& A : Addons[UID])
		{
			Insert.addBindValue(UID);
			Insert.addBindValue(A.first);
			Insert.addBindValue(A.second);
			Insert.addBindValue(N++);

			Insert.exec();
		}

		emit onUpdateProgress(++Step);
	}

	appendLog("Dociąganie_do_urządzeń", Mods);

	return Count;
}

void DatabaseDriver::getCommon(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onCommonReady(QList<int>()); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, false, 1);
	const QList<int> Used = getCommonFields(Tasks.keys());

	emit onEndProgress();
	emit onCommonReady(Used);
}

void DatabaseDriver::getPreset(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPresetReady(QList<QHash<int, QVariant>>(), QList<int>()); return; }

	const QMap<QString, QSet<int>> Tasks = getClassGroups(Items, false, 1);
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

void DatabaseDriver::getJoins(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onJoinsReady(QHash<QString, QString>(), QHash<QString, QString>(), QHash<QString, QString>()); return; }

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<QString, QString> Points, Lines, Circles; int Step = 0;

	emit onBeginProgress(tr("Preparing classes"));
	emit onSetupProgress(0, Items.size()); Step = 0;

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

	for (const auto& UID : Items)
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

void DatabaseDriver::getClass(const QSet<int>& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onClassReady(QHash<QString, QString>(), QHash<QString, QHash<int, QString>>(), QHash<QString, QHash<int, QString>>(), QHash<QString, QHash<int, QString>>()); return; }

	QSqlQuery QueryA(Database), QueryB(Database), QueryC(Database), QueryD(Database),
			QueryE(Database), QueryF(Database), QueryG(Database), QueryH(Database);

	QHash<QString, QHash<int, QString>> Lines, Points, Texts;
	QHash<QString, QString> Classes;

	const int Type = 8 | 2 | 256; int Step = 0;

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

	emit onBeginProgress(tr("Selecting layers data"));
	emit onSetupProgress(0, Tables.size());  Step = 0;

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


void DatabaseDriver::setDateOverride(bool Override)
{
	QMutexLocker Locker(&Terminator); Dateupdate = Override;
}

void DatabaseDriver::setHistoryMake(bool Make)
{
	QMutexLocker Locker(&Terminator); makeHistory = Make;
}

void DatabaseDriver::unifyJobs(void)
{
	if (!Database.open()) { emit onError(tr("Database is not opened")); emit onJobsUnify(0); return; } int Step(0);

	emit onBeginProgress("Loading jobs");
	emit onSetupProgress(0, 0);

	QSqlQuery Query(Database); Query.setForwardOnly(true);

	QHash<QString, QList<int>> Jobs; int Count(0);

	Query.prepare(
		"SELECT "
			"UID, NUMER "
		"FROM "
			"EW_OPERATY "
		"ORDER BY "
			"OPERACJA DESC");

	if (Query.exec()) while (Query.next())
	{
		Jobs[Query.value(1).toString()].append(Query.value(0).toInt());
	}

	emit onBeginProgress("Updating jobs");
	emit onSetupProgress(0, Jobs.size());

	for (auto& List : Jobs) if (List.size() > 1)
	{
		const int New = List.takeFirst(); QStringList Old;

		for (const auto& ID : List)
		{
			Old.append(QString::number(ID));
		}

		Query.exec(QString("UPDATE EW_OBIEKTY SET OPERAT = %1 WHERE OPERAT IN (%2)")
				 .arg(New).arg(Old.join(',')));
		Query.exec(QString("UPDATE EW_POLYLINE SET OPERAT = %1 WHERE OPERAT IN (%2)")
				 .arg(New).arg(Old.join(',')));
		Query.exec(QString("UPDATE EW_TEXT SET OPERAT = %1 WHERE OPERAT IN (%2)")
				 .arg(New).arg(Old.join(',')));

		Query.exec(QString("DELETE FROM EW_OPERATY WHERE UID IN (%1)").arg(Old.join(',')));
		Query.exec(QString("UPDATE EW_OPERATY SET "
					    "OSOZ = NULL, DTZ = NULL, OPERACJA = 1 "
					    "WHERE UID = %1)").arg(New));

		emit onUpdateProgress(++Step); Count += 1;
	}
	else emit onUpdateProgress(++Step);

	emit onEndProgress();
	emit onJobsUnify(Count);
}

void DatabaseDriver::refactorJobs(const QHash<QString, QString>& Dict)
{
	if (!Database.open()) { emit onError(tr("Database is not opened")); emit onJobsRefactor(0); return; } int Step(0);

	emit onBeginProgress("Loading jobs");
	emit onSetupProgress(0, 0);

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QHash<int, QString> Jobs; int Count(0);

	Query.prepare("SELECT UID, NUMER FROM EW_OPERATY");

	if (Query.exec()) while (Query.next())
	{
		Jobs.insert(Query.value(0).toInt(), Query.value(1).toString());
	}

	emit onBeginProgress("Updating jobs");
	emit onSetupProgress(0, Jobs.size());

	Query.prepare("UPDATE EW_OPERATY O SET O.NUMER = ?, "
			    "O.OPERACJA = (SELECT COALESCE(MAX(N.OPERACJA), 0) FROM EW_OPERATY N WHERE N.NUMER = ?) + 1 "
			    "WHERE O.UID = ?");

	for (auto i = Jobs.constBegin(); i != Jobs.constEnd(); ++i)
	{
		if (Dict.contains(i.value()))
		{
			const auto New = Dict.value(i.value());

			Query.addBindValue(New);
			Query.addBindValue(New);
			Query.addBindValue(i.key());

			if (Query.exec()) ++Count;
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onJobsRefactor(Count);
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

QVariant getDataFromDict(const QVariant& Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type)
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

QVariant getDataByDict(const QVariant& Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type)
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

	if (Dict.isEmpty() || Dict.keys().contains(Value)) return Value;

	for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		if (i.value() == Value) return i.key();
	}

	return 0;
}

bool isVariantEmpty(const QVariant& Value)
{
	if (Value.isNull()) return true;
	else switch (Value.type())
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

double getSurface(const QPolygonF& P)
{
	double sum(0.0); for (int i = 0; i < P.size(); ++i)
	{
		const double yn = (i + 1) == P.size() ? P[0].y() : P[i + 1].y();
		const double yp = i == 0 ? P.last().y() : P[i - 1].y();

		const double xi = P[i].x();

		sum += xi * (yn - yp);
	}

	return qAbs(sum / 2.0);
}

QVariant castVariantTo(const QVariant& Variant, DatabaseDriver::TYPE Type)
{
	if (Variant.type() == QVariant::String) switch (Type)
	{
		case DatabaseDriver::READONLY:
		case DatabaseDriver::STRING:
			return Variant;
		case DatabaseDriver::MASK:
		case DatabaseDriver::INTEGER:
		case DatabaseDriver::SMALLINT:
			return Variant.toInt();
		case DatabaseDriver::BOOL:
			return Variant.toBool();
		case DatabaseDriver::DOUBLE:
			return Variant.toDouble();
		case DatabaseDriver::DATE:
			return castStrToDate(Variant.toString());
		case DatabaseDriver::DATETIME:
			return castStrToDatetime(Variant.toString());
	}
	else return Variant;
}

QDateTime castStrToDatetime(const QString& String)
{
	static const QStringList Formats =
	{
		"d.M.yyyy h:m:s", "d/M/yyyy h:m:s", "yyyy-M-d h:m:s"
	};

	for (const auto& Fmt : Formats)
	{

		QDateTime Date = QDateTime::fromString(String, Fmt);
		if (Date.isValid()) return Date;
	}

	return QDateTime::fromString(String, Qt::DefaultLocaleShortDate);
}

QDate castStrToDate(const QString& String)
{
	static const QStringList Formats =
	{
		"d.M.yyyy", "d/M/yyyy", "yyyy-M-d"
	};

	for (const auto& Fmt : Formats)
	{

		QDate Date = QDate::fromString(String, Fmt);
		if (Date.isValid()) return Date;
	}

	return QDate::fromString(String, Qt::DefaultLocaleShortDate);
}
