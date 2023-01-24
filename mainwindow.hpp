/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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

#include <QInputDialog>
#include <QProgressBar>
#include <QTextStream>
#include <QMainWindow>
#include <QFileDialog>
#include <QTextStream>
#include <QToolButton>
#include <QUdpSocket>
#include <QTreeView>
#include <QThread>

#include "databasedriver.hpp"

#include "copyfieldsdialog.hpp"
#include "sqleditordialog.hpp"
#include "variablesdialog.hpp"
#include "harmonizedialog.hpp"
#include "progresswidget.hpp"
#include "settingsdialog.hpp"
#include "selectordialog.hpp"
#include "connectdialog.hpp"
#include "columnsdialog.hpp"
#include "insertdialog.hpp"
#include "filterdialog.hpp"
#include "updatedialog.hpp"
#include "exportdialog.hpp"
#include "scriptdialog.hpp"
#include "breaksdialog.hpp"
#include "groupdialog.hpp"
#include "aboutdialog.hpp"
#include "classdialog.hpp"
#include "mergedialog.hpp"
#include "batchdialog.hpp"
#include "labeldialog.hpp"
#include "edgesdialog.hpp"
#include "kergdialog.hpp"
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
		ProgressWidget* Progress;

		QPushButton* Terminator;
		QComboBox* Selector;
		QSpinBox* Color;

		DatabaseDriver* Driver;

		CopyfieldsDialog* Copyfields;
		SqleditorDialog* Sqleditor;
		SelectorDialog* Loader;
		VariablesDialog* Variable;
		HarmonizeDialog* Fit;
		ColumnsDialog* Columns;
		InsertDialog* Insert;
		GroupDialog* Groups;
		FilterDialog* Filter;
		UpdateDialog* Update;
		ExportDialog* Export;
		MergeDialog* Merge;
		LabelDialog* Label;
		TextDialog* Text;
		CutDialog* Cut;
		KergDialog* Kerg;
		ScriptDialog* Script;
		BreaksDialog* Breaks;
		EdgesDialog* Edges;

		QThread Thread;

		QMap<QString, QString> Codes;
		QStringList labelCodes;
		QStringList allHeaders;
		QString dbPath;

		QByteArray headerState;
		QSet<int> hiddenRows;

		int CommonAction = 0;

	private:

		void lockUi(STATUS Status);

		void updateView(RecordModel* Model);
		void updateHidden(void);

		void registerSockets(const QString& Database);
		void freeSockets(void);

	public:

		explicit MainWindow(QWidget* Parent = nullptr);
		virtual ~MainWindow(void) override;

		static void unhideAll(QTreeView* View, QModelIndex Index);

	private slots:

		void connectActionClicked(void);
		void settingsActionClicked(void);
		void deleteActionClicked(void);
		void removelabActionClicked(void);
		void refreshActionClicked(void);
		void editActionClicked(void);
		void joinActionClicked(void);
		void restoreActionClicked(void);
		void historyActionClicked(void);
		void mergeActionClicked(void);
		void edgesActionClicked(void);
		void classActionClicked(void);
		void hideActionClicked(void);
		void unhideActionClicked(void);
		void batchActionClicked(void);
		void interfaceActionClicked(void);
		void fitActionClicked(void);
		void unifyActionClicked(void);
		void unifyjobsActionClicked(void);
		void fixgeometryActionClicked(void);
		void refactorjobsActionClicked(void);

		void selectionActionToggled(bool Allow);

		void selectionChanged(void);

		void databaseConnected(const QList<DatabaseDriver::FIELD>& Fields,
						   const QList<DatabaseDriver::TABLE>& Classes,
						   const QStringList& Headers, unsigned Common,
						   const QHash<QString, QSet<QString>>& Variables);

		void databaseDisconnected(void);
		void databaseError(const QString& Error);
		void databaseLogin(bool OK);

		void updateGroups(const QList<int>& Columns);
		void updateColumns(const QList<int>& Columns);
		void updateValues(const QHash<int, QVariant>& Values,
					   const QHash<int, int>& Reasons);
		void refreshData(const QString& Where, const QString& Script,
					  const QList<int>& Used,
					  const QHash<int, QVariant>& Geometry,
					  const QHash<int, QVariant>& Redaction,
					  const QString& Limiter, double Radius, int Mode);

		void updateRow(int Index, const QHash<int, QVariant>& Data);
		void removeRow(int Index);

		void connectData(const QString& Point, const QString& Line,
					  bool Override, int Type, double Radius);
		void disconnectData(const QString& Point, const QString& Line, int Type);
		void mergeData(const QList<int>& Fields, const QStringList& Points, double Angle);
		void cutData(const QStringList& Points, int Endings);
		void changeClass(const QString& Class, int Line, int Point, int Text,
					  const QString& Symbol, int Style, const QString& Label,
					  int Actions, double Radius);
		void editText(bool Move, int Justify, bool Rotate, bool Sort, double Length, bool Ignrel);
		void insertLabel(const QString& Text, int J, double X, double Y, bool P, double L, double R);
		void fitData(const QString& File, int Jobtype, int X1, int Y1, int X2, int Y2, double R, double L, bool E);
		void insertBreaks(int Mode, double Radius, const QString& Path);
		void relabelData(const QString& Label, int Underline, int Pointer, double Rotation, int Posset, double dX, double dY);
		void execBreaks(int Flags, double Angle, double Length, bool Mode);

		void hideEdges(const QList<int>& List);

		void prepareCommonact(const QList<int>& Used);
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

		void execBatch(const QList<BatchWidget::RECORD>& Roles,
					const QList<QStringList>& Data);
		void execCopy(const QList<CopyfieldsWidget::RECORD>& Roles, bool Nulls);
		void execScript(const QString& Code);

		void updateKerg(const QString& Path, int Action, int Elements);

		void loadRequest(const QStringList& List, int Field, int Action);

		void loadData(RecordModel* Model);

		void removeRows(const QSet<int>& List);
		void updateRows(const QHash<int, QHash<int, QVariant>>& Data);

		void updateUids(const QHash<int, int>& Newuids);

		void removeData(void);
		void updateData(void);
		void groupData(void);

		void joinData(int Count);
		void textEdit(int Count);
		void labelInsert(int Count);
		void labelEdit(int Count);
		void labelDelete(int Count);
		void breaksInsert(int Count);
		void breaksReduced(int Count);

		void edgesHidden(int Count);

		void batchExec(int Count);

		void restoreJob(int Count);
		void removeHistory(int Count);
		void updatedKerg(int Count);

		void dataMerged(int Count);
		void dataCutted(int Count);
		void dataFitted(int Count);
		void dataUnified(int Count);

		void refactorData(int Count);

		void jobsUnified(int Count);
		void jobsRefactored(int Count);

		void geometryFixed(int Count);

		void loginAttempt(void);

		void readDatagram(void);
		void readRequest(void);

	signals:

		void onLoadRequest(const QStringList&, int, int,
					    RecordModel*, const QSet<int>&);
		void onReloadRequest(const QString&, const QString&,
						 const QList<int>&,
						 const QHash<int, QVariant>&,
						 const QHash<int, QVariant>&,
						 const QString&, double, int,
						 RecordModel*, const QSet<int>&);

		void onCommonRequest(const QSet<int>&);
		void onEditRequest(const QSet<int>&);
		void onRemoveRequest(const QSet<int>&);
		void onUpdateRequest(const QSet<int>&, const QHash<int, QVariant>&, const QHash<int, int>&, bool);
		void onBatchRequest(const QSet<int>&, const QList<BatchWidget::RECORD>&, const QList<QStringList>&);
		void onCopyfieldsRequest(const QSet<int>&, const QList<CopyfieldsWidget::RECORD>&, bool);
		void onScriptRequest(const QSet<int>&, const QString&);

		void onJoinRequest(const QSet<int>&, const QString&, const QString&, bool, int, double);
		void onSplitRequest(const QSet<int>&, const QString&, const QString&, int);

		void onMergeRequest(const QSet<int>&, const QList<int>&, const QStringList&, double);
		void onCutRequest(const QSet<int>&, const QStringList&, int);

		void onRefactorRequest(const QSet<int>&, const QString&, int, int, int,
						   const QString&, int, const QString&, int, double);

		void onClassRequest(const QSet<int>&);

		void onListRequest(const QSet<int>&);

		void onRestoreRequest(const QSet<int>&);
		void onHistoryRequest(const QSet<int>&);

		void onTextRequest(const QSet<int>&, bool, int, bool, bool, double, bool);

		void onLabelRequest(const QSet<int>&, const QString&, int, double, double, bool, double, double);

		void onRelabelRequest(const QSet<int>&, const QString, int, int, double, int, double, double);

		void onRemovelabelRequest(const QSet<int>&);

		void onFitRequest(const QSet<int>&, const QString&, int, int, int, int, int, double, double, bool);
		void onUnifyRequest(const QSet<int>&, double);

		void onInsertRequest(const QSet<int>&, int, double, const QString&);

		void onBreaksRequest(const QSet<int>&, int, double, double, bool);

		void onKergRequest(const QSet<int>&, const QString&, int, int);

		void onGroupRequest(const QList<int>&);

		void onEdgesRequest(const QSet<int>&, const QList<int>&);

		void onFixgeometryRequest(const QSet<int>&);

		void onUnifyjobsRequest(void);
		void onRefactorJobsRequest(const QHash<QString, QString>&);

};

#endif // MAINWINDOW_HPP
