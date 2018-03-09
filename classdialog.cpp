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
	ui->setupUi(this); static const QStringList Policy =
	{
		tr("Convert surfaces into points"),
		tr("Convert surfaces into lines"),
		tr("Convert lines into surfaces"),
		tr("Convert points into circles")
	};

	auto Model = new QStandardItemModel(0, 1, this);
	auto Item = new QStandardItem(tr("Geometry conversion options"));

	int i(0); for (const auto& Text : Policy)
	{
		auto Item = new QStandardItem(Text);

		Item->setData(i);
		Item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		Item->setCheckState(Qt::Unchecked);

		Model->insertRow(i++, Item);
	}

	Model->item(3)->setData(qVariantFromValue(ui->radiusSpin), Qt::UserRole);
	Item->setFlags(Qt::ItemIsEnabled);
	Model->insertRow(0, Item);

	ui->strategyCombo->setModel(Model);

	for (auto i = Classes.constBegin(); i != Classes.constEnd(); ++i)
	{
		ui->Class->addItem(i.value(), i.key());
	}

	ui->Class->model()->sort(0);
	ui->Class->setCurrentIndex(0);

	ui->Label->addItems(Variables);
	ui->Label->model()->sort(0);
	ui->Label->setCurrentIndex(0);

	ui->radiusSpin->setVisible(false);

	ui->Style->setValidator(new QIntValidator(0, 10000, this));

	connect(Model, &QStandardItemModel::itemChanged, this, &ClassDialog::geometryActionsChanged);
}

ClassDialog::~ClassDialog(void)
{
	delete ui;
}

void ClassDialog::accept(void)
{
	int Mask(0); auto M = dynamic_cast<QStandardItemModel*>(ui->strategyCombo->model());

	for (int i = 1; i < M->rowCount(); ++i)
		if (M->item(i)->checkState() == Qt::Checked)
		{
			Mask |= (1 << (i - 1));
		}

	const QString Text = ui->Label->currentText();
	const int Index = ui->Label->findText(Text);
	const QString Label = Index == -1 ? Text : QString("${u.%1}").arg(Text);

	QDialog::accept(); emit onChangeRequest(
			ui->classCheck->isChecked() ? ui->Class->currentData().toString() : QString("NULL"),
			ui->lineCheck->isChecked() ? ui->Line->currentData().toInt() : -1,
			ui->pointCheck->isChecked() ? ui->Point->currentData().toInt() : -1,
			ui->textCheck->isChecked() ? ui->Text->currentData().toInt() : -1,
			ui->symbolCheck->isChecked() ? ui->Symbol->text() : QString("NULL"),
			ui->styleCheck->isChecked() ? ui->Style->text().toInt() : -1,
			ui->labelCheck->isChecked() ? Label : QString("NULL"),
			ui->strategyCombo->isEnabled() ? Mask : 0,
			ui->radiusSpin->value());
}

void ClassDialog::geometryActionsChanged(QStandardItem* Item)
{
	if (auto W = Item->data(Qt::UserRole).value<QWidget*>())
	{
		W->setVisible(Item->checkState() == Qt::Checked);
	}
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

	ui->Text->model()->sort(0); ui->Text->setCurrentText(Label);
	ui->Point->model()->sort(0); ui->Point->setCurrentText(Label);
	ui->Line->model()->sort(0); ui->Line->setCurrentText(Label);

	ui->Text->setEnabled(ui->textCheck->isChecked() && ui->Text->count());
	ui->Point->setEnabled(ui->pointCheck->isChecked() && ui->Point->count());
	ui->Line->setEnabled(ui->lineCheck->isChecked() && ui->Line->count());
}

void ClassDialog::classCheckToggled(bool Status)
{
	if (!Status)
	{
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
