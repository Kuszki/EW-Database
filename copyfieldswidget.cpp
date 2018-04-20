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

#include "copyfieldswidget.hpp"
#include "ui_copyfieldswidget.h"

CopyfieldsWidget::CopyfieldsWidget(const QStringList& Fields, QWidget* Parent)
: QWidget(Parent), ui(new Ui::CopyfieldsWidget)
{
	ui->setupUi(this); setData(Fields);
}

CopyfieldsWidget::~CopyfieldsWidget(void)
{
	delete ui;
}

CopyfieldsWidget::RECORD CopyfieldsWidget::getFunction(void) const
{
	return qMakePair(getAction(), qMakePair(getSrc(), getDest()));
}

CopyfieldsWidget::FUNCTION CopyfieldsWidget::getAction(void) const
{
	return FUNCTION(ui->functionCombo->currentIndex());
}

int CopyfieldsWidget::getSrc(void) const
{
	return ui->srcCombo->currentIndex();
}

int CopyfieldsWidget::getDest(void) const
{
	return ui->destCombo->currentIndex();
}

void CopyfieldsWidget::setData(const QStringList& Fields)
{
	ui->srcCombo->clear(); ui->srcCombo->addItems(Fields);
	ui->destCombo->clear(); ui->destCombo->addItems(Fields);
}
