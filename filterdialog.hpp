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

#ifndef FILTERDIALOG_HPP
#define FILTERDIALOG_HPP

#include <QAbstractButton>
#include <QPushButton>
#include <QClipboard>
#include <QCheckBox>
#include <QDialog>
#include <QHash>

#include "databasedriver.hpp"
#include "filterwidget.hpp"

namespace Ui
{
	class FilterDialog;
}

class FilterDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::FilterDialog* ui;

		QList<QSet<int>> Attributes;

		unsigned Above = 0;

	public:

		explicit FilterDialog(QWidget* Parent = nullptr,
						  const QList<DatabaseDriver::FIELD>& Fields = QList<DatabaseDriver::FIELD>(),
						  const QList<DatabaseDriver::TABLE>& Tables = QList<DatabaseDriver::TABLE>(),
						  unsigned Common = 0);
		virtual ~FilterDialog(void) override;

		QString getFilterRules(void) const;
		QList<int> getUsedFields(void) const;

	private slots:

		void operatorTextChanged(const QString& Operator);

		void classSearchEdited(const QString& Search);
		void simpleSearchEdited(const QString& Search);

		void buttonBoxClicked(QAbstractButton* Button);

		void classBoxChecked(void);

		void tabIndexChanged(int Index);

		void addButtonClicked(void);
		void copyButtonClicked(void);
		void selectButtonClicked(void);
		void unselectButtonClicked(void);

	public slots:

		virtual void accept(void) override;

		void setFields(const QList<DatabaseDriver::FIELD>& Fields, const QList<DatabaseDriver::TABLE>& Tables, unsigned Common = 0);

	signals:

		void onFiltersUpdate(const QString&, const QList<int>&);

};

#endif // FILTERDIALOG_HPP
