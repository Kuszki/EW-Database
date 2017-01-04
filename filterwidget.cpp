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

#include "filterwidget.hpp"
#include "ui_filterwidget.h"

FilterWidget::FilterWidget(const QString& Name, const QString& Key, QWidget* Parent, const QHash<int, QString>& Dictionary)
: QWidget(Parent), ui(new Ui::FilterWidget)
{
	ui->setupUi(this); setObjectName(Key);

	if (Dictionary.isEmpty())
	{
		auto Edit = new QLineEdit(this); Widget = Edit;

		connect(Edit, &QLineEdit::editingFinished, this, &FilterWidget::editFinished);
	}
	else
	{
		auto Combo = new QComboBox(this); Widget = Combo; Combo->setEditable(true);

		for (auto i = Dictionary.constBegin(); i != Dictionary.constEnd(); ++i)
		{
			Combo->addItem(i.value(), i.key());
		}

		connect(Combo, &QComboBox::currentTextChanged, this, &FilterWidget::editFinished);
	}

	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	Widget->setEnabled(ui->Field->isChecked());

	ui->Field->setText(Name);
	ui->Operator->addItems(DatabaseDriver::fieldOperators);
	ui->horizontalLayout->addWidget(Widget);

	connect(ui->Field, &QCheckBox::toggled, Widget, &QWidget::setEnabled);
	connect(ui->Field, &QCheckBox::toggled, this, &FilterWidget::onStatusChanged);
}

FilterWidget::~FilterWidget(void)
{
	delete ui;
}

QString FilterWidget::getCondition(void) const
{
	if (ui->Operator->currentText() == "IN" || ui->Operator->currentText() == "NOT IN")
	{
		return QString("%1 %2 ('%3')")
				.arg(objectName())
				.arg(ui->Operator->currentText())
				.arg(getValue()
					.split(QRegExp("\\s*,\\s*"),
						  QString::SkipEmptyParts)
					.join("', '"));
	}
	else
	{
		return QString("%1 %2 '%3'")
				.arg(objectName())
				.arg(ui->Operator->currentText())
				.arg(getValue());
	}
}

QString FilterWidget::getValue(void) const
{
	QString Text;

	if (auto W = dynamic_cast<QComboBox*>(Widget)) Text = W->currentData(Qt::UserRole).toString();
	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) Text = W->text();

	return Text;
}

QString FilterWidget::getLabel(void) const
{
	return ui->Field->text();
}

void FilterWidget::editFinished(void)
{
	emit onValueUpdate(objectName(), getValue());
}

void FilterWidget::setParameters(const QString& Name, const QString& Key, const QString& Value)
{
	ui->Field->setText(Name); setValue(Value); setObjectName(Key);
}

void FilterWidget::setName(const QString& Name)
{
	ui->Field->setText(Name);
}

void FilterWidget::setKey(const QString& Key)
{
	setObjectName(Key);
}

void FilterWidget::setValue(const QString& Value)
{
	if (auto W = dynamic_cast<QComboBox*>(Widget)) W->setCurrentText(Value);
	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) W->setText(Value);
}

bool FilterWidget::isChecked(void) const
{
	return ui->Field->isChecked();
}

void FilterWidget::reset(void)
{
	if (auto W = dynamic_cast<QComboBox*>(Widget)) W->clear();
	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) W->clear();

	ui->Field->setChecked(false);
}
