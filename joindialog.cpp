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

#include "joindialog.hpp"
#include "ui_joindialog.h"

JoinDialog::JoinDialog(const QMap<QString, QString>& Points, const QMap<QString, QString>& Lines, QWidget* Parent)
: QDialog(Parent), ui(new Ui::JoinDialog)
{
	ui->setupUi(this);

	for (auto i = Points.constBegin(); i != Points.constEnd(); ++i)
	{
		ui->Point->addItem(i.value(), i.key());
	}

	for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
	{
		ui->Line->addItem(i.value(), i.key());
	}
}

JoinDialog::~JoinDialog(void)
{
	delete ui;
}

void JoinDialog::buttonBoxClicked(QAbstractButton* Button)
{
	if (Button == ui->buttonBox->button(QDialogButtonBox::Reset))
	{
		emit onDeleteRequest(ui->Point->currentData().toString(), ui->Line->currentData().toString());
	}
	else if (Button == ui->buttonBox->button(QDialogButtonBox::Apply))
	{
		emit onCreateRequest(ui->Point->currentData().toString(), ui->Line->currentData().toString());
	}
}

void JoinDialog::completeActions(int Count)
{
	QMessageBox::information(this, tr("Progress"), tr("Proceeded %n item(s)", nullptr, Count));
}
