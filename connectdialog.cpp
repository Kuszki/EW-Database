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

#include "connectdialog.hpp"
#include "ui_connectdialog.h"

ConnectDialog::ConnectDialog(QWidget *Parent)
: QDialog(Parent), ui(new Ui::ConnectDialog)
{
	ui->setupUi(this);

	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
}

ConnectDialog::~ConnectDialog(void)
{
	delete ui;
}

void ConnectDialog::edited(void)
{
	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(
				!ui->Server->text().isEmpty() &&
				!ui->Database->text().isEmpty() &&
				!ui->User->text().isEmpty() &&
				!ui->Password->text().isEmpty());
}

void ConnectDialog::accept(void)
{
	setEnabled(false);

	emit onAccept(ui->Server->text(), ui->Database->text(),
			    ui->User->text(), ui->Password->text());
}

void ConnectDialog::reject(void)
{
	ui->Password->clear();

	QDialog::reject();
}

void ConnectDialog::refused(const QString& Error)
{
	QMessageBox::critical(this, tr("Error"), Error);

	setEnabled(true);
}

void ConnectDialog::connected(void)
{
	setEnabled(true);

	QDialog::accept();

	ui->Password->clear();
}
