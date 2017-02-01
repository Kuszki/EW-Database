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
#include <QTextStream>
#include <QMainWindow>
#include <QFileDialog>
#include <QThread>

#include "databasedriver.hpp"
#include "connectdialog.hpp"
#include "columnsdialog.hpp"
#include "filterdialog.hpp"
#include "updatedialog.hpp"
#include "exportdialog.hpp"
#include "groupdialog.hpp"
#include "aboutdialog.hpp"
#include "joindialog.hpp"

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

		DatabaseDriver* Driver;
		ColumnsDialog* Columns;
		GroupDialog* Groups;
		FilterDialog* Filter;
		UpdateDialog* Update;
		ExportDialog* Export;

		QThread Thread;

	private:

		void lockUi(STATUS Status);

		void updateView(RecordModel* Model);

	public:

		explicit MainWindow(QWidget* Parent = nullptr);
		virtual ~MainWindow(void) override;

	private slots:

		void connectActionClicked(void);
		void deleteActionClicked(void);
		void refreshActionClicked(void);
		void editActionClicked(void);
		void joinActionClicked(void);
		void restoreActionClicked(void);
		void historyActionClicked(void);

		void selectionChanged(void);

		void databaseConnected(const QList<DatabaseDriver::FIELD>& Fields,
						   const QList<DatabaseDriver::TABLE>& Classes,
						   const QStringList& Headers, unsigned Common);

		void databaseDisconnected(void);
		void databaseError(const QString& Error);
		void databaseLogin(bool OK);

		void updateGroups(const QList<int>& Columns);
		void updateColumns(const QList<int>& Columns);
		void updateValues(const QMap<int, QVariant>& Values);
		void refreshData(const QString& Where,
					  const QList<int>& Used);

		void updateRow(int Index, const QMap<int, QVariant>& Data);
		void removeRow(const QModelIndex& Index);

		void connectData(const QString& Point,
					  const QString& Line,
					  bool Override, int Type);
		void disconnectData(const QString& Point,
						const QString& Line,
						int Type);

		void prepareEdit(const QList<QMap<int, QVariant>>& Values,
					  const QList<int>& Used);
		void prepareJoin(const QMap<QString, QString>& Points,
					  const QMap<QString, QString>& Lines,
					  const QMap<QString, QString>& Circles);

		void saveData(const QList<int>& Fields, int Type);

		void loadData(RecordModel* Model);
		void removeData(RecordModel* Model);
		void updateData(RecordModel* Model);

		void groupData(void);
		void joinData(void);

		void restoreJob(int Count);
		void removeHistory(int Count);

		void loginAttempt(void);

	signals:

		void onReloadRequest(const QString&, const QList<int>&);
		void onEditRequest(RecordModel*, const QModelIndexList&);
		void onRemoveRequest(RecordModel*, const QModelIndexList&);
		void onUpdateRequest(RecordModel*, const QModelIndexList&,
						 const QMap<int, QVariant>&);

		void onJoinRequest(RecordModel*, const QModelIndexList&,
					    const QString&, const QString&, bool, int);
		void onSplitRequest(RecordModel*, const QModelIndexList&,
						const QString&, const QString&, int);

		void onListRequest(RecordModel*, const QModelIndexList&);

		void onRestoreRequest(RecordModel*, const QModelIndexList&);
		void onHistoryRequest(RecordModel*, const QModelIndexList&);

		void onGroupRequest(const QList<int>&);

};

#endif // MAINWINDOW_HPP
