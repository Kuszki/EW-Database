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

#ifndef DATABASEDRIVER_V2_HPP
#define DATABASEDRIVER_V2_HPP

#include <QSharedDataPointer>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QObject>
#include <QHash>

#include <QDebug>

#include "recordmodel.hpp"

class DatabaseDriver_v2 : public QObject
{

		Q_OBJECT

	public: enum TYPE
	{
		READONLY	= 0,
		STRING	= 1,
		FOREGIN	= 4,
		INT		= 5,
		BOOL		= 7,
		DOUBLE	= 8,
		DATE		= 101,
		MASK		= 102
	};

	public: struct FIELD
	{
		TYPE Type;

		QString Name;
		QString Label;

		QMap<int, QString> Dict;
	};

	public: struct TABLE
	{
		QString Name;
		QString Label;
		QString Data;

		QList<FIELD> Fields;
		QList<int> Indexes;
		QList<int> Headers;
	};

	private:

		QSqlDatabase Database;
		QStringList Headers;

		QList<TABLE> Tables;
		QList<FIELD> Fields;
		QList<FIELD> Common;

	public:

		explicit DatabaseDriver_v2(QObject* Parent = nullptr);
		virtual ~DatabaseDriver_v2(void) override;

		QMap<int, QStringList> getAttribList(void) const;
		QMap<int, FIELD> getFilterList(void) const;

	protected:

		QList<FIELD> loadCommon(void) const;
		QList<TABLE> loadTables(void) const;

		QList<FIELD> loadFields(const QString& Table) const;
		QMap<int, QString> loadDict(const QString& Field, const QString& Table) const;

		QList<FIELD> normalizeFields(QList<TABLE>& Tabs, const QList<FIELD>& Base) const;
		QStringList normalizeHeaders(QList<TABLE>& Tabs, const QList<FIELD>& Base) const;

	public slots:

		bool openDatabase(const QString& Server,
					   const QString& Base,
					   const QString& User,
					   const QString& Pass);

		bool closeDatabase(void);

	signals:

		void onError(const QString&);

		void onConnect(const QMap<int, FIELD>&, const QMap<int, QStringList>&, const QStringList&);
		void onDisconnect(void);

		void onBeginProgress(const QString&);
		void onSetupProgress(int, int);
		void onUpdateProgress(int);
		void onEndProgress(void);

};

bool operator == (const DatabaseDriver_v2::FIELD& One, const DatabaseDriver_v2::FIELD& Two);
bool operator == (const DatabaseDriver_v2::TABLE& One, const DatabaseDriver_v2::TABLE& Two);

DatabaseDriver_v2::FIELD& getFieldByName(QList<DatabaseDriver_v2::FIELD>& Fields, const QString& Name);

#endif // DATABASEDRIVER_V2_HPP
