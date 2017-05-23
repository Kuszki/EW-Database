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

FilterDialog::FilterDialog(QWidget* Parent, const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables, unsigned Common)
: QDialog(Parent), ui(new Ui::FilterDialog)
{
	ui->setupUi(this); setFields(Fields, Tables, Common); filterRulesChanged();

	ui->Operator->addItems(DatabaseDriver::Operators);

	ui->classLayout->setAlignment(Qt::AlignTop);
	ui->simpleLayout->setAlignment(Qt::AlignTop);
	ui->geometryLayout->setAlignment(Qt::AlignTop);

	ui->rightSpacer->changeSize(ui->copyButton->sizeHint().width(), 0);
}

FilterDialog::~FilterDialog(void)
{
	delete ui;
}

QString FilterDialog::getLimiterFile(void) const
{
	return Limiter;
}

QString FilterDialog::getFilterRules(void) const
{
	if (ui->specialCheck->isChecked())
	{
		if (ui->Setup->document()->toPlainText().isEmpty()) return QString();
		return ui->Setup->document()->toPlainText();
	}
	else
	{
		QStringList Rules, Classes;

		for (int i = 0; i < ui->classLayout->count(); ++i)
		{
			if (auto W = qobject_cast<QCheckBox*>(ui->classLayout->itemAt(i)->widget()))
			{
				if (W->isChecked()) Classes.append(W->toolTip());
			}
		}

		for (int i = 0; i < ui->simpleLayout->count(); ++i)
		{
			if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
			{
				if (W->isChecked()) Rules.append(W->getCondition());
			}
		}

		if (!Classes.isEmpty()) Rules.insert(0, QString("EW_OBIEKTY.KOD IN ('%1')").arg(Classes.join("', '")));

		if (Rules.isEmpty()) return QString();
		else return Rules.join(" AND ");
	}
}

QList<int> FilterDialog::getUsedFields(void) const
{
	if (ui->specialCheck->isChecked()) return QList<int>();
	else
	{
		QSet<int> Used;

		for (int i = 0; i < ui->simpleLayout->count(); ++i)
		{
			if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
			{
				if (W->isChecked()) Used.insert(W->getIndex());
			}
		}

		return Used.toList();
	}
}

QHash<int, QVariant> FilterDialog::getGeometryRules(void) const
{
	QHash<int, QVariant> Rules;

	for (int i = 0; i < ui->geometryLayout->count(); ++i)
		if (auto W = dynamic_cast<GeometryWidget*>(ui->geometryLayout->itemAt(i)->widget()))
		{
			const QPair<int, QVariant> Rule = W->getCondition();

			if (!Rules.contains(Rule.first))
			{
				Rules.insert(Rule.first, QVariant(Rule.first > 1 ? QVariant::StringList : QVariant::Double));

				if (Rule.first < 2) Rules[Rule.first] = Rule.first ? DBL_MAX : DBL_MIN;
			}

			if (Rule.first == 0)
			{
				Rules[Rule.first] = qMax(Rules[Rule.first].toDouble(),
									Rule.second.toDouble());
			}
			else if (Rule.first == 1)
			{
				Rules[Rule.first] = qMin(Rules[Rule.first].toDouble(),
									Rule.second.toDouble());
			}
			else if (!Rules[Rule.first].toStringList().contains(Rule.second.toString()))
			{
				Rules[Rule.first] = Rules[Rule.first].toStringList() << Rule.second.toString();
			}
		}

	return Rules;
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

	for (int i = 0; i < ui->geometryLayout->count(); ++i)
		if (auto W = qobject_cast<GeometryWidget*>(ui->geometryLayout->itemAt(i)->widget()))
		{
			W->deleteLater();
		}

	ui->Setup->document()->clear();
}

void FilterDialog::linitBoxChecked(bool Checked)
{
	if (Checked)
	{
		Limiter = QFileDialog::getOpenFileName(this, tr("Select objects list"), QString(),
									    tr("Text files (*.txt);;All files (*.*)"));

		if (Limiter.isEmpty()) ui->limitCheck->setChecked(false);
	}
	else Limiter.clear();
}

void FilterDialog::classBoxChecked(void)
{
	QSet<int> Disabled, All;

	for (int i = Above; i < ui->simpleLayout->count(); ++i)
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			All.insert(W->getIndex());
		}

	for (int i = 0; i < ui->classLayout->count(); ++i)
		if (auto C = qobject_cast<QCheckBox*>(ui->classLayout->itemAt(i)->widget()))
			if (C->isChecked())
			{
				Disabled.unite(QSet<int>(All).subtract(Attributes[i]));
			}

	for (int i = Above; i < ui->simpleLayout->count(); ++i)
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			W->setEnabled(!Disabled.contains(W->getIndex()));
		}

	simpleSearchEdited(ui->simpleSearch->text());
}

void FilterDialog::filterRulesChanged(void)
{
	const int Index = ui->tabWidget->currentIndex();
	const bool Special = ui->specialCheck->isChecked();

	ui->copyButton->setVisible(Index == 0);
	ui->simpleSearch->setVisible(Index == 0 && !Special);

	ui->classSearch->setVisible(Index == 1);
	ui->selectButton->setVisible(Index == 1);
	ui->unselectButton->setVisible(Index == 1);

	ui->newButton->setVisible(Index == 2);
	ui->limitCheck->setVisible(Index == 2);

	ui->simpleScrool->setVisible(!Special);
	ui->specialWidget->setVisible(Special);
}

void FilterDialog::addButtonClicked(void)
{
	QString Line;

	if (!ui->Setup->document()->toPlainText().trimmed().isEmpty()) Line.append(ui->Action->currentText()).append(' ');

	if (ui->Operator->currentText() == "IS NULL" || ui->Operator->currentText() == "IS NOT NULL")
	{
		Line.append(QString("%1 %2")
				.arg(ui->Field->currentData(Qt::UserRole).toString())
				.arg(ui->Operator->currentText()));
	}
	else if (ui->Operator->currentText() == "IN" || ui->Operator->currentText() == "NOT IN")
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

void FilterDialog::newButtonClicked(void)
{
	ui->geometryLayout->addWidget(new GeometryWidget(Classes, Points, this));
}

void FilterDialog::copyButtonClicked(void)
{
	QApplication::clipboard()->setText(getFilterRules());
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
	emit onFiltersUpdate(getFilterRules(), getUsedFields(), getGeometryRules(), Limiter); QDialog::accept();
}

void FilterDialog::setFields(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables, unsigned Common)
{
	Classes.clear(); Points.clear(); Attributes.clear();
	ui->Field->clear(); Above = Common; QStringList Used;

	while (auto I = ui->classLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();
	while (auto I = ui->simpleLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Tables.size(); ++i)
	{
		auto Check = new QCheckBox(Tables[i].Label, this);

		if (Tables[i].Point) Points.insert(Tables[i].Name, Tables[i].Label);
		else Classes.insert(Tables[i].Name, Tables[i].Label);

		Check->setToolTip(Tables[i].Name);
		ui->classLayout->addWidget(Check);

		Attributes.append(Tables[i].Indexes.toSet());

		connect(Check, &QCheckBox::toggled, this, &FilterDialog::classBoxChecked);
	}

	for (int i = 0; i < Fields.size(); ++i)
	{
		const bool Singleton = (Fields[i].Dict.size() == 2 && Fields[i].Dict.contains(0));

		if (!Singleton) ui->simpleLayout->addWidget(new FilterWidget(i, Fields[i], this));

		if (!Used.contains(Fields[i].Name))
		{
			const int Count = ui->Field->count();

			ui->Field->addItem(Fields[i].Label);
			ui->Field->setItemData(Count, Fields[i].Name);

			Used.append(Fields[i].Name);
		}
	}
}
