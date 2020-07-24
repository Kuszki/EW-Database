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

#include "copyfieldsdialog.hpp"
#include "ui_copyfieldsdialog.h"

CopyfieldsDialog::CopyfieldsDialog(const QStringList& Fields, QWidget* Parent)
: QDialog(Parent), ui(new Ui::CopyfieldsDialog)
{
	ui->setupUi(this); setFields(Fields);

	ui->fieldsLayout->setAlignment(Qt::AlignTop);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	ui->toolButton->setFixedSize(ui->buttonBox->sizeHint().height(),
						    ui->buttonBox->sizeHint().height());
}

CopyfieldsDialog::~CopyfieldsDialog(void)
{
	Count = 0; delete ui;
}

QList<CopyfieldsWidget::RECORD> CopyfieldsDialog::getFunctions(void) const
{
	QList<CopyfieldsWidget::RECORD> List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<CopyfieldsWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
			List.append(W->getFunction());

	return List;
}

void CopyfieldsDialog::newButtonClicked(void)
{
	auto W = new CopyfieldsWidget(List, this);

	ui->fieldsLayout->addWidget(W); ++Count;

	connect(W, &CopyfieldsWidget::destroyed,
		   this, &CopyfieldsDialog::copyWidgetDeleted);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void CopyfieldsDialog::copyWidgetDeleted(void)
{
	if (!--Count) ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void CopyfieldsDialog::accept(void)
{
	QDialog::accept(); emit onCopyRequest(getFunctions(), ui->nullCheck->isChecked());
}

void CopyfieldsDialog::setFields(const QStringList& Fields)
{
	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<CopyfieldsWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
			W->setData(Fields);

	List = Fields;
}
