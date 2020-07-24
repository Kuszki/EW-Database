/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@o2.pl             *
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

#include "variablesdialog.hpp"
#include "ui_variablesdialog.h"

VariablesDialog::VariablesDialog(const QHash<QString, QSet<QString>>& Items, QWidget* Parent)
: QDialog(Parent), Data(Items), ui(new Ui::VariablesDialog)
{
	ui->setupUi(this);

	for (const auto& Var: Data.keys()) ui->variableCombo->addItem(Var);

	ui->variableCombo->model()->sort(0);
	ui->variableCombo->insertItem(0, tr("Keep current"));
	ui->variableCombo->setCurrentIndex(0);

	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
}

VariablesDialog::~VariablesDialog(void)
{
	delete ui;
}

void VariablesDialog::accept(void)
{
	const QString Variable = QString("${u.%1}").arg(ui->variableCombo->currentText());
	const int Underline = ui->underlineCombo->currentIndex();
	const int Pointer = ui->pointerCombo->currentIndex();

	const bool Varchange = ui->variableCombo->currentIndex();

	const double Rotation = ui->rotationSpin->value();

	QDialog::accept(); emit onChangeRequest(Varchange ? Variable : QString(),
									Underline, Pointer,
									Rotation != -1 ? Rotation : NAN);
}

void VariablesDialog::variableIndexChanged(int Index)
{
	if (Index == -1) return; QStringList Items;

	const QString Variable = ui->variableCombo->itemText(Index);

	for (const auto& Info : Data[Variable])
	{
		Items.append(Info);
	}

	ui->affectsLabel->setText(Items.join('\n'));
}

void VariablesDialog::dialogParamsChanged(void)
{
	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(
				ui->variableCombo->currentIndex() ||
				ui->underlineCombo->currentIndex() ||
				ui->pointerCombo->currentIndex() ||
				ui->rotationSpin->value() != -1.0);
}
