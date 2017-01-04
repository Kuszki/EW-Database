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

const QList<QPair<QString, QString>> DatabaseDriver::commonAttribs =
{
	{ "EW_OBIEKTY.UID",			tr("Database ID") },
	{ "EW_OBIEKTY.KOD",			tr("Object code") },
	{ "EW_OBIEKTY.NUMER",		tr("Object ID") },
	{ "EW_OBIEKTY.POZYSKANIE",	tr("Source of data") },
	{ "EW_OBIEKTY.DTU",			tr("Creation date") },
	{ "EW_OBIEKTY.DTW",			tr("Modification date") },
	{ "EW_OBIEKTY.DTR",			tr("Delete date") },
	{ "EW_OBIEKTY.STATUS",		tr("Object status") },
	{ "EW_OPERATY.NUMER",		tr("Job name") },
	{ "EW_OB_OPISY.OPIS",		tr("Code description") }
};

const QList<QPair<QString, QString>> DatabaseDriver::writeAttribs =
{
	{ "EW_OBIEKTY.KOD",			tr("Object code") },
	{ "EW_OBIEKTY.POZYSKANIE",	tr("Source of data") },
	{ "EW_OBIEKTY.DTU",			tr("Creation date") },
	{ "EW_OBIEKTY.DTW",			tr("Modification date") },
	{ "EW_OBIEKTY.DTR",			tr("Delete date") },
	{ "EW_OBIEKTY.STATUS",		tr("Object status") },
	{ "EW_OBIEKTY.OPERAT",		tr("Job name") }
};

const QStringList DatabaseDriver::fieldOperators =
{
	"=", "<>", ">=", ">", "<=", "<",
	"LIKE", "NOT LIKE",
	"IS", "IS NOT",
	"IN", "NOT IN"
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

	QSqlQuery Query(Database);
	QStringList List;

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

	for (const auto& Field : commonAttribs) List.append(Field.first);

	if (Query.exec()) while (Query.next())
	{
		List.append(QString("D.%1").arg(Query.value(0).toString()));
	}

	return List;
}

QStringList DatabaseDriver::getValuesFields(const QString& Values)
{
	if (Values.isEmpty()) return QStringList(); QStringList Fields;

	QRegExp Exp(QString("\\b(\\S+)\\b\\s*(?:%1)").arg(fieldOperators.join('|')));

	int i = 0; while ((i = Exp.indexIn(Values, i)) != -1)
	{
		Fields.append(Exp.capturedTexts().last());

		i += Exp.matchedLength();
	}

	return Fields;
}

QStringList DatabaseDriver::getQueryFields(QStringList All, const QStringList& Table)
{
	for (auto& Field : All) if (!Table.contains(Field)) Field = "NULL"; return All;
}

QStringList DatabaseDriver::getDataQueries(const QStringList& Tables, const QString& Values)
{
	QStringList Queries, Attribs;

	for (const auto& Field : allAttributes()) Attribs.append(Field.first);

	for (const auto& Table : Tables)
	{
		const QStringList Fields = getTableFields(Table);
		const QStringList Used = getValuesFields(Values);

		if (!checkFieldsInQuery(Used, Fields)) continue;

		auto allFields = getQueryFields(Attribs, Fields);

		Queries.append(QString(
			"SELECT "
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
				"%2 D "
			"ON "
				"EW_OBIEKTY.UID=D.UIDO")
					.arg(allFields.join(", "))
					.arg(Table));

		if (!Values.isEmpty()) Queries.last().append(" WHERE ").append(Values);
	}

	return Queries;
}

QHash<int, QHash<int, QString>> DatabaseDriver::indexDictionary(void)
{
	const auto Attribs = allAttributes();
	const auto Data = allDictionary();

	QHash<int, QHash<int, QString>> Result;

	for (auto i = Data.constBegin(); i != Data.constEnd(); ++i) for (int j = 0; j < Attribs.size(); ++j)
	{
		if (Attribs[j].first == i.key()) Result.insert(j, i.value());
	}

	return Result;
}

bool DatabaseDriver::checkFieldsInQuery(const QStringList& Used, const QStringList& Table) const
{
	QStringList Common; for (const auto& Field : commonAttribs) Common.append(Field.first);

	for (const auto Field : Used)
	{
		if (!Table.contains(Field) && !Common.contains(Field)) return false;
	}

	return true;
}

DatabaseDriver::DatabaseDriver(QObject* Parent)
: QObject(Parent)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Database = QSqlDatabase::addDatabase(Settings.value("driver", "QIBASE").toString());
	Settings.endGroup();

	// TODO add basic dictionary to resources and copy if not exists
	Settings.beginGroup("Columns");
	Dictionary = Settings.value("dictionary", "Dictionary.ini").toString();
	Settings.endGroup();
}

DatabaseDriver::~DatabaseDriver(void) {}

QList<QPair<QString, QString>> DatabaseDriver::getAttributes(const QStringList& Keys)
{
	QList<QPair<QString, QString>> Res;

	if (Database.isOpen())
	{
		QSqlQuery Query(Database); QString Text = "SELECT DISTINCT NAZWA, TYTUL FROM EW_OB_DDSTR";

		if (!Keys.isEmpty()) Text.append(QString(" WHERE KOD IN ('%1')").arg(Keys.join("','")));

		if (Query.exec(Text)) while (Query.next()) Res.append(qMakePair(
											QString("D.%1").arg(Query.value(0).toString()),
											Query.value(1).toString()));
	}

	return Res;
}

QList<QPair<QString, QString>> DatabaseDriver::getAttributes(const QString& Key)
{
	QList<QPair<QString, QString>> Res;

	if (Database.isOpen())
	{
		QSqlQuery Query(Database);

		Query.prepare(QString(
			"SELECT DISTINCT "
				"NAZWA, "
				"TYTUL "
			"FROM "
				"EW_OB_DDSTR "
			"WHERE "
				"KOD='%1'")
				    .arg(Key));

		if (Query.exec()) while (Query.next()) Res.append(qMakePair(
										QString("D.%1").arg(Query.value(0).toString()),
										Query.value(1).toString()));
	}

	return Res;
}

QList<QPair<QString, QString>> DatabaseDriver::allAttributes(bool Write)
{
	auto Attrib = Write ? writeAttribs : commonAttribs; Attrib.append(getAttributes()); return Attrib;
}

QHash<int, QString> DatabaseDriver::getDictionary(const QString& Field) const
{
	QSettings Settings(Dictionary, QSettings::IniFormat);
	QHash<int, QString> Result;

	if (Settings.contains(Field))
	{
		Settings.beginGroup(Field);

		for (const QString& Key : Settings.childKeys())
		{
			bool OK = false; const int ID = Key.toInt(&OK);
			if (OK) Result.insert(ID, Settings.value(Key).toString());
		}

		Settings.endGroup();
	}

	return Result;
}

QHash<QString, QHash<int, QString>> DatabaseDriver::allDictionary(void) const
{
	QSettings Settings(Dictionary, QSettings::IniFormat);
	QHash<QString, QHash<int, QString>> Result;

	for (const auto& Field : Settings.childGroups())
	{
		QHash<int, QString> Group;

		Settings.beginGroup(Field);

		for (const QString& Key : Settings.childKeys())
		{
			bool OK = false; const int ID = Key.toInt(&OK);
			if (OK) Group.insert(ID, Settings.value(Key).toString());
		}

		Settings.endGroup();

		Result.insert(Field, Group);
	}

	return Result;
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

void DatabaseDriver::updateData(const QString& Filter)
{
	if (!Database.isOpen()) return;

	const auto Queries = getDataQueries(getAttribTables(), Filter);
	const auto Aliases = indexDictionary();
	const auto Fields = allAttributes();
	const int Size = Fields.size();

	RecordModel* Model = new RecordModel(Fields, this);
	int Progress = 0;

	emit onSetupProgress(0, Queries.size());
	emit onBeginProgress();

	for (const auto& Request : Queries)
	{
		QList<QList<QPair<int, QVariant>>> Objects;
		QSqlQuery Query(Database);

		if (Query.exec(Request)) while (Query.next())
		{
			QList<QPair<int, QVariant>> Object;

			for (int i = 0; i < Size; ++i)
			{
				QVariant Value = Query.value(i);

				if (Value.isValid() && !Value.isNull())
				{
					if (Aliases.contains(i) && Aliases[i].contains(Value.toInt()))
					{
						Object.append(qMakePair(i, Aliases[i][Value.toInt()]));
					}
					else Object.append(qMakePair(i, Value));
				}
			}

			if (!Object.isEmpty()) Objects.append(Object);
		}

		Model->addItems(Objects); emit onUpdateProgress(++Progress);
	}

	emit onEndProgress();
	emit onDataLoad(Model);
}
