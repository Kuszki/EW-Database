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
	QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	ui->driverCombo->setCurrentText(Settings.value("driver", "QIBASE").toString());
	ui->querySpin->setValue(Settings.value("binded", 2500).toUInt());
	ui->logCombo->setCurrentIndex(Settings.value("logen", false).toBool());
	ui->logdirEdit->setText(Settings.value("logdir").toString());
	Settings.endGroup();

	Settings.beginGroup("History");
	ui->serversEdit->setPlainText(Settings.value("server").toStringList().join('\n'));
	ui->basesEdit->setPlainText(Settings.value("path").toStringList().join('\n'));
	Settings.endGroup();

	Settings.beginGroup("Locale");
	ui->csvSep->setText(Settings.value("csv").toString());
	ui->txtSep->setText(Settings.value("txt").toString());
	Settings.endGroup();
}

void SettingsDialog::dialogButtonClicked(QAbstractButton* Button)
{
	const auto RD = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
	const auto RS = ui->buttonBox->button(QDialogButtonBox::Reset);
	const auto RJ = ui->buttonBox->button(QDialogButtonBox::Discard);

	const auto BT = dynamic_cast<QPushButton*>(Button);

	if (BT == RS) loadValues();
	else if (BT == RJ) reject();
	else if (BT == RD)
	{
		ui->driverCombo->setCurrentText("QIBASE");
		ui->querySpin->setValue(2500);

		ui->serversEdit->clear();
		ui->basesEdit->clear();

		ui->csvSep->clear();
		ui->txtSep->clear();

		ui->logCombo->setCurrentIndex(0);
		ui->logdirEdit->clear();
	}
}

void SettingsDialog::openButtonClicked(void)
{
	const QString Dir = QFileDialog::getExistingDirectory(this, tr("Select log directory"));

	if (!Dir.isEmpty()) ui->logdirEdit->setText(Dir);
}

void SettingsDialog::accept(void)
{
	QDialog::accept(); QSettings Settings("EW-Database");

	Settings.beginGroup("Database");
	Settings.setValue("driver", ui->driverCombo->currentText());
	Settings.setValue("binded", ui->querySpin->value());
	Settings.setValue("logen", bool(ui->logCombo->currentIndex()));
	Settings.setValue("logdir", ui->logdirEdit->text());
	Settings.endGroup();

	Settings.beginGroup("History");
	Settings.setValue("server", ui->serversEdit->toPlainText().split('\n', QString::SkipEmptyParts));
	Settings.setValue("path", ui->basesEdit->toPlainText().split('\n', QString::SkipEmptyParts));
	Settings.endGroup();

	Settings.beginGroup("Locale");

	if (ui->csvSep->text().isEmpty()) Settings.remove("csv");
	else Settings.setValue("csv", ui->csvSep->text());

	if (ui->txtSep->text().isEmpty()) Settings.remove("txt");
	else Settings.setValue("txt", ui->txtSep->text());

	Settings.endGroup();
}
