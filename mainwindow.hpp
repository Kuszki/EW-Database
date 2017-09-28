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
#include <QTreeView>
#include <QThread>

#include "databasedriver.hpp"

#include "harmonizedialog.hpp"
#include "connectdialog.hpp"
#include "columnsdialog.hpp"
#include "filterdialog.hpp"
#include "updatedialog.hpp"
#include "exportdialog.hpp"
#include "groupdialog.hpp"
#include "aboutdialog.hpp"
#include "classdialog.hpp"
#include "mergedialog.hpp"
#include "batchdialog.hpp"
#include "labeldialog.hpp"
#include "joindialog.hpp"
#include "textdialog.hpp"
#include "cutdialog.hpp"

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

		QUdpSocket* Marker;
		QUdpSocket* Socket;

		AboutDialog* About;

		QProgressBar* Progress;
		QComboBox* Selector;

		DatabaseDriver* Driver;

		HarmonizeDialog* Fit;
		ColumnsDialog* Columns;
		GroupDialog* Groups;
		FilterDialog* Filter;
		UpdateDialog* Update;
		ExportDialog* Export;
		MergeDialog* Merge;
		LabelDialog* Label;
		TextDialog* Text;
		CutDialog* Cut;

		QThread Thread;

		QMap<QString, QString> Codes;
		QStringList allHeaders;
		QString dbPath;

	private:

		void lockUi(STATUS Status);

		void updateView(RecordModel* Model);

		void registerSockets(const QString& Database);
		void freeSockets(void);

	public:

		explicit MainWindow(QWidget* Parent = nullptr);
		virtual ~MainWindow(void) override;

		static void unhideAll(QTreeView* View, QModelIndex Index);

	private slots:

		void connectActionClicked(void);
		void deleteActionClicked(void);
		void refreshActionClicked(void);
		void editActionClicked(void);
		void joinActionClicked(void);
		void restoreActionClicked(void);
		void historyActionClicked(void);
		void loadActionClicked(void);
		void mergeActionClicked(void);
		void classActionClicked(void);
		void hideActionClicked(void);
		void unhideActionClicked(void);
		void batchActionClicked(void);
		void interfaceActionClicked(void);
		void fitActionClicked(void);

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
					  const QHash<int, QVariant>& Geometry,
					  const QString& Limiter, double Radius, int Mode);

		void updateRow(int Index, const QHash<int, QVariant>& Data);
		void removeRow(const QModelIndex& Index);

		void connectData(const QString& Point, const QString& Line,
					  bool Override, int Type, double Radius);
		void disconnectData(const QString& Point,
						const QString& Line,
						int Type);
		void mergeData(const QList<int>& Fields, const QStringList& Points);
		void cutData(const QStringList& Points, bool Endings);
		void changeClass(const QString& Class,
					  int Line, int Point, int Text);
		void editText(bool Move, bool Justify, bool Rotate, bool Sort, double Length);
		void insertLabel(const QString Text, int J, double X, double Y, bool P, double L, double R);
		void fitData(const QString& File, bool Points, int X1, int Y1, int X2, int Y2, double R, double L);

		void prepareMerge(const QList<int>& Used);
		void prepareEdit(const QList<QHash<int, QVariant>>& Values,
					  const QList<int>& Used);
		void prepareJoin(const QHash<QString, QString>& Points,
					  const QHash<QString, QString>& Lines,
					  const QHash<QString, QString>& Circles);
		void prepareClass(const QHash<QString, QString>& Classes,
					   const QHash<QString, QHash<int, QString>>& Lines,
					   const QHash<QString, QHash<int, QString>>& Points,
					   const QHash<QString, QHash<int, QString>>& Texts);

		void saveData(const QList<int>& Fields, int Type, bool Header);

		void execBatch(const QList<QPair<int, BatchWidget::FUNCTION>>& Roles,
					const QList<QStringList>& Data);

		void loadData(RecordModel* Model);
		void removeData(RecordModel* Model);
		void updateData(RecordModel* Model);

		void groupData(void);
		void joinData(int Count);
		void textEdit(int Count);
		void labelInsert(int Count);

		void batchExec(int Count);

		void restoreJob(int Count);
		void removeHistory(int Count);

		void dataMerged(int Count);
		void dataCutted(int Count);
		void dataFitted(int Count);

		void refactorData(int Count);

		void loginAttempt(void);

		void readDatagram(void);
		void readRequest(void);

	signals:

		void onLoadRequest(const QStringList&);
		void onReloadRequest(const QString&, const QList<int>&,
						 const QHash<int, QVariant>&,
						 const QString&, double, int,
						 RecordModel*, const QModelIndexList&);

		void onCommonRequest(RecordModel*, const QModelIndexList&);
		void onEditRequest(RecordModel*, const QModelIndexList&);
		void onRemoveRequest(RecordModel*, const QModelIndexList&);
		void onUpdateRequest(RecordModel*, const QModelIndexList&,
						 const QHash<int, QVariant>&, bool);
		void onBatchRequest(RecordModel*, const QModelIndexList&,
						const QList<QPair<int, BatchWidget::FUNCTION>>&,
						const QList<QStringList>&);

		void onJoinRequest(RecordModel*, const QModelIndexList&,
					    const QString&, const QString&,
					    bool, int, double);
		void onSplitRequest(RecordModel*, const QModelIndexList&,
						const QString&, const QString&, int);

		void onMergeRequest(RecordModel*, const QModelIndexList&,
						const QList<int>&, const QStringList&);
		void onCutRequest(RecordModel*, const QModelIndexList&,
					   const QStringList&, bool);

		void onRefactorRequest(RecordModel*, const QModelIndexList&,
						   const QString, int, int, int);

		void onClassRequest(RecordModel*, const QModelIndexList&);

		void onListRequest(RecordModel*, const QModelIndexList&);

		void onRestoreRequest(RecordModel*, const QModelIndexList&);
		void onHistoryRequest(RecordModel*, const QModelIndexList&);

		void onTextRequest(RecordModel*, const QModelIndexList&,
					    bool, bool, bool, bool, double);

		void onLabelRequest(RecordModel*, const QModelIndexList&, const QString&,
						int, double, double, bool, double, double);

		void onFitRequest(RecordModel*, const QModelIndexList&, const QString&,
					   bool, int, int, int, int, double, double);

		void onGroupRequest(const QList<int>&);

};

#endif // MAINWINDOW_HPP
