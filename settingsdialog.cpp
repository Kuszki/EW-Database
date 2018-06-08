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

#include "settingsdialog.hpp"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::SettingsDialog)
{
	ui->setupUi(this); loadValues();
}

SettingsDialog::~SettingsDialog(void)
{
	delete ui;
}

void SettingsDialog::loadValues(void)
{
	Settings.beginGroup("Database");
	ui->driverCombo->setCurrentText(Settings.value("driver", "QIBASE").toString());
	ui->querySpin->setValue(Settings.value("binded", 2500).toUInt());
	Settings.endGroup();

	Settings.beginGroup("History");
	ui->serversEdit->setPlainText(Settings.value("server").toStringList().join(endl));
	ui->basesEdit->setPlainText(Settings.value("path").toStringList().join(endl));
	Settings.endGroup();

	Settings.beginGroup("Locale");
	ui->csvSep->setText(Settings.value("csv").toString());
	ui->txtSep->setText(Settings.value("txt").toString());
	Settings.endGroup();
}

void SettingsDialog::dialogButtonClicked(QAbstractButton* Button)
{
	if (ui->buttonBox->buttonRole(Button) == QDialogButtonBox::ResetRole) loadValues();
	else if (ui->buttonBox->buttonRole(Button) == QDialogButtonBox::RestoreDefaults)
	{
		ui->driverCombo->setCurrentText("QIBASE");
		ui->querySpin->setValue(2500);

		ui->serversEdit->clear();
		ui->basesEdit->clear();

		ui->csvSep->clear();
		ui->txtSep->clear();
	}
}

void SettingsDialog::accept(void)
{
	QDialog::accept(); QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Settings.setValue("driver", ui->driverCombo->currentText());
	Settings.setValue("binded", ui->querySpin->value());
	Settings.endGroup();

	Settings.beginGroup("History");
	Settings.setValue("server", ui->serversEdit->toPlainText().split(endl, QString::SkipEmptyParts));
	Settings.setValue("path", ui->basesEdit->toPlainText().split(endl, QString::SkipEmptyParts));
	Settings.endGroup();

	Settings.beginGroup("Locale");
	Settings.setValue("csv", ui->csvSep->text());
	Settings.setValue("txt", ui->txtSep->text());
	Settings.endGroup();
}
