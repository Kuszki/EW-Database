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

const QStringList ColumnsDialog::Default =
{
	"EW_OBIEKTY.KOD", "EW_OBIEKTY.NUMER", "EW_OBIEKTY.POZYSKANIE", "EW_OBIEKTY.DTU", "EW_OBIEKTY.OPERAT"
};

ColumnsDialog::ColumnsDialog(QWidget* Parent, const QList<QPair<QString, QString>>& Common, const QList<QPair<QString, QString>>& Special)
: QDialog(Parent), ui(new Ui::ColumnsDialog)
{
	ui->setupUi(this);

	ui->commonLayout->setAlignment(Qt::AlignTop);
	ui->specialLayout->setAlignment(Qt::AlignTop);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	const auto Enabled = Settings.value("enabled", Default).toStringList();
	Settings.endGroup();

	for (const auto& Field : Common)
	{
		QCheckBox* Check = new QCheckBox(Field.second, this);

		Check->setChecked(Enabled.contains(Field.first));
		Check->setProperty("KEY", Field.first);
		Check->setToolTip(Field.first);

		ui->commonLayout->addWidget(Check);
	}

	for (const auto& Field : Special)
	{
		QCheckBox* Check = new QCheckBox(Field.second, this);

		Check->setChecked(Enabled.contains(Field.first));
		Check->setProperty("KEY", Field.first);
		Check->setToolTip(Field.first);

		ui->specialLayout->addWidget(Check);
	}

}

ColumnsDialog::~ColumnsDialog(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	Settings.setValue("enabled", QStringList(getEnabledColumns()));
	Settings.endGroup();

	delete ui;
}

QStringList ColumnsDialog::getEnabledColumns(void)
{
	QStringList Enabled;

	for (int i = 0; i < ui->commonLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->commonLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(W->property("KEY").toString());

	for (int i = 0; i < ui->specialLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->specialLayout->itemAt(i)->widget()))
			if (W->isChecked()) Enabled.append(W->property("KEY").toString());

	return Enabled;
}

void ColumnsDialog::searchEdited(const QString& Search)
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
	emit onColumnsUpdate(getEnabledColumns()); QDialog::accept();
}

void ColumnsDialog::setSpecialAttributes(const QList<QPair<QString, QString>>& Attributes)
{
	while (auto I = ui->specialLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	const auto Enabled = Settings.value("enabled", Default).toStringList();
	Settings.endGroup();

	for (const auto& Field : Attributes)
	{
		QCheckBox* Check = new QCheckBox(Field.second, this);

		Check->setChecked(Enabled.contains(Field.first));
		Check->setProperty("KEY", Field.first);

		ui->specialLayout->addWidget(Check);
	}

	emit onColumnsUpdate(getEnabledColumns());
}
