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

#include "edgesdialog.hpp"
#include "ui_edgesdialog.h"

EdgesDialog::EdgesDialog(QWidget* Parent, const QList<DatabaseDriver::FIELD>& Fields)
: QDialog(Parent), ui(new Ui::EdgesDialog)
{
	ui->setupUi(this); setFields(Fields);

	ui->fieldsLayout->setAlignment(Qt::AlignTop);

	ui->allButton->setFixedSize(ui->buttonBox->sizeHint().height(),
						   ui->buttonBox->sizeHint().height());
}

EdgesDialog::~EdgesDialog(void)
{
	delete ui;
}

QList<int> EdgesDialog::getSelectedFields(void) const
{
	QList<int> List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
			if (W->isEnabled() && W->isChecked()) List.append(W->property("ID").toInt());

	return List;
}

void EdgesDialog::searchBoxEdited(const QString& Search)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setVisible(W->isEnabled() && W->text().contains(Search, Qt::CaseInsensitive));
		}
}

void EdgesDialog::allButtonChecked(bool Enabled)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = dynamic_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			const bool OK = Active.contains(W->property("ID").toInt());

			W->setEnabled(Enabled || OK);
			W->setVisible(Enabled || OK);
		}
}

void EdgesDialog::accept(void)
{
	QDialog::accept(); emit onEdgeRequest(getSelectedFields());
}

void EdgesDialog::setFields(const QList<DatabaseDriver::FIELD>& Fields)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Fields.size(); ++i) if (Fields[i].Type != DatabaseDriver::READONLY)
	{
		auto Widget = new QCheckBox(this);

		Widget->setToolTip(Fields[i].Name);
		Widget->setText(Fields[i].Label);
		Widget->setProperty("ID", i);

		ui->fieldsLayout->addWidget(Widget);
	}
}

void EdgesDialog::setActive(const QList<int>& Indexes)
{
	Active = Indexes; allButtonChecked(ui->allButton->isChecked());
}

void EdgesDialog::setUnchecked(void)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = dynamic_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setChecked(false);
		}
}
