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

#include "updatewidget.hpp"
#include "ui_updatewidget.h"

UpdateWidget::UpdateWidget(const QString& Name, const QString& Key, QWidget* Parent, const QHash<int, QString>& Dictionary)
: QWidget(Parent), ui(new Ui::UpdateWidget)
{
	ui->setupUi(this); setObjectName(Key);

	if (Dictionary.isEmpty())
	{
		auto Edit = new QLineEdit(this); Widget = Edit;

		connect(Edit, &QLineEdit::editingFinished, this, &UpdateWidget::editFinished);
	}
	else
	{
		auto Combo = new QComboBox(this); Widget = Combo;

		for (auto i = Dictionary.constBegin(); i != Dictionary.constEnd(); ++i)
		{
			Combo->addItem(i.value(), i.key());
		}

		connect(Combo, &QComboBox::currentTextChanged, this, &UpdateWidget::editFinished);
	}

	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	Widget->setEnabled(ui->Field->isChecked());

	ui->Field->setText(Name);
	ui->horizontalLayout->addWidget(Widget);

	connect(ui->Field, &QCheckBox::toggled, Widget, &QWidget::setEnabled);
	connect(ui->Field, &QCheckBox::toggled, this, &UpdateWidget::onStatusChanged);
}

UpdateWidget::~UpdateWidget(void)
{
	delete ui;
}

QString UpdateWidget::getAssigment(void) const
{
	return QString("%1 = '%2'").arg(objectName(), getValue());
}

QString UpdateWidget::getValue(void) const
{
	QString Text;

	if (auto W = dynamic_cast<QComboBox*>(Widget)) Text = W->currentData(Qt::UserRole).toString();
	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) Text = W->text();

	return Text;
}

QString UpdateWidget::getLabel(void) const
{
	return ui->Field->text();
}

void UpdateWidget::editFinished(void)
{
	emit onValueUpdate(objectName(), getValue());
}

void UpdateWidget::setParameters(const QString& Name, const QString& Key, const QString& Value)
{
	ui->Field->setText(Name); setValue(Value); setObjectName(Key);
}

void UpdateWidget::setName(const QString& Name)
{
	ui->Field->setText(Name);
}

void UpdateWidget::setKey(const QString& Key)
{
	setObjectName(Key);
}

void UpdateWidget::setValue(const QString& Value)
{
	if (auto W = dynamic_cast<QComboBox*>(Widget)) W->setCurrentText(Value);
	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) W->setText(Value);
}

bool UpdateWidget::isChecked(void) const
{
	return ui->Field->isChecked();
}

void UpdateWidget::reset(void)
{
	if (auto W = dynamic_cast<QComboBox*>(Widget)) W->clear();
	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) W->clear();

	ui->Field->setChecked(false);
}
