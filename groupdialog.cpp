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

const QStringList GroupDialog::Default =
{
	"EW_OPERATY.NUMER"
};

GroupDialog::GroupDialog(QWidget* Parent, const QMap<QString, QString>& Attributes)
: QDialog(Parent), ui(new Ui::GroupDialog)
{
	ui->setupUi(this);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Groups");
	setAvailableAttributes(Attributes, Settings.value("enabled").toStringList());
	Settings.endGroup();
}

GroupDialog::~GroupDialog(void)
{
	QSettings Settings("EW-Database"); QStringList Enabled;

	for (int i = 0; i < ui->Enabled->count(); ++i)
		Enabled.append(ui->Enabled->item(i)->data(Qt::UserRole).toString());

	Settings.beginGroup("Groups");
	Settings.setValue("enabled", Enabled);
	Settings.endGroup();

	delete ui;
}

QStringList GroupDialog::getEnabledGroups(void)
{
	QStringList Enabled;

	for (int i = 0; i < ui->Enabled->count(); ++i)
		Enabled.append(ui->Enabled->item(i)->data(Qt::UserRole).toString());

	return Enabled;
}

void GroupDialog::searchEdited(const QString& Search)
{
	for (int i = 0; i < ui->Enabled->count(); ++i)
		ui->Enabled->setRowHidden(i, !ui->Enabled->item(i)->text().contains(Search, Qt::CaseInsensitive));

	for (int i = 0; i < ui->Disabled->count(); ++i)
		ui->Disabled->setRowHidden(i, !ui->Disabled->item(i)->text().contains(Search, Qt::CaseInsensitive));
}

void GroupDialog::accept(void)
{
	emit onGroupsUpdate(getEnabledGroups()); QDialog::accept();
}

void GroupDialog::setAvailableAttributes(QMap<QString, QString> Attributes, const QStringList& Enabled)
{
	for (int i = 0; i < ui->Disabled->count(); ++i) delete ui->Disabled->takeItem(i);

	for (int i = 0; i < ui->Enabled->count(); ++i)
	{
		const QString Key = ui->Enabled->item(i)->data(Qt::UserRole).toString();

		if (Attributes.contains(Key)) Attributes.remove(Key);
		else delete ui->Enabled->takeItem(i--);
	}

	for (const auto& Key : Enabled) if (Attributes.contains(Key))
	{
		QListWidgetItem* Item = new QListWidgetItem(Attributes[Key]);

		Item->setData(Qt::UserRole, Key);
		ui->Enabled->addItem(Item);

		Attributes.remove(Key);
	}

	for (auto i = Attributes.constBegin(); i != Attributes.constEnd(); ++i)
	{
		QListWidgetItem* Item = new QListWidgetItem(i.value());

		Item->setData(Qt::UserRole, i.key());
		ui->Disabled->addItem(Item);
	}

	emit onGroupsUpdate(getEnabledGroups());
}
