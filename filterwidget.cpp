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

#include "filterwidget.hpp"
#include "ui_filterwidget.h"

FilterWidget::FilterWidget(const QString& Name, const QString& Key, QWidget* Parent)
: QWidget(Parent), ui(new Ui::FilterWidget)
{
	ui->setupUi(this); setName(Key);

	ui->Field->setProperty("KEY", Key);
	ui->Field->setText(Name);
}

FilterWidget::~FilterWidget(void)
{
	delete ui;
}

QString FilterWidget::getCondition(void) const
{
	if (ui->Operator->currentText() == "ON" || ui->Operator->currentText() == "NOT ON")
	{
		return QString("%1 %2 ('%3')")
				.arg(objectName())
				.arg(ui->Operator->currentText())
				.arg(ui->Value->text().split(',', QString::SkipEmptyParts).join("', "));
	}
	else
	{
		return QString("%1 %2 '%3'")
				.arg(objectName())
				.arg(ui->Operator->currentText())
				.arg(ui->Value->text());
	}
}

QString FilterWidget::getValue(void) const
{
	return ui->Value->text();
}

void FilterWidget::editFinished(void)
{
	emit onValueUpdate(ui->Field->property("KEY").toString(), ui->Value->text());
}

void FilterWidget::setParameters(const QString& Name, const QString& Key, const QString& Value)
{
	ui->Field->setProperty("KEY", Key);
	ui->Field->setText(Name);
	ui->Value->setText(Value);

	setName(Key);
}

void FilterWidget::setName(const QString& Name)
{
	ui->Field->setText(Name);
}

void FilterWidget::setKey(const QString& Key)
{
	ui->Field->setProperty("KEY", Key); setName(Key);
}

void FilterWidget::setValue(const QString& Value)
{
	ui->Value->setText(Value);
}

void FilterWidget::reset(void)
{
	ui->Field->setChecked(false);
	ui->Value->clear();
}
