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

FilterDialog::FilterDialog(QWidget* Parent, const QVector<DatabaseDriver_v2::FIELD>& Fields, const QVector<DatabaseDriver_v2::TABLE>& Tables)
: QDialog(Parent), ui(new Ui::FilterDialog)
{
	ui->setupUi(this); setFields(Fields, Tables);

	ui->Operator->addItems(DatabaseDriver_v2::Operators);
	ui->classLayout->setAlignment(Qt::AlignTop);
	ui->simpleLayout->setAlignment(Qt::AlignTop);

	tabIndexChanged(ui->tabWidget->currentIndex());
}

FilterDialog::~FilterDialog(void)
{
	delete ui;
}

QString FilterDialog::getFilterRules(void)
{
//	if (ui->tabWidget->currentIndex() == 1)
//	{
//		if (ui->Setup->document()->toPlainText().isEmpty()) return QString();
//		return ui->Setup->document()->toPlainText();
//	}
//	else
//	{
//		QStringList Rules; for (int i = 0; i < ui->simpleLayout->count(); ++i)
//		{
//			if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
//			{
//				if (W->isChecked()) Rules.append(W->getCondition());
//			}
//		}

//		if (Rules.isEmpty()) return QString();
//		else return Rules.join(" AND ");
//	}
}

void FilterDialog::operatorTextChanged(const QString& Operator)
{
	ui->Value->setVisible(Operator != "IS NULL" && Operator != "IS NOT NULL");
}

void FilterDialog::classSearchEdited(const QString& Search)
{
	for (int i = 0; i < ui->classLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->classLayout->itemAt(i)->widget()))
		{
			W->setVisible(W->isEnabled() && W->text().contains(Search, Qt::CaseInsensitive));
		}
}

void FilterDialog::simpleSearchEdited(const QString& Search)
{
	for (int i = 0; i < ui->simpleLayout->count(); ++i)
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			W->setVisible(W->isEnabled() && W->getLabel().contains(Search, Qt::CaseInsensitive));
		}
}

void FilterDialog::buttonBoxClicked(QAbstractButton* Button)
{
	if (Button != ui->buttonBox->button(QDialogButtonBox::Reset)) return;

	for (int i = 0; i < ui->simpleLayout->count(); ++i)
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			W->reset();
		}

	ui->Setup->document()->clear();
}

void FilterDialog::classBoxChecked(void)
{
	QSet<int> Disabled, All;

	for (int i = 0; i < ui->simpleLayout->count(); ++i) All.insert(i);

	for (int i = 0; i < ui->classLayout->count(); ++i) if (auto W = ui->classLayout->itemAt(i)->widget())
	{
		if (auto C = qobject_cast<QCheckBox*>(W)) if (C->isChecked())
		{
			const int ID = W->property("KEY").toInt();

			Disabled.unite(QSet<int>(All).subtract(Attributes[ID].toSet()));
		}
	}

	for (int i = 0; i < ui->simpleLayout->count(); ++i) if (auto W = ui->simpleLayout->itemAt(i)->widget())
	{
		W->setEnabled(!Disabled.contains(i));
	}

	simpleSearchEdited(ui->simpleSearch->text());
}

void FilterDialog::tabIndexChanged(int Index)
{
	ui->classSearch->setVisible(Index == 0);
	ui->simpleSearch->setVisible(Index == 1);

	ui->selectButton->setVisible(Index == 0);
	ui->unselectButton->setVisible(Index == 0);
}

void FilterDialog::addButtonClicked(void)
{
//	QString Line;

//	if (!ui->Setup->document()->toPlainText().trimmed().isEmpty()) Line.append(ui->Action->currentText()).append(' ');

//	if (ui->Operator->currentText() == "IS NULL" || ui->Operator->currentText() == "IS NOT NULL")
//	{
//		Line.append(QString("%1 %2")
//				.arg(ui->Field->currentData(Qt::UserRole).toString())
//				.arg(ui->Operator->currentText()));
//	}
//	else if (ui->Operator->currentText() == "IN" || ui->Operator->currentText() == "NOT IN")
//	{
//		Line.append(QString("%1 %2 ('%3')")
//				.arg(ui->Field->currentData(Qt::UserRole).toString())
//				.arg(ui->Operator->currentText())
//				.arg(ui->Value->text().
//					split(QRegExp("\\s*,\\s*"),
//						 QString::SkipEmptyParts)
//					.join("', '")));
//	}
//	else
//	{
//		Line.append(QString("%1 %2 '%3'")
//				.arg(ui->Field->currentData(Qt::UserRole).toString())
//				.arg(ui->Operator->currentText())
//				.arg(ui->Value->text()));
//	}

	//	ui->Setup->appendPlainText(Line);
}

void FilterDialog::selectButtonClicked(void)
{
	for (int i = 0; i < ui->classLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->classLayout->itemAt(i)->widget()))
		{
			W->blockSignals(true);
			if (W->isVisible()) W->setChecked(true);
			W->blockSignals(false);
		}

	classBoxChecked();
}

void FilterDialog::unselectButtonClicked(void)
{
	for (int i = 0; i < ui->classLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->classLayout->itemAt(i)->widget()))
		{
			W->blockSignals(true);
			if (W->isVisible()) W->setChecked(false);
			W->blockSignals(false);
		}

	classBoxChecked();
}

void FilterDialog::accept(void)
{
	emit onFiltersUpdate(getFilterRules()); QDialog::accept();
}

void FilterDialog::setFields(const QVector<DatabaseDriver_v2::FIELD>& Fields, const QVector<DatabaseDriver_v2::TABLE>& Tables)
{
	Attributes.clear(); ui->Field->clear();

	while (auto I = ui->classLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();
	while (auto I = ui->simpleLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Tables.size(); ++i)
	{
		auto Check = new QCheckBox(Tables[i].Label, this);

		Check->setProperty("KEY", i);
		Attributes.append(Tables[i].Indexes);
		ui->classLayout->addWidget(Check);

		connect(Check, &QCheckBox::toggled, this, &FilterDialog::classBoxChecked);
	}

	for (int i = 0; i < Fields.size(); ++i)
	{
		const int Count = ui->Field->count();

		ui->simpleLayout->addWidget(new FilterWidget(i, Fields[i], this));

		ui->Field->addItem(Fields[i].Label);
		ui->Field->setItemData(Count, Fields[i].Name);
	}
}
