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

#include "cutdialog.hpp"
#include "ui_cutdialog.h"

CutDialog::CutDialog(QWidget* Parent, const QList<DatabaseDriver::TABLE>& Tables)
: QDialog(Parent), ui(new Ui::CutDialog)
{
	ui->setupUi(this); setFields(Tables);

	ui->fieldsLayout->setAlignment(Qt::AlignTop);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

CutDialog::~CutDialog(void)
{
	delete ui;
}

QStringList CutDialog::getSelectedClasses(void) const
{
	QStringList List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
			if (W->isChecked()) List.append(W->toolTip());

	return List;
}

bool CutDialog::isEndingsChecked(void) const
{
	return ui->endingsCheck->isChecked();
}

void CutDialog::searchBoxEdited(const QString& Search)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setVisible(W->text().contains(Search, Qt::CaseInsensitive));
		}
}

void CutDialog::fieldButtonChecked(bool Enabled)
{
	if (Enabled) ++Count; else --Count;

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(Count);
}

void CutDialog::accept(void)
{
	emit onClassesUpdate(getSelectedClasses(), isEndingsChecked()); QDialog::accept();
}

void CutDialog::setFields(const QList<DatabaseDriver::TABLE>& Tables)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Tables.size(); ++i) if (Tables[i].Point)
	{
		auto Widget = new QCheckBox(this);

		Widget->setToolTip(Tables[i].Name);
		Widget->setText(Tables[i].Label);

		ui->fieldsLayout->addWidget(Widget);

		connect(Widget, &QCheckBox::toggled, this, &CutDialog::fieldButtonChecked);
	}
}

void CutDialog::setUnchecked(void)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = dynamic_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setChecked(false);
		}
}
