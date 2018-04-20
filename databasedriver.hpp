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

#include "recordmodel.hpp"
#include "batchwidget.hpp"
#include "copyfieldswidget.hpp"

class DatabaseDriver : public QObject
{

		Q_OBJECT

	public: using SUBOBJECTSTABLE = QList<QPair<QPair<int, QString>, QPair<int, QString>>>;

	public: struct POINT
	{
		int IDE;

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

		bool Missing;

		QMap<QVariant, QString> Dict;
	};

	public: struct TABLE
	{
		QString Name;
		QString Label;
		QString Data;

		bool Point;
		int Type;

		QList<FIELD> Fields;
		QList<int> Indexes;
		QList<int> Headers;
	};

	public: struct OBJECT
	{
		int UID, ID;
		QString Class;
		int Type;

		QVariant Geometry;

		unsigned Mask = 0;
	};

	public: struct REDACTION
	{
		int UID, ID, Type;

		QVariant Format;

		double Angle;
		int Just;
	};

	protected:

		mutable QMutex Terminator;
		mutable bool Terminated;
		mutable bool Dateupdate;

		unsigned maxBindedSize;

		QSqlDatabase Database;
		QStringList Headers;

		QList<TABLE> Tables;
		QList<FIELD> Fields;
		QList<FIELD> Common;

		QHash<QString, QSet<QString>> Variables;

	public:

		static const QStringList Operators;

		explicit DatabaseDriver(QObject* Parent = nullptr);
		virtual ~DatabaseDriver(void) override;

		QString getDatabaseName(void) const;
		QString getDatabasePath(void) const;

		bool isTerminated(void) const;

		static QStringList nullReasons(void);

	protected:

		QList<FIELD> loadCommon(bool Emit = false);
		QList<TABLE> loadTables(bool Emit = false);

		QList<FIELD> loadFields(const QString& Table) const;
		QMap<QVariant, QString> loadDict(const QString& Field, const QString& Table) const;
		QHash<QString, QSet<QString>> loadVariables(void) const;

		QList<FIELD> normalizeFields(QList<TABLE>& Tabs, const QList<FIELD>& Base) const;
		QStringList normalizeHeaders(QList<TABLE>& Tabs, const QList<FIELD>& Base) const;

		QMap<QString, QSet<int>> getClassGroups(const QSet<int>& Indexes,
										bool Common, int Index);

		QHash<int, QHash<int, QVariant>> loadData(const TABLE& Table,
										  const QSet<int>& Filter,
										  const QString& Where,
										  bool Dict, bool View);

		QHash<int, QHash<int, QVariant>> filterData(const QHash<int, QHash<int, QVariant>>& Data,
										    const QHash<int, QVariant>& Geometry,
										    const QHash<int, QVariant>& Redaction,
										    const QString& Limiter, double Radius);

		void performDataUpdates(const QMap<QString, QSet<int>> Tasklist,
						    const QSet<int>& Items,
						    const QHash<int, QVariant>& Values,
						    const QHash<int, int>& Reasons, bool Emit);

		QSet<int> performBatchUpdates(const QSet<int>& Items,
								const QList<BatchWidget::RECORD>& Functions,
								const QList<QStringList>& Values);

		QList<int> getUsedFields(const QString& Filter) const;
		QList<int> getCommonFields(const QStringList& Classes) const;

		QHash<int, QSet<int>> joinSurfaces(const QHash<int, QSet<int>>& Geometry,
									const QList<DatabaseDriver::POINT>& Points,
									const QSet<int>& Tasks, const QString& Class,
									double Radius = 0.0);
		QHash<int, QSet<int>> joinLines(const QHash<int, QSet<int>>& Geometry,
								  const QList<DatabaseDriver::POINT>& Points,
								  const QSet<int>& Tasks, const QString& Class,
								  double Radius = 0.0);
		QHash<int, QSet<int>> joinPoints(const QHash<int, QSet<int>>& Geometry,
								   const QList<DatabaseDriver::POINT>& Points,
								   const QSet<int>& Tasks, const QString& Class,
								   double Radius = 0.0);

		void convertSurfaceToPoint(const QSet<int>& Objects, const QString& Symbol, int Layer);
		void convertPointToSurface(const QSet<int>& Objects, int Style, int Layer, double Radius);

		void convertSurfaceToLine(const QSet<int>& Objects);
		void convertLineToSurface(const QSet<int>& Objects);

		QList<OBJECT> loadGeometry(const QSet<int>& Limiter = QSet<int>());
		QList<REDACTION> loadRedaction(const QSet<int>& Limiter = QSet<int>());
		SUBOBJECTSTABLE loadSubobjects(void);

		QSet<int> filterDataByLength(const QList<OBJECT>& Data, double Minimum, double Maximum, int Count = 0);
		QSet<int> filterDataBySurface(const QList<OBJECT>& Data, double Minimum, double Maximum, int Count = 0);

		QSet<int> filterDataByIspartof(const QList<OBJECT>& Data, double Radius = 0.001, bool Not = false, int Count = 0);
		QSet<int> filterDataByContaining(const QList<OBJECT>& Data, double Radius = 0.001, bool Not = false, int Count = 0);
		QSet<int> filterDataByEndswith(const QList<OBJECT>& Data, double Radius = 0.001, bool Not = false, int Count = 0);
		QSet<int> filterDataByIsnear(const QList<OBJECT>& Data, double Radius = 0.001, bool Not = false, int Count = 0);

		QSet<int> filterDataByIsSubobject(const QSet<int>& Data, const QSet<int>& Objects,
								    const SUBOBJECTSTABLE& Table, bool Not = false);
		QSet<int> filterDataByHasSubobject(const QSet<int>& Data, const QSet<int>& Objects,
									const SUBOBJECTSTABLE& Table, bool Not = false);

		QSet<int> filterDataBySymbolAngle(const QList<REDACTION>& Data, double Minimum, double Maximum);
		QSet<int> filterDataByLabelAngle(const QList<REDACTION>& Data, double Minimum, double Maximum);

		QSet<int> filterDataBySymbolText(const QList<REDACTION>& Data, const QStringList& Text, bool Not = false);
		QSet<int> filterDataByLabelText(const QList<REDACTION>& Data, const QStringList& Text, bool Not = false);
		QSet<int> filterDataByLineStyle(const QList<REDACTION>& Data, const QStringList& Style, bool Not = false);

		QSet<int> filterDataByLabelStyle(const QList<REDACTION>& Data, int Style, bool Not = false);

		QSet<int> filterDataByHasGeoemetry(const QSet<int>& Data, const QSet<int>& Types);
		QSet<int> filterDataByHasMulrel(const QSet<int>& Data);

		int insertBreakpoints(const QSet<int> Tasks, int Mode, double Radius);

		bool hasAllIndexes(const TABLE& Tab, const QList<int>& Used);

		void updateModDate(const QSet<int>& Objects, int Type = 0);

	public slots:

		bool openDatabase(const QString& Server, const QString& Base,
					   const QString& User, const QString& Pass);

		bool closeDatabase(void);

		void loadList(const QStringList& Filter, int Index, int Action,
				    const RecordModel* Current = nullptr,
				    const QSet<int>& Items = QSet<int>());
		void reloadData(const QString& Filter, QList<int> Used,
					 const QHash<int, QVariant>& Geometry,
					 const QHash<int, QVariant>& Redaction,
					 const QString& Limiter, double Radius,
					 int Mode, const RecordModel* Current = nullptr,
					 const QSet<int>& Items = QSet<int>());
		void updateData(const QSet<int>& Items,
					 const QHash<int, QVariant>& Values,
					 const QHash<int, int>& Reasons);
		void removeData(const QSet<int>& Items);

		void execBatch(const QSet<int>& Items,
					const QList<BatchWidget::RECORD>& Functions,
					const QList<QStringList>& Values);

		void execFieldcopy(const QSet<int>& Items, const QList<CopyfieldsWidget::RECORD>& Functions, bool Nulls);

		void splitData(const QSet<int>& Items, const QString& Point, const QString& From, int Type);

		void joinData(const QSet<int>& Items, const QString& Point, const QString& Join,
				    bool Override, int Type, double Radius);

		void mergeData(const QSet<int>& Items, const QList<int>& Values, const QStringList& Points);

		void cutData(const QSet<int>& Items, const QStringList& Points, bool Endings);

		void refactorData(const QSet<int>& Items, const QString& Class, int Line, int Point, int Text,
					   const QString& Symbol, int Style, const QString& Label, int Actions, double Radius);

		void copyData(const QSet<int>& Items, const QString& Class,
				    int Line, int Point, int Text,
				    const QString& Symbol, int Style);

		void fitData(const QSet<int>& Items, const QString& Path,
				   bool Points, int X1, int Y1, int X2, int Y2,
				   double Radius, double Length, bool Endings);

		void restoreJob(const QSet<int>& Items);

		void removeHistory(const QSet<int>& Items);

		void editText(const QSet<int>& Items, bool Move, int Justify,
				    bool Rotate, bool Sort, double Length);

		void insertLabel(const QSet<int>& Items, const QString& Label,
					  int J, double X, double Y, bool P, double L, double R);

		void removeLabel(const QSet<int>& Items);

		void editLabel(const QSet<int>& Items, const QString& Label,
					int Underline, int Pointer, double Rotation);

		void insertPoints(const QSet<int>& Items, int Mode, double Radius, bool Recursive);

		void updateKergs(const QSet<int>& Items, const QString& Path, int Action, int Elements);

		void getCommon(const QSet<int>& Items);
		void getPreset(const QSet<int>& Items);
		void getJoins(const QSet<int>& Items);
		void getClass(const QSet<int>& Items);

		bool addInterface(const QString& Path, int Type, bool Modal);

		void setDateOverride(bool Override);

		void unterminate(void);
		void terminate(void);

	signals:

		void onError(const QString&);

		void onConnect(const QList<FIELD>&, const QList<TABLE>&,
					const QStringList&, unsigned,
					const QHash<QString, QSet<QString>>&);
		void onDisconnect(void);
		void onLogin(bool);

		void onBeginProgress(const QString&);
		void onSetupProgress(int, int);
		void onUpdateProgress(int);
		void onEndProgress(void);

		void onDataLoad(RecordModel*);
		void onDataRemove(void);
		void onDataUpdate(void);
		void onDataJoin(int);
		void onDataSplit(int);
		void onDataRefactor(int);
		void onDataCopy(int);
		void onDataFit(int);

		void onBatchExec(int);
		void onCopyExec(int);

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

		void onRowsUpdate(const QHash<int, QHash<int, QVariant>>&);
		void onRowsRemove(const QSet<int>&);

		void onRowUpdate(int, const QHash<int, QVariant>&);
		void onRowRemove(int);

		void onJobsRestore(int);
		void onHistoryRemove(int);
		void onKergUpdate(int);

		void onDataMerge(int);
		void onDataCut(int);

		void onTextEdit(int);
		void onLabelEdit(int);

		void onLabelInsert(int);
		void onPointInsert(int);
		void onLabelDelete(int);

};

bool operator == (const DatabaseDriver::FIELD& One, const DatabaseDriver::FIELD& Two);
bool operator == (const DatabaseDriver::TABLE& One, const DatabaseDriver::TABLE& Two);

QVariant getDataFromDict(const QVariant& Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type);
QVariant getDataByDict(const QVariant& Value, const QMap<QVariant, QString>& Dict, DatabaseDriver::TYPE Type);

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
bool hasItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

bool pointComp(const QPointF& A, const QPointF& B, double d = 0.001);

bool isVariantEmpty(const QVariant& Value);

double getSurface(const QPolygonF& P);

#endif // DATABASEDRIVER_HPP
