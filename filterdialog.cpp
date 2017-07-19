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
	QStringList Rules, Classes; QString Values, Class;

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

	Values = Rules.join(QString(" %1 ").arg(ui->operatorBox->currentText()));
	Class = Classes.join("', '");

	if (!Classes.isEmpty())
	{
		if (Rules.isEmpty()) return QString("EW_OBIEKTY.KOD IN ('%1')").arg(Class);
		else return QString("EW_OBIEKTY.KOD IN ('%1') AND (%2)").arg(Class, Values);
	}
	else if (!Rules.isEmpty()) return Values;
	else return QString();
}

QList<int> FilterDialog::getUsedFields(void) const
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

QHash<QString, QVariant> FilterDialog::getFieldsRules(void) const
{
	QHash<QString, QVariant> Rules;

	for (int i = 0; i < ui->simpleLayout->count(); ++i)
	{
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			if (W->isChecked())
			{
				const auto Rule = W->getBinding();

				Rules.insert(Rule.first, Rule.second);
			}
		}
	}

	return Rules;
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
}

void FilterDialog::linitBoxChecked(bool Checked)
{
	if (!Checked) Limiter.clear();
	else
	{
		Limiter = QFileDialog::getOpenFileName(this, tr("Select objects list"), QString(),
									    tr("Text files (*.txt);;All files (*.*)"));

		if (Limiter.isEmpty()) ui->limitCheck->setChecked(false);
	}

	ui->limitCheck->setToolTip(Limiter);
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

	ui->classSearch->setVisible(Index == 0);
	ui->selectButton->setVisible(Index == 0);
	ui->unselectButton->setVisible(Index == 0);

	ui->copyButton->setVisible(Index == 1);
	ui->simpleSearch->setVisible(Index == 1);
	ui->operatorBox->setVisible(Index == 1);

	ui->newButton->setVisible(Index == 2);
	ui->limitCheck->setVisible(Index == 2);
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
	Classes.clear(); Points.clear(); Attributes.clear(); Above = Common;

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
	}
}
