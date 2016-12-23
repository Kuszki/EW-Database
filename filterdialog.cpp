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

#include "filterdialog.hpp"
#include "ui_filterdialog.h"

FilterDialog::FilterDialog(QWidget* Parent, const QMap<QString, QString>& Fields)
: QDialog(Parent), ui(new Ui::FilterDialog)
{
	ui->setupUi(this); setAvailableFields(Fields);

	connect(ui->tabWidget, &QTabWidget::currentChanged, ui->searchEdit, &QLineEdit::setHidden);
}

FilterDialog::~FilterDialog(void)
{
	delete ui;
}

QString FilterDialog::getFilterRules(void)
{
	if (ui->tabWidget->currentIndex() == 1)
	{
		if (ui->Setup->document()->toPlainText().isEmpty()) return QString();
		return QString("WHERE %1").arg(ui->Setup->document()->toPlainText());
	}
	else
	{
		QStringList Rules; for (int i = 0; i < ui->simpleLayout->count(); ++i)
		{
			if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
			{
				Rules.append(W->getCondition());
			}
		}

		if (Rules.isEmpty()) return QString();
		else return QString("WHERE %1").arg(Rules.join(" AND "));
	}
}

void FilterDialog::searchEdited(const QString& Search)
{
	for (int i = 0; i < ui->simpleLayout->count(); ++i)
		if (auto W = ui->simpleLayout->itemAt(i)->widget())
			W->setVisible(W->objectName().contains(Search, Qt::CaseInsensitive));
}

void FilterDialog::buttonClicked(QAbstractButton* Button)
{
	if (Button != ui->buttonBox->button(QDialogButtonBox::Reset)) return;

	for (int i = 0; i < ui->simpleLayout->count(); ++i)
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
			W->reset();

	ui->Setup->document()->clear();
}

void FilterDialog::accept(void)
{
	emit onFiltersUpdate(getFilterRules()); QDialog::accept();
}

void FilterDialog::setAvailableFields(const QMap<QString, QString>& Fields)
{
	while (auto I = ui->simpleLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (auto i = Fields.constBegin(); i != Fields.constEnd(); ++i)
	{
		ui->simpleLayout->addWidget(new FilterWidget(i.value(), i.key(), this));
	}
}
