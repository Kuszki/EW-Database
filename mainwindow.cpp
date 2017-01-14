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
		break;
		case BUSY:
			ui->actionDisconnect->setEnabled(false);
			ui->actionView->setEnabled(false);
			ui->actionGroup->setEnabled(false);
			ui->actionFilter->setEnabled(false);
			ui->actionReload->setEnabled(false);
			ui->actionEdit->setEnabled(false);
			ui->actionDelete->setEnabled(false);
			ui->Data->setEnabled(false);
		break;
		case DONE:
			ui->statusBar->showMessage(tr("Job done"));
			ui->actionDisconnect->setEnabled(true);
			ui->actionView->setEnabled(true);
			ui->actionGroup->setEnabled(true);
			ui->actionFilter->setEnabled(true);
			ui->actionReload->setEnabled(true);
			ui->Data->setEnabled(true);
			ui->tipLabel->setVisible(false);
			ui->Data->setVisible(true);
		break;
	}
}

MainWindow::MainWindow(QWidget* Parent)
: QMainWindow(Parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this); lockUi(DISCONNECTED);

	Progress = new QProgressBar(this);
	Driver = new DatabaseDriver_v2(nullptr);
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

	connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::DeleteActionClicked);
	connect(ui->actionEdit, &QAction::triggered, this, &MainWindow::prepareEdit);
	connect(ui->actionAbout, &QAction::triggered, About, &AboutDialog::open);

	connect(ui->actionReload, &QAction::triggered, this, &MainWindow::refreshData);
	connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::ConnectActionClicked);
	connect(ui->actionDisconnect, &QAction::triggered, Driver, &DatabaseDriver_v2::closeDatabase);

	connect(Driver, &DatabaseDriver_v2::onConnect, this, &MainWindow::databaseConnected);
	connect(Driver, &DatabaseDriver_v2::onDisconnect, this, &MainWindow::databaseDisconnected);
	connect(Driver, &DatabaseDriver_v2::onError, this, &MainWindow::databaseError);

//	connect(Driver, &DatabaseDriver::onDataLoad, this, &MainWindow::loadData);
//	connect(Driver, &DatabaseDriver::onDataUpdate, this, &MainWindow::reloadData);
//	connect(Driver, &DatabaseDriver::onDataRemove, this, &MainWindow::removeData);

	connect(Driver, &DatabaseDriver_v2::onBeginProgress, Progress, &QProgressBar::show);
	connect(Driver, &DatabaseDriver_v2::onSetupProgress, Progress, &QProgressBar::setRange);
	connect(Driver, &DatabaseDriver_v2::onUpdateProgress, Progress, &QProgressBar::setValue);
	connect(Driver, &DatabaseDriver_v2::onEndProgress, Progress, &QProgressBar::hide);

//	connect(this, &MainWindow::onUpdateRequest, Driver, &DatabaseDriver::updateData);
//	connect(this, &MainWindow::onEditRequest, Driver, &DatabaseDriver::setData);
//	connect(this, &MainWindow::onRemoveRequest, Driver, &DatabaseDriver::removeData);

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

void MainWindow::ConnectActionClicked(void)
{
	ConnectDialog* Dialog = new ConnectDialog(this);

	connect(Dialog, &ConnectDialog::onAccept, Driver, &DatabaseDriver_v2::openDatabase);
	connect(Dialog, &ConnectDialog::accepted, Dialog, &ConnectDialog::deleteLater);
	connect(Dialog, &ConnectDialog::rejected, Dialog, &ConnectDialog::deleteLater);

	connect(Driver, &DatabaseDriver_v2::onConnect, Dialog, &ConnectDialog::connected);
	connect(Driver, &DatabaseDriver_v2::onError, Dialog, &ConnectDialog::refused);

	Dialog->open();
}

void MainWindow::DeleteActionClicked(void)
{
//	const auto Selected = ui->Data->selectionModel()->selectedRows();
//	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

//	if (QMessageBox::question(this, tr("Delete %n object(s)", nullptr, Selected.count()),
//						 tr("Are you sure to delete selected items?"),
//						 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
//	{
//		lockUi(BUSY); emit onRemoveRequest(Model, Selected);
//	}
}

void MainWindow::selectionChanged(void)
{
	const int Count = ui->Data->selectionModel()->selectedRows().count();
	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());

	ui->statusBar->showMessage(tr("Selected %1 from %n object(s)", nullptr, Model ? Model->totalCount() : 0).arg(Count));

	ui->actionDelete->setEnabled(Count);
	ui->actionEdit->setEnabled(Count);
}

void MainWindow::refreshData(void)
{
//	lockUi(BUSY); ui->statusBar->showMessage(tr("Querying database"));

//	emit onUpdateRequest(Filter->getFilterRules());
}

void MainWindow::databaseConnected(const QVector<DatabaseDriver_v2::FIELD>& Fields, const QVector<DatabaseDriver_v2::TABLE>& Classes, const QStringList& Headers)
{
//	const auto Dict = Driver->allDictionary();
//	const auto Spec = Driver->getAttributes();
//	const auto All = Driver->allAttributes(false);
//	const auto Edit = Driver->allAttributes(true);

	Columns = new ColumnsDialog(this, Headers);
	Groups = new GroupDialog(this, Headers);
	Filter = new FilterDialog(this, Fields, Classes);
//	Update = new UpdateDialog(this, Edit, Dict);

	connect(Columns, &ColumnsDialog::onColumnsUpdate, this, &MainWindow::updateColumns);
	connect(Groups, &GroupDialog::onGroupsUpdate, this, &MainWindow::updateGroups);
	connect(Filter, &FilterDialog::onFiltersUpdate, this, &MainWindow::refreshData);
//	connect(Update, &UpdateDialog::onValuesUpdate, this, &MainWindow::updateData);

	connect(ui->actionView, &QAction::triggered, Columns, &ColumnsDialog::open);
	connect(ui->actionGroup, &QAction::triggered, Groups, &GroupDialog::open);
	connect(ui->actionFilter, &QAction::triggered, Filter, &FilterDialog::open);

	ui->tipLabel->setText(tr("Press F5 or use Refresh action to load data"));

	lockUi(CONNECTED);
}

void MainWindow::databaseDisconnected(void)
{
	ui->tipLabel->setText(tr("Press Ctrl+O or use Connect action to connect to Database"));

	Columns->deleteLater();
	Groups->deleteLater();
	Filter->deleteLater();
	Update->deleteLater();

	lockUi(DISCONNECTED);

	emit onDeleteRequest();
}

void MainWindow::databaseError(const QString& Error)
{
	ui->statusBar->showMessage(Error);
}

void MainWindow::updateGroups(const QList<int>& Columns)
{
	if (!dynamic_cast<RecordModel*>(ui->Data->model())) return;

	ui->statusBar->showMessage(tr("Grouping items"));

	Progress->setRange(0, 0); Progress->show();

	lockUi(BUSY); emit onGroupRequest(Columns);
}

void MainWindow::updateColumns(const QList<int>& Columns)
{
	if (!dynamic_cast<RecordModel*>(ui->Data->model())) return;

	for (int i = 0; i < ui->Data->model()->columnCount(); ++i)
	{
		ui->Data->setColumnHidden(i, !Columns.contains(i));
	}
}

void MainWindow::updateData(const QHash<QString, QString>& Values)
{
//	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
//	auto Selection = ui->Data->selectionModel();

//	lockUi(BUSY); ui->statusBar->showMessage(tr("Updating data"));

//	emit onEditRequest(Model, Selection->selectedRows(), Values);
}

void MainWindow::loadData(RecordModel* Model)
{
//	ui->Data->setModel(Model); updateColumns(Columns->getEnabledColumns());

//	const auto Groupby = Groups->getEnabledGroups(); emit onDeleteRequest();

//	connect(ui->Data->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::selectionChanged);
//	connect(this, &MainWindow::onGroupRequest, Model, &RecordModel::groupBy);
//	connect(this, &MainWindow::onDeleteRequest, Model, &RecordModel::deleteLater);
//	connect(Model, &RecordModel::onGroupComplete, this, &MainWindow::completeGrouping);

//	if (Groupby.isEmpty()) lockUi(DONE);
//	else updateGroups(Groupby);
}

void MainWindow::reloadData(RecordModel* Model)
{
//	lockUi(DONE); ui->statusBar->showMessage(tr("Data updated, to reenable filter use reload action"));
}

void MainWindow::removeData(RecordModel* Model)
{
//	lockUi(DONE); ui->statusBar->showMessage(tr("Data removed"));
}

void MainWindow::completeGrouping(void)
{
	lockUi(DONE); Progress->hide();
}

void MainWindow::prepareEdit(void)
{
//	auto Model = dynamic_cast<RecordModel*>(ui->Data->model());
//	auto Selection = ui->Data->selectionModel();
//	auto Fields = Driver->getEditValues(Model, Selection->selectedRows().first());

//	Update->setFieldsData(Fields);
//	Update->setFieldsUnchecked();
//	Update->open();
}
