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

#include "textdialog.hpp"
#include "ui_textdialog.h"

TextDialog::TextDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::TextDialog)
{
	ui->setupUi(this); CheckStatusChanged();
}

TextDialog::~TextDialog(void)
{
	delete ui;
}

void TextDialog::CheckStatusChanged(void)
{
	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(
		ui->Justify->isChecked() ||
		ui->Move->isChecked());

	ui->Rotate->setEnabled(ui->Justify->isChecked());
	ui->Sort->setEnabled(ui->Rotate->isChecked());
}

void TextDialog::accept(void)
{
	QDialog::accept();

	emit onEditRequest(ui->Move->isChecked(),
				    ui->Justify->isChecked(),
				    ui->Rotate->isChecked(),
				    ui->Sort->isChecked(),
				    ui->Length->value());
}
