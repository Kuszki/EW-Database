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

#include "scriptdialog.hpp"
#include "ui_scriptdialog.h"

ScriptDialog::ScriptDialog(const QStringList& Fields, QWidget* Parent)
: QDialog(Parent), ui(new Ui::ScriptDialog)
{
	ui->setupUi(this); setFields(Fields);

	Highlighter = new KLHighlighter(ui->scriptEdit->document());

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

ScriptDialog::~ScriptDialog(void)
{
	delete ui;
}

QPair<QString, int> ScriptDialog::validateScript(const QString& Script) const
{
	if (Script.trimmed().isEmpty()) return QPair<QString, int>(); QJSEngine Engine;

	auto Model = ui->variablesList->model();
	auto Root = ui->variablesList->rootIndex();

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

void ScriptDialog::accept(void)
{
	const auto V = validateScript(ui->scriptEdit->toPlainText());

	if (V.second) QMessageBox::critical(this, tr("Syntax error in line %1").arg(V.second), V.first);
	else
	{
		QDialog::accept(); emit onRunRequest(ui->scriptEdit->toPlainText().trimmed());
	}
}

void ScriptDialog::setFields(const QStringList& Fields)
{
	auto newModel = getJsHelperModel(this, Fields);

	auto oldModel = ui->variablesList->model();
	auto oldSelect = ui->variablesList->selectionModel();

	ui->helperCombo->setModel(newModel);
	ui->variablesList->setModel(newModel);
	ui->variablesList->setRootIndex(newModel->index(0, 0));

	oldModel->deleteLater(); oldSelect->deleteLater();
}

void ScriptDialog::validateButtonClicked(void)
{
	const auto V = validateScript(ui->scriptEdit->toPlainText());

	if (!V.second) ui->helpLabel->setText(tr("Script is ok"));
	else ui->helpLabel->setText(tr("Syntax error in line %1: %2")
						   .arg(V.second).arg(V.first));
}

void ScriptDialog::scriptTextChanged(void)
{
	const bool OK = !ui->scriptEdit->toPlainText().trimmed().isEmpty();

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(OK);
}

void ScriptDialog::helperIndexChanged(int Index)
{
	auto Model = ui->helperCombo->model();

	if (Model && Index != -1)
	{
		auto Root = Model->index(Index, 0);

		ui->variablesList->setRootIndex(Root);
	}

	ui->helpLabel->clear();
}

void ScriptDialog::tooltipShowRequest(QModelIndex Index)
{
	ui->helpLabel->setText(ui->variablesList->model()->data(Index, Qt::ToolTipRole).toString());
}

void ScriptDialog::variablePasteRequest(QModelIndex Index)
{
	const QString Value = ui->variablesList->model()->data(Index).toString();

	if (!Value.isEmpty()) ui->scriptEdit->insertPlainText(Value);
}
