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

	connect(ui->tableView->selectionModel(),
		   &QItemSelectionModel::selectionChanged,
		   this, &SqleditorDialog::recordItemSelected);
}

SqleditorDialog::SqleditorDialog(QSqlDatabase& database, QWidget *parent)
: QDialog(parent), ui(new Ui::SqleditorDialog), db(database)
{
	ui->setupUi(this); new SqlHighlighter(ui->queryEdit->document());

	const auto mask = QSql::Tables | QSql::Views;
	const QStringList tabs = db.tables(QSql::TableType(mask));

	list = new QStringListModel(this);
	res = new QSqlQueryModel(this);
	tab = new QSqlTableModel(this, db);

	tab->setEditStrategy(QSqlTableModel::OnManualSubmit);

	QStringListModel* model = new QStringListModel(tabs, this);
	QStandardItemModel* helper = SqlHighlighter::getSqlHelperModel(this);

	ui->tableView->model()->deleteLater();
	ui->tableView->selectionModel()->deleteLater();
	ui->tableView->setModel(list);

	ui->tabsView->model()->deleteLater();
	ui->tabsView->selectionModel()->deleteLater();
	ui->tabsView->setModel(model);

	ui->fieldsView->model()->deleteLater();
	ui->fieldsView->selectionModel()->deleteLater();
	ui->fieldsView->setModel(new QStringListModel(this));

	ui->splitter->setSizes({ 200, 575 });

	ui->helperView->model()->deleteLater();
	ui->helperView->selectionModel()->deleteLater();

	ui->helperCombo->setModel(helper);
	ui->helperView->setModel(helper);
	ui->helperView->setRootIndex(helper->index(0, 0));

	connect(ui->tabsView->selectionModel(),
		   &QItemSelectionModel::currentRowChanged,
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

	connect(ui->exitButton, &QToolButton::clicked,
		   this, &SqleditorDialog::reject);

	connect(ui->helperView, &QListView::doubleClicked,
		   this, &SqleditorDialog::helperPasteRequest);

	connect(ui->helperView->selectionModel(),
		   &QItemSelectionModel::currentChanged,
		   this, &SqleditorDialog::helperTooltipRequest);

	connect(ui->helperCombo, qOverload<int>(&QComboBox::currentIndexChanged),
		   this, &SqleditorDialog::helperIndexChanged);
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

	std::sort(rows.begin(), rows.end(),
	[] (const QModelIndex& a, const QModelIndex& b) -> bool
	{
		return a.row() > b.row();
	});

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

void SqleditorDialog::helperIndexChanged(int Index)
{
	auto Model = ui->helperCombo->model();

	if (Model && Index != -1)
	{
		auto Root = Model->index(Index, 0);

		ui->helperView->setRootIndex(Root);
	}
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

void SqleditorDialog::helperTooltipRequest(const QModelIndex& Index)
{
	ui->helperLabel->setText(ui->helperView->model()->data(Index, Qt::ToolTipRole).toString());
}

void SqleditorDialog::helperPasteRequest(const QModelIndex& Index)
{
	const QString Value = ui->helperView->model()->data(Index).toString();

	if (!Value.isEmpty()) ui->queryEdit->insertPlainText(Value);
}
