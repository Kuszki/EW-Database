/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  {description}                                                          *
 *  Copyright (C) 2020  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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

#include "sqleditordialog.hpp"
#include "ui_sqleditordialog.h"

void SqleditorDialog::switchModel(QAbstractItemModel* model)
{
	if (ui->tableView->model() == model) return;

	ui->tableView->selectionModel()->deleteLater();
	ui->tableView->setModel(model);

	ui->delButton->setEnabled(false);
	ui->addButton->setEnabled(model == tab);

	connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
		   this, &SqleditorDialog::recordItemSelected);
}

SqleditorDialog::SqleditorDialog(QSqlDatabase& database, QWidget *parent)
: QDialog(parent), ui(new Ui::SqleditorDialog), db(database)
{
	ui->setupUi(this);

	list = new QStringListModel(this);
	res = new QSqlQueryModel(this);
	tab = new QSqlTableModel(this, db);

	tab->setEditStrategy(QSqlTableModel::OnManualSubmit);

	QStringListModel* model = new QStringListModel(db.tables(), this);

	ui->tableView->model()->deleteLater();
	ui->tableView->setModel(list);

	ui->tabsView->model()->deleteLater();
	ui->tabsView->setModel(model);

	ui->fieldsView->model()->deleteLater();
	ui->fieldsView->setModel(new QStringListModel(this));

	ui->splitter->setSizes({ 200, 575 });

	connect(ui->tabsView->selectionModel(), &QItemSelectionModel::currentRowChanged,
		   this, &SqleditorDialog::tableItemSelected);

	connect(ui->tabsView, &QListView::doubleClicked,
		   this, &SqleditorDialog::tableItemClicked);

	connect(ui->fieldsView, &QListView::doubleClicked,
		   this, &SqleditorDialog::fieldItemClicked);

	connect(ui->runButton, &QToolButton::clicked,
		   this, &SqleditorDialog::executeActionClicked);

	connect(ui->delButton, &QToolButton::clicked,
		   this, &SqleditorDialog::deleteButtonClicked);

	connect(ui->undoButton, &QToolButton::clicked,
		   this, &SqleditorDialog::rollbackButtonClicked);

	connect(ui->saveButton, &QToolButton::clicked,
		   this, &SqleditorDialog::commitActionClicked);

	connect(ui->addButton, &QToolButton::clicked,
		   this, &SqleditorDialog::appendButtonClicked);
}

SqleditorDialog::~SqleditorDialog(void)
{
	delete ui;
}

void SqleditorDialog::executeActionClicked(void)
{
	const QString text = ui->queryEdit->toPlainText()
					 .replace(QRegExp("\\s+"), " ")
					 .trimmed();

	if (ui->tableView->model() == tab)
		tab->revertAll();

	if (text.isEmpty()) return;
	else if (text.startsWith("where ", Qt::CaseInsensitive))
	{
		tab->setFilter(text.mid(6));
		tab->select();

		list->setStringList({ tab->lastError().text() });

		if (tab->lastError().isValid())
			this->switchModel(list);
		else
			this->switchModel(tab);
	}
	else if (text.startsWith("select ", Qt::CaseInsensitive))
	{
		res->setQuery(text, db);

		list->setStringList({ res->lastError().text() });

		if (res->lastError().isValid())
			this->switchModel(list);
		else
			this->switchModel(res);
	}
	else
	{
		if (!trans) trans = db.transaction();
		QSqlQuery query(text, db);

		if (query.lastError().isValid())
		{
			list->setStringList({ query.lastError().text() });
		}
		else
		{
			list->setStringList({ tr("Query complete, %n row(s) affected",
							  nullptr, query.numRowsAffected()) });
		}

		this->switchModel(list);
	}
}

void SqleditorDialog::commitActionClicked(void)
{
	if (ui->tableView->model() == tab)
		tab->submitAll();

	if (trans) db.commit();

	trans = false;
}

void SqleditorDialog::rollbackButtonClicked(void)
{
	if (ui->tableView->model() == tab)
		tab->revertAll();

	if (trans) db.rollback();

	trans = false;
}

void SqleditorDialog::appendButtonClicked(void)
{
	if (ui->tableView->model() == tab)
		tab->insertRow(0);
}

void SqleditorDialog::deleteButtonClicked(void)
{
	if (ui->tableView->model() != tab) return;
	auto rows = ui->tableView->selectionModel()->selectedRows();

	std::sort(rows.begin(), rows.end(), qGreater<QModelIndex>());

	for (const auto r : rows) tab->removeRow(r.row(), r.parent());

	ui->delButton->setEnabled(false);
}

void SqleditorDialog::tableItemSelected(const QModelIndex& index)
{
	if (!index.isValid()) return;
	QStringList list;

	auto model = dynamic_cast<QStringListModel*>(ui->fieldsView->model());

	const auto r = db.record(index.data().toString());

	for (int i = 0; i < r.count(); ++i)
	{
		list.append(r.fieldName(i));
	}

	model->setStringList(list);
}

void SqleditorDialog::recordItemSelected(void)
{
	const bool mok = ui->tableView->model() == tab;
	const auto rows = ui->tableView->selectionModel()->selectedRows();

	ui->delButton->setEnabled(mok && !rows.isEmpty());
}

void SqleditorDialog::tableItemClicked(const QModelIndex& index)
{
	if (!index.isValid()) return;

	tab->revertAll();
	tab->setTable(index.data().toString());
	tab->select();

	this->switchModel(tab);
}

void SqleditorDialog::fieldItemClicked(const QModelIndex& index)
{
	if (!index.isValid()) return;

	ui->queryEdit->appendPlainText(index.data().toString());
}
