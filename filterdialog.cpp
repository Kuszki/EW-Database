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

FilterDialog::FilterDialog(QWidget* Parent, const QStringList& Variables, const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables, unsigned Common, bool Singletons)
: QDialog(Parent), ui(new Ui::FilterDialog)
{
	ui->setupUi(this); setFields(Variables, Fields, Tables, Common, Singletons); filterRulesChanged();

	Highlighter = new KLHighlighter(ui->advancedEdit->document());

	QMenu* resetMenu = new QMenu(this);

	resetClass = new QAction(tr("Reset class list"), this);
	resetFields = new QAction(tr("Reset fields list"), this);
	resetGeometry = new QAction(tr("Reset geometry filters"), this);
	resetRedaction = new QAction(tr("Reset redaction filters"), this);
	resetAdvanced = new QAction(tr("Reset advanced filters"), this);

	resetClass->setCheckable(true); resetMenu->addAction(resetClass);
	resetFields->setCheckable(true); resetMenu->addAction(resetFields);
	resetGeometry->setCheckable(true); resetMenu->addAction(resetGeometry);
	resetRedaction->setCheckable(true); resetMenu->addAction(resetRedaction);
	resetAdvanced->setCheckable(true); resetMenu->addAction(resetAdvanced);

	QMenu* saveMenu = new QMenu(this);

	QAction* saveNew = new QAction(tr("Perform new search"), this);
	QAction* saveCur = new QAction(tr("Filter current selection"), this);
	QAction* saveAdd = new QAction(tr("Append to current selection"), this);
	QAction* saveSub = new QAction(tr("Subtract from current selection"), this);

	saveMode = new QActionGroup(this);

	saveNew->setCheckable(true); saveNew->setData(0);
	saveCur->setCheckable(true); saveCur->setData(1);
	saveAdd->setCheckable(true); saveAdd->setData(2);
	saveSub->setCheckable(true); saveSub->setData(3);

	saveMenu->addAction(saveNew); saveMode->addAction(saveNew);
	saveMenu->addAction(saveCur); saveMode->addAction(saveCur);
	saveMenu->addAction(saveAdd); saveMode->addAction(saveAdd);
	saveMenu->addAction(saveSub); saveMode->addAction(saveSub);

	saveMode->setExclusive(true); saveNew->setChecked(true);

	QSettings Settings("EW-Database");

	Settings.beginGroup("Filter");
	resetClass->setChecked(Settings.value("class", true).toBool());
	resetFields->setChecked(Settings.value("fields", true).toBool());
	resetGeometry->setChecked(Settings.value("geometry", true).toBool());
	resetRedaction->setChecked(Settings.value("redaction", true).toBool());
	resetAdvanced->setChecked(Settings.value("advanced", true).toBool());
	Settings.endGroup();

	QToolButton* Button = new QToolButton(this);

	Button->setText(tr("Reset"));
	Button->setPopupMode(QToolButton::MenuButtonPopup);
	Button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	Button->setMenu(resetMenu);

	ui->buttonBox->addButton(Button, QDialogButtonBox::ResetRole);

	QToolButton* Save = new QToolButton(this);

	Save->setText(tr("Apply"));
	Save->setPopupMode(QToolButton::MenuButtonPopup);
	Save->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	Save->setMenu(saveMenu);

	ui->buttonBox->addButton(Save, QDialogButtonBox::YesRole);

	ui->classLayout->setAlignment(Qt::AlignTop);
	ui->simpleLayout->setAlignment(Qt::AlignTop);
	ui->geometryLayout->setAlignment(Qt::AlignTop);
	ui->redactionLayout->setAlignment(Qt::AlignTop);

	ui->newButton->setFixedSize(ui->buttonBox->sizeHint().height(),
						   ui->buttonBox->sizeHint().height());
	ui->selectButton->setFixedSize(ui->buttonBox->sizeHint().height(),
							 ui->buttonBox->sizeHint().height());
	ui->unselectButton->setFixedSize(ui->buttonBox->sizeHint().height(),
							   ui->buttonBox->sizeHint().height());
	ui->validateButton->setFixedSize(ui->buttonBox->sizeHint().height(),
							   ui->buttonBox->sizeHint().height());
	ui->limiterCheck->setFixedSize(ui->buttonBox->sizeHint().height(),
							 ui->buttonBox->sizeHint().height());

	ui->rightSpacer->changeSize(ui->validateButton->sizeHint().width(), 0);

	ui->advancedEdit->setFont(QFont("monospace"));

	connect(Button, &QToolButton::clicked, this, &FilterDialog::resetButtonClicked);
}

FilterDialog::~FilterDialog(void)
{
	QSettings Settings("EW-Database");

	Settings.beginGroup("Filter");
	Settings.setValue("class", resetClass->isChecked());
	Settings.setValue("fields", resetFields->isChecked());
	Settings.setValue("geometry", resetGeometry->isChecked());
	Settings.setValue("redaction", resetRedaction->isChecked());
	Settings.setValue("advanced", resetAdvanced->isChecked());
	Settings.endGroup();

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

QString FilterDialog::getAdvancedRules(void) const
{
	return ui->advancedEdit->document()->toPlainText().trimmed();
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
				Rules.insert(Rule.first, QVariant(Rule.first > 3 ? QVariant::StringList : QVariant::Double));

				if (Rule.first < 4) Rules[Rule.first] = Rule.second.toDouble();
			}

			if (Rule.first == 0 || Rule.first == 2)
			{
				Rules[Rule.first] = qMax(Rules[Rule.first].toDouble(),
									Rule.second.toDouble());
			}
			else if (Rule.first == 1 || Rule.first == 3)
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

QHash<int, QVariant> FilterDialog::getRedactionRules(void) const
{
	QHash<int, QVariant> Rules;

	for (int i = 0; i < ui->redactionLayout->count(); ++i)
		if (auto W = dynamic_cast<RedactionWidget*>(ui->redactionLayout->itemAt(i)->widget()))
		{
			const QPair<int, QVariant> Rule = W->getCondition();

			if (!Rules.contains(Rule.first)) switch (Rule.second.type())
			{
				case QVariant::Double:
				case QVariant::Int:
					Rules.insert(Rule.first, Rule.second);
				break;
				case QVariant::String:
					Rules[Rule.first] = QStringList() << Rule.second.toString();
				break;
			}

			if (Rule.first == 0 || Rule.first == 2)
			{
				Rules[Rule.first] = qMax(Rules[Rule.first].toDouble(),
									Rule.second.toDouble());
			}
			else if (Rule.first == 1 || Rule.first == 3)
			{
				Rules[Rule.first] = qMin(Rules[Rule.first].toDouble(),
									Rule.second.toDouble());
			}
			else if (Rule.second.type() == QVariant::Int)
			{
				Rules[Rule.first] = Rules[Rule.first].toInt() | Rule.second.toInt();
			}
			else if (!Rules[Rule.first].toStringList().contains(Rule.second.toString()))
			{
				Rules[Rule.first] = Rules[Rule.first].toStringList() << Rule.second.toString();
			}
		}

	return Rules;
}

QHash<int, QVariant> FilterDialog::getFieldsRules(void) const
{
	QHash<int, QVariant> Rules;

	for (int i = 0; i < ui->simpleLayout->count(); ++i)
	{
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			if (W->isChecked()) Rules.insert(W->getIndex(), W->getValue());
		}
	}

	return Rules;
}

double FilterDialog::getRadius(void) const
{
	return ui->radiusSpin->value();
}

QPair<QString, int> FilterDialog::validateScript(const QString& Script) const
{
	if (Script.trimmed().isEmpty()) return QPair<QString, int>(); QJSEngine Engine;

	auto Model = ui->variablesList->model();
	auto Root = Model->index(0, 0);

	for (int i = 0; i < Model->rowCount(Root); ++i)
	{
		const auto Index = Model->index(i, 0, Root);
		const auto V = Model->data(Index).toString();

		Engine.globalObject().setProperty(V, QJSValue());
	}

	const auto V = Engine.evaluate(Script);

	if (!V.isError()) return qMakePair(QString(), int(0));
	else return qMakePair(V.toString(), V.property("lineNumber").toInt());
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

void FilterDialog::resetButtonClicked(void)
{
	if (resetClass->isChecked()) for (int i = 0; i < ui->classLayout->count(); ++i)
		if (auto W = qobject_cast<QCheckBox*>(ui->classLayout->itemAt(i)->widget()))
		{
			W->setChecked(false);
		}

	if (resetFields->isChecked()) for (int i = 0; i < ui->simpleLayout->count(); ++i)
		if (auto W = qobject_cast<FilterWidget*>(ui->simpleLayout->itemAt(i)->widget()))
		{
			W->reset();
		}

	if (resetGeometry->isChecked()) for (int i = 0; i < ui->geometryLayout->count(); ++i)
		if (auto W = qobject_cast<GeometryWidget*>(ui->geometryLayout->itemAt(i)->widget()))
		{
			W->deleteLater();
		}

	if (resetRedaction->isChecked()) for (int i = 0; i < ui->redactionLayout->count(); ++i)
		if (auto W = qobject_cast<RedactionWidget*>(ui->redactionLayout->itemAt(i)->widget()))
		{
			W->deleteLater();
		}

	if (resetAdvanced->isChecked()) ui->advancedEdit->clear();
}

void FilterDialog::limiterBoxChecked(bool Checked)
{
	if (!Checked) Limiter.clear();
	else
	{
		Limiter = QFileDialog::getOpenFileName(this, tr("Select objects list"), QString(),
									    tr("Text files (*.txt);;All files (*.*)"));

		if (Limiter.isEmpty()) ui->limiterCheck->setChecked(false);
	}

	ui->limiterCheck->setToolTip(Limiter);
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

	ui->newButton->setVisible(Index == 2 || Index == 3);

	ui->classSearch->setVisible(Index == 0);
	ui->selectButton->setVisible(Index == 0);
	ui->unselectButton->setVisible(Index == 0);

	ui->simpleSearch->setVisible(Index == 1);
	ui->operatorBox->setVisible(Index == 1);

	ui->limiterCheck->setVisible(Index == 2);
	ui->radiusSpin->setVisible(Index == 2);

	ui->validateButton->setVisible(Index == 4);
	ui->helpLabel->setVisible(Index == 4);
}

void FilterDialog::newButtonClicked(void)
{
	const int Index = ui->tabWidget->currentIndex();

	if (Index == 2)
	{
		ui->geometryLayout->addWidget(new GeometryWidget(Classes, Points, this));
	}
	else
	{
		ui->redactionLayout->addWidget(new RedactionWidget(this));
	}
}

void FilterDialog::validateButtonClicked(void)
{
	const auto V = validateScript(ui->advancedEdit->toPlainText());

	if (!V.second) ui->helpLabel->setText(tr("Script is ok"));
	else ui->helpLabel->setText(tr("Syntax error in line %1: %2")
						   .arg(V.second).arg(V.first));
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

void FilterDialog::helperIndexChanged(int Index)
{
	auto Model = ui->helperCombo->model();

	if (Model && Index != -1)
	{
		auto Root = Model->index(Index, 0);

		ui->variablesList->setRootIndex(Root);
	}

	ui->helpLabel->clear();
}

void FilterDialog::tooltipShowRequest(QModelIndex Index)
{
	ui->helpLabel->setText(ui->variablesList->model()->data(Index, Qt::ToolTipRole).toString());
}


void FilterDialog::variablePasteRequest(QModelIndex Index)
{
	const QString Value = ui->variablesList->model()->data(Index).toString();

	if (!Value.isEmpty()) ui->advancedEdit->insertPlainText(Value);
}

void FilterDialog::accept(void)
{
	const auto V = validateScript(ui->advancedEdit->toPlainText());

	if (V.second) QMessageBox::critical(this, tr("Syntax error in line %1").arg(V.second), V.first);
	else
	{
		QDialog::accept();

		emit onFiltersUpdate(getFilterRules(), getAdvancedRules(), getUsedFields(),
						 getGeometryRules(), getRedactionRules(), Limiter,
						 ui->radiusSpin->value(), saveMode->checkedAction()->data().toInt());
	}
}

void FilterDialog::setFields(const QStringList& Variables, const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables, unsigned Common, bool Singletons)
{
	Classes.clear(); Points.clear(); Lines.clear(); Surfaces.clear(); Attributes.clear(); Above = Common;

	while (auto I = ui->classLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();
	while (auto I = ui->simpleLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < Tables.size(); ++i)
	{
		auto Check = new QCheckBox(Tables[i].Label, this);

		if (Tables[i].Type & 256) Points.insert(Tables[i].Name, Tables[i].Label);
		if (Tables[i].Type & 8) Lines.insert(Tables[i].Name, Tables[i].Label);
		if (Tables[i].Type & 2) Surfaces.insert(Tables[i].Name, Tables[i].Label);

		Classes.insert(Tables[i].Name, Tables[i].Label);

		Check->setToolTip(Tables[i].Name);
		ui->classLayout->addWidget(Check);

		Attributes.append(Tables[i].Indexes.toSet());

		connect(Check, &QCheckBox::toggled, this, &FilterDialog::classBoxChecked);
	}

	for (int i = 0; i < Fields.size(); ++i)
	{
		const bool Singleton = !Singletons && (Fields[i].Dict.size() == 2 && Fields[i].Dict.contains(0));

		if (!Singleton) ui->simpleLayout->addWidget(new FilterWidget(i, Fields[i], this));
	}

	auto newModel = getJsHelperModel(this, Variables);

	auto oldModel = ui->variablesList->model();
	auto oldSelect = ui->variablesList->selectionModel();

	ui->helperCombo->setModel(newModel);
	ui->variablesList->setModel(newModel);
	ui->variablesList->setRootIndex(newModel->index(0, 0));

	oldModel->deleteLater(); oldSelect->deleteLater();

	connect(ui->variablesList->selectionModel(),
		   &QItemSelectionModel::currentChanged,
		   this, &FilterDialog::tooltipShowRequest);
}
