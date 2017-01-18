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

QMap<int, DatabaseDriver::FIELD> DatabaseDriver::getFilterList(void) const
{
	QMap<int, FIELD> List; int i = 0;

	for (const auto& Field : Fields)
	{
		if ((Field.Type != INTEGER && Field.Type != MASK) || Field.Dict.size() > 1) List.insert(i, Field); ++i;
	}

	return List;
}

QList<DatabaseDriver::FIELD> DatabaseDriver::loadCommon(bool Emit)
{
	if (!Database.isOpen()) return QList<FIELD>();

	QList<FIELD> Fields =
	{
		{ INTEGER,	"EW_OBIEKTY.OPERAT",	tr("Job name")			},
		{ READONLY,	"EW_OBIEKTY.KOD",		tr("Object code")		},
		{ READONLY,	"EW_OBIEKTY.NUMER",		tr("Object ID")		},
		{ DATE,		"EW_OBIEKTY.DTU",		tr("Creation date")		},
		{ DATE,		"EW_OBIEKTY.DTW",		tr("Modification date")	},
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

QMap<QString, QStringList> DatabaseDriver::getClassGroups(const QList<int>& Indexes, bool Common) const
{
	if (!Database.isOpen()) return QMap<QString, QStringList>();

	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QMap<QString, QStringList> List;

	if (Common) List.insert("EW_OBIEKTY", QStringList());

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

			if (Common) List["EW_OBIEKTY"].append(QString::number(ID));

			List[Table].append(QString::number(ID));
		}
	}

	return List;
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

	const QMap<QString, QStringList> Tasks = getClassGroups(Model->getUids(Items), true);
	const QList<int> Used = Values.keys(); int Step = 0;
	QSqlQuery Query(Database); Query.setForwardOnly(true);
	QStringList All; QMap<int, QVariant> Copy = Values;

	emit onBeginProgress(tr("Updating data"));
	emit onSetupProgress(0, Tasks.size() * 2);

	for (int i = 0; i < Common.size(); ++i) if (Values.contains(i))
	{
		All.append(QString("%1 = '%2'").arg(Common[i].Name).arg(Values[i].toString()));
	}

	if (!All.isEmpty())
	{
		Query.exec(QString(
			"UPDATE "
				"EW_OBIEKTY "
			"SET "
				"%1 "
			"WHERE "
				"UID IN ('%2')")
				 .arg(All.join(", "))
				 .arg(Tasks.first().join("', '")));
	}

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		const auto& Table = getItemByField(Tables, i.key(), &TABLE::Data);
		QStringList Updates;

		for (const auto& Index : Used) if (Table.Indexes.contains(Index))
		{
			Updates.append(QString("%1 = '%2'").arg(Fields[Index].Name).arg(Values[Index].toString()));
		}

		if (Updates.isEmpty()) continue;

		Query.exec(QString(
			"UPDATE "
				"%1 EW_DATA "
			"SET "
				"%2 "
			"WHERE "
				"EW_DATA.UIDO IN ('%3')")
				 .arg(i.key())
				 .arg(Updates.join(", "))
				 .arg(i.value().join("', '")));

		emit onUpdateProgress(++Step);
	}

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
				"EW_OBIEKTY.STATUS = 0  AND EW_OBIEKTY.UID IN ('%3')")
				    .arg(Attribs.join(", "))
				    .arg(Table.Data)
				    .arg(i.value().join("', '")));

		if (Query.exec()) while (Query.next())
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
	}

	emit onEndProgress();
	emit onDataUpdate();
}

void DatabaseDriver::removeData(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onDataRemove(); return; }

	const QMap<QString, QStringList> Tasks = getClassGroups(Model->getUids(Items), true);
	QSqlQuery Query(Database); Query.setForwardOnly(true); int Step = 0;

	emit onBeginProgress(tr("Removing data"));
	emit onSetupProgress(0, Tasks.size());

	const QStringList Main = Tasks.first();

	if (!Main.isEmpty())
	{
		Query.exec(QString(
			"DELETE FROM "
				"EW_OBIEKTY "
			"WHERE "
				"UID IN ('%1')")
				 .arg(Main.join("', '")));

		emit onUpdateProgress(++Step);
	}

	for (auto i = Tasks.constBegin() + 1; i != Tasks.constEnd(); ++i)
	{
		Query.exec(QString(
			"DELETE FROM "
				"%1 "
			"WHERE "
				"UIDO IN ('%2')")
				 .arg(i.key())
				 .arg(i.value().join("', '")));

		emit onUpdateProgress(++Step);
	}

	for (const auto Item : Items) Model->removeItem(Item);

	emit onEndProgress();
	emit onDataRemove();
}

void DatabaseDriver::getPreset(RecordModel* Model, const QModelIndexList& Items)
{
	if (!Database.isOpen()) { emit onError(tr("Database is not opened")); emit onPresetReady(QList<QMap<int, QVariant>>(), QList<int>()); return; }

	const QMap<QString, QStringList> Tasks = getClassGroups(Model->getUids(Items), false);
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
				"%1 "
			"FROM "
				"EW_OBIEKTY "
			"INNER JOIN "
				"%2 EW_DATA "
			"ON "
				"EW_OBIEKTY.UID = EW_DATA.UIDO "
			"WHERE "
				"EW_OBIEKTY.STATUS = 0 AND EW_OBIEKTY.UID IN ('%3')")
					.arg(Attribs.join(", "))
					.arg(i.key())
					.arg(i.value().join("', '")));

		if (Query.exec()) while (Query.next())
		{
			QMap<int, QVariant> Value; int i = 0;

			for (int j = 0; j < Common.size(); ++j)
			{
				Value.insert(j, getDataFromDict(Query.value(i++), Common[j].Dict, Common[j].Type));
			}

			for (int j = 0; j < Table.Headers.size(); ++j)
			{
				Value.insert(Table.Headers[j], getDataFromDict(Query.value(i++), Table.Fields[j].Dict, Table.Fields[j].Type));
			}

			if (!Value.isEmpty()) Values.append(Value);
		}

		emit onUpdateProgress(++Step);
	}

	emit onEndProgress();
	emit onPresetReady(Values, Used);
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
