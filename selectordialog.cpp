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

#include "selectordialog.hpp"
#include "ui_selectordialog.h"

SelectorDialog::SelectorDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::SelectorDialog)
{
	ui->setupUi(this); ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

SelectorDialog::~SelectorDialog(void)
{
	delete ui;
}

void SelectorDialog::openButtonClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open list file"), QString(),
											tr("Text files (*.txt);;All files (*.*)"));

	if (!Path.isEmpty()) ui->sourceEdit->setText(Path);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!Path.isEmpty());
}

void SelectorDialog::accept(void)
{
	QDialog::accept();

	emit onDataAccepted(ui->sourceEdit->text(),
					ui->comboBox->currentIndex());
}
