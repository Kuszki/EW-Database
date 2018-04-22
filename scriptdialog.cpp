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

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

ScriptDialog::~ScriptDialog(void)
{
	delete ui;
}

QJSValue ScriptDialog::validateScript(const QString& Script) const
{
	auto Model = dynamic_cast<QStringListModel*>(ui->variablesList->model());

	if (Script.trimmed().isEmpty()) return QJSValue();

	QJSEngine Engine; for (const auto& V : Model->stringList())
	{
		Engine.globalObject().setProperty(V, QJSValue());
	}

	return Engine.evaluate(Script);
}

void ScriptDialog::accept(void)
{
	const auto V = validateScript(ui->scriptEdit->toPlainText());

	if (V.isError()) QMessageBox::critical(this, tr("Syntax error in line %1")
								    .arg(V.property("lineNumber").toInt()),
								    V.toString());
	else
	{
		QDialog::accept(); emit onRunRequest(ui->scriptEdit->toPlainText().trimmed());
	}
}

void ScriptDialog::setFields(const QStringList& Fields)
{
	auto oldModel = ui->variablesList->model();
	auto oldSelect = ui->variablesList->selectionModel();

	ui->variablesList->setModel(new QStringListModel(Fields, this));

	oldModel->deleteLater(); oldSelect->deleteLater();
}

void ScriptDialog::validateButtonClicked(void)
{
	const auto V = validateScript(ui->scriptEdit->toPlainText());

	if (V.isError()) QMessageBox::critical(this, tr("Syntax error in line %1")
								    .arg(V.property("lineNumber").toInt()),
								    V.toString());
	else QMessageBox::information(this, tr("Syntax check"), tr("Script is ok"));
}

void ScriptDialog::scriptTextChanged(void)
{
	const bool OK = !ui->scriptEdit->toPlainText().trimmed().isEmpty();

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(OK);
}

void ScriptDialog::variablePasteRequest(QModelIndex Index)
{
	const QString Value = ui->variablesList->model()->data(Index).toString();

	if (!Value.isEmpty()) ui->scriptEdit->insertPlainText(Value);
}
