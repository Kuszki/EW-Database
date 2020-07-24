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

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* Parent)
: QMainWindow(Parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this); lockUi(DISCONNECTED);

	Terminator = new QPushButton(QIcon::fromTheme("process-stop"), tr("Stop"), this);

	Color = new QSpinBox(this);
	Selector = new QComboBox(this);
	Progress = new QProgressBar(this);
	Driver = new DatabaseDriver(nullptr);
	About = new AboutDialog(this);

	Terminator->hide();
	Progress->hide();
	Driver->moveToThread(&Thread);
	Thread.start();

	Selector->addItems(QStringList()
				    << tr("No selection") << tr("Single selection")
				    << tr("Add to selection") << tr("Remove from selection")
				    << tr("Unhide item") << tr("Hide item"));
	Selector->setLayoutDirection(Qt::LeftToRight);

	Color->setRange(0, 14);
	Color->setPrefix(tr("Color "));
	Color->setSpecialValueText(tr("Default color"));
	Color->setLayoutDirection(Qt::LeftToRight);

	ui->supportTool->insertWidget(ui->actionUnhide, Color);
	ui->supportTool->insertWidget(ui->actionUnhide, Selector);
	ui->supportTool->insertSeparator(ui->actionUnhide);
	ui->statusBar->addPermanentWidget(Progress);
	ui->statusBar->addPermanentWidget(Terminator);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Interface");
	ui->actionSingleton->setChecked(Settings.value("singletons").toBool());
	ui->actionSelection->setChecked(Settings.value("groupselect").toBool());
	ui->actionDateoverride->setChecked(Settings.value("override").toBool());
	ui->actionCreatehistory->setChecked(Settings.value("history").toBool());
	Color->setValue(Settings.value("color", 0).toInt());
	Settings.endGroup();

	Settings.beginGroup("Window");
	restoreGeometry(Settings.value("geometry").toByteArray());
	restoreState(Settings.value("state").toByteArray());
	headerState = Settings.value("header").toByteArray();
	Settings.endGroup();

	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::deleteActionClicked);
	connect(ui->actionDeletelab, &QAction::triggered, this, &MainWindow::removelabActionClicked);
	connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::editActionClicked);
	connect(ui->actionJoin, &QAction::triggered, this, &MainWindow::joinActionClicked);
	connect(ui->actionRestore, &QAction::triggered, this, &MainWindow::restoreActionClicked);
	connect(ui->actionHistory, &QAction::triggered, this, &MainWindow::historyActionClicked);
	connect(ui->actionRefactor, &QAction::triggered, this, &MainWindow::classActionClicked);
	connect(ui->actionBatch, &QAction::triggered, this, &MainWindow::batchActionClicked);
	connect(ui->actionMerge, &QAction::triggered, this, &MainWindow::mergeActionClicked);
	connect(ui->actionEdges, &QAction::triggered, this, &MainWindow::edgesActionClicked);
	connect(ui->actionInterface, &QAction::triggered, this, &MainWindow::interfaceActionClicked);
	connect(ui->actionFit, &QAction::triggered, this, &MainWindow::fitActionClicked);

	connect(ui->actionAbout, &QAction::triggered, About, &AboutDialog::open);

	connect(ui->actionReload, &QAction::triggered, this, &MainWindow::refreshActionClicked);
	connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::connectActionClicked);
	connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::settingsActionClicked);
	connect(ui->actionDisconnect, &QAction::triggered, Driver, &DatabaseDriver::closeDatabase);

	connect(ui->actionDateoverride, &QAction::toggled, Driver, &DatabaseDriver::setDateOverride);
	connect(ui->actionCreatehistory, &QAction::toggled, Driver, &DatabaseDriver::setHistoryMake);
	connect(ui->actionSelection, &QAction::toggled, this, &MainWindow::selectionActionToggled);

	connect(ui->actionHide, &QAction::triggered, this, &MainWindow::hideActionClicked);
	connect(ui->actionUnhide, &QAction::triggered, this, &MainWindow::unhideActionClicked);

	connect(ui->actionUnifyJobs, &QAction::triggered, this, &MainWindow::unifyjobsActionClicked);
	connect(ui->actionRefactorJobs, &QAction::triggered, this, &MainWindow::refactorjobsActionClicked);

	connect(Driver, &DatabaseDriver::onConnect, this, &MainWindow::databaseConnected);
	connect(Driver, &DatabaseDriver::onDisconnect, this, &MainWindow::databaseDisconnected);
	connect(Driver, &DatabaseDriver::onError, this, &MainWindow::databaseError);
	connect(Driver, &DatabaseDriver::onLogin, this, &MainWindow::databaseLogin);

	connect(Driver, &DatabaseDriver::onDataLoad, this, &MainWindow::loadData);
	connect(Driver, &DatabaseDriver::onDataUpdate, this, &MainWindow::updateData);
	connect(Driver, &DatabaseDriver::onDataRemove, this, &MainWindow::removeData);
	connect(Driver, &DatabaseDriver::onCommonReady, this, &MainWindow::prepareCommonact);
	connect(Driver, &DatabaseDriver::onPresetReady, this, &MainWindow::prepareEdit);
	connect(Driver, &DatabaseDriver::onJoinsReady, this, &MainWindow::prepareJoin);
	connect(Driver, &DatabaseDriver::onDataJoin, this, &MainWindow::joinData);
	connect(Driver, &DatabaseDriver::onDataSplit, this, &MainWindow::joinData);
	connect(Driver, &DatabaseDriver::onJobsRestore, this, &MainWindow::restoreJob);
	connect(Driver, &DatabaseDriver::onHistoryRemove, this, &MainWindow::removeHistory);
	connect(Driver, &DatabaseDriver::onDataRefactor, this, &MainWindow::refactorData);
	connect(Driver, &DatabaseDriver::onClassReady, this, &MainWindow::prepareClass);
	connect(Driver, &DatabaseDriver::onTextEdit, this, &MainWindow::textEdit);
	connect(Driver, &DatabaseDriver::onLabelInsert, this, &MainWindow::labelInsert);
	connect(Driver, &DatabaseDriver::onDataMerge, this, &MainWindow::dataMerged);
	connect(Driver, &DatabaseDriver::onDataCut, this, &MainWindow::dataCutted);
	connect(Driver, &DatabaseDriver::onBatchExec, this, &MainWindow::batchExec);
	connect(Driver, &DatabaseDriver::onCopyExec, this, &MainWindow::batchExec);
	connect(Driver, &DatabaseDriver::onScriptExec, this, &MainWindow::batchExec);
	connect(Driver, &DatabaseDriver::onDataFit, this, &MainWindow::dataFitted);
	connect(Driver, &DatabaseDriver::onPointInsert, this, &MainWindow::breaksInsert);
	connect(Driver, &DatabaseDriver::onLabelDelete, this, &MainWindow::labelDelete);
	connect(Driver, &DatabaseDriver::onLabelEdit, this, &MainWindow::labelEdit);
	connect(Driver, &DatabaseDriver::onKergUpdate, this, &MainWindow::updatedKerg);
	connect(Driver, &DatabaseDriver::onSegmentReduce, this, &MainWindow::breaksReduced);
	connect(Driver, &DatabaseDriver::onEdgesHide, this, &MainWindow::edgesHidden);

	connect(Driver, &DatabaseDriver::onRowUpdate, this, &MainWindow::updateRow);
	connect(Driver, &DatabaseDriver::onRowRemove, this, &MainWindow::removeRow);

	connect(Driver, &DatabaseDriver::onRowsUpdate, this, &MainWindow::updateRows);
	connect(Driver, &DatabaseDriver::onRowsRemove, this, &MainWindow::removeRows);

	connect(Driver, &DatabaseDriver::onUidsUpdate, this, &MainWindow::updateUids);

	connect(Driver, &DatabaseDriver::onJobsUnify, this, &MainWindow::jobsUnified);
	connect(Driver, &DatabaseDriver::onJobsRefactor, this, &MainWindow::jobsRefactored);

	connect(Driver, &DatabaseDriver::onSetupProgress, Progress, &QProgressBar::show);
	connect(Driver, &DatabaseDriver::onSetupProgress, Progress, &QProgressBar::setRange);
	connect(Driver, &DatabaseDriver::onUpdateProgress, Progress, &QProgressBar::setValue);
	connect(Driver, &DatabaseDriver::onSetupProgress, Progress, &QProgressBar::setValue);
	connect(Driver, &DatabaseDriver::onEndProgress, Progress, &QProgressBar::hide);

	connect(Driver, &DatabaseDriver::onSetupProgress, Terminator, &QToolButton::show);
	connect(Driver, &DatabaseDriver::onEndProgress, Terminator, &QToolButton::hide);

	connect(this, &MainWindow::onLoadRequest, Driver, &DatabaseDriver::loadList);
	connect(this, &MainWindow::onReloadRequest, Driver, &DatabaseDriver::reloadData);
	connect(this, &MainWindow::onRemoveRequest, Driver, &DatabaseDriver::removeData);
	connect(this, &MainWindow::onUpdateRequest, Driver, &DatabaseDriver::updateData);

	connect(this, &MainWindow::onJoinRequest, Driver, &DatabaseDriver::joinData);
	connect(this, &MainWindow::onSplitRequest, Driver, &DatabaseDriver::splitData);

	connect(this, &MainWindow::onCommonRequest, Driver, &DatabaseDriver::getCommon);
	connect(this, &MainWindow::onEditRequest, Driver, &DatabaseDriver::getPreset);
	connect(this, &MainWindow::onListRequest, Driver, &DatabaseDriver::getJoins);

	connect(this, &MainWindow::onRestoreRequest, Driver, &DatabaseDriver::restoreJob);
	connect(this, &MainWindow::onHistoryRequest, Driver, &DatabaseDriver::removeHistory);
	connect(this, &MainWindow::onKergRequest, Driver, &DatabaseDriver::updateKergs);

	connect(this, &MainWindow::onClassRequest, Driver, &DatabaseDriver::getClass);
	connect(this, &MainWindow::onRefactorRequest, Driver, &DatabaseDriver::refactorData);

	connect(this, &MainWindow::onTextRequest, Driver, &DatabaseDriver::editText);
	connect(this, &MainWindow::onLabelRequest, Driver, &DatabaseDriver::insertLabel);
	connect(this, &MainWindow::onRemovelabelRequest, Driver, &DatabaseDriver::removeLabel);
	connect(this, &MainWindow::onRelabelRequest, Driver, &DatabaseDriver::editLabel);

	connect(this, &MainWindow::onMergeRequest, Driver, &DatabaseDriver::mergeData);
	connect(this, &MainWindow::onCutRequest, Driver, &DatabaseDriver::cutData);

	connect(this, &MainWindow::onBatchRequest, Driver, &DatabaseDriver::execBatch);
	connect(this, &MainWindow::onCopyfieldsRequest, Driver, &DatabaseDriver::execFieldcopy);
	connect(this, &MainWindow::onScriptRequest, Driver, &DatabaseDriver::execScript);

	connect(this, &MainWindow::onFitRequest, Driver, &DatabaseDriver::fitData);
	connect(this, &MainWindow::onInsertRequest, Driver, &DatabaseDriver::insertPoints);
	connect(this, &MainWindow::onBreaksRequest, Driver, &DatabaseDriver::mergeSegments);
	connect(this, &MainWindow::onEdgesRequest, Driver, &DatabaseDriver::hideEdges);

	connect(this, &MainWindow::onUnifyjobsRequest, Driver, &DatabaseDriver::unifyJobs);
	connect(this, &MainWindow::onRefactorJobsRequest, Driver, &DatabaseDriver::refactorJobs);

	connect(Terminator, &QPushButton::clicked, Terminator, &QPushButton::hide, Qt::DirectConnection);
	connect(Terminator, &QPushButton::clicked, Driver, &DatabaseDriver::terminate, Qt::DirectConnection);

	connect(Driver, SIGNAL(onBeginProgress(QString)), ui->statusBar, SLOT(showMessage(QString)));
}

MainWindow::~MainWindow(void)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (Model) headerState = ui->Data->header()->saveState();

	QSettings Settings("EW-Database");

	Settings.beginGroup("Interface");
	Settings.setValue("singletons", ui->actionSingleton->isChecked());
	Settings.setValue("groupselect", ui->actionSelection->isChecked());
	Settings.setValue("override", ui->actionDateoverride->isChecked());
	Settings.setValue("history", ui->actionCreatehistory->isChecked());
	Settings.setValue("color", Color->value());
	Settings.endGroup();

	Settings.beginGroup("Window");
	Settings.setValue("state", saveState());
	Settings.setValue("geometry", saveGeometry());
	Settings.setValue("header", headerState);
	Settings.endGroup();

	Settings.beginGroup("Sockets");
	Settings.remove(dbPath);

	Driver->terminate();

	Thread.exit();
	Thread.wait();

	delete Driver;
	delete ui;
}

void MainWindow::unhideAll(QTreeView* View, QModelIndex Index)
{
	if (Index.isValid()) for (int i = 0; i < Index.model()->rowCount(Index); ++i)
	{
		View->setRowHidden(i, Index, false); unhideAll(View, Index.child(i, 0));
	}
}

void MainWindow::connectActionClicked(void)
{
	ConnectDialog* Dialog = new ConnectDialog(this); Dialog->open();

	connect(Dialog, &ConnectDialog::onAccept, this, &MainWindow::loginAttempt);

	connect(Dialog, &ConnectDialog::onAccept, Driver, &DatabaseDriver::openDatabase);
	connect(Dialog, &ConnectDialog::accepted, Dialog, &ConnectDialog::deleteLater);
	connect(Dialog, &ConnectDialog::rejected, Dialog, &ConnectDialog::deleteLater);

	connect(Driver, &DatabaseDriver::onLogin, Dialog, &ConnectDialog::connected);
	connect(Driver, &DatabaseDriver::onError, Dialog, &ConnectDialog::refused);
}

void MainWindow::settingsActionClicked(void)
{
	SettingsDialog* Dialog = new SettingsDialog(this); Dialog->open();

	connect(Dialog, &SettingsDialog::accepted, Dialog, &SettingsDialog::deleteLater);
	connect(Dialog, &SettingsDialog::rejected, Dialog, &SettingsDialog::deleteLater);
}

void MainWindow::deleteActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (QMessageBox::question(this, tr("Delete %n object(s)", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		ui->Data->selectionModel()->clearSelection();

		lockUi(BUSY); emit onRemoveRequest(Model->getUids(Selected).subtract(hiddenRows));
	}
}

void MainWindow::removelabActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	if (QMessageBox::question(this, tr("Delete %n object(s) labels", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items labels?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onRemovelabelRequest(Set);
	}
}

void MainWindow::refreshActionClicked(void)
{
	refreshData(Filter->getFilterRules(), Filter->getAdvancedRules(), Filter->getUsedFields(),
			  Filter->getGeometryRules(), Filter->getRedactionRules(),
			  Filter->getLimiterFile(), Filter->getRadius(), 0);
}

void MainWindow::editActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onEditRequest(Set);
}

void MainWindow::joinActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onListRequest(Set);
}

void MainWindow::restoreActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	if (QMessageBox::question(this, tr("Restore %n object(s) oryginal job name", nullptr, Selected.count()),
						 tr("Are you sure to restore selected items first job name?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onRestoreRequest(Set);
	}
}

void MainWindow::historyActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	if (QMessageBox::question(this, tr("Delete %n object(s) history", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items history?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onHistoryRequest(Set);
	}
}

void MainWindow::mergeActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); CommonAction = 1; emit onCommonRequest(Set);
}

void MainWindow::edgesActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); CommonAction = 2; emit onCommonRequest(Set);
}

void MainWindow::classActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onClassRequest(Set);
}

void MainWindow::hideActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	for (const auto Item : Selected)
		if (Model->getUid(Item) != -1)
		{
			ui->Data->setRowHidden(Item.row(), Item.parent(), true);
		}
		else for (const auto Row : Model->getIndexes(Item))
		{
			ui->Data->setRowHidden(Row.row(), Row.parent(), true);
		}

	hiddenRows |= Model->getUids(Selected);

	ui->Data->selectionModel()->clearSelection();
}

void MainWindow::unhideActionClicked(void)
{
	if (ui->Data->model()) for (int i = 0; i < ui->Data->model()->rowCount(); ++i)
	{
		ui->Data->setRowHidden(i, QModelIndex(), false);
		unhideAll(ui->Data, ui->Data->model()->index(i, 0));
	}

	hiddenRows.clear();
}

void MainWindow::batchActionClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open data file"), QString(),
											tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));

	QSettings Settings("EW-Database");

	Settings.beginGroup("Locale");
	const auto csvSep = Settings.value("csv", ",").toString();
	const auto txtSep = Settings.value("txt", "\\s+").toString();
	Settings.endGroup();

	if (!Path.isEmpty())
	{
		QFile File(Path); File.open(QFile::ReadOnly | QFile::Text);

		if (File.isOpen())
		{
			QList<QStringList> Values; QRegExp Separator; int Count = 0;

			const QString Extension = QFileInfo(Path).suffix();

			if (Extension != "csv") Separator = QRegExp(txtSep);
			else Separator = QRegExp(QString("\\s*%1\\s*").arg(csvSep));

			while (!File.atEnd())
			{
				const QString Line = File.readLine().trimmed(); if (Line.isEmpty()) continue;
				const QStringList Data = Line.split(Separator, QString::KeepEmptyParts);

				if (!Count) Count = Data.size();
				if (Data.size() == Count) Values.append(Data);

			}

			if (Count && !Values.isEmpty())
			{
				auto Dialog = new BatchDialog(allHeaders, Values, this); Dialog->open();

				connect(Dialog, &BatchDialog::accepted, Dialog, &BatchDialog::deleteLater);
				connect(Dialog, &BatchDialog::rejected, Dialog, &BatchDialog::deleteLater);

				connect(Dialog, &BatchDialog::onBatchRequest, this, &MainWindow::execBatch);
			}
			else ui->statusBar->showMessage(tr("Loaded data is invalid"));
		}
		else
		{
			ui->statusBar->showMessage(tr("Error with opening file: ") + File.errorString());
		}
	}
}

void MainWindow::interfaceActionClicked(void)
{
	const QString Path = QApplication::applicationDirPath();

	bool OK = Driver->addInterface(Path + "/EW-Marker.exe", 1, true);
	OK = OK && Driver->addInterface(Path + "/EW-Selector.exe", 9, false);

	if (OK) ui->statusBar->showMessage(tr("Intreface registered"));
	else ui->statusBar->showMessage(tr("Error with registering interface"));
}

void MainWindow::fitActionClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open data file"), QString(),
											tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));

	if (!Path.isEmpty()) Fit->open(Path);
}

void MainWindow::unifyjobsActionClicked(void)
{
	lockUi(BUSY); emit onUnifyjobsRequest();
}

void MainWindow::refactorjobsActionClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open data file"), QString(),
											tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));

	QSettings Settings("EW-Database");

	Settings.beginGroup("Locale");
	const auto csvSep = Settings.value("csv", ",").toString();
	const auto txtSep = Settings.value("txt", "\\s+").toString();
	Settings.endGroup();

	QHash<QString, QString> Dict;

	if (!Path.isEmpty())
	{
		QFile File(Path); File.open(QFile::ReadOnly | QFile::Text);

		if (File.isOpen())
		{
			QList<QStringList> Values; QRegExp Separator;

			const QString Extension = QFileInfo(Path).suffix();

			if (Extension != "csv") Separator = QRegExp(txtSep);
			else Separator = QRegExp(QString("\\s*%1\\s*").arg(csvSep));

			while (!File.atEnd())
			{
				const QString Line = File.readLine().trimmed(); if (Line.isEmpty()) continue;
				const QStringList Data = Line.split(Separator, QString::KeepEmptyParts);

				if (Data.size() == 2) Dict.insert(Data[0], Data[1]);
			}

			if (!Dict.isEmpty()) { lockUi(BUSY); emit onRefactorJobsRequest(Dict); }
			else ui->statusBar->showMessage(tr("Loaded data is invalid"));
		}
		else
		{
			ui->statusBar->showMessage(tr("Error with opening file: ") + File.errorString());
		}
	}
}

void MainWindow::selectionActionToggled(bool Allow)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (Model)
	{
		ui->Data->selectionModel()->clearSelection();

		Model->setGroupsSelectable(Allow);

		ui->Data->update();
	}
}

void MainWindow::selectionChanged(void)
{
	if (!ui->Data->updatesEnabled()) return;

	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	if (!Model) return;
	auto Selection = ui->Data->selectionModel()->selectedRows();

	auto Count = Model->getUids(Selection).subtract(hiddenRows).size();

	const int From = Model ? Model->totalCount() : 0;

	ui->actionDelete->setEnabled(Count > 0);
	ui->actionDeletelab->setEnabled(Count > 0);
	ui->actionEdit->setEnabled(Count > 0);
	ui->actionSave->setEnabled(Count > 0);
	ui->actionRestore->setEnabled(Count > 0);
	ui->actionHistory->setEnabled(Count > 0);
	ui->actionMerge->setEnabled(Count > 0);
	ui->actionSplit->setEnabled(Count > 0);
	ui->actionRefactor->setEnabled(Count > 0);
	ui->actionBatch->setEnabled(Count > 0);
	ui->actionText->setEnabled(Count > 0);
	ui->actionLabel->setEnabled(Count > 0);
	ui->actionRelabel->setEnabled(Count > 0);
	ui->actionKerg->setEnabled(Count > 0);
	ui->actionCopyfields->setEnabled(Count > 0);
	ui->actionScript->setEnabled(Count > 0);
	ui->actionBreaks->setEnabled(Count > 0);
	ui->actionFit->setEnabled(Count > 0);
	ui->actionHide->setEnabled(Count > 0);
	ui->actionInsert->setEnabled(Count > 1);
	ui->actionJoin->setEnabled(Count > 1);
	ui->actionEdges->setEnabled(Count > 1);

	ui->statusBar->showMessage(tr("Selected %1 from %n object(s)", nullptr, From).arg(Count));
}

void MainWindow::refreshData(const QString& Where, const QString& Script, const QList<int>& Used, const QHash<int, QVariant>& Geometry, const QHash<int, QVariant>& Redaction, const QString& Limiter, double Radius, int Mode)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model()); hiddenRows.clear();
	const auto Selected = Model ? ui->Data->selectionModel()->selectedRows() : QModelIndexList();
	const auto Set = Model ? Model->getUids(Selected).subtract(hiddenRows) : QSet<int>();

	lockUi(BUSY); emit onReloadRequest(Where, Script, Used, Geometry, Redaction, Limiter, Radius, Mode, Model, Set);
}

void MainWindow::updateRow(int Index, const QHash<int, QVariant>& Data)
{
	dynamic_cast<RecordModel*>(ui->Data->model())->setData(Index, Data);
}

void MainWindow::removeRow(int Index)
{
	dynamic_cast<RecordModel*>(ui->Data->model())->removeItem(Index);
}

void MainWindow::connectData(const QString& Point, const QString& Line, bool Override, int Type, double Radius)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onJoinRequest(Set, Point, Line, Override, Type, Radius);
}

void MainWindow::disconnectData(const QString& Point, const QString& Line, int Type)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onSplitRequest(Set, Point, Line, Type);
}

void MainWindow::mergeData(const QList<int>& Fields, const QStringList& Points, double Angle)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onMergeRequest(Set, Fields, Points, Angle);
}

void MainWindow::cutData(const QStringList& Points, int Endings)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onCutRequest(Set, Points, Endings);
}

void MainWindow::changeClass(const QString& Class, int Line, int Point, int Text, const QString& Symbol, int Style, const QString& Label, int Actions, double Radius)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onRefactorRequest(Set, Class, Line, Point, Text, Symbol, Style, Label, Actions, Radius);
}

void MainWindow::editText(bool Move, int Justify, bool Rotate, bool Sort, double Length)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onTextRequest(Set, Move, Justify, Rotate, Sort, Length);
}

void MainWindow::insertLabel(const QString Text, int J, double X, double Y, bool P, double L, double R)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onLabelRequest(Set, Text, J, X, Y, P, L, R);
}

void MainWindow::fitData(const QString& File, bool Points, int X1, int Y1, int X2, int Y2, double R, double L, bool E)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onFitRequest(Set, File, Points, X1, Y1, X2, Y2, R, L, E);
}

void MainWindow::insertBreaks(int Mode, double Radius)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onInsertRequest(Set, Mode, Radius);
}

void MainWindow::relabelData(const QString& Label, int Underline, int Pointer, double Rotation)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onRelabelRequest(Set, Label, Underline, Pointer, Rotation);
}

void MainWindow::execBreaks(int Flags, double Angle, double Length, bool Mode)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onBreaksRequest(Set, Flags, Angle, Length, Mode);
}

void MainWindow::hideEdges(const QList<int>& List)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onEdgesRequest(Set, List);
}

void MainWindow::databaseConnected(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Classes, const QStringList& Headers, unsigned Common, const QHash<QString, QSet<QString>>& Variables)
{
	Codes.clear(); for (const auto& Code : Classes) Codes.insert(Code.Label, Code.Name);

	const QStringList Props = QStringList(Headers).replaceInStrings(QRegExp("\\W+"), " ")
										 .replaceInStrings(QRegExp("\\s+"), "_");

	const bool Singletons = ui->actionSingleton->isChecked();
	allHeaders = Headers; labelCodes = Variables.keys();

	Columns = new ColumnsDialog(this, Headers, Common);
	Groups = new GroupDialog(this, Headers);
	Filter = new FilterDialog(this, Props, Fields, Classes, Common, Singletons);
	Update = new UpdateDialog(this, Fields, Singletons);
	Export = new ExportDialog(this, Headers);
	Merge = new MergeDialog(this, Fields, Classes);
	Cut = new CutDialog(this, Classes);
	Fit = new HarmonizeDialog(this);
	Label = new LabelDialog(labelCodes, this);
	Text = new TextDialog(this);
	Insert = new InsertDialog(this);
	Kerg = new KergDialog(this);
	Variable = new VariablesDialog(Variables, this);
	Loader = new SelectorDialog(this);
	Copyfields = new CopyfieldsDialog(allHeaders, this);
	Script = new ScriptDialog(Props, this);
	Breaks = new BreaksDialog(this);
	Edges = new EdgesDialog(this, Fields);

	connect(Columns, &ColumnsDialog::onColumnsUpdate, this, &MainWindow::updateColumns);
	connect(Groups, &GroupDialog::onGroupsUpdate, this, &MainWindow::updateGroups);
	connect(Filter, &FilterDialog::onFiltersUpdate, this, &MainWindow::refreshData);
	connect(Update, &UpdateDialog::onValuesUpdate, this, &MainWindow::updateValues);
	connect(Export, &ExportDialog::onExportRequest, this, &MainWindow::saveData);
	connect(Merge, &MergeDialog::onMergeRequest, this, &MainWindow::mergeData);
	connect(Cut, &CutDialog::onClassesUpdate, this, &MainWindow::cutData);
	connect(Label, &LabelDialog::onLabelRequest, this, &MainWindow::insertLabel);
	connect(Text, &TextDialog::onEditRequest, this, &MainWindow::editText);
	connect(Fit, &HarmonizeDialog::onFitRequest, this, &MainWindow::fitData);
	connect(Kerg, &KergDialog::onUpdateRequest, this, &MainWindow::updateKerg);
	connect(Insert, &InsertDialog::onInsertRequest, this, &MainWindow::insertBreaks);
	connect(Variable, &VariablesDialog::onChangeRequest, this, &MainWindow::relabelData);
	connect(Loader, &SelectorDialog::onDataAccepted, this, &MainWindow::loadRequest);
	connect(Copyfields, &CopyfieldsDialog::onCopyRequest, this, &MainWindow::execCopy);
	connect(Script, &ScriptDialog::onRunRequest, this, &MainWindow::execScript);
	connect(Breaks, &BreaksDialog::onReduceRequest, this, &MainWindow::execBreaks);
	connect(Edges, &EdgesDialog::onEdgeRequest, this, &MainWindow::hideEdges);

	connect(ui->actionView, &QAction::triggered, Columns, &ColumnsDialog::open);
	connect(ui->actionGroup, &QAction::triggered, Groups, &GroupDialog::open);
	connect(ui->actionFilter, &QAction::triggered, Filter, &FilterDialog::open);
	connect(ui->actionSave, &QAction::triggered, Export, &ExportDialog::open);
	connect(ui->actionSplit, &QAction::triggered, Cut, &CutDialog::open);
	connect(ui->actionLabel, &QAction::triggered, Label, &LabelDialog::open);
	connect(ui->actionText, &QAction::triggered, Text, &TextDialog::open);
	connect(ui->actionInsert, &QAction::triggered, Insert, &TextDialog::open);
	connect(ui->actionKerg, &QAction::triggered, Kerg, &KergDialog::open);
	connect(ui->actionRelabel, &QAction::triggered, Variable, &VariablesDialog::open);
	connect(ui->actionLoad, &QAction::triggered, Loader, &SelectorDialog::open);
	connect(ui->actionCopyfields, &QAction::triggered, Copyfields, &CopyfieldsDialog::open);
	connect(ui->actionScript, &QAction::triggered, Script, &ScriptDialog::open);
	connect(ui->actionBreaks, &QAction::triggered, Breaks, &BreaksDialog::open);

	Driver->setDateOverride(ui->actionDateoverride->isChecked());
	Driver->setHistoryMake(ui->actionCreatehistory->isChecked());

	setWindowTitle(tr("EW-Database") + " (" + Driver->getDatabaseName() + ")");
	registerSockets(Driver->getDatabasePath());

	lockUi(CONNECTED); ui->tipLabel->setText(tr("Press F5 or use Refresh action to load data"));
}

void MainWindow::databaseDisconnected(void)
{
	ui->tipLabel->setText(tr("Press Ctrl+O or use Connect action to connect to Database"));

	Columns->deleteLater();
	Groups->deleteLater();
	Filter->deleteLater();
	Update->deleteLater();
	Export->deleteLater();
	Merge->deleteLater();
	Cut->deleteLater();
	Fit->deleteLater();
	Label->deleteLater();
	Text->deleteLater();
	Kerg->deleteLater();
	Insert->deleteLater();
	Variable->deleteLater();
	Loader->deleteLater();
	Copyfields->deleteLater();
	Script->deleteLater();
	Breaks->deleteLater();

	setWindowTitle(tr("EW-Database"));
	freeSockets();

	updateView(nullptr);
	lockUi(DISCONNECTED);
}

void MainWindow::databaseError(const QString& Error)
{
	ui->statusBar->showMessage(Error);
}

void MainWindow::databaseLogin(bool OK)
{
	ui->actionConnect->setEnabled(!OK);
}

void MainWindow::updateGroups(const QList<int>& Columns)
{
	if (!dynamic_cast<RecordModel*>(ui->Data->model())) return;

	if (!Columns.size()) ui->Data->setTreePosition(-1);
	else ui->Data->setTreePosition(Columns.first());

	lockUi(BUSY); emit onGroupRequest(Columns);
}

void MainWindow::updateColumns(const QList<int>& Columns)
{
	if (!dynamic_cast<RecordModel*>(ui->Data->model())) return;

	for (int i = 0; i < ui->Data->model()->columnCount(); ++i)
	{
		ui->Data->setColumnHidden(i, !(Columns.isEmpty() || Columns.contains(i)));
	}
}

void MainWindow::updateValues(const QHash<int, QVariant>& Values, const QHash<int, int>& Reasons)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onUpdateRequest(Set, Values, Reasons, true);
}

void MainWindow::loadData(RecordModel* Model)
{
	const auto Groupby = Groups->getEnabledGroupsIndexes();

	if (!Groupby.isEmpty()) Model->groupByInt(Groupby);

	updateView(Model);
	updateColumns(Columns->getEnabledColumnsIndexes());
	updateHidden();

	if (!Groupby.size()) ui->Data->setTreePosition(-1);
	else ui->Data->setTreePosition(Groupby.first());

	connect(this, &MainWindow::onGroupRequest, Model, &RecordModel::groupByInt);
	connect(Model, &RecordModel::onGroupComplete, this, &MainWindow::groupData);

	lockUi(DONE); ui->statusBar->showMessage(tr("Data loaded"));
}

void MainWindow::removeRows(const QSet<int>& List)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (List.size() > 10) ui->Data->setUpdatesEnabled(false);

	if (Model) for (const auto& Index : List) Model->removeItem(Index);

	if (List.size() > 10) ui->Data->setUpdatesEnabled(true);

	if (List.size()) selectionChanged();
}

void MainWindow::updateRows(const QHash<int, QHash<int, QVariant>>& Data)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (Data.size() > 25) ui->Data->setUpdatesEnabled(false);

	if (Model) for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
	{
		Model->setData(i.key(), i.value());
	}

	if (Data.size() > 25) ui->Data->setUpdatesEnabled(true);
}

void MainWindow::updateUids(const QHash<int, int>& Newuids)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (Model) for (auto i = Newuids.constBegin(); i != Newuids.constEnd(); ++i)
	{
		Model->updateUid(i.key(), i.value());
	}
}

void MainWindow::removeData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data removed"));
}

void MainWindow::updateData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data updated"));
}

void MainWindow::groupData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data groupped"));
}

void MainWindow::joinData(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Joined %n object(s)", nullptr, Count));
}

void MainWindow::textEdit(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Edited %n text(s)", nullptr, Count));
}

void MainWindow::labelInsert(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Inserted %n label(s)", nullptr, Count));
}

void MainWindow::labelEdit(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Updated %n label(s)", nullptr, Count));
}

void MainWindow::labelDelete(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Removed %n label(s)", nullptr, Count));
}

void MainWindow::breaksInsert(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Inserted %n breakpoint(s)", nullptr, Count));
}

void MainWindow::breaksReduced(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Removed %n breakpoint(s)", nullptr, Count));
}

void MainWindow::edgesHidden(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Hidden %n edge(s)", nullptr, Count));
}

void MainWindow::batchExec(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Executed %n updates(s)", nullptr, Count));
}

void MainWindow::restoreJob(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Restored %n job(s)", nullptr, Count));
}

void MainWindow::removeHistory(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Removed %n historic object(s)", nullptr, Count));
}

void MainWindow::updatedKerg(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Updated %n object(s)", nullptr, Count));
}

void MainWindow::dataMerged(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Merged %n object(s)", nullptr, Count));
}

void MainWindow::dataCutted(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Splitted %n object(s)", nullptr, Count));
}

void MainWindow::dataFitted(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Changed %n segment(s)", nullptr, Count));
}

void MainWindow::refactorData(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Changed %n object(s)", nullptr, Count));
}

void MainWindow::jobsUnified(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Unified %n job(s)", nullptr, Count));
}

void MainWindow::jobsRefactored(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Refactored %n job(s)", nullptr, Count));
}

void MainWindow::loginAttempt(void)
{
	ui->actionConnect->setEnabled(false);
}

void MainWindow::readDatagram(void)
{
	static const QVector<QItemSelectionModel::SelectionFlag> Actions =
	{
		QItemSelectionModel::ClearAndSelect,
		QItemSelectionModel::Select,
		QItemSelectionModel::Deselect
	};

	QByteArray Data;

	Data.resize(Socket->pendingDatagramSize());
	Socket->readDatagram(Data.data(), Data.size());

	QStringList List = QString::fromUtf8(Data).split('\n');

	if (!List.isEmpty()) if (int Action = Selector->currentIndex())
	{
		auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
		auto Selection = ui->Data->selectionModel();

		if (Model && Selection) for (const auto& UUID : List)
		{
			const QModelIndex Index = Model->find(2, UUID);

			if (Index.isValid())
			{
				if (Action < 4)
				{
					Selection->select(Index, Actions[Action - 1] | QItemSelectionModel::Rows);
				}
				else
				{
					ui->Data->setRowHidden(Index.row(), Index.parent(), Action == 5);

					if (Action == 5)
					{
						Selection->select(Index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
					}

					if (Action == 5) hiddenRows.insert(Model->getUid(Index));
					else hiddenRows.remove(Model->getUid(Index));
				}

				ui->Data->scrollTo(Index, QAbstractItemView::PositionAtCenter);
			}
		}
	}
}

void MainWindow::readRequest(void)
{
	if (!ui->Data->model()) return; QUdpSocket Sender; QByteArray Array;

	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	Array.resize(Marker->pendingDatagramSize());
	Marker->readDatagram(Array.data(), Array.size());

	const QString Format = QString("%1 13\n").arg(Color->value());
	const int Port = QString::fromUtf8(Array).toInt();

	if (Model) for (const auto& Index : Model->getUids(Selected))
	{
		const auto Mid = Model->index(Index);

		QString Data = QString()
			.append(Format)
			.append(Codes.value(Model->fieldData(Mid, 0).toString()))
			.append(";")
			.append(Model->fieldData(Mid, 2).toString())
			.append("\n");

		Sender.writeDatagram(Data.toUtf8(), QHostAddress::LocalHost, Port);
	}

	Sender.writeDatagram("\n\n", QHostAddress::LocalHost, Port);
}

void MainWindow::prepareCommonact(const QList<int>& Used)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Job done"));

	if (!Used.isEmpty()) switch (CommonAction)
	{
		case 1:
			Merge->setActive(Used);
			Merge->open();
		break;
		case 2:
			Edges->setActive(Used);
			Edges->open();
		break;
	}
	else ui->statusBar->showMessage(tr("Unable to prepare dialog data"));
}

void MainWindow::prepareEdit(const QList<QHash<int, QVariant>>& Values, const QList<int>& Used)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Job done"));

	if (!Values.isEmpty()) { Update->setPrepared(Values, Used); Update->open(); }
	else ui->statusBar->showMessage(tr("Unable to prepare dialog data"));
}

void MainWindow::prepareJoin(const QHash<QString, QString>& Points, const QHash<QString, QString>& Lines, const QHash<QString, QString>& Circles)
{
	lockUi(DONE); JoinDialog* Join = new JoinDialog(Points, Lines, Circles, this); Join->open();

	connect(Join, &JoinDialog::onCreateRequest, this, &MainWindow::connectData);
	connect(Join, &JoinDialog::onDeleteRequest, this, &MainWindow::disconnectData);

	connect(Join, &JoinDialog::accepted, Join, &JoinDialog::deleteLater);
	connect(Join, &JoinDialog::rejected, Join, &JoinDialog::deleteLater);

	connect(Driver, &DatabaseDriver::onDataJoin, Join, &JoinDialog::completeActions);
	connect(Driver, &DatabaseDriver::onDataSplit, Join, &JoinDialog::completeActions);
}

void MainWindow::prepareClass(const QHash<QString, QString>& Classes, const QHash<QString, QHash<int, QString>>& Lines, const QHash<QString, QHash<int, QString>>& Points, const QHash<QString, QHash<int, QString>>& Texts)
{
	lockUi(DONE); if (Classes.isEmpty()) { QMessageBox::critical(this, tr("Set object class"), tr("Selected objects have different types")); return; }

	ClassDialog* Class = new ClassDialog(Classes, Lines, Points, Texts, labelCodes, this); Class->open();

	connect(Class, &ClassDialog::onChangeRequest, this, &MainWindow::changeClass);

	connect(Class, &ClassDialog::accepted, Class, &ClassDialog::deleteLater);
	connect(Class, &ClassDialog::rejected, Class, &ClassDialog::deleteLater);
}

void MainWindow::saveData(const QList<int>& Fields, int Type, bool Header)
{
	const QString Path = QFileDialog::getSaveFileName(this, tr("Select file to save data"), QString(),
											tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));

	if (Path.isEmpty()) return;

	QSettings Settings("EW-Database");

	Settings.beginGroup("Locale");
	const auto csvSep = Settings.value("csv", ",").toString();
	const auto txtSep = Settings.value("txt", "\t").toString();
	Settings.endGroup();

	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selection = ui->Data->selectionModel()->selectedRows();
	auto Set = Model->getUids(Selection).subtract(hiddenRows);

	if (Type != 3)
	{
		const QString Extension = QFileInfo(Path).suffix();
		const QString Sep(Extension == "csv" ? csvSep : txtSep);

		QList<int> Enabled;

		switch (Type)
		{
			case 0:
				for (int i = 0; i < Model->columnCount(); ++i)
				{
					Enabled.append(i);
				}
			break;
			case 1:
				Enabled = Columns->getEnabledColumnsIndexes();
			break;
			case 2:
				Enabled = Fields;
			break;
		}

		if (Model->saveToFile(Path, Enabled, Selection, Header, Sep))
		{
			ui->statusBar->showMessage(tr("Data saved to file %1").arg(Path));
		}
		else
		{
			ui->statusBar->showMessage(tr("Error while saving data"));
		}
	}
	else Driver->saveGeometry(Set, Path);
}

void MainWindow::execBatch(const QList<BatchWidget::RECORD>& Roles, const QList<QStringList>& Data)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onBatchRequest(Set, Roles, Data);
}

void MainWindow::execCopy(const QList<CopyfieldsWidget::RECORD>& Roles, bool Nulls)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onCopyfieldsRequest(Set, Roles, Nulls);
}

void MainWindow::execScript(const QString& Code)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onScriptRequest(Set, Code);
}

void MainWindow::updateKerg(const QString& Path, int Action, int Elements)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Set = Model->getUids(Selected).subtract(hiddenRows);

	lockUi(BUSY); emit onKergRequest(Set, Path, Action, Elements);
}

void MainWindow::loadRequest(const QStringList& List, int Field, int Action)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model()); hiddenRows.clear();
	const auto Selected = Model ? ui->Data->selectionModel()->selectedRows() : QModelIndexList();
	const auto Set = Model ? Model->getUids(Selected).subtract(hiddenRows) : QSet<int>();

	lockUi(BUSY); hiddenRows.clear(); emit onLoadRequest(List, Field, Action, Model, Set);
}

void MainWindow::lockUi(MainWindow::STATUS Status)
{
	switch (Status)
	{
		case CONNECTED:
			ui->statusBar->showMessage(tr("Database connected"));
			ui->actionConnect->setEnabled(false);
			ui->actionDisconnect->setEnabled(true);
			ui->actionView->setEnabled(true);
			ui->actionGroup->setEnabled(true);
			ui->actionFilter->setEnabled(true);
			ui->actionReload->setEnabled(true);
			ui->actionLoad->setEnabled(true);
			ui->actionUnhide->setEnabled(true);
			ui->actionInterface->setEnabled(true);
			ui->actionUnifyJobs->setEnabled(true);
			ui->actionRefactorJobs->setEnabled(true);
			ui->actionSingleton->setEnabled(false);
		break;
		case DISCONNECTED:
			ui->statusBar->showMessage(tr("Database disconnected"));
			ui->Data->setVisible(false);
			ui->tipLabel->setVisible(true);
			ui->actionConnect->setEnabled(true);
			ui->actionDisconnect->setEnabled(false);
			ui->actionView->setEnabled(false);
			ui->actionGroup->setEnabled(false);
			ui->actionFilter->setEnabled(false);
			ui->actionReload->setEnabled(false);
			ui->actionLoad->setEnabled(false);
			ui->actionEdit->setEnabled(false);
			ui->actionDelete->setEnabled(false);
			ui->actionDeletelab->setEnabled(false);
			ui->actionJoin->setEnabled(false);
			ui->actionSave->setEnabled(false);
			ui->actionRestore->setEnabled(false);
			ui->actionHistory->setEnabled(false);
			ui->actionMerge->setEnabled(false);
			ui->actionSplit->setEnabled(false);
			ui->actionRefactor->setEnabled(false);
			ui->actionBatch->setEnabled(false);
			ui->actionText->setEnabled(false);
			ui->actionLabel->setEnabled(false);
			ui->actionRelabel->setEnabled(false);
			ui->actionFit->setEnabled(false);
			ui->actionKerg->setEnabled(false);
			ui->actionInsert->setEnabled(false);
			ui->actionHide->setEnabled(false);
			ui->actionUnhide->setEnabled(false);
			ui->actionInterface->setEnabled(false);
			ui->actionCopyfields->setEnabled(false);
			ui->actionScript->setEnabled(false);
			ui->actionBreaks->setEnabled(false);
			ui->actionEdges->setEnabled(false);
			ui->actionUnifyJobs->setEnabled(false);
			ui->actionRefactorJobs->setEnabled(false);
			ui->actionSingleton->setEnabled(true);
		break;
		case BUSY:
			ui->actionDisconnect->setEnabled(false);
			ui->actionView->setEnabled(false);
			ui->actionGroup->setEnabled(false);
			ui->actionFilter->setEnabled(false);
			ui->actionReload->setEnabled(false);
			ui->actionLoad->setEnabled(false);
			ui->actionEdit->setEnabled(false);
			ui->actionDelete->setEnabled(false);
			ui->actionDeletelab->setEnabled(false);
			ui->actionJoin->setEnabled(false);
			ui->actionSave->setEnabled(false);
			ui->actionRestore->setEnabled(false);
			ui->actionHistory->setEnabled(false);
			ui->actionMerge->setEnabled(false);
			ui->actionSplit->setEnabled(false);
			ui->actionRefactor->setEnabled(false);
			ui->actionBatch->setEnabled(false);
			ui->actionText->setEnabled(false);
			ui->actionLabel->setEnabled(false);
			ui->actionRelabel->setEnabled(false);
			ui->actionFit->setEnabled(false);
			ui->actionKerg->setEnabled(false);
			ui->actionInsert->setEnabled(false);
			ui->actionHide->setEnabled(false);
			ui->actionUnhide->setEnabled(false);
			ui->actionInterface->setEnabled(false);
			ui->actionCopyfields->setEnabled(false);
			ui->actionScript->setEnabled(false);
			ui->actionBreaks->setEnabled(false);
			ui->actionEdges->setEnabled(false);
			ui->actionUnifyJobs->setEnabled(false);
			ui->actionRefactorJobs->setEnabled(false);
			ui->Data->setEnabled(false);

			Driver->unterminate();
		break;
		case DONE:
			ui->statusBar->showMessage(tr("Job done"));
			ui->actionDisconnect->setEnabled(true);
			ui->actionView->setEnabled(true);
			ui->actionGroup->setEnabled(true);
			ui->actionFilter->setEnabled(true);
			ui->actionReload->setEnabled(true);
			ui->actionLoad->setEnabled(true);
			ui->actionUnhide->setEnabled(true);
			ui->actionInterface->setEnabled(true);
			ui->actionUnifyJobs->setEnabled(true);
			ui->actionRefactorJobs->setEnabled(true);
			ui->tipLabel->setVisible(false);
			ui->Data->setEnabled(true);
			ui->Data->setVisible(true);

			selectionChanged();

			Driver->unterminate();
		break;
	}
}

void MainWindow::updateView(RecordModel* Model)
{
	const bool GS = ui->actionSelection->isChecked();

	auto Selection = ui->Data->selectionModel();
	auto Old = ui->Data->model();

	if (Model)
	{
		Model->setGroupsSelectable(GS);
		Model->setParent(this);
	}

	if (dynamic_cast<RecordModel*>(Old))
	{
		headerState = ui->Data->header()->saveState();
	}

	ui->Data->setModel(Model);
	ui->Data->header()->restoreState(headerState);

	delete Selection; Old->deleteLater();

	connect(ui->Data->selectionModel(),
		   &QItemSelectionModel::selectionChanged,
		   this, &MainWindow::selectionChanged);

	connect(ui->Data->model(),
		   &QAbstractItemModel::modelReset,
		   this, &MainWindow::updateHidden);
}

void MainWindow::updateHidden(void)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	for (const auto& UID : hiddenRows)
	{
		const auto Index = Model->index(UID);

		if (Index.isValid())
		{
			ui->Data->setRowHidden(Index.row(), Index.parent(), true);
		}
	}
}

void MainWindow::registerSockets(const QString& Database)
{
	QSettings Settings("EW-Database"); dbPath = Database;

	Settings.beginGroup("Sockets"); Settings.beginGroup(Database);

	qsrand(QDateTime::currentMSecsSinceEpoch());

	Marker = new QUdpSocket(this); Socket = new QUdpSocket(this);

	while (!Marker->bind(QHostAddress::LocalHost, qrand() & 0xFFFF));
	while (!Socket->bind(QHostAddress::LocalHost, qrand() & 0xFFFF));

	Settings.setValue("marker", Marker->localPort());
	Settings.setValue("selector", Socket->localPort());

	connect(Marker, &QUdpSocket::readyRead, this, &MainWindow::readRequest);
	connect(Socket, &QUdpSocket::readyRead, this, &MainWindow::readDatagram);
}

void MainWindow::freeSockets(void)
{
	if (Marker) Marker->deleteLater();
	if (Socket) Socket->deleteLater();

	QSettings Settings("EW-Database");

	Settings.beginGroup("Sockets");
	Settings.remove(dbPath);
}
