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

JoinDialog::JoinDialog(const QMap<QString, QString>& Points, const QMap<QString, QString>& Lines, const QMap<QString, QString>& Circles, QWidget* Parent)
: QDialog(Parent), ui(new Ui::JoinDialog)
{
	ui->setupUi(this); typeIndexChanged(ui->Type->currentIndex());

	for (auto i = Points.constBegin(); i != Points.constEnd(); ++i)
	{
		ui->Join->addItem(i.value(), i.key());
	}

	for (auto i = Points.constBegin(); i != Points.constEnd(); ++i)
	{
		ui->Point->addItem(i.value(), i.key());
	}

	for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
	{
		ui->Line->addItem(i.value(), i.key());
	}

	for (auto i = Circles.constBegin(); i != Circles.constEnd(); ++i)
	{
		ui->Circle->addItem(i.value(), i.key());
	}
}

JoinDialog::~JoinDialog(void)
{
	delete ui;
}

void JoinDialog::buttonBoxClicked(QAbstractButton* Button)
{
	QComboBox* Join = ui->Type->currentIndex() == 0 ? ui->Line :
				   ui->Type->currentIndex() == 1 ? ui->Point :
				   ui->Type->currentIndex() == 2 ? ui->Circle : nullptr;

	if (Button == ui->buttonBox->button(QDialogButtonBox::Reset))
	{
		emit onDeleteRequest(ui->Join->currentData().toString(),
						 Join->currentData().toString(),
						 ui->Type->currentIndex());
	}
	else if (Button == ui->buttonBox->button(QDialogButtonBox::Apply))
	{
		emit onCreateRequest(ui->Join->currentData().toString(),
						 Join->currentData().toString(),
						 ui->replaceCheck->isChecked(),
						 ui->Type->currentIndex(),
						 ui->Radius->value());
	}

	if (Button != ui->buttonBox->button(QDialogButtonBox::Close)) setEnabled(false);
}

void JoinDialog::typeIndexChanged(int Index)
{
	ui->Line->setVisible(Index == 0);
	ui->Point->setVisible(Index == 1);
	ui->Circle->setVisible(Index == 2);
}

void JoinDialog::targetNameChanged(void)
{
	QComboBox* Join = ui->Type->currentIndex() == 0 ? ui->Line :
				   ui->Type->currentIndex() == 1 ? ui->Point :
				   ui->Type->currentIndex() == 2 ? ui->Circle : nullptr;

	const bool OK = !Join->currentText().isEmpty() &&
				 !ui->Join->currentText().isEmpty() &&
				 ui->Join->currentText() != Join->currentText();

	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(OK);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(OK);
}

void JoinDialog::completeActions(int Count)
{
	QMessageBox::information(this, tr("Progress"), tr("Proceeded %n item(s)", nullptr, Count)); setEnabled(true);
}
