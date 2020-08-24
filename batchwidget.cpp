/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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

#include "batchwidget.hpp"
#include "ui_batchwidget.h"

BatchWidget::BatchWidget(int ID, const QString& Tip, const QStringList& Fields, QWidget* Parent)
: QWidget(Parent), ui(new Ui::BatchWidget)
{
	ui->setupUi(this); setData(ID, Tip, Fields);
}

BatchWidget::~BatchWidget(void)
{
	delete ui;
}

BatchWidget::RECORD BatchWidget::getFunction(void) const
{
	return qMakePair(getField(), getAction());
}

BatchWidget::FUNCTION BatchWidget::getAction(void) const
{
	return FUNCTION(ui->Function->currentIndex());
}

int BatchWidget::getField(void) const
{
	return ui->Field->currentIndex();
}

int BatchWidget::getIndex(void) const
{
	return Index;
}

void BatchWidget::headerChecked(bool Checked)
{
	if (Checked) ui->Field->setCurrentText(ui->Column->toolTip());
}

void BatchWidget::setData(int ID, const QString& Tip, const QStringList& Fields)
{
	ui->Field->clear(); Index = ID;

	ui->Column->setToolTip(Tip);
	ui->Column->setText(QString::number(ID + 1));
	ui->Field->addItems(Fields);
}
