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

FilterWidget::FilterWidget(int ID, const DatabaseDriver_v2::FIELD& Field, QWidget* Parent)
: QWidget(Parent), ui(new Ui::FilterWidget)
{
	ui->setupUi(this); setParameters(ID, Field);

	ui->Operator->addItems(DatabaseDriver_v2::Operators);

	connect(ui->Field, &QCheckBox::toggled, this, &FilterWidget::onStatusChanged);
}

FilterWidget::~FilterWidget(void)
{
	delete ui;
}

QString FilterWidget::getCondition(void) const
{
//	if (ui->Operator->currentText() == "IS NULL" || ui->Operator->currentText() == "IS NOT NULL")
//	{
//		return QString("%1 %2")
//				.arg(objectName())
//				.arg(ui->Operator->currentText());
//	}
//	else if (ui->Operator->currentText() == "IN" || ui->Operator->currentText() == "NOT IN")
//	{
//		return QString("%1 %2 ('%3')")
//				.arg(objectName())
//				.arg(ui->Operator->currentText())
//				.arg(getValue()
//					.split(QRegExp("\\s*,\\s*"),
//						  QString::SkipEmptyParts)
//					.join("', '"));
//	}
//	else
//	{
//		return QString("%1 %2 '%3'")
//				.arg(objectName())
//				.arg(ui->Operator->currentText())
//				.arg(getValue());
//	}
}

QString FilterWidget::getValue(void) const
{
//	if (auto W = dynamic_cast<QComboBox*>(Widget))
//	{
//		const auto Text = W->currentText();;

//		if (W->findText(Text) == -1) return Text;
//		else return W->currentData().toString();

//	}
//	else if (auto W = dynamic_cast<QLineEdit*>(Widget))
//	{
//		return W->text();
//	}
//	else return QString();
}

QString FilterWidget::getLabel(void) const
{
	return ui->Field->text();
}

int FilterWidget::getIndex(void) const
{
	return Index;
}

void FilterWidget::operatorChanged(const QString& Name)
{
	Widget->setVisible(Name != "IS NULL" && Name != "IS NOT NULL");
}

void FilterWidget::editFinished(void)
{
	emit onValueUpdate(objectName(), getValue());
}

void FilterWidget::setParameters(int ID, const DatabaseDriver_v2::FIELD& Field)
{
	setObjectName(Field.Name); Index = ID; if (Widget) Widget->deleteLater();

	if (!Field.Dict.isEmpty()) switch (Field.Type)
	{
		case DatabaseDriver_v2::MASK:
		{
			auto Combo = new QComboBox(this); Widget = Combo; int j = 1;
			auto Model = new QStandardItemModel(Field.Dict.size() + 1, 1, Widget);
			auto Item = new QStandardItem(tr("Select values"));

			for (auto i = Field.Dict.constBegin(); i != Field.Dict.constEnd(); ++i)
			{
				auto Item = new QStandardItem(i.value());

				Item->setData(i.key());
				Item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
				Item->setCheckState(Qt::Unchecked);

				Model->setItem(j++, Item);
			}

			Item->setFlags(Qt::ItemIsEnabled);
			Model->setItem(0, Item);
			Combo->setModel(Model);

			connect(Combo, &QComboBox::currentTextChanged, boost::bind(&QComboBox::setCurrentIndex, Combo, 0));
		}
		break;
		default:
		{
			auto Combo = new QComboBox(this); Widget = Combo;

			for (auto i = Field.Dict.constBegin(); i != Field.Dict.constEnd(); ++i)
			{
				Combo->addItem(i.value(), i.key());
			}

			Combo->setEditable(true);
		}
	}
	else switch (Field.Type)
	{
		case DatabaseDriver_v2::INTEGER:
		case DatabaseDriver_v2::SMALLINT:
		{
			auto Spin = new QSpinBox(this); Widget = Spin;

			Spin->setSingleStep(1);
			Spin->setRange(0, 100);
		}
		break;
		case DatabaseDriver_v2::BOOL:
		{
			auto Combo = new QComboBox(this); Widget = Combo;

			Combo->addItem(tr("Yes"), true);
			Combo->addItem(tr("No"), false);
		}
		break;
		case DatabaseDriver_v2::DOUBLE:
		{
			auto Spin = new QDoubleSpinBox(this); Widget = Spin;

			Spin->setSingleStep(1.0);
			Spin->setRange(0.0, 10000.0);
		}
		break;
		case DatabaseDriver_v2::DATE:
		{
			auto Date = new QDateTimeEdit(this); Widget = Date;
		}
		break;
		default:
		{
			auto Edit = new QLineEdit(this); Widget = Edit;

			Edit->setClearButtonEnabled(true);
		}
		break;
	}

	Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	Widget->setEnabled(ui->Field->isChecked());

	ui->Field->setText(Field.Label);
	ui->horizontalLayout->addWidget(Widget);

	connect(ui->Field, &QCheckBox::toggled, Widget, &QWidget::setEnabled);
}

void FilterWidget::setValue(const QVariant& Value)
{
//	if (auto W = dynamic_cast<QComboBox*>(Widget)) W->setCurrentText(Value);
//	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) W->setText(Value);
}


void FilterWidget::setChecked(bool Checked)
{
	ui->Field->setChecked(Checked);
}

bool FilterWidget::isChecked(void) const
{
	return ui->Field->isChecked();
}

void FilterWidget::reset(void)
{
//	if (auto W = dynamic_cast<QComboBox*>(Widget)) W->clearEditText();
//	else if (auto W = dynamic_cast<QLineEdit*>(Widget)) W->clear();

	ui->Field->setChecked(false);
}
