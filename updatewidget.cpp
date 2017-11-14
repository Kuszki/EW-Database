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

UpdateWidget::UpdateWidget(int ID, const DatabaseDriver::FIELD& Field, QWidget* Parent)
: QWidget(Parent), ui(new Ui::UpdateWidget)
{
	Group = new QActionGroup(this); Menu = new QMenu(this);

	int i(0); for (const auto& Title : DatabaseDriver::nullReasons())
	{
		QAction* A = new QAction(Title, this);

		A->setCheckable(true);
		A->setData(i++);

		Group->addAction(A);
		Menu->addAction(A);

		if (i == 1) Menu->addSeparator();
	}

	Group->actions().first()->setChecked(true); Group->setExclusive(true);

	ui->setupUi(this); setParameters(ID, Field); toggleWidget();

	connect(ui->Field, &QCheckBox::toggled, this, &UpdateWidget::onStatusChanged);
	connect(ui->nullButton, &QToolButton::customContextMenuRequested, this, &UpdateWidget::menuRequested);
}

UpdateWidget::~UpdateWidget(void)
{
	delete ui;
}

QString UpdateWidget::getAssigment(void) const
{
	const QVariant Value = getValue();

	if (!Value.isNull()) return QString("%1 = '%2'").arg(objectName(), Value.toString());
	else
	{
		if (ui->nullButton->contextMenuPolicy() == Qt::NoContextMenu)
		{
			return QString("%1 = NULL").arg(objectName());
		}
		else
		{
			const int Reason = Group->checkedAction()->data().toInt();

			return QString("%1 = NULL, %1_V = '%2'").arg(objectName()).arg(Reason);
		}
	}
}

QVariant UpdateWidget::getValue(void) const
{
	if (ui->nullButton->isChecked()) return QVariant();

	if (auto W = dynamic_cast<QComboBox*>(Widget))
	{
		if (W->property("MASK").toBool())
		{
			auto M = dynamic_cast<QStandardItemModel*>(W->model());

			int Mask = 0;

			for (int i = 1; i < M->rowCount(); ++i)
				if (M->item(i)->checkState() == Qt::Checked)
				{
					Mask |= 1 << M->item(i)->data().toInt();
				}

			return Mask;
		}
		else
		{
			const auto Text = W->currentText();
			const int Index = W->findText(Text);

			if (Index == -1) return Text;
			else return W->itemData(Index);
		}
	}
	else if (auto W = dynamic_cast<QLineEdit*>(Widget))
	{
		return W->text().trimmed().replace("'", "''");
	}
	else if (auto W = dynamic_cast<QSpinBox*>(Widget))
	{
		return W->value();
	}
	else if (auto W = dynamic_cast<QDoubleSpinBox*>(Widget))
	{
		return W->value();
	}
	else if (auto W = dynamic_cast<QDateEdit*>(Widget))
	{
		return W->date().toString("dd.MM.yyyy");
	}
	else if (auto W = dynamic_cast<QDateTimeEdit*>(Widget))
	{
		return W->dateTime().toString("dd.MM.yyyy hh:mm:ss");
	}
	else return QVariant();
}

QString UpdateWidget::getLabel(void) const
{
	return ui->Field->text();
}

int UpdateWidget::getNullreason(void) const
{
	return Group->checkedAction()->data().toInt();
}

int UpdateWidget::getIndex(void) const
{
	return Index;
}

void UpdateWidget::menuRequested(const QPoint& Pos)
{
	Menu->popup(ui->nullButton->mapToGlobal(Pos));
}

void UpdateWidget::textChanged(const QString& Text)
{
	if (auto W = dynamic_cast<QComboBox*>(Widget))
	{
		emit onDataChecked(ui->nullButton->isChecked() || W->findText(Text) != -1);
	}
}

void UpdateWidget::toggleWidget(void)
{
	if (Widget) Widget->setEnabled(!ui->nullButton->isChecked() && ui->Field->isChecked());
}

void UpdateWidget::undoClicked(void)
{
	setValue(Default);
}

void UpdateWidget::editFinished(void)
{
	emit onValueUpdate(objectName(), getValue());
}

void UpdateWidget::resetIndex(void)
{
	if (auto C = qobject_cast<QComboBox*>(sender())) C->setCurrentIndex(0);
}

void UpdateWidget::setParameters(int ID, const DatabaseDriver::FIELD& Field)
{
	ui->Field->setText(Field.Label); ui->Field->setToolTip(Field.Name); Index = ID;

	ui->nullButton->setContextMenuPolicy(Field.Missing ? Qt::CustomContextMenu : Qt::NoContextMenu);

	if (Widget) Widget->deleteLater(); Widget = nullptr;

	if (!Field.Dict.isEmpty()) switch (Field.Type)
	{
		case DatabaseDriver::MASK:
		{
			auto Combo = new QComboBox(this); Widget = Combo; int j = 1;
			auto Model = new QStandardItemModel(Field.Dict.size() + 1, 1, Widget);
			auto Item = new QStandardItem(tr("Select values"));

			for (auto i = Field.Dict.constBegin(); i != Field.Dict.constEnd(); ++i)
			{
				auto Item = new QStandardItem(QString(i.value()).replace('|', '\n'));

				Item->setData(i.key());
				Item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
				Item->setCheckState(Qt::Unchecked);

				Model->setItem(j++, Item);
			}

			Combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

			Item->setFlags(Qt::ItemIsEnabled);
			Model->setItem(0, Item);
			Combo->setModel(Model);
			Combo->setProperty("MASK", true);

			connect(Combo, &QComboBox::currentTextChanged, this, &UpdateWidget::resetIndex);
		}
		break;
		default:
		{
			auto Combo = new QComboBox(this); Widget = Combo;

			Combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

			for (auto i = Field.Dict.constBegin(); i != Field.Dict.constEnd(); ++i)
			{
				Combo->addItem(QString(i.value()).replace('|', '\n'), i.key());
			}

			Combo->model()->sort(0);
			Combo->setEditable(true);
			Combo->setProperty("MASK", false);

			connect(Combo, &QComboBox::editTextChanged, this, &UpdateWidget::textChanged);
		}
	}
	else switch (Field.Type)
	{
		case DatabaseDriver::INTEGER:
		case DatabaseDriver::SMALLINT:
		{
			auto Spin = new QSpinBox(this); Widget = Spin;

			Spin->setSingleStep(1);
			Spin->setRange(0, 10000);
		}
		break;
		case DatabaseDriver::BOOL:
		{
			auto Combo = new QComboBox(this); Widget = Combo;

			Combo->addItem(tr("Yes"), 1);
			Combo->addItem(tr("No"), 0);
			Combo->setProperty("MASK", false);
		}
		break;
		case DatabaseDriver::DOUBLE:
		{
			auto Spin = new QDoubleSpinBox(this); Widget = Spin;

			Spin->setSingleStep(1.0);
			Spin->setRange(0.0, 10000.0);
		}
		break;
		case DatabaseDriver::DATE:
		{
			auto Date = new QDateEdit(this); Widget = Date;

			Date->setDisplayFormat("dd.MM.yyyy");
			Date->setCalendarPopup(true);
		}
		break;
		case DatabaseDriver::DATETIME:
		{
			auto Date = new QDateTimeEdit(this); Widget = Date;

			Date->setDisplayFormat("dd.MM.yyyy hh:mm:ss");
			Date->setCalendarPopup(true);
		}
		break;
		default:
		{
			auto Edit = new QLineEdit(this); Widget = Edit;

			Edit->setClearButtonEnabled(true);
		}
		break;
	}

	if (Widget)
	{
		Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		Widget->setEnabled(!ui->nullButton->isChecked() && ui->Field->isChecked());

		ui->horizontalLayout->insertWidget(1, Widget);
	}

	setObjectName(Field.Name);
}

void UpdateWidget::setChecked(bool Checked)
{
	ui->Field->setChecked(Checked);
}

void UpdateWidget::setValue(const QVariant& Value)
{
	ui->nullButton->setChecked(Value.isNull());

	if (auto W = dynamic_cast<QComboBox*>(Widget))
	{
		if (W->property("MASK").toBool())
		{
			auto M = dynamic_cast<QStandardItemModel*>(W->model());

			for (int i = 1; i < M->rowCount(); ++i)
			{
				const bool Checked = Value.toInt() & (1 << M->item(i)->data().toInt());

				M->item(i)->setCheckState(Checked ? Qt::Checked : Qt::Unchecked);
			}

		}
		else
		{
			W->setCurrentIndex(W->findData(Value));
		}

	}
	else if (auto W = dynamic_cast<QLineEdit*>(Widget))
	{
		W->setText(Value.toString());
	}
	else if (auto W = dynamic_cast<QSpinBox*>(Widget))
	{
		W->setValue(Value.toInt());
	}
	else if (auto W = dynamic_cast<QDoubleSpinBox*>(Widget))
	{
		W->setValue(Value.toDouble());
	}
	else if (auto W = dynamic_cast<QDateEdit*>(Widget))
	{
		W->setDate(Value.toDate());
	}
	else if (auto W = dynamic_cast<QDateTimeEdit*>(Widget))
	{
		W->setDateTime(Value.toDateTime());
	}

	if (Value.isValid()) Default = Value;
}

bool UpdateWidget::isChecked(void) const
{
	return isEnabled() && ui->Field->isChecked();
}

void UpdateWidget::reset(void)
{
	setChecked(false); setValue(QVariant());
}
