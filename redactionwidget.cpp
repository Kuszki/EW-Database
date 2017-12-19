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

#include "redactionwidget.hpp"
#include "ui_redactionwidget.h"

const QVector<int> RedactionWidget::Numbers = { 0, 1, 2, 3 };
const QVector<int> RedactionWidget::Texts = { 6, 7, 8, 9, 10, 11 };
const QVector<int> RedactionWidget::Combos = { 4, 5 };

RedactionWidget::RedactionWidget(QWidget *Parent)
: QWidget(Parent), ui(new Ui::RedactionWidget)
{
	ui->setupUi(this);

	ui->formatCombo->setItemData(0, 16);
	ui->formatCombo->setItemData(1, 64);
	ui->formatCombo->setItemData(2, 32);

	typeChanged(ui->typeCombo->currentIndex());
}

RedactionWidget::~RedactionWidget(void)
{
	delete ui;
}

QPair<int, QVariant> RedactionWidget::getCondition(void) const
{
	static const double MUL = M_PI / 180.0;

	const int Index = ui->typeCombo->currentIndex();

	if (Numbers.contains(Index)) return qMakePair(Index, ui->valueSpin->value() * MUL);
	if (Texts.contains(Index)) return qMakePair(Index, ui->symbolEdit->text());
	if (Combos.contains(Index)) return qMakePair(Index, ui->formatCombo->currentData().toInt());

	return qMakePair(Index, QVariant());
}

void RedactionWidget::typeChanged(int Index)
{
	ui->valueSpin->setVisible(Numbers.contains(Index));
	ui->symbolEdit->setVisible(Texts.contains(Index));
	ui->formatCombo->setVisible(Combos.contains(Index));

	if (Index == 6 || Index == 7)
	{
		ui->symbolEdit->setPlaceholderText(tr("Any symbol"));
	}

	if (Index == 8 || Index == 9)
	{
		ui->symbolEdit->setPlaceholderText(tr("Any style"));
	}

	if (Index == 10 || Index == 11)
	{
		ui->symbolEdit->setPlaceholderText(tr("Any label"));
	}
}

void RedactionWidget::editFinished(void)
{
	emit onValueUpdate(getCondition());
}
