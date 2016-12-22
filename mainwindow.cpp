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
	ui->setupUi(this);

	ui->actionDisconnect->setEnabled(false);
	ui->Data->header()->setSectionsMovable(true);

	Driver = new DatabaseDriver(nullptr);
	Columns = new ColumnsDialog(this);

	Driver->moveToThread(&Thread);
	Thread.start();

	connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::ConnectActionClicked);
	connect(ui->actionDisconnect, &QAction::triggered, Driver, &DatabaseDriver::closeDatabase);

	connect(ui->actionView, &QAction::triggered, Columns, &ColumnsDialog::open);

	connect(Driver, &DatabaseDriver::onConnect, this, &MainWindow::databaseConnected);
	connect(Driver, &DatabaseDriver::onDisconnect, this, &MainWindow::databaseDisconnected);
	connect(Driver, &DatabaseDriver::onError, this, &MainWindow::databaseError);
}

MainWindow::~MainWindow(void)
{
	Thread.exit();
	Thread.wait();

	delete Driver;
	delete ui;
}

void MainWindow::ConnectActionClicked(void)
{
	ConnectDialog* Dialog = new ConnectDialog(this);

	connect(Dialog, &ConnectDialog::onAccept, Driver, &DatabaseDriver::openDatabase);
	connect(Dialog, &ConnectDialog::accepted, Dialog, &ConnectDialog::deleteLater);
	connect(Dialog, &ConnectDialog::rejected, Dialog, &ConnectDialog::deleteLater);

	connect(Driver, &DatabaseDriver::onConnect, Dialog, &ConnectDialog::connected);
	connect(Driver, &DatabaseDriver::onError, Dialog, &ConnectDialog::refused);

	Dialog->open();
}

void MainWindow::databaseConnected(void)
{
	ui->actionConnect->setEnabled(false);
	ui->actionDisconnect->setEnabled(true);

	ui->statusBar->showMessage(tr("Database connected"));

	Columns->setSpecialAttributes(Driver->getAttributes());
}

void MainWindow::databaseDisconnected(void)
{
	ui->actionConnect->setEnabled(true);
	ui->actionDisconnect->setEnabled(false);

	ui->statusBar->showMessage(tr("Database disconnected"));
}

void MainWindow::databaseError(const QString& Error)
{
	ui->statusBar->showMessage(Error);
}
