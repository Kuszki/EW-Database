/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@o2.pl             *
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

#include "kergdialog.hpp"
#include "ui_kergdialog.h"

KergDialog::KergDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::KergDialog)
{
	ui->setupUi(this); static const QStringList Policy =
	{
		tr("Load lines"),
		tr("Load symbols"),
		tr("Load labels")
	};

	auto Model = new QStandardItemModel(0, 1, this);
	auto Item = new QStandardItem(tr("Loaded elements"));

	Checked = Policy.size();

	int i(0); for (const auto& Text : Policy)
	{
		auto Item = new QStandardItem(Text);

		Item->setData(i);
		Item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		Item->setCheckState(Qt::Checked);

		Model->insertRow(i++, Item);
	}

	Item->setFlags(Qt::ItemIsEnabled);
	Model->insertRow(0, Item);

	ui->optionsCombo->setModel(Model);

	actionIndexChanged(ui->actionCombo->currentIndex());

	ui->openButton->setFixedSize(ui->sourceEdit->sizeHint().height(),
						    ui->sourceEdit->sizeHint().height());

	connect(Model, &QStandardItemModel::itemChanged, this, &KergDialog::elementsActionsChanged);
}

KergDialog::~KergDialog(void)
{
	delete ui;
}

void KergDialog::accept(void)
{
	int Mask(0); auto M = dynamic_cast<QStandardItemModel*>(ui->optionsCombo->model());

	for (int i = 1; i < M->rowCount(); ++i)
		if (M->item(i)->checkState() == Qt::Checked)
		{
			Mask |= (1 << (i - 1));
		}

	QDialog::accept();

	emit onUpdateRequest(ui->sourceEdit->text(),
					 ui->actionCombo->currentIndex(),
					 Mask);
}

void KergDialog::elementsActionsChanged(QStandardItem* Item)
{
	if (Item->checkState() == Qt::Checked) ++Checked;
	if (Item->checkState() == Qt::Unchecked) --Checked;

	dialogParamsChanged();
}

void KergDialog::actionIndexChanged(int Index)
{
	ui->sourceEdit->setEnabled(Index == 1);
	ui->openButton->setEnabled(Index == 1);

	dialogParamsChanged();
}

void KergDialog::dialogParamsChanged(void)
{
	const bool Path = ui->actionCombo->currentIndex() == 1;

	const bool OK = Checked && (!Path || !ui->sourceEdit->text().isEmpty());

	ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(OK);
}

void KergDialog::openButtonClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open data file"), QString(),
											tr("CSV files (*.csv);;Text files (*.txt);;All files (*.*)"));

	if (!Path.isEmpty()) ui->sourceEdit->setText(Path); dialogParamsChanged();
}
