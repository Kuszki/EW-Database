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

	ui->supportTool->insertWidget(ui->actionUnhide, Selector);
	ui->supportTool->insertSeparator(ui->actionUnhide);
	ui->statusBar->addPermanentWidget(Progress);
	ui->statusBar->addPermanentWidget(Terminator);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Interface");
	ui->actionSingleton->setChecked(Settings.value("singletons").toBool());
	ui->actionDateoverride->setChecked(Settings.value("override").toBool());
	Settings.endGroup();

	Settings.beginGroup("Window");
	restoreGeometry(Settings.value("geometry").toByteArray());
	restoreState(Settings.value("state").toByteArray());
	Settings.endGroup();

	connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::loadActionClicked);
	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::deleteActionClicked);
	connect(ui->actionDeletelab, &QAction::triggered, this, &MainWindow::removelabActionClicked);
	connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::editActionClicked);
	connect(ui->actionJoin, &QAction::triggered, this, &MainWindow::joinActionClicked);
	connect(ui->actionRestore, &QAction::triggered, this, &MainWindow::restoreActionClicked);
	connect(ui->actionHistory, &QAction::triggered, this, &MainWindow::historyActionClicked);
	connect(ui->actionRefactor, &QAction::triggered, this, &MainWindow::classActionClicked);
	connect(ui->actionBatch, &QAction::triggered, this, &MainWindow::batchActionClicked);
	connect(ui->actionMerge, &QAction::triggered, this, &MainWindow::mergeActionClicked);
	connect(ui->actionInterface, &QAction::triggered, this, &MainWindow::interfaceActionClicked);
	connect(ui->actionFit, &QAction::triggered, this, &MainWindow::fitActionClicked);

	connect(ui->actionAbout, &QAction::triggered, About, &AboutDialog::open);

	connect(ui->actionReload, &QAction::triggered, this, &MainWindow::refreshActionClicked);
	connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::connectActionClicked);
	connect(ui->actionDisconnect, &QAction::triggered, Driver, &DatabaseDriver::closeDatabase);

	connect(ui->actionDateoverride, &QAction::toggled, Driver, &DatabaseDriver::setDateOverride);

	connect(ui->actionHide, &QAction::triggered, this, &MainWindow::hideActionClicked);
	connect(ui->actionUnhide, &QAction::triggered, this, &MainWindow::unhideActionClicked);

	connect(Driver, &DatabaseDriver::onConnect, this, &MainWindow::databaseConnected);
	connect(Driver, &DatabaseDriver::onDisconnect, this, &MainWindow::databaseDisconnected);
	connect(Driver, &DatabaseDriver::onError, this, &MainWindow::databaseError);
	connect(Driver, &DatabaseDriver::onLogin, this, &MainWindow::databaseLogin);

	connect(Driver, &DatabaseDriver::onDataLoad, this, &MainWindow::loadData);
	connect(Driver, &DatabaseDriver::onDataUpdate, this, &MainWindow::updateData);
	connect(Driver, &DatabaseDriver::onDataRemove, this, &MainWindow::removeData);
	connect(Driver, &DatabaseDriver::onCommonReady, this, &MainWindow::prepareMerge);
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
	connect(Driver, &DatabaseDriver::onDataFit, this, &MainWindow::dataFitted);
	connect(Driver, &DatabaseDriver::onPointInsert, this, &MainWindow::breaksInsert);
	connect(Driver, &DatabaseDriver::onLabelDelete, this, &MainWindow::labelDelete);
	connect(Driver, &DatabaseDriver::onLabelEdit, this, &MainWindow::labelEdit);

	connect(Driver, &DatabaseDriver::onRowUpdate, this, &MainWindow::updateRow);
	connect(Driver, &DatabaseDriver::onRowRemove, this, &MainWindow::removeRow);

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

	connect(this, &MainWindow::onClassRequest, Driver, &DatabaseDriver::getClass);
	connect(this, &MainWindow::onRefactorRequest, Driver, &DatabaseDriver::refactorData);

	connect(this, &MainWindow::onTextRequest, Driver, &DatabaseDriver::editText);
	connect(this, &MainWindow::onLabelRequest, Driver, &DatabaseDriver::insertLabel);
	connect(this, &MainWindow::onRemovelabelRequest, Driver, &DatabaseDriver::removeLabel);
	connect(this, &MainWindow::onRelabelRequest, Driver, &DatabaseDriver::editLabel);

	connect(this, &MainWindow::onMergeRequest, Driver, &DatabaseDriver::mergeData);
	connect(this, &MainWindow::onCutRequest, Driver, &DatabaseDriver::cutData);

	connect(this, &MainWindow::onBatchRequest, Driver, &DatabaseDriver::execBatch);

	connect(this, &MainWindow::onFitRequest, Driver, &DatabaseDriver::fitData);
	connect(this, &MainWindow::onInsertRequest, Driver, &DatabaseDriver::insertPoints);

	connect(Terminator, &QPushButton::clicked, Terminator, &QPushButton::hide, Qt::DirectConnection);
	connect(Terminator, &QPushButton::clicked, Driver, &DatabaseDriver::terminate, Qt::DirectConnection);

	connect(Driver, SIGNAL(onBeginProgress(QString)), ui->statusBar, SLOT(showMessage(QString)));
}

MainWindow::~MainWindow(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Interface");
	Settings.setValue("singletons", ui->actionSingleton->isChecked());
	Settings.setValue("override", ui->actionDateoverride->isChecked());
	Settings.endGroup();

	Settings.beginGroup("Window");
	Settings.setValue("state", saveState());
	Settings.setValue("geometry", saveGeometry());
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

void MainWindow::deleteActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (QMessageBox::question(this, tr("Delete %n object(s)", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		ui->Data->selectionModel()->clearSelection();

		lockUi(BUSY); emit onRemoveRequest(Model, Selected);
	}
}

void MainWindow::removelabActionClicked()
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (QMessageBox::question(this, tr("Delete %n object(s) labels", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items labels?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onRemovelabelRequest(Model, Selected);
	}
}

void MainWindow::refreshActionClicked(void)
{
	refreshData(Filter->getFilterRules(), Filter->getUsedFields(), Filter->getGeometryRules(), Filter->getLimiterFile(), Filter->getRadius(), 0);
}

void MainWindow::editActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onEditRequest(Model, Selected);
}

void MainWindow::joinActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onListRequest(Model, Selected);
}

void MainWindow::restoreActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (QMessageBox::question(this, tr("Restore %n object(s) oryginal job name", nullptr, Selected.count()),
						 tr("Are you sure to restore selected items first job name?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onRestoreRequest(Model, Selected);
	}
}

void MainWindow::historyActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (QMessageBox::question(this, tr("Delete %n object(s) history", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items history?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onHistoryRequest(Model, Selected);
	}
}

void MainWindow::loadActionClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Select file to load list"));

	QFile File(Path); if (File.open(QFile::ReadOnly | QFile::Text))
	{
		QTextStream Stream(&File); QStringList List;

		while (!Stream.atEnd()) List << Stream.readLine().trimmed();

		lockUi(BUSY); emit onLoadRequest(List);
	}
}

void MainWindow::mergeActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onCommonRequest(Model, Selected);
}

void MainWindow::classActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onClassRequest(Model, Selected);
}

void MainWindow::hideActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();

	for (const auto Item : Selected)
	{
		ui->Data->setRowHidden(Item.row(), Item.parent(), true);
	}

	ui->Data->selectionModel()->clearSelection();
}

void MainWindow::unhideActionClicked(void)
{
	if (ui->Data->model()) for (int i = 0; i < ui->Data->model()->rowCount(); ++i)
	{
		ui->Data->setRowHidden(i, QModelIndex(), false);
		unhideAll(ui->Data, ui->Data->model()->index(i, 0));
	}
}

void MainWindow::batchActionClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open data file"), QString(), tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));

	if (!Path.isEmpty())
	{
		QFile File(Path); File.open(QFile::ReadOnly | QFile::Text);

		if (File.isOpen())
		{
			QList<QStringList> Values; QRegExp Separator; int Count = 0;

			const QString Extension = QFileInfo(Path).suffix();

			if (Extension == "csv") Separator = QRegExp("\\s*,\\s*");
			else Separator = QRegExp("\\s+");

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

void MainWindow::selectionChanged(void)
{
	const int Count = ui->Data->selectionModel()->selectedRows().count();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
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
	ui->actionFit->setEnabled(Count > 0);
	ui->actionHide->setEnabled(Count > 0);
	ui->actionInsert->setEnabled(Count > 1);
	ui->actionJoin->setEnabled(Count > 1);

	ui->statusBar->showMessage(tr("Selected %1 from %n object(s)", nullptr, From).arg(Count));
}

void MainWindow::refreshData(const QString& Where, const QList<int>& Used, const QHash<int, QVariant>& Geometry, const QString& Limiter, double Radius, int Mode)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	const auto Selected = Model ? ui->Data->selectionModel()->selectedRows() : QModelIndexList();

	lockUi(BUSY); emit onReloadRequest(Where, Used, Geometry, Limiter, Radius, Mode, Model, Selected);
}

void MainWindow::updateRow(int Index, const QHash<int, QVariant>& Data)
{
	dynamic_cast<RecordModel*>(ui->Data->model())->setData(Index, Data);
}

void MainWindow::removeRow(const QModelIndex& Index)
{
	dynamic_cast<RecordModel*>(ui->Data->model())->removeItem(Index);
}

void MainWindow::connectData(const QString& Point, const QString& Line, bool Override, int Type, double Radius)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onJoinRequest(Model, Selected, Point, Line, Override, Type, Radius);
}

void MainWindow::disconnectData(const QString& Point, const QString& Line, int Type)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onSplitRequest(Model, Selected, Point, Line, Type);
}

void MainWindow::mergeData(const QList<int>& Fields, const QStringList& Points)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onMergeRequest(Model, Selected, Fields, Points);
}

void MainWindow::cutData(const QStringList& Points, bool Endings)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onCutRequest(Model, Selected, Points, Endings);
}

void MainWindow::changeClass(const QString& Class, int Line, int Point, int Text, const QString& Symbol, int Style, const QString& Label, int Actions, double Radius)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onRefactorRequest(Model, Selected, Class, Line, Point, Text, Symbol, Style, Label, Actions, Radius);
}

void MainWindow::editText(bool Move, bool Justify, bool Rotate, bool Sort, double Length)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onTextRequest(Model, Selected, Move, Justify, Rotate, Sort, Length);
}

void MainWindow::insertLabel(const QString Text, int J, double X, double Y, bool P, double L, double R)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onLabelRequest(Model, Selected, Text, J, X, Y, P, L, R);
}

void MainWindow::fitData(const QString& File, bool Points, int X1, int Y1, int X2, int Y2, double R, double L)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onFitRequest(Model, Selected, File, Points, X1, Y1, X2, Y2, R, L);
}

void MainWindow::insertBreaks(int Mode, double Radius, double Recursive)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onInsertRequest(Model, Selected, Mode, Radius, Recursive);
}

void MainWindow::relabelData(const QString& Label, int Underline, int Pointer)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onRelabelRequest(Model, Selected, Label, Underline, Pointer);
}

void MainWindow::databaseConnected(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Classes, const QStringList& Headers, unsigned Common, const QHash<QString, QSet<QString>>& Variables)
{
	Codes.clear(); for (const auto& Code : Classes) Codes.insert(Code.Label, Code.Name);

	const bool Singletons = ui->actionSingleton->isChecked();
	allHeaders = Headers; labelCodes = Variables.keys();

	Columns = new ColumnsDialog(this, Headers, Common);
	Groups = new GroupDialog(this, Headers);
	Filter = new FilterDialog(this, Fields, Classes, Common, Singletons);
	Update = new UpdateDialog(this, Fields, Singletons);
	Export = new ExportDialog(this, Headers);
	Merge = new MergeDialog(this, Fields, Classes);
	Cut = new CutDialog(this, Classes);
	Fit = new HarmonizeDialog(this);
	Label = new LabelDialog(labelCodes, this);
	Text = new TextDialog(this);
	Insert = new InsertDialog(this);
	Variable = new VariablesDialog(Variables, this);

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
	connect(Insert, &InsertDialog::onInsertRequest, this, &MainWindow::insertBreaks);
	connect(Variable, &VariablesDialog::onChangeRequest, this, &MainWindow::relabelData);

	connect(ui->actionView, &QAction::triggered, Columns, &ColumnsDialog::open);
	connect(ui->actionGroup, &QAction::triggered, Groups, &GroupDialog::open);
	connect(ui->actionFilter, &QAction::triggered, Filter, &FilterDialog::open);
	connect(ui->actionSave, &QAction::triggered, Export, &ExportDialog::open);
	connect(ui->actionSplit, &QAction::triggered, Cut, &CutDialog::open);
	connect(ui->actionLabel, &QAction::triggered, Label, &LabelDialog::open);
	connect(ui->actionText, &QAction::triggered, Text, &TextDialog::open);
	connect(ui->actionInsert, &QAction::triggered, Insert, &TextDialog::open);
	connect(ui->actionRelabel, &QAction::triggered, Variable, &VariablesDialog::open);

	Driver->setDateOverride(ui->actionDateoverride->isChecked());

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
	Insert->deleteLater();
	Variable->deleteLater();

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
	auto Selection = ui->Data->selectionModel();

	lockUi(BUSY); emit onUpdateRequest(Model, Selection->selectedRows(), Values, Reasons, true);
}

void MainWindow::loadData(RecordModel* Model)
{
	updateView(Model); updateColumns(Columns->getEnabledColumnsIndexes());

	const auto Groupby = Groups->getEnabledGroupsIndexes();

	connect(this, &MainWindow::onGroupRequest, Model, &RecordModel::groupByInt);
	connect(Model, &RecordModel::onGroupComplete, this, &MainWindow::groupData);

	if (Groupby.isEmpty()) lockUi(DONE); else updateGroups(Groupby);
}

void MainWindow::removeData(RecordModel* Model)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data removed"));
}

void MainWindow::updateData(RecordModel* Model)
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

	const int Port = QString::fromUtf8(Array).toInt();

	if (Model) for (const auto& Index : Selected)
	{
		QString Data = QString()
			.append("11 13\n")
			.append(Codes.value(Model->fieldData(Index, 0).toString()))
			.append(";")
			.append(Model->fieldData(Index, 2).toString())
			.append("\n");

		Sender.writeDatagram(Data.toUtf8(), QHostAddress::LocalHost, Port);
	}

	Sender.writeDatagram("\n\n", QHostAddress::LocalHost, Port);
}

void MainWindow::prepareMerge(const QList<int>& Used)
{
	lockUi(DONE); Merge->setActive(Used); Merge->open();
}

void MainWindow::prepareEdit(const QList<QHash<int, QVariant>>& Values, const QList<int>& Used)
{
	lockUi(DONE); Update->setPrepared(Values, Used); Update->open();
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
	const QString Path = QFileDialog::getSaveFileName(this, tr("Select file to save data"));

	if (!Path.isEmpty())
	{
		auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
		auto Selection = ui->Data->selectionModel()->selectedRows();

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

		if (Model->saveToFile(Path, Enabled, Selection, Header))
		{
			ui->statusBar->showMessage(tr("Data saved to file %1").arg(Path));
		}
		else
		{
			ui->statusBar->showMessage(tr("Error while saving data"));
		}
	}
}

void MainWindow::execBatch(const QList<QPair<int, BatchWidget::FUNCTION>>& Roles, const QList<QStringList>& Data)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selection = ui->Data->selectionModel();

	lockUi(BUSY); emit onBatchRequest(Model, Selection->selectedRows(), Roles, Data);
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
			ui->actionInsert->setEnabled(false);
			ui->actionHide->setEnabled(false);
			ui->actionUnhide->setEnabled(false);
			ui->actionInterface->setEnabled(false);
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
			ui->actionInsert->setEnabled(false);
			ui->actionHide->setEnabled(false);
			ui->actionUnhide->setEnabled(false);
			ui->actionInterface->setEnabled(false);
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
	auto Selection = ui->Data->selectionModel();
	auto Old = ui->Data->model();

	ui->Data->setModel(Model);

	delete Selection; Old->deleteLater();

	connect(ui->Data->selectionModel(),
		   &QItemSelectionModel::selectionChanged,
		   this, &MainWindow::selectionChanged);
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
