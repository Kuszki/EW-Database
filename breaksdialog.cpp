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

#include "breaksdialog.hpp"
#include "ui_breaksdialog.h"

BreaksDialog::BreaksDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::BreaksDialog)
{
	ui->setupUi(this); dialogParamsChanged();
}

BreaksDialog::~BreaksDialog(void)
{
	delete ui;
}

void BreaksDialog::accept(void)
{
	int Flags(0); QDialog::accept(); const bool Job = ui->modeCombo->currentIndex();

	if (!ui->pointCheck->isChecked()) Flags = Flags | 0x01;
	if (!ui->breakCheck->isChecked()) Flags = Flags | 0x02;

	emit onReduceRequest(Flags, ui->angleSpin->value(), ui->lengthSpin->value(), Job);
}

void BreaksDialog::dialogParamsChanged(void)
{
	const bool Job = ui->modeCombo->currentIndex();

	const bool Angle = ui->angleSpin->value() != 0.0;
	const bool Length = ui->lengthSpin->value() != 0.0;

	const bool OK = (Job == 0 && Length) || (Job == 1 && Angle);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(OK);
}
