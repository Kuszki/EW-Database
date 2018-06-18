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
	ui->setupUi(this); static const QStringList Flags =
	{
		tr("On point objects"),
		tr("On common breakpoints")
	};

	auto Model = new QStandardItemModel(0, 1, this);
	auto Item = new QStandardItem(tr("Remove breakpoints"));

	int i(0); for (const auto& Text : Flags)
	{
		auto Item = new QStandardItem(Text);

		Item->setData(i);
		Item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		Item->setCheckState(Qt::Unchecked);

		Model->insertRow(i++, Item);
	}

	Item->setFlags(Qt::ItemIsEnabled);
	Model->insertRow(0, Item);

	ui->flagsCombo->setModel(Model);

	dialogParamsChanged();
}

BreaksDialog::~BreaksDialog(void)
{
	delete ui;
}

void BreaksDialog::accept(void)
{
	const auto M = dynamic_cast<QStandardItemModel*>(ui->flagsCombo->model());

	int Flags(0); QDialog::accept(); const bool Job = ui->modeCombo->currentIndex();

	if (M->item(1)->checkState() == Qt::Checked) Flags = Flags | 0x01;
	if (M->item(2)->checkState() == Qt::Checked) Flags = Flags | 0x02;

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
