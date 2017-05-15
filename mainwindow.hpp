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
#include <QTextStream>
#include <QUdpSocket>
#include <QThread>

#include "databasedriver.hpp"
#include "connectdialog.hpp"
#include "columnsdialog.hpp"
#include "filterdialog.hpp"
#include "updatedialog.hpp"
#include "exportdialog.hpp"
#include "groupdialog.hpp"
#include "aboutdialog.hpp"
#include "classdialog.hpp"
#include "joindialog.hpp"
#include "textdialog.hpp"

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

		QUdpSocket* Socket;

		AboutDialog* About;

		QProgressBar* Progress;
		QComboBox* Selector;

		DatabaseDriver* Driver;
		ColumnsDialog* Columns;
		GroupDialog* Groups;
		FilterDialog* Filter;
		UpdateDialog* Update;
		ExportDialog* Export;
		TextDialog* Text;

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
		void loadActionClicked(void);
		void classActionClicked(void);

		void selectionChanged(void);

		void databaseConnected(const QList<DatabaseDriver::FIELD>& Fields,
						   const QList<DatabaseDriver::TABLE>& Classes,
						   const QStringList& Headers, unsigned Common);

		void databaseDisconnected(void);
		void databaseError(const QString& Error);
		void databaseLogin(bool OK);

		void updateGroups(const QList<int>& Columns);
		void updateColumns(const QList<int>& Columns);
		void updateValues(const QHash<int, QVariant>& Values);
		void refreshData(const QString& Where, const QList<int>& Used,
					  const QHash<int, QVariant>& Geometry);

		void updateRow(int Index, const QHash<int, QVariant>& Data);
		void removeRow(const QModelIndex& Index);

		void connectData(const QString& Point, const QString& Line,
					  bool Override, int Type, double Radius);
		void disconnectData(const QString& Point,
						const QString& Line,
						int Type);
		void changeClass(const QString& Class,
					  int Line, int Point, int Text);
		void editText(bool Move, bool Justify, bool Rotate, double Length);

		void prepareEdit(const QList<QHash<int, QVariant>>& Values,
					  const QList<int>& Used);
		void prepareJoin(const QHash<QString, QString>& Points,
					  const QHash<QString, QString>& Lines,
					  const QHash<QString, QString>& Circles);
		void prepareClass(const QHash<QString, QString>& Classes,
					   const QHash<QString, QHash<int, QString>>& Lines,
					   const QHash<QString, QHash<int, QString>>& Points,
					   const QHash<QString, QHash<int, QString>>& Texts);

		void saveData(const QList<int>& Fields, int Type);

		void loadData(RecordModel* Model);
		void removeData(RecordModel* Model);
		void updateData(RecordModel* Model);

		void groupData(void);
		void joinData(void);
		void textEdit(void);

		void restoreJob(int Count);
		void removeHistory(int Count);

		void refactorData(void);

		void loginAttempt(void);

		void readDatagram(void);

	signals:

		void onLoadRequest(const QStringList&);
		void onReloadRequest(const QString&, const QList<int>&,
						 const QHash<int, QVariant>&);
		void onEditRequest(RecordModel*, const QModelIndexList&);
		void onRemoveRequest(RecordModel*, const QModelIndexList&);
		void onUpdateRequest(RecordModel*, const QModelIndexList&,
						 const QHash<int, QVariant>&);

		void onJoinRequest(RecordModel*, const QModelIndexList&,
					    const QString&, const QString&,
					    bool, int, double);
		void onSplitRequest(RecordModel*, const QModelIndexList&,
						const QString&, const QString&, int);

		void onRefactorRequest(RecordModel*, const QModelIndexList&,
						   const QString, int, int, int);

		void onClassRequest(RecordModel*, const QModelIndexList&);

		void onListRequest(RecordModel*, const QModelIndexList&);

		void onRestoreRequest(RecordModel*, const QModelIndexList&);
		void onHistoryRequest(RecordModel*, const QModelIndexList&);

		void onTextRequest(RecordModel*, const QModelIndexList&,
					    bool, bool, bool, double);

		void onGroupRequest(const QList<int>&);

};

#endif // MAINWINDOW_HPP
