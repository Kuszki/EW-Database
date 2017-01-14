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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QProgressBar>
#include <QMainWindow>
#include <QThread>

#include "databasedriver_v2.hpp"
#include "databasedriver.hpp"
#include "connectdialog.hpp"
#include "columnsdialog.hpp"
#include "filterdialog.hpp"
#include "updatedialog.hpp"
#include "groupdialog.hpp"
#include "aboutdialog.hpp"

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{

		Q_OBJECT

	public: enum STATUS
	{
		CONNECTED,
		DISCONNECTED,
		BUSY,
		DONE
	};

	private:

		Ui::MainWindow* ui;

		AboutDialog* About;

		QProgressBar* Progress;

		DatabaseDriver_v2* Driver;
		ColumnsDialog* Columns;
		GroupDialog* Groups;
		FilterDialog* Filter;
		UpdateDialog* Update;

		QThread Thread;

	private:

		void lockUi(STATUS Status);

	public:

		explicit MainWindow(QWidget* Parent = nullptr);
		virtual ~MainWindow(void) override;

	private slots:

		void ConnectActionClicked(void);
		void DeleteActionClicked(void);

		void selectionChanged(void);

		void databaseConnected(const QVector<DatabaseDriver_v2::FIELD>& Fields,
						   const QVector<DatabaseDriver_v2::TABLE>& Classes,
						   const QStringList& Headers);
		void databaseDisconnected(void);
		void databaseError(const QString& Error);

		void updateGroups(const QList<int>& Columns);
		void updateColumns(const QList<int>& Columns);
		void updateData(const QHash<QString, QString>& Values);

		void loadData(RecordModel* Model);
		void reloadData(RecordModel* Model);
		void removeData(RecordModel* Model);
		void refreshData(void);

		void completeGrouping(void);

		void prepareEdit(void);

	signals:

		void onGroupRequest(const QList<int>&);

		void onUpdateRequest(const QString&);

		void onEditRequest(RecordModel*, const QModelIndexList&, const QHash<QString, QString>&);

		void onRemoveRequest(RecordModel*, const QModelIndexList&);

		void onDeleteRequest(void);

};

#endif // MAINWINDOW_HPP
