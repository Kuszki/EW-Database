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

	private:

		QList<QPair<QString, QString>> Attributes;

		QSqlDatabase Database;

		QString Dictionary;

		QStringList getAttribTables(void);
		QString getAttribTable(int ID);

		QStringList getTableFields(const QString& Table, bool Write = false);
		QStringList getValuesFields(const QString& Values);
		QStringList getQueryFields(QStringList All, const QStringList& Table);

		QStringList getDataQueries(const QStringList& Tables, const QString& Values = QString());
		QStringList getUpdateQueries(const QList<int>& Indexes, const QHash<QString, QString>& Values);
		QStringList getRemoveQueries(const QList<int>& Indexes);

		QHash<int, QHash<int, QString>> indexDictionary(void);

		bool checkFieldsInQuery(const QStringList& Used, const QStringList& Table) const;

	public:

		static const QList<QPair<QString, QString>> commonAttribs;
		static const QList<QPair<QString, QString>> writeAttribs;
		static const QHash<QString, QString> writeBridges;
		static const QHash<QString, QString> dictQueries;
		static const QStringList fieldOperators;

		explicit DatabaseDriver(QObject* Parent = nullptr);
		virtual ~DatabaseDriver(void) override;

		QList<QPair<QString, QString>> getAttributes(const QStringList& Keys = QStringList());
		QList<QPair<QString, QString>> getAttributes(const QString& Key);

		QList<QPair<QString, QString>> allAttributes(bool Write = false);

		QHash<int, QString> getDictionary(const QString& Field) const;

		QHash<QString, QHash<int, QString>> allDictionary(void) const;

		QHash<QString, QString> getEditValues(RecordModel* Model, const QModelIndex& Index);

	public slots:

		bool openDatabase(const QString& Server,
					   const QString& Base,
					   const QString& User,
					   const QString& Pass);

		bool closeDatabase(void);

		void updateData(const QString& Filter);

		void setData(RecordModel* Model, const QModelIndexList& Items, const QHash<QString, QString>& Values);

		void removeData(RecordModel* Model, const QModelIndexList& Items);

	signals:

		void onDataLoad(RecordModel*);
		void onDataUpdate(RecordModel*);
		void onDataRemove(RecordModel*);

		void onAttributesLoad(const QList<QPair<QString, QString>>&);
		void onError(const QString&);

		void onConnect(void);
		void onDisconnect(void);

		void onBeginProgress(void);
		void onSetupProgress(int, int);
		void onUpdateProgress(int);
		void onEndProgress(void);

};

#endif // DATABASEDRIVER_HPP
