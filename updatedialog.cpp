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

#include "updatedialog.hpp"
#include "ui_updatedialog.h"

UpdateDialog::UpdateDialog(QWidget* Parent, const QList<QPair<QString, QString>>& Fields, const QHash<QString, QHash<int, QString>>& Dictionary)
: QDialog(Parent), ui(new Ui::UpdateDialog)
{
	ui->setupUi(this); setAvailableFields(Fields, Dictionary);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	ui->fieldsLayout->setAlignment(Qt::AlignTop);
}

UpdateDialog::~UpdateDialog(void)
{
	delete ui;
}

QHash<QString, QString> UpdateDialog::getUpdateRules(void)
{
	QHash<QString, QString> Rules;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
	{
		if (auto W = qobject_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			if (W->isChecked()) Rules.insert(W->objectName(), W->getValue());
		}
	}

	return Rules;
}

void UpdateDialog::searchEdited(const QString& Search)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
			W->setVisible(W->getLabel().contains(Search, Qt::CaseInsensitive));
}

void UpdateDialog::fieldChecked(bool Enabled)
{
	if (Enabled) ++Count; else --Count;

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(Count);
}

void UpdateDialog::accept(void)
{
	emit onValuesUpdate(getUpdateRules()); QDialog::accept();
}

void UpdateDialog::setAvailableFields(const QList<QPair<QString, QString>>& Fields, const QHash<QString, QHash<int, QString>>& Dictionary)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (const auto& Field : Fields)
	{
		auto Widget = new UpdateWidget(Field.second, Field.first, this, Dictionary.value(Field.first));

		ui->fieldsLayout->addWidget(Widget);

		connect(Widget, &UpdateWidget::onStatusChanged, this, &UpdateDialog::fieldChecked);
	}
}

void UpdateDialog::setFieldsData(const QHash<QString, QString>& Data)
{
	for (auto i = Data.constBegin(); i != Data.constEnd(); ++i) for (int j = 0; j < ui->fieldsLayout->count(); ++j)
	{
		if (auto W = dynamic_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(j)->widget()))
		{
			if (W->objectName() == i.key()) W->setValue(i.value());
		}
	}
}
