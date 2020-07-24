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

#include "batchdialog.hpp"
#include "ui_batchdialog.h"

BatchDialog::BatchDialog(const QStringList& Fields, const QList<QStringList>& Data, QWidget* Parent)
: QDialog(Parent), ui(new Ui::BatchDialog)
{
	ui->setupUi(this); setParameters(Fields, Data);

	ui->fieldsLayout->setAlignment(Qt::AlignTop);
}

BatchDialog::~BatchDialog(void)
{
	delete ui;
}

QList<BatchWidget::RECORD> BatchDialog::getFunctions(void) const
{
	QList<BatchWidget::RECORD> List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<BatchWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
			List.append(W->getFunction());

	return List;
}

void BatchDialog::accept(void)
{
	QList<QStringList> Data = Values; if (ui->headerCheck->isChecked()) Data.removeFirst();

	QDialog::accept(); emit onBatchRequest(getFunctions(), Data);
}

void BatchDialog::setParameters(const QStringList& Fields, const QList<QStringList>& Data)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Data.first().size(); ++i)
	{
		auto Widget = new BatchWidget(i, Data.first()[i], Fields, this);

		connect(ui->headerCheck, &QCheckBox::toggled, Widget, &BatchWidget::headerChecked);

		ui->fieldsLayout->addWidget(Widget);
	}

	Values = Data;
}
