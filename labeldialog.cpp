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

#include "labeldialog.hpp"
#include "ui_labeldialog.h"

LabelDialog::LabelDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::LabelDialog)
{
	ui->setupUi(this);
}

LabelDialog::~LabelDialog(void)
{
	delete ui;
}

void LabelDialog::labelTextChanged(const QString& Text)
{
	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(!Text.isEmpty());
}

void LabelDialog::accept(void)
{
	emit onLabelRequest(
			ui->textEdit->text(),
			ui->justifySpin->value(),
			ui->xSpin->value(),
			ui->ySpin->value(),
			ui->pointerBox->isChecked(),
			ui->limiterSpin->value(),
			ui->repeatSpin->value());

	QDialog::accept();
}
