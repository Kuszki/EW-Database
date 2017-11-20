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

#include "groupdialog.hpp"
#include "ui_groupdialog.h"

GroupDialog::GroupDialog(QWidget* Parent, const QStringList& Attributes)
: QDialog(Parent), ui(new Ui::GroupDialog)
{
	ui->setupUi(this); setAttributes(Attributes);
}

GroupDialog::~GroupDialog(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Groups");
	Settings.setValue("enabled", getEnabledGroupsNames());
	Settings.endGroup();

	delete ui;
}

QList<int> GroupDialog::getEnabledGroupsIndexes(void)
{
	QList<int> Enabled;

	for (int i = 0; i < ui->Enabled->count(); ++i)
		Enabled.append(ui->Enabled->item(i)->data(Qt::UserRole).toInt());

	return Enabled;
}

QStringList GroupDialog::getEnabledGroupsNames(void)
{
	QStringList Enabled;

	for (int i = 0; i < ui->Enabled->count(); ++i)
		Enabled.append(ui->Enabled->item(i)->text());

	return Enabled;
}

void GroupDialog::searchEdited(const QString& Search)
{
	for (int i = 0; i < ui->Disabled->count(); ++i)
		ui->Disabled->setRowHidden(i, !ui->Disabled->item(i)->text().contains(Search, Qt::CaseInsensitive));
}

void GroupDialog::accept(void)
{
	QDialog::accept(); emit onGroupsUpdate(getEnabledGroupsIndexes());
}

void GroupDialog::setAttributes(QStringList Attributes)
{
	while (ui->Disabled->count()) delete ui->Disabled->takeItem(0);
	while (ui->Enabled->count()) delete ui->Enabled->takeItem(0);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Groups");
	auto Enabled = Settings.value("enabled").toStringList();
	Settings.endGroup();

	for (auto& Field : Attributes) if (Enabled.contains(Field))
	{
		QListWidgetItem* Item = new QListWidgetItem(Field);

		Item->setData(Qt::UserRole, Attributes.indexOf(Field));
		ui->Enabled->addItem(Item);

		Field = QString();
	}

	for (auto& Field : Attributes) if (!Field.isEmpty())
	{
		QListWidgetItem* Item = new QListWidgetItem(Field);

		Item->setData(Qt::UserRole, Attributes.indexOf(Field));
		ui->Disabled->addItem(Item);
	}

	emit onGroupsUpdate(getEnabledGroupsIndexes());
}
