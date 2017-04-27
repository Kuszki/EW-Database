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

	Progress = new QProgressBar(this);
	Driver = new DatabaseDriver(nullptr);
	About = new AboutDialog(this);

	Progress->hide();
	Driver->moveToThread(&Thread);
	Thread.start();

	ui->statusBar->addPermanentWidget(Progress);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Window");
	restoreGeometry(Settings.value("geometry").toByteArray());
	restoreState(Settings.value("state").toByteArray());
	Settings.endGroup();

	connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::loadActionClicked);
	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::deleteActionClicked);
	connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::editActionClicked);
	connect(ui->actionJoin, &QAction::triggered, this, &MainWindow::joinActionClicked);
	connect(ui->actionRestore, &QAction::triggered, this, &MainWindow::restoreActionClicked);
	connect(ui->actionHistory, &QAction::triggered, this, &MainWindow::historyActionClicked);
	connect(ui->actionRefactor, &QAction::triggered, this, &MainWindow::classActionClicked);
	connect(ui->actionAbout, &QAction::triggered, About, &AboutDialog::open);

	connect(ui->actionReload, &QAction::triggered, this, &MainWindow::refreshActionClicked);
	connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::connectActionClicked);
	connect(ui->actionDisconnect, &QAction::triggered, Driver, &DatabaseDriver::closeDatabase);

	connect(Driver, &DatabaseDriver::onConnect, this, &MainWindow::databaseConnected);
	connect(Driver, &DatabaseDriver::onDisconnect, this, &MainWindow::databaseDisconnected);
	connect(Driver, &DatabaseDriver::onError, this, &MainWindow::databaseError);
	connect(Driver, &DatabaseDriver::onLogin, this, &MainWindow::databaseLogin);

	connect(Driver, &DatabaseDriver::onDataLoad, this, &MainWindow::loadData);
	connect(Driver, &DatabaseDriver::onDataUpdate, this, &MainWindow::updateData);
	connect(Driver, &DatabaseDriver::onDataRemove, this, &MainWindow::removeData);
	connect(Driver, &DatabaseDriver::onPresetReady, this, &MainWindow::prepareEdit);
	connect(Driver, &DatabaseDriver::onJoinsReady, this, &MainWindow::prepareJoin);
	connect(Driver, &DatabaseDriver::onDataJoin, this, &MainWindow::joinData);
	connect(Driver, &DatabaseDriver::onDataSplit, this, &MainWindow::joinData);
	connect(Driver, &DatabaseDriver::onJobsRestore, this, &MainWindow::restoreJob);
	connect(Driver, &DatabaseDriver::onHistoryRemove, this, &MainWindow::removeHistory);
	connect(Driver, &DatabaseDriver::onDataRefactor, this, &MainWindow::refactorData);
	connect(Driver, &DatabaseDriver::onClassReady, this, &MainWindow::prepareClass);
	connect(Driver, &DatabaseDriver::onTextEdit, this, &MainWindow::textEdit);

	connect(Driver, &DatabaseDriver::onRowUpdate, this, &MainWindow::updateRow);
	connect(Driver, &DatabaseDriver::onRowRemove, this, &MainWindow::removeRow);

	connect(Driver, &DatabaseDriver::onBeginProgress, Progress, &QProgressBar::show);
	connect(Driver, &DatabaseDriver::onSetupProgress, Progress, &QProgressBar::setRange);
	connect(Driver, &DatabaseDriver::onUpdateProgress, Progress, &QProgressBar::setValue);
	connect(Driver, &DatabaseDriver::onEndProgress, Progress, &QProgressBar::hide);

	connect(this, &MainWindow::onLoadRequest, Driver, &DatabaseDriver::loadList);
	connect(this, &MainWindow::onReloadRequest, Driver, &DatabaseDriver::reloadData);
	connect(this, &MainWindow::onRemoveRequest, Driver, &DatabaseDriver::removeData);
	connect(this, &MainWindow::onUpdateRequest, Driver, &DatabaseDriver::updateData);

	connect(this, &MainWindow::onJoinRequest, Driver, &DatabaseDriver::joinData);
	connect(this, &MainWindow::onSplitRequest, Driver, &DatabaseDriver::splitData);

	connect(this, &MainWindow::onEditRequest, Driver, &DatabaseDriver::getPreset);
	connect(this, &MainWindow::onListRequest, Driver, &DatabaseDriver::getJoins);

	connect(this, &MainWindow::onRestoreRequest, Driver, &DatabaseDriver::restoreJob);
	connect(this, &MainWindow::onHistoryRequest, Driver, &DatabaseDriver::removeHistory);

	connect(this, &MainWindow::onClassRequest, Driver, &DatabaseDriver::getClass);
	connect(this, &MainWindow::onRefactorRequest, Driver, &DatabaseDriver::refactorData);

	connect(this, &MainWindow::onTextRequest, Driver, &DatabaseDriver::editText);

	connect(Driver, SIGNAL(onBeginProgress(QString)), ui->statusBar, SLOT(showMessage(QString)));
}

MainWindow::~MainWindow(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Window");
	Settings.setValue("state", saveState());
	Settings.setValue("geometry", saveGeometry());
	Settings.endGroup();

	Thread.exit();
	Thread.wait();

	delete Driver;
	delete ui;
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

void MainWindow::refreshActionClicked(void)
{
	refreshData(Filter->getFilterRules(), Filter->getUsedFields(), Filter->getGeometryRules());
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

		while (!Stream.atEnd()) List << Stream.readLine();

		lockUi(BUSY); emit onLoadRequest(List);
	}
}

void MainWindow::classActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onClassRequest(Model, Selected);
}

void MainWindow::selectionChanged(void)
{
	const int Count = ui->Data->selectionModel()->selectedRows().count();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	const int From = Model ? Model->totalCount() : 0;

	ui->statusBar->showMessage(tr("Selected %1 from %n object(s)", nullptr, From).arg(Count));

	ui->actionDelete->setEnabled(Count > 0);
	ui->actionEdit->setEnabled(Count > 0);
	ui->actionSave->setEnabled(Count > 0);
	ui->actionRestore->setEnabled(Count > 0);
	ui->actionHistory->setEnabled(Count > 0);
	ui->actionRefactor->setEnabled(Count > 0);
	ui->actionText->setEnabled(Count > 0);
	ui->actionJoin->setEnabled(Count > 1);
}

void MainWindow::refreshData(const QString& Where, const QList<int>& Used, const QHash<int, QVariant>& Geometry)
{
	lockUi(BUSY); emit onReloadRequest(Where, Used, Geometry);
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

void MainWindow::changeClass(const QString& Class, int Line, int Point, int Text)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onRefactorRequest(Model, Selected, Class, Line, Point, Text);
}

void MainWindow::editText(bool Move, bool Justify, bool Rotate)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onTextRequest(Model, Selected, Move, Justify, Rotate);
}

void MainWindow::databaseConnected(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Classes, const QStringList& Headers, unsigned Common)
{
	Columns = new ColumnsDialog(this, Headers, Common);
	Groups = new GroupDialog(this, Headers);
	Filter = new FilterDialog(this, Fields, Classes, Common);
	Update = new UpdateDialog(this, Fields);
	Export = new ExportDialog(this, Headers);
	Text = new TextDialog(this);

	connect(Columns, &ColumnsDialog::onColumnsUpdate, this, &MainWindow::updateColumns);
	connect(Groups, &GroupDialog::onGroupsUpdate, this, &MainWindow::updateGroups);
	connect(Filter, &FilterDialog::onFiltersUpdate, this, &MainWindow::refreshData);
	connect(Update, &UpdateDialog::onValuesUpdate, this, &MainWindow::updateValues);
	connect(Export, &ExportDialog::onExportRequest, this, &MainWindow::saveData);
	connect(Text, &TextDialog::onEditRequest, this, &MainWindow::editText);

	connect(ui->actionView, &QAction::triggered, Columns, &ColumnsDialog::open);
	connect(ui->actionGroup, &QAction::triggered, Groups, &GroupDialog::open);
	connect(ui->actionFilter, &QAction::triggered, Filter, &FilterDialog::open);
	connect(ui->actionSave, &QAction::triggered, Export, &ExportDialog::open);
	connect(ui->actionText, &QAction::triggered, Text, &TextDialog::open);

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
	Text->deleteLater();

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

void MainWindow::updateValues(const QHash<int, QVariant>& Values)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selection = ui->Data->selectionModel();

	lockUi(BUSY); emit onUpdateRequest(Model, Selection->selectedRows(), Values);
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

void MainWindow::joinData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data joined"));
}

void MainWindow::textEdit(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Text edited"));
}

void MainWindow::restoreJob(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Restored %n job(s)", nullptr, Count));
}

void MainWindow::removeHistory(int Count)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Removed %n historic object(s)", nullptr, Count));
}

void MainWindow::refactorData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Class changed"));
}

void MainWindow::loginAttempt(void)
{
	ui->actionConnect->setEnabled(false);
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

	ClassDialog* Class = new ClassDialog(Classes, Lines, Points, Texts, this); Class->open();

	connect(Class, &ClassDialog::onChangeRequest, this, &MainWindow::changeClass);

	connect(Class, &ClassDialog::accepted, Class, &ClassDialog::deleteLater);
	connect(Class, &ClassDialog::rejected, Class, &ClassDialog::deleteLater);
}

void MainWindow::saveData(const QList<int>& Fields, int Type)
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

		if (Model->saveToFile(Path, Enabled, Selection))
		{
			ui->statusBar->showMessage(tr("Data saved to file %1").arg(Path));
		}
		else
		{
			ui->statusBar->showMessage(tr("Error while saving data"));
		}
	}
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
			ui->actionJoin->setEnabled(false);
			ui->actionSave->setEnabled(false);
			ui->actionRestore->setEnabled(false);
			ui->actionHistory->setEnabled(false);
			ui->actionRefactor->setEnabled(false);
			ui->actionText->setEnabled(false);
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
			ui->actionJoin->setEnabled(false);
			ui->actionSave->setEnabled(false);
			ui->actionRestore->setEnabled(false);
			ui->actionHistory->setEnabled(false);
			ui->actionRefactor->setEnabled(false);
			ui->actionText->setEnabled(false);
			ui->Data->setEnabled(false);
		break;
		case DONE:
			ui->statusBar->showMessage(tr("Job done"));
			ui->actionDisconnect->setEnabled(true);
			ui->actionView->setEnabled(true);
			ui->actionGroup->setEnabled(true);
			ui->actionFilter->setEnabled(true);
			ui->actionReload->setEnabled(true);
			ui->actionLoad->setEnabled(true);
			ui->tipLabel->setVisible(false);
			ui->Data->setEnabled(true);
			ui->Data->setVisible(true);

			selectionChanged();
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
