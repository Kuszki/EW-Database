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

#include "joindialog.hpp"
#include "ui_joindialog.h"

JoinDialog::JoinDialog(const QHash<QString, QString>& Points, const QHash<QString, QString>& Lines, const QHash<QString, QString>& Circles, QWidget* Parent)
: QDialog(Parent), ui(new Ui::JoinDialog), P(Points), L(Lines), C(Circles)
{
	ui->setupUi(this); typeIndexChanged(ui->Type->currentIndex());
}

JoinDialog::~JoinDialog(void)
{
	delete ui;
}

void JoinDialog::buttonBoxClicked(QAbstractButton* Button)
{
	if (Button == ui->buttonBox->button(QDialogButtonBox::Reset))
	{
		emit onDeleteRequest(ui->Join->currentData().toString(),
						 ui->Point->currentData().toString(),
						 ui->Type->currentIndex());
	}
	else if (Button == ui->buttonBox->button(QDialogButtonBox::Apply))
	{
		emit onCreateRequest(ui->Join->currentData().toString(),
						 ui->Point->currentData().toString(),
						 ui->replaceCheck->isChecked(),
						 ui->Type->currentIndex(),
						 ui->Radius->value());
	}

	if (Button != ui->buttonBox->button(QDialogButtonBox::Close)) setEnabled(false);
}

void JoinDialog::typeIndexChanged(int Index)
{
	const QString lastJ = ui->Join->currentText();
	const QString lastP = ui->Point->currentText();

	ui->Join->clear(); QSet<QString> jUsed;
	ui->Point->clear(); QSet<QString> pUsed;

	switch (Index)
	{
		case 3:
			for (auto i = L.constBegin(); i != L.constEnd(); ++i)
			{
				if (jUsed.contains(i.key())) continue;

				ui->Join->addItem(i.value(), i.key());
				jUsed.insert(i.key());
			}
			for (auto i = C.constBegin(); i != C.constEnd(); ++i)
			{
				if (jUsed.contains(i.key())) continue;

				ui->Join->addItem(i.value(), i.key());
				jUsed.insert(i.key());
			}
		default:
			for (auto i = P.constBegin(); i != P.constEnd(); ++i)
			{
				if (jUsed.contains(i.key())) continue;

				ui->Join->addItem(i.value(), i.key());
				jUsed.insert(i.key());
			}
	}

	if (Index == 0 || Index == 3) for (auto i = L.constBegin(); i != L.constEnd(); ++i)
	{
		if (pUsed.contains(i.key())) continue;

		ui->Point->addItem(i.value(), i.key());
		pUsed.insert(i.key());
	}

	if (Index == 1 || Index == 3) for (auto i = P.constBegin(); i != P.constEnd(); ++i)
	{
		if (pUsed.contains(i.key())) continue;

		ui->Point->addItem(i.value(), i.key());
		pUsed.insert(i.key());
	}

	if (Index == 2 || Index == 3) for (auto i = C.constBegin(); i != C.constEnd(); ++i)
	{
		if (pUsed.contains(i.key())) continue;

		ui->Point->addItem(i.value(), i.key());
		pUsed.insert(i.key());
	}

	ui->Join->model()->sort(0); ui->Join->setCurrentText(lastJ);
	ui->Point->model()->sort(0); ui->Point->setCurrentText(lastP);
}

void JoinDialog::targetNameChanged(void)
{
	const bool OK = !ui->Point->currentText().isEmpty() &&
				 !ui->Join->currentText().isEmpty() &&
				 ui->Join->currentText() != ui->Point->currentText();

	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(OK);
	ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(OK);
}

void JoinDialog::completeActions(int Count)
{
	QMessageBox::information(this, tr("Progress"), tr("Proceeded %n item(s)", nullptr, Count)); setEnabled(true);
}
