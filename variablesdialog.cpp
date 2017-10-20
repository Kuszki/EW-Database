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

#include "variablesdialog.hpp"
#include "ui_variablesdialog.h"

VariablesDialog::VariablesDialog(const QHash<QString, QSet<QString>>& Items, QWidget* Parent)
: QDialog(Parent), Data(Items), ui(new Ui::VariablesDialog)
{
	ui->setupUi(this);

	for (const auto& Var: Data.keys()) ui->variableCombo->addItem(Var);

	ui->variableCombo->model()->sort(0);
	ui->variableCombo->setCurrentIndex(0);
}

VariablesDialog::~VariablesDialog(void)
{
	delete ui;
}

void VariablesDialog::accept(void)
{
	const QString Variable = QString("${u.%1}").arg(ui->variableCombo->currentText());

	QDialog::accept(); emit onChangeRequest(Variable);
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
