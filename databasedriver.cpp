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
	{"EW_OBIEKTY.UID",			tr("Database ID")},
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

const QStringList DatabaseDriver::readAttribs =
{
	"EW_OBIEKTY.UID", "EW_OBIEKTY.KOD", "EW_OBIEKTY.NUMER", "EW_OPERATY.NUMER", "EW_OB_OPISY.OPIS"
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

	QStringList List = commonAttribs.keys();
	QSqlQuery Query(Database);

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

QStringList DatabaseDriver::getDataQueries(const QStringList& Tables, const QString& Values)
{
	QStringList Queries; static QStringList Keys = commonAttribs.keys();

	for (const auto& Table : Tables)
	{
		const QStringList Fields = getTableFields(Table);
		const QStringList Used = getValuesFields(Values);

		if (!checkFieldsInQuery(Used, Fields)) continue;

		Queries.append(QString(
			"SELECT "
				"%1, %2 "
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
				"%3 D "
			"ON "
				"EW_OBIEKTY.UID=D.UIDO")
					.arg(Keys.join(", "))
					.arg(Fields.join(", "))
					.arg(Table));

		if (!Values.isEmpty()) Queries.last().append(" WHERE ").append(Values);
	}

	return Queries;
}

bool DatabaseDriver::checkFieldsInQuery(const QStringList& Used, const QStringList& Table) const
{
	for (const auto Field : Used)
	{
		if (!Table.contains(Field) && !commonAttribs.contains(Field)) return false;
	}

	return true;
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

		if (Query.exec(Text)) while (Query.next())
			Res.insert(QString("D.%1").arg(Query.value(0).toString()), Query.value(1).toString());
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

void DatabaseDriver::updateData(const QString& Filter)
{
	const QMap<QString, QString> Attribs = getAttributes();

	QMap<QString, QString> Fields = commonAttribs;
	QList<QPair<QString, QStringList>> Pairs;

	for (auto i = Attribs.constBegin(); i != Attribs.constEnd(); ++i)
	{
		Fields.insert(i.key(), i.value());
	}

	for (const auto& Table : getAttribTables())
	{
		const QStringList Current = QStringList() << Table;
		QPair<QString, QStringList> Pair;

		Pair.first = getDataQueries(Current, Filter).first();
		Pair.second = getAttributes(Current).keys();

		Pairs.append(Pair);
	}

	const auto Keys = Fields.keys();

	RecordModel* Model = new RecordModel(Fields, this);

	for (const auto& Pair : Pairs)
	{
		QList<QMap<int, QVariant>> Items;
		QSqlQuery Query(Database);

		if (Query.exec(Pair.first)) while (Query.next())
		{
			QMap<int, QVariant> Object;

			int i = 0; for (const auto& Field : Pair.second)
			{
				Object.insert(Keys.indexOf(Field), Query.value(i++));
			}

			Items.append(Object);
		}
		else qDebug() << Query.lastQuery() << Query.lastError().text();

		Model->addItems(Items);
	}

	emit onDataLoad(Model);
}
