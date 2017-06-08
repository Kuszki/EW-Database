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

#include "mergedialog.hpp"
#include "ui_mergedialog.h"

MergeDialog::MergeDialog(QWidget* Parent, const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables)
: QDialog(Parent), ui(new Ui::MergeDialog)
{
	ui->setupUi(this); setFields(Fields, Tables);

	ui->fieldsLayout->setAlignment(Qt::AlignTop);
}

MergeDialog::~MergeDialog(void)
{
	delete ui;
}

QList<int> MergeDialog::getSelectedFields(void) const
{
	QList<int> List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
			if (W->isEnabled() && W->isChecked()) List.append(W->property("ID").toInt());

	return List;
}

QStringList MergeDialog::getFilterClasses(void) const
{

}

void MergeDialog::searchBoxEdited(const QString& Search)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setVisible(W->isEnabled() && W->text().contains(Search, Qt::CaseInsensitive));
		}
}

void MergeDialog::allButtonChecked(bool Enabled)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = dynamic_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setEnabled(Enabled || Active.contains(W->property("ID").toInt()));
		}

	searchBoxEdited(ui->searchEdit->text());
}

void MergeDialog::accept(void)
{
	emit onFieldsUpdate(getSelectedFields(), getFilterClasses()); QDialog::accept();
}

void MergeDialog::setFields(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Fields.size(); ++i)
	{
		auto Widget = new QCheckBox(this);

		Widget->setToolTip(Fields[i].Name);
		Widget->setText(Fields[i].Label);
		Widget->setProperty("ID", i);

		ui->fieldsLayout->addWidget(Widget);
	}

	auto Model = new QStandardItemModel(Tables.size(), 1, this);
	auto Item = new QStandardItem(tr("Skip merge on points"));
	auto Oldmodel = ui->filterBox->model(); int j = 0;

	for (const auto& Table : Tables)
	{
		auto Item = new QStandardItem(Table.Label);

		Item->setData(Table.Name);
		Item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		Item->setCheckState(Qt::Unchecked);

		Model->setItem(j++, Item);
	}

	Model->sort(0);
	Item->setFlags(Qt::ItemIsEnabled);
	Model->insertRow(0, Item);

	ui->filterBox->setModel(Model);

	Oldmodel->deleteLater();
}

void MergeDialog::setActive(const QList<int>& Indexes)
{
	Active = Indexes; allButtonChecked(ui->allButton->isChecked());
}

void MergeDialog::setUnchecked(void)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = dynamic_cast<QCheckBox*>(ui->fieldsLayout->itemAt(i)->widget()))
		{
			W->setChecked(false);
		}
}
