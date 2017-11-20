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

#include "exportdialog.hpp"
#include "ui_exportdialog.h"

ExportDialog::ExportDialog(QWidget *Parent, const QStringList& Headers)
: QDialog(Parent), ui(new Ui::ExportDialog)
{
	ui->setupUi(this); setAttributes(Headers);

	ui->itemsLayout->setAlignment(Qt::AlignTop);
}

ExportDialog::~ExportDialog(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	Settings.setValue("saved", QStringList(getEnabledColumnsNames()));
	Settings.endGroup();

	delete ui;
}

QList<int> ExportDialog::getEnabledColumnsIndexes(void)
{
	QList<int> Enabled; int Item = 0;

	for (int i = 0; i < ui->itemsLayout->count(); ++i, ++Item)
		if (auto W = qobject_cast<QCheckBox*>(ui->itemsLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(Item);

	return Enabled;
}

QStringList ExportDialog::getEnabledColumnsNames(void)
{
	QStringList Enabled;

	for (int i = 0; i < ui->itemsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->itemsLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(W->text());

	return Enabled;
}

void ExportDialog::selectButtonClicked(void)
{
	for (int i = 0; i < ui->itemsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->itemsLayout->itemAt(i)->widget()))
		{
			if (W->isVisible()) W->setChecked(true);
		}
}

void ExportDialog::unselectButtonClicked(void)
{
	for (int i = 0; i < ui->itemsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->itemsLayout->itemAt(i)->widget()))
		{
			if (W->isVisible()) W->setChecked(false);
		}
}

void ExportDialog::searchTextEdited(const QString& Search)
{
	for (int i = 0; i < ui->itemsLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->itemsLayout->itemAt(i)->widget()))
			W->setVisible(W->text().contains(Search, Qt::CaseInsensitive));
}

void ExportDialog::typeIndexChanged(int Type)
{
	ui->classSearch->setEnabled(Type == 2);
	ui->selectButton->setEnabled(Type == 2);
	ui->unselectButton->setEnabled(Type == 2);
	ui->itemsScrool->setEnabled(Type == 2);

	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(Type != 2 || Count);
}

void ExportDialog::itemCheckChanged(bool Checked)
{
	bool OK = ui->typeCombo->currentIndex() != 2; if (Checked) ++Count; else --Count;

	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(OK || Count);
}

void ExportDialog::accept(void)
{
	QDialog::accept(); emit onExportRequest(getEnabledColumnsIndexes(), ui->typeCombo->currentIndex(), ui->headerCheck->isChecked());
}

void ExportDialog::setAttributes(const QStringList& Headers)
{
	while (auto I = ui->itemsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater(); Count = 0;

	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	const auto Enabled = Settings.value("saved").toStringList();
	Settings.endGroup();

	for (const auto& Field : Headers)
	{
		QCheckBox* Check = new QCheckBox(Field, this);
		const bool Checked = Enabled.contains(Field);

		Check->setChecked(Checked);
		ui->itemsLayout->addWidget(Check);
		Count += Checked;

		connect(Check, &QCheckBox::toggled, this, &ExportDialog::itemCheckChanged);
	}

	typeIndexChanged(ui->typeCombo->currentIndex());
}
