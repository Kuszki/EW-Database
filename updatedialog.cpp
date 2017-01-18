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

UpdateDialog::UpdateDialog(QWidget* Parent, const QList<DatabaseDriver_v2::FIELD>& Fields)
: QDialog(Parent), ui(new Ui::UpdateDialog)
{
	ui->setupUi(this); setFields(Fields);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	ui->fieldsLayout->setAlignment(Qt::AlignTop);
	ui->prevButton->setEnabled(false);
	ui->nextButton->setEnabled(false);
}

UpdateDialog::~UpdateDialog(void)
{
	delete ui;
}

QMap<int, QVariant> UpdateDialog::getUpdatedValues(void) const
{
	QMap<int, QVariant> List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
			if (W->isChecked()) List.insert(W->getIndex(), W->getValue());

	return List;
}

void UpdateDialog::searchBoxEdited(const QString& Search)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setVisible(W->isEnabled() && W->getLabel().contains(Search, Qt::CaseInsensitive));
		}
}

void UpdateDialog::fieldButtonChecked(bool Enabled)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(Enabled ? ++Count : --Count);
}

void UpdateDialog::allButtonChecked(bool Enabled)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
	{
		if (auto W = dynamic_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setEnabled(Enabled || Active.contains(W->getIndex()));
		}
	}

	searchBoxEdited(ui->searchEdit->text());
}

void UpdateDialog::clearButtonClicked()
{
	setData(Values[Index]);
}

void UpdateDialog::prevButtonClicked(void)
{
	setData(Values[--Index]);

	ui->prevButton->setEnabled(Index);
	ui->nextButton->setEnabled(Index + 1 < Values.size());
}

void UpdateDialog::nextButtonClicked(void)
{
	setData(Values[++Index]);

	ui->prevButton->setEnabled(Index);
	ui->nextButton->setEnabled(Index + 1 < Values.size());
}

void UpdateDialog::accept(void)
{
	emit onValuesUpdate(getUpdatedValues()); QDialog::accept();
}

void UpdateDialog::setFields(const QList<DatabaseDriver_v2::FIELD>& Fields)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Fields.size(); ++i) if (Fields[i].Type != DatabaseDriver_v2::READONLY)
	{
		auto Widget = new UpdateWidget(i, Fields[i], this);

		ui->fieldsLayout->addWidget(Widget);

		connect(Widget, &UpdateWidget::onStatusChanged, this, &UpdateDialog::fieldButtonChecked);
	}
}

void UpdateDialog::setPrepared(const QList<QMap<int, QVariant> >& Data, const QList<int>& Indexes)
{
	setData(Data); setActive(Indexes); setUnchecked();
}

void UpdateDialog::setData(const QList<QMap<int, QVariant>>& Data)
{
	Values = Data; Index = 0;

	if (!Values.isEmpty()) setData(Values.first());

	ui->clearButton->setVisible(!Values.isEmpty());
	ui->nextButton->setVisible(!Values.isEmpty());
	ui->prevButton->setVisible(!Values.isEmpty());

	ui->nextButton->setEnabled(Values.size() > 1);
	ui->prevButton->setEnabled(false);
}

void UpdateDialog::setData(const QMap<int, QVariant>& Data)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
	{
		if (auto W = dynamic_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setValue(Data.value(W->getIndex()));
		}
	}
}

void UpdateDialog::setActive(const QList<int>& Indexes)
{
	Active = Indexes; allButtonChecked(ui->allButton->isChecked());
}

void UpdateDialog::setUnchecked(void)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
	{
		if (auto W = dynamic_cast<UpdateWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setChecked(false);
		}
	}
}
