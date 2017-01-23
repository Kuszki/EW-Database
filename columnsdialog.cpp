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

#include "columnsdialog.hpp"
#include "ui_columnsdialog.h"

ColumnsDialog::ColumnsDialog(QWidget* Parent, const QStringList Headers, int Common)
: QDialog(Parent), ui(new Ui::ColumnsDialog)
{
	ui->setupUi(this); setAttributes(Headers, Common);

	ui->commonLayout->setAlignment(Qt::AlignTop);
	ui->specialLayout->setAlignment(Qt::AlignTop);
}

ColumnsDialog::~ColumnsDialog(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	Settings.setValue("enabled", QStringList(getEnabledColumnsNames()));
	Settings.endGroup();

	delete ui;
}

QList<int> ColumnsDialog::getEnabledColumnsIndexes(void)
{
	QList<int> Enabled; int Item = 0;

	for (int i = 0; i < ui->commonLayout->count(); ++i, ++Item)
		if (auto W = qobject_cast<QCheckBox*>(ui->commonLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(Item);

	for (int i = 0; i < ui->specialLayout->count(); ++i, ++Item)
		if (auto W = qobject_cast<QCheckBox*>(ui->specialLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(Item);

	return Enabled;
}

QStringList ColumnsDialog::getEnabledColumnsNames(void)
{
	QStringList Enabled;

	for (int i = 0; i < ui->commonLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->commonLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(W->text());

	for (int i = 0; i < ui->specialLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->specialLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(W->text());

	return Enabled;
}

void ColumnsDialog::selectButtonClicked(void)
{
	auto Layout = ui->tabWidget->currentIndex() ? ui->specialLayout : ui->commonLayout;

	for (int i = 0; i < Layout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(Layout->itemAt(i)->widget()))
		{
			if (W->isVisible()) W->setChecked(true);
		}
}

void ColumnsDialog::unselectButtonClicked(void)
{
	auto Layout = ui->tabWidget->currentIndex() ? ui->specialLayout : ui->commonLayout;

	for (int i = 0; i < Layout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(Layout->itemAt(i)->widget()))
		{
			if (W->isVisible()) W->setChecked(false);
		}
}

void ColumnsDialog::searchTextEdited(const QString& Search)
{
	for (int i = 0; i < ui->commonLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->commonLayout->itemAt(i)->widget()))
			W->setVisible(W->text().contains(Search, Qt::CaseInsensitive));

	for (int i = 0; i < ui->specialLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->specialLayout->itemAt(i)->widget()))
			W->setVisible(W->text().contains(Search, Qt::CaseInsensitive));
}

void ColumnsDialog::accept(void)
{
	emit onColumnsUpdate(getEnabledColumnsIndexes()); QDialog::accept();
}

void ColumnsDialog::setAttributes(const QStringList Headers, int Common)
{
	while (auto I = ui->commonLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();
	while (auto I = ui->specialLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	const auto Enabled = Settings.value("enabled").toStringList();
	Settings.endGroup();

	int i = 0; for (const auto& Field : Headers)
	{
		QCheckBox* Check = new QCheckBox(Field, this);

		Check->setChecked(Enabled.contains(Field));

		if (i++ < Common) ui->commonLayout->addWidget(Check);
		else ui->specialLayout->addWidget(Check);
	}

	emit onColumnsUpdate(getEnabledColumnsIndexes());
}
