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

class DatabaseDriver : public QObject
{

		Q_OBJECT

	public: enum TYPE
	{
		READONLY	= 0,
		STRING	= 1,
		INTEGER	= 4,
		SMALLINT	= 5,
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

		QMap<QVariant, QString> Dict;
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

		static const QStringList Operators;

		explicit DatabaseDriver(QObject* Parent = nullptr);
		virtual ~DatabaseDriver(void) override;

	protected:

		QList<FIELD> loadCommon(bool Emit = false);
		QList<TABLE> loadTables(bool Emit = false);

		QList<FIELD> loadFields(const QString& Table) const;
		QMap<QVariant, QString> loadDict(const QString& Field, const QString& Table) const;

		QList<FIELD> normalizeFields(QList<TABLE>& Tabs, const QList<FIELD>& Base) const;
		QStringList normalizeHeaders(QList<TABLE>& Tabs, const QList<FIELD>& Base) const;

		QMap<QString, QStringList> getClassGroups(const QList<int>& Indexes, bool Common) const;

		QList<int> getUsedFields(const QString& Filter) const;
		QList<int> getCommonFields(const QStringList& Classes) const;

		bool hasAllIndexes(const TABLE& Tab, const QList<int>& Used);

	public slots:

		bool openDatabase(const QString& Server, const QString& Base,
					   const QString& User, const QString& Pass);

		bool closeDatabase(void);

		void reloadData(const QString& Filter, QList<int> Used = QList<int>());
		void updateData(RecordModel* Model, const QModelIndexList& Items, const QMap<int, QVariant>& Values);
		void removeData(RecordModel* Model, const QModelIndexList& Items);

		void getPreset(RecordModel* Model, const QModelIndexList& Items);

	signals:

		void onError(const QString&);

		void onConnect(const QList<FIELD>&, const QList<TABLE>&, const QStringList&, unsigned);
		void onDisconnect(void);
		void onLogin(bool);

		void onBeginProgress(const QString&);
		void onSetupProgress(int, int);
		void onUpdateProgress(int);
		void onEndProgress(void);

		void onDataLoad(RecordModel*);
		void onDataRemove(void);
		void onDataUpdate(void);

		void onPresetReady(const QList<QMap<int, QVariant>>&, const QList<int>&);

};

bool operator == (const DatabaseDriver::FIELD& One, const DatabaseDriver::FIELD& Two);
bool operator == (const DatabaseDriver::TABLE& One, const DatabaseDriver::TABLE& Two);

QVariant getDataFromDict(QVariant Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type);

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

#endif // DATABASEDRIVER_V2_HPP
