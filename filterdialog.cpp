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

FilterDialog::FilterDialog(QWidget* Parent, const QList<QPair<QString, QString>>& Fields)
: QDialog(Parent), ui(new Ui::FilterDialog)
{
	ui->setupUi(this); setAvailableFields(Fields);

	ui->Operator->addItems(DatabaseDriver::fieldOperators);

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
		return ui->Setup->document()->toPlainText();
	}
	else
	{
		QStringList Rules; for (int i = 0; i < ui->simpleLayout->count(); ++i)
		{
			if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
			{
				if (W->isChecked()) Rules.append(W->getCondition());
			}
		}

		if (Rules.isEmpty()) return QString();
		else return Rules.join(" AND ");
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

void FilterDialog::addClicked(void)
{
	QString Line;

	if (!ui->Setup->document()->toPlainText().trimmed().isEmpty()) Line.append(ui->Action->currentText()).append(' ');

	if (ui->Operator->currentText() == "IN" || ui->Operator->currentText() == "NOT IN")
	{
		Line.append(QString("%1 %2 ('%3')")
				.arg(ui->Field->currentData(Qt::UserRole).toString())
				.arg(ui->Operator->currentText())
				.arg(ui->Value->text().
					split(QRegExp("\\s*,\\s*"),
						 QString::SkipEmptyParts)
					.join("', '")));
	}
	else
	{
		Line.append(QString("%1 %2 '%3'")
				.arg(ui->Field->currentData(Qt::UserRole).toString())
				.arg(ui->Operator->currentText())
				.arg(ui->Value->text()));
	}

	ui->Setup->appendPlainText(Line);
}

void FilterDialog::accept(void)
{
	emit onFiltersUpdate(getFilterRules()); QDialog::accept();
}

void FilterDialog::setAvailableFields(const QList<QPair<QString, QString>>& Fields)
{
	if (ui->Field->count()) ui->Field->clear();

	while (auto I = ui->simpleLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	int i = 0; for (const auto& Field : Fields)
	{
		ui->simpleLayout->addWidget(new FilterWidget(Field.second, Field.first, this));
		ui->Field->addItem(Field.second);
		ui->Field->setItemData(i, Field.first, Qt::UserRole);
	}
}
