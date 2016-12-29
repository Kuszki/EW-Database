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
#include <QMap>

#include <QDebug>

#include "recordmodel.hpp"

class DatabaseDriver : public QObject
{

		Q_OBJECT

	private:

		QMap<QString, QString> Attributes;
		QMap<QString, int> Indexes;

		QSqlDatabase Database;

		QStringList getAttribTables(void);

		QStringList getTableFields(const QString& Table);
		QStringList getValuesFields(const QString& Values);
		QStringList getQueryFields(QStringList All, const QStringList& Table);

		QStringList getDataQueries(const QStringList& Tables, const QString& Values = QString());

		bool checkFieldsInQuery(const QStringList& Used, const QStringList& Table) const;

	public:

		static const QMap<QString, QString> commonAttribs;
		static const QStringList fieldOperators;
		static const QStringList readAttribs;

		explicit DatabaseDriver(QObject* Parent = nullptr);
		virtual ~DatabaseDriver(void) override;

		QMap<QString, QString> getAttributes(const QStringList& Keys = QStringList());
		QMap<QString, QString> getAttributes(const QString& Key);

		QMap<QString, QString> allAttributes(void);

	public slots:

		bool openDatabase(const QString& Server,
					   const QString& Base,
					   const QString& User,
					   const QString& Pass);

		bool closeDatabase(void);

		void updateData(const QString& Filter);

	signals:

		void onDataLoad(RecordModel*);

		void onAttributesLoad(const QMap<QString, QString>&);
		void onError(const QString&);

		void onConnect(void);
		void onDisconnect(void);

		void onBeginProgress(void);
		void onSetupProgress(int, int);
		void onUpdateProgress(int);
		void onEndProgress(void);

};

#endif // DATABASEDRIVER_HPP
