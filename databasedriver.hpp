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

#ifndef DATABASEDRIVER_HPP
#define DATABASEDRIVER_HPP

#include <QSharedDataPointer>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QPolygonF>
#include <QSettings>
#include <QVariant>
#include <QObject>
#include <QHash>

#include <QtConcurrent>

#include <QDebug>

#include "recordmodel.hpp"
#include "batchwidget.hpp"

class DatabaseDriver : public QObject
{

		Q_OBJECT

	public: struct POINT
	{
		int ID;

		double X;
		double Y;
	};

	public: enum TYPE
	{
		READONLY	= 0,
		STRING	= 1,
		INTEGER	= 4,
		SMALLINT	= 5,
		BOOL		= 7,
		DOUBLE	= 8,
		DATE		= 101,
		MASK		= 102,
		DATETIME	= 1000
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

		bool Point;

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

		QMap<QString, QList<int>> getClassGroups(const QList<int>& Indexes,
										  bool Common, int Index);

		QHash<int, QHash<int, QVariant>> loadData(const TABLE& Table,
										  const QList<int>& Filter,
										  const QString& Where,
										  bool Dict, bool View);

		QHash<int, QHash<int, QVariant>> filterData(QHash<int, QHash<int, QVariant>> Data,
										    const QHash<int, QVariant>& Geometry,
										    const QString& Limiter);

		QList<int> getUsedFields(const QString& Filter) const;
		QList<int> getCommonFields(const QStringList& Classes) const;

		QHash<int, QSet<int>> joinSurfaces(const QHash<int, QSet<int>>& Geometry,
									const QList<DatabaseDriver::POINT>& Points,
									const QList<int>& Tasks, const QString Class,
									double Radius = 0.0);
		QHash<int, QSet<int>> joinLines(const QHash<int, QSet<int>>& Geometry,
								  const QList<DatabaseDriver::POINT>& Points,
								  const QList<int>& Tasks, const QString Class,
								  double Radius = 0.0);
		QHash<int, QSet<int>> joinPoints(const QHash<int, QSet<int>>& Geometry,
								   const QList<DatabaseDriver::POINT>& Points,
								   const QList<int>& Tasks, const QString Class,
								   double Radius = 0.0);

		bool hasAllIndexes(const TABLE& Tab, const QList<int>& Used);

	public slots:

		bool openDatabase(const QString& Server, const QString& Base,
					   const QString& User, const QString& Pass);

		bool closeDatabase(void);

		void loadList(const QStringList& Filter);
		void reloadData(const QString& Filter, QList<int> Used,
					 const QHash<int, QVariant>& Geometry,
					 const QString& Limiter);
		void updateData(RecordModel* Model, const QModelIndexList& Items,
					 const QHash<int, QVariant>& Values, bool Emit = true);
		void removeData(RecordModel* Model, const QModelIndexList& Items);

		void execBatch(RecordModel* Model, const QModelIndexList& Items,
					const QList<QPair<int, BatchWidget::FUNCTION>>& Functions,
					const QList<QStringList>& Values);

		void splitData(RecordModel* Model, const QModelIndexList& Items,
					const QString& Point, const QString& From, int Type);

		void joinData(RecordModel* Model, const QModelIndexList& Items,
				    const QString& Point, const QString& Join,
				    bool Override, int Type, double Radius);

		void mergeData(RecordModel* Model, const QModelIndexList& Items,
					const QList<int>& Values, const QStringList& Points);

		void cutData(RecordModel* Model, const QModelIndexList& Items,
				   const QStringList& Points, bool Endings);

		void refactorData(RecordModel* Model, const QModelIndexList& Items,
					   const QString& Class, int Line, int Point, int Text);

		void restoreJob(RecordModel* Model, const QModelIndexList& Items);

		void removeHistory(RecordModel* Model, const QModelIndexList& Items);

		void editText(RecordModel* Model, const QModelIndexList& Items,
				    bool Move, bool Justify, bool Rotate, bool Sort, double Length);

		void getCommon(RecordModel* Model, const QModelIndexList& Items);
		void getPreset(RecordModel* Model, const QModelIndexList& Items);
		void getJoins(RecordModel* Model, const QModelIndexList& Items);
		void getClass(RecordModel* Model, const QModelIndexList& Items);

	signals:

		void onError(const QString&);

		void onConnect(const QList<FIELD>&, const QList<TABLE>&,
					const QStringList&, unsigned);
		void onDisconnect(void);
		void onLogin(bool);

		void onBeginProgress(const QString&);
		void onSetupProgress(int, int);
		void onUpdateProgress(int);
		void onEndProgress(void);

		void onDataLoad(RecordModel*);
		void onDataRemove(RecordModel*);
		void onDataUpdate(RecordModel*);
		void onDataJoin(int);
		void onDataSplit(int);
		void onDataRefactor(void);

		void onBatchExec(int);

		void onCommonReady(const QList<int>&);
		void onPresetReady(const QList<QHash<int, QVariant>>&,
					    const QList<int>&);
		void onJoinsReady(const QHash<QString, QString>&,
					   const QHash<QString, QString>&,
					   const QHash<QString, QString>&);
		void onClassReady(const QHash<QString, QString>&,
					   const QHash<QString, QHash<int, QString>>&,
					   const QHash<QString, QHash<int, QString>>&,
					   const QHash<QString, QHash<int, QString>>&);

		void onRowUpdate(int, const QHash<int, QVariant>&);
		void onRowRemove(const QModelIndex&);

		void onJobsRestore(int);
		void onHistoryRemove(int);

		void onDataMerge(int);
		void onDataCut(int);

		void onTextEdit(int);

};

bool operator == (const DatabaseDriver::FIELD& One, const DatabaseDriver::FIELD& Two);
bool operator == (const DatabaseDriver::TABLE& One, const DatabaseDriver::TABLE& Two);

QVariant getDataFromDict(QVariant Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type);
QVariant getDataByDict(QVariant Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type);

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
bool hasItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

#endif // DATABASEDRIVER_HPP
