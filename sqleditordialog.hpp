/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  {description}                                                          *
 *  Copyright (C) 2020  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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

#ifndef SQLEDITORDIALOG_HPP
#define SQLEDITORDIALOG_HPP

#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QStringListModel>

#include <QItemSelectionModel>
#include <QItemSelection>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QDialog>

#include <QDebug>

namespace Ui {	class SqleditorDialog; }

class SqleditorDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::SqleditorDialog *ui;
		QSqlDatabase& db;

		QStringListModel* list;
		QSqlQueryModel* res;
		QSqlTableModel* tab;

		bool trans = false;

	protected:

		void switchModel(QAbstractItemModel* model);

	public:

		explicit SqleditorDialog(QSqlDatabase& database,
							QWidget *parent = nullptr);
		virtual ~SqleditorDialog(void) override;

	private slots:

		void executeActionClicked(void);
		void commitActionClicked(void);
		void rollbackButtonClicked(void);
		void appendButtonClicked(void);
		void deleteButtonClicked(void);

		void tableItemSelected(const QModelIndex& index);
		void recordItemSelected(void);

		void tableItemClicked(const QModelIndex& index);
		void fieldItemClicked(const QModelIndex& index);

};

#endif // SQLEDITORDIALOG_HPP
