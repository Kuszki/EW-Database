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

#include "classdialog.hpp"
#include "ui_classdialog.h"

ClassDialog::ClassDialog(const QHash<QString, QString>& Classes, const QHash<QString, QHash<int, QString>>& Lines, const QHash<QString, QHash<int, QString>>& Points, const QHash<QString, QHash<int, QString>>& Texts, QWidget* Parent)
: QDialog(Parent), lineLayers(Lines), pointLayers(Points), textLayers(Texts), ui(new Ui::ClassDialog)
{
	ui->setupUi(this); for (auto i = Classes.constBegin(); i != Classes.constEnd(); ++i) ui->Class->addItem(i.value(), i.key());
}

ClassDialog::~ClassDialog(void)
{
	delete ui;
}

void ClassDialog::accept(void)
{
	emit onChangeRequest(ui->Class->currentData().toString(),
					 ui->Line->currentData().toInt(),
					 ui->Point->currentData().toInt(),
					 ui->Text->currentData().toInt());

	QDialog::accept();
}

void ClassDialog::classIndexChanged(int Index)
{
	const QString Class = ui->Class->itemData(Index).toString();
	const QString Label = ui->Class->itemText(Index);

	const auto& Texts = textLayers[Class];
	const auto& Lines = lineLayers[Class];
	const auto& Points = pointLayers[Class];

	ui->Line->clear(); ui->Text->clear();

	for (auto i = Texts.constBegin(); i != Texts.constEnd(); ++i)
	{
		ui->Text->addItem(i.value(), i.key());
	}

	for (auto i = Points.constBegin(); i != Points.constEnd(); ++i)
	{
		ui->Point->addItem(i.value(), i.key());
	}

	for (auto i = Lines.constBegin(); i != Lines.constEnd(); ++i)
	{
		ui->Line->addItem(i.value(), i.key());
	}

	ui->Text->setCurrentText(Label);
	ui->Point->setCurrentText(Label);
	ui->Line->setCurrentText(Label);

	ui->advancedCheck->setChecked(
		(Texts.size() && !Texts.values().contains(Label)) ||
		(Lines.size() && !Lines.values().contains(Label)) ||
		(Points.size() && !Points.values().contains(Label)));
}
