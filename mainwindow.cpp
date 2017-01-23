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

	ui->statusBar->addPermanentWidget(Progress);

	Progress->hide();
	Driver->moveToThread(&Thread);
	Thread.start();

	QSettings Settings("EW-Database");

	Settings.beginGroup("Window");
	restoreGeometry(Settings.value("geometry").toByteArray());
	restoreState(Settings.value("state").toByteArray());
	Settings.endGroup();

	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::deleteActionClicked);
	connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::editActionClicked);
	connect(ui->actionJoin, &QAction::triggered, this, &MainWindow::joinActionClicked);
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

	connect(Driver, &DatabaseDriver::onBeginProgress, Progress, &QProgressBar::show);
	connect(Driver, &DatabaseDriver::onSetupProgress, Progress, &QProgressBar::setRange);
	connect(Driver, &DatabaseDriver::onUpdateProgress, Progress, &QProgressBar::setValue);
	connect(Driver, &DatabaseDriver::onEndProgress, Progress, &QProgressBar::hide);

	connect(this, &MainWindow::onReloadRequest, Driver, &DatabaseDriver::reloadData);
	connect(this, &MainWindow::onRemoveRequest, Driver, &DatabaseDriver::removeData);
	connect(this, &MainWindow::onUpdateRequest, Driver, &DatabaseDriver::updateData);

	connect(this, &MainWindow::onJoinptlRequest, Driver, &DatabaseDriver::joinLines);
	connect(this, &MainWindow::onJoinptpRequest, Driver, &DatabaseDriver::joinPoints);
	connect(this, &MainWindow::onSplitRequest, Driver, &DatabaseDriver::splitData);

	connect(this, &MainWindow::onEditRequest, Driver, &DatabaseDriver::getPreset);
	connect(this, &MainWindow::onListRequest, Driver, &DatabaseDriver::getJoins);

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
	ConnectDialog* Dialog = new ConnectDialog(this);

	connect(Dialog, &ConnectDialog::onAccept, this, &MainWindow::loginAttempt);

	connect(Dialog, &ConnectDialog::onAccept, Driver, &DatabaseDriver::openDatabase);
	connect(Dialog, &ConnectDialog::accepted, Dialog, &ConnectDialog::deleteLater);
	connect(Dialog, &ConnectDialog::rejected, Dialog, &ConnectDialog::deleteLater);

	connect(Driver, &DatabaseDriver::onLogin, Dialog, &ConnectDialog::connected);
	connect(Driver, &DatabaseDriver::onError, Dialog, &ConnectDialog::refused);

	Dialog->open();
}

void MainWindow::deleteActionClicked(void)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	if (QMessageBox::question(this, tr("Delete %n object(s)", nullptr, Selected.count()),
						 tr("Are you sure to delete selected items?"),
						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		lockUi(BUSY); emit onRemoveRequest(Model, Selected);
	}
}

void MainWindow::refreshActionClicked(void)
{
	refreshData(Filter->getFilterRules(), Filter->getUsedFields());
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

void MainWindow::selectionChanged(void)
{
	const int Count = ui->Data->selectionModel()->selectedRows().count();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	const int From = Model ? Model->totalCount() : 0;

	ui->statusBar->showMessage(tr("Selected %1 from %n object(s)", nullptr, From).arg(Count));

	ui->actionDelete->setEnabled(Count > 0);
	ui->actionEdit->setEnabled(Count > 0);
	ui->actionJoin->setEnabled(Count > 1);
}

void MainWindow::refreshData(const QString& Where, const QList<int>& Used)
{
	lockUi(BUSY); emit onReloadRequest(Where, Used);
}

void MainWindow::connectData(const QString& Point, const QString& Line, bool Override, bool Type)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); ui->tipLabel->setText(tr("Joining data"));

	if (Type) emit onJoinptlRequest(Model, Selected, Point, Line, Override);
	else emit onJoinptpRequest(Model, Selected, Point, Line, Override);
}

void MainWindow::disconnectData(const QString& Point, const QString& Line)
{
	const auto Selected = ui->Data->selectionModel()->selectedRows();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	lockUi(BUSY); emit onSplitRequest(Model, Selected, Point, Line);
}

void MainWindow::databaseConnected(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Classes, const QStringList& Headers, unsigned Common)
{
	Columns = new ColumnsDialog(this, Headers, Common);
	Groups = new GroupDialog(this, Headers);
	Filter = new FilterDialog(this, Fields, Classes, Common);
	Update = new UpdateDialog(this, Fields);

	connect(Columns, &ColumnsDialog::onColumnsUpdate, this, &MainWindow::updateColumns);
	connect(Groups, &GroupDialog::onGroupsUpdate, this, &MainWindow::updateGroups);
	connect(Filter, &FilterDialog::onFiltersUpdate, this, &MainWindow::refreshData);
	connect(Update, &UpdateDialog::onValuesUpdate, this, &MainWindow::updateValues);

	connect(ui->actionView, &QAction::triggered, Columns, &ColumnsDialog::open);
	connect(ui->actionGroup, &QAction::triggered, Groups, &GroupDialog::open);
	connect(ui->actionFilter, &QAction::triggered, Filter, &FilterDialog::open);

	connect(Update, &UpdateDialog::accepted, this, &MainWindow::selectionChanged);
	connect(Update, &UpdateDialog::rejected, this, &MainWindow::selectionChanged);

	lockUi(CONNECTED); ui->tipLabel->setText(tr("Press F5 or use Refresh action to load data"));
}

void MainWindow::databaseDisconnected(void)
{
	ui->tipLabel->setText(tr("Press Ctrl+O or use Connect action to connect to Database"));

	Columns->deleteLater();
	Groups->deleteLater();
	Filter->deleteLater();
	Update->deleteLater();

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

void MainWindow::updateValues(const QMap<int, QVariant>& Values)
{
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
	auto Selection = ui->Data->selectionModel();

	lockUi(BUSY); emit onUpdateRequest(Model, Selection->selectedRows(), Values);
}

void MainWindow::loadData(RecordModel* Model)
{
	ui->Data->setModel(Model); updateColumns(Columns->getEnabledColumnsIndexes());

	const auto Groupby = Groups->getEnabledGroupsIndexes(); emit onDeleteRequest();

	connect(ui->Data->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::selectionChanged);
	connect(this, &MainWindow::onDeleteRequest, Model, &RecordModel::deleteLater, Qt::DirectConnection);
	connect(this, &MainWindow::onGroupRequest, Model, &RecordModel::groupByInt);
	connect(Model, &RecordModel::onGroupComplete, this, &MainWindow::groupData);

	if (Groupby.isEmpty()) lockUi(DONE);
	else updateGroups(Groupby);
}

void MainWindow::loginAttempt(void)
{
	ui->actionConnect->setEnabled(false);
}

void MainWindow::reloadData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data updated, to reenable filter use reload action"));
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

void MainWindow::joinData(void)
{
	lockUi(DONE); ui->statusBar->showMessage(tr("Data joined"));
}

void MainWindow::prepareEdit(const QList<QMap<int, QVariant>>& Values, const QList<int>& Used)
{
	lockUi(DONE); Update->setPrepared(Values, Used); Update->open();
}

void MainWindow::prepareJoin(const QMap<QString, QString>& Points, const QMap<QString, QString>& Lines)
{
	lockUi(DONE); JoinDialog* Join = new JoinDialog(Points, Lines, this); Join->open();

	connect(Join, &JoinDialog::onCreateRequest, this, &MainWindow::connectData);
	connect(Join, &JoinDialog::onDeleteRequest, this, &MainWindow::disconnectData);

	connect(Join, &JoinDialog::accepted, Join, &JoinDialog::deleteLater);
	connect(Join, &JoinDialog::rejected, Join, &JoinDialog::deleteLater);

	connect(Join, &JoinDialog::accepted, this, &MainWindow::selectionChanged);
	connect(Join, &JoinDialog::rejected, this, &MainWindow::selectionChanged);

	connect(Driver, &DatabaseDriver::onDataJoin, Join, &JoinDialog::completeActions);
	connect(Driver, &DatabaseDriver::onDataSplit, Join, &JoinDialog::completeActions);
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
			ui->actionEdit->setEnabled(false);
			ui->actionDelete->setEnabled(false);
			ui->actionJoin->setEnabled(false);
		break;
		case BUSY:
			ui->actionDisconnect->setEnabled(false);
			ui->actionView->setEnabled(false);
			ui->actionGroup->setEnabled(false);
			ui->actionFilter->setEnabled(false);
			ui->actionReload->setEnabled(false);
			ui->actionEdit->setEnabled(false);
			ui->actionDelete->setEnabled(false);
			ui->actionJoin->setEnabled(false);
			ui->Data->setEnabled(false);
		break;
		case DONE:
			ui->statusBar->showMessage(tr("Job done"));
			ui->actionDisconnect->setEnabled(true);
			ui->actionView->setEnabled(true);
			ui->actionGroup->setEnabled(true);
			ui->actionFilter->setEnabled(true);
			ui->actionReload->setEnabled(true);
			ui->tipLabel->setVisible(false);
			ui->Data->setEnabled(true);
			ui->Data->setVisible(true);
		break;
	}
}
