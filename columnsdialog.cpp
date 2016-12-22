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

const QMap<QString, QString> ColumnsDialog::Common =
{
	{"KOD",			tr("Object code")},
	{"NUMER",			tr("Object ID")},
	{"POZYSKANIE",		tr("Source of data")},
	{"DTU",			tr("Creation date")},
	{"DTW",			tr("Modification date")},
	{"DTR",			tr("Delete date")},
	{"STATUS",		tr("Object status")},
	{"OPERAT",		tr("Job name")},
	{"OPERATR",		tr("Operator name")}
};

const QStringList ColumnsDialog::Default =
{
	"KOD", "NUMER", "POZYSKANIE", "DTU", "OPERAT"
};

ColumnsDialog::ColumnsDialog(QWidget* Parent, const QMap<QString, QString>& Attributes)
: QDialog(Parent), ui(new Ui::ColumnsDialog)
{
	ui->setupUi(this);

	ui->commonLayout->setAlignment(Qt::AlignTop);
	ui->specialLayout->setAlignment(Qt::AlignTop);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	const auto Enabled = Settings.value("enabled", Default).toStringList();
	Settings.endGroup();

	for (auto i = Common.constBegin(); i != Common.constEnd(); ++i)
	{
		QCheckBox* Check = new QCheckBox(i.value(), this);

		Check->setChecked(Enabled.contains(i.key()));
		Check->setProperty("KEY", i.key());

		ui->commonLayout->addWidget(Check);
	}

	for (auto i = Attributes.constBegin(); i != Attributes.constEnd(); ++i)
	{
		QCheckBox* Check = new QCheckBox(i.value(), this);

		Check->setChecked(Enabled.contains(i.key()));
		Check->setProperty("KEY", i.key());

		ui->specialLayout->addWidget(Check);
	}

}

ColumnsDialog::~ColumnsDialog(void)
{
	delete ui;
}

void ColumnsDialog::setSpecialAttributes(const QMap<QString, QString>& Attributes)
{
	while (auto I = ui->specialLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	QSettings Settings("EW-Database");

	Settings.beginGroup("Columns");
	const auto Enabled = Settings.value("enabled", Default).toStringList();
	Settings.endGroup();

	for (auto i = Attributes.constBegin(); i != Attributes.constEnd(); ++i)
	{
		QCheckBox* Check = new QCheckBox(i.value(), this);

		Check->setChecked(Enabled.contains(i.key()));
		Check->setProperty("KEY", i.key());

		ui->specialLayout->addWidget(Check);
	}
}
