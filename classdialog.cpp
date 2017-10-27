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

ClassDialog::ClassDialog(const QHash<QString, QString>& Classes, const QHash<QString, QHash<int, QString>>& Lines, const QHash<QString, QHash<int, QString>>& Points, const QHash<QString, QHash<int, QString>>& Texts, const QStringList& Variables, QWidget* Parent)
: QDialog(Parent), lineLayers(Lines), pointLayers(Points), textLayers(Texts), ui(new Ui::ClassDialog)
{
	ui->setupUi(this);

	for (auto i = Classes.constBegin(); i != Classes.constEnd(); ++i)
	{
		ui->Class->addItem(i.value(), i.key());
	}

	ui->Class->model()->sort(0);
	ui->Class->setCurrentIndex(0);

	ui->Label->addItems(Variables);
	ui->Label->model()->sort(0);
	ui->Label->setCurrentIndex(0);

	ui->Style->setValidator(new QIntValidator(0, 10000, this));
}

ClassDialog::~ClassDialog(void)
{
	delete ui;
}

void ClassDialog::accept(void)
{
	QDialog::accept();

	emit onChangeRequest(
				ui->classCheck->isChecked() ? ui->Class->currentData().toString() : QString("NULL"),
				ui->lineCheck->isChecked() ? ui->Line->currentData().toInt() : -1,
				ui->pointCheck->isChecked() ? ui->Point->currentData().toInt() : -1,
				ui->textCheck->isChecked() ? ui->Text->currentData().toInt() : -1,
				ui->symbolCheck->isChecked() ? ui->Symbol->text() : QString("NULL"),
				ui->styleCheck->isChecked() ? ui->Style->text().toInt() : -1,
				ui->labelCheck->isChecked() ? QString("${u.%1}").arg(ui->Label->currentText()) : QString("NULL"));
}

void ClassDialog::classIndexChanged(int Index)
{
	const QString Class = ui->Class->itemData(Index).toString();
	const QString Label = ui->Class->itemText(Index);

	const auto& Texts = textLayers[Class]; ui->Text->clear();
	const auto& Lines = lineLayers[Class]; ui->Line->clear();
	const auto& Points = pointLayers[Class]; ui->Point->clear();

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

	ui->Text->model()->sort(0);
	ui->Point->model()->sort(0);
	ui->Line->model()->sort(0);

	ui->Text->setCurrentText(Label);
	ui->Point->setCurrentText(Label);
	ui->Line->setCurrentText(Label);

	ui->Text->setEnabled(ui->Text->count());
	ui->Point->setEnabled(ui->Point->count());
	ui->Line->setEnabled(ui->Line->count());

	ui->advancedCheck->setChecked(ui->advancedCheck->isChecked() ||
		(Texts.size() && !Texts.values().contains(Label)) ||
		(Lines.size() && !Lines.values().contains(Label)) ||
				(Points.size() && !Points.values().contains(Label)));
}

void ClassDialog::classCheckToggled(bool Status)
{
	if (!Status)
	{
		ui->advancedCheck->setChecked(true);
		ui->lineCheck->setChecked(false);
		ui->pointCheck->setChecked(false);
		ui->textCheck->setChecked(false);
	}
}

void ClassDialog::dialogParamsChanged(void)
{
	const bool anyChecked = ui->classCheck->isChecked() ||
					    ui->lineCheck->isChecked() ||
					    ui->pointCheck->isChecked() ||
					    ui->textCheck->isChecked() ||
					    ui->symbolCheck->isChecked() ||
					    ui->styleCheck->isChecked() ||
					    ui->labelCheck->isChecked();

	const bool layersChecked = ui->lineCheck->isChecked() ||
						  ui->pointCheck->isChecked() ||
						  ui->textCheck->isChecked();

	const bool styleOK = !ui->styleCheck->isChecked() ||
					 ui->lineCheck->isChecked() ||
					 !ui->Style->text().isEmpty();

	const bool symbolOK = !ui->symbolCheck->isChecked() ||
					  ui->pointCheck->isChecked() ||
					  !ui->Symbol->text().isEmpty();

	const bool logicFalse = layersChecked && !ui->classCheck->isChecked();

	const bool OK = anyChecked && styleOK && symbolOK && !logicFalse;

	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(OK);
}
