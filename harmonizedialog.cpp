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

#include "harmonizedialog.hpp"
#include "ui_harmonizedialog.h"

HarmonizeDialog::HarmonizeDialog(QWidget* Parent, const QString& Path)
: QDialog(Parent), ui(new Ui::HarmonizeDialog), File(Path)
{
	ui->setupUi(this); sourceTypeChanged(ui->sourceCombo->currentIndex());
}

HarmonizeDialog::~HarmonizeDialog(void)
{
	delete ui;
}

void HarmonizeDialog::accept(void)
{
	QDialog::accept();

	emit onFitRequest(File, ui->sourceCombo->currentIndex(),
	                  ui->x1Spin->value() - 1,
	                  ui->y1Spin->value() - 1,
	                  ui->x2Spin->value() - 1,
	                  ui->y2Spin->value() - 1,
	                  ui->distanceSpin->value(),
	                  ui->lengthSpin->value(),
	                  ui->endingsCheck->isChecked());
}

void HarmonizeDialog::open(const QString& Path)
{
	QDialog::open(); File = Path;
}

void HarmonizeDialog::setPath(const QString& Path)
{
	File = Path;
}

QString HarmonizeDialog::getPath(void) const
{
	return File;
}

void HarmonizeDialog::sourceTypeChanged(int Type)
{
	ui->x2Spin->setDisabled(Type == 0);
	ui->y2Spin->setDisabled(Type == 0);
	ui->lengthSpin->setDisabled(Type == 0);

	ui->lengthLabel->setVisible(Type == 2);
	ui->lengthSpin->setVisible(Type == 2);

	ui->endingsCheck->setVisible(Type != 2);

	fitParametersChanged();
}

void HarmonizeDialog::fitParametersChanged(void)
{
	QSet<int> Indexes; bool Accepted;

	const bool Range = ui->lengthSpin->value() >= ui->distanceSpin->value();
	const bool Point = ui->sourceCombo->currentIndex() != 2;

	Indexes.insert(ui->x1Spin->value());
	Indexes.insert(ui->y1Spin->value());

	if (!Point)
	{
		Indexes.insert(ui->x2Spin->value());
		Indexes.insert(ui->y2Spin->value());
	}

	Accepted = (Range || Point) && (Indexes.size() == (Point ? 2 : 4));

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(Accepted);
}
