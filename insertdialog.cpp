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

#include "insertdialog.hpp"
#include "ui_insertdialog.h"

InsertDialog::InsertDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::InsertDialog)
{
	ui->setupUi(this); ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

InsertDialog::~InsertDialog(void)
{
	delete ui;
}

void InsertDialog::accept(void)
{
	const int Mode =
		(ui->endsCheck->isChecked() << 0) |
		(ui->breaksCheck->isChecked() << 1) |
		(ui->intersectCheck->isChecked() << 2) |
		(ui->symbolCheck->isChecked() << 3);

	QDialog::accept(); emit onInsertRequest(Mode, ui->radiusSpin->value(),
									ui->recursiveCheck->isChecked());
}

void InsertDialog::insertParamsChanged(void)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
		ui->endsCheck->isChecked() ||
		ui->breaksCheck->isChecked() ||
		ui->intersectCheck->isChecked() ||
		ui->symbolCheck->isChecked());
}
