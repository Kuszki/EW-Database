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

	public:

		explicit FilterDialog(QWidget* Parent = nullptr,
						  const QList<QPair<QString, QString>>& Fields = QList<QPair<QString, QString>>(),
						  const QHash<QString, QHash<int, QString>>& Dictionary = QHash<QString, QHash<int, QString>>());
		virtual ~FilterDialog(void) override;

		QString getFilterRules(void);

	private slots:

		void searchEdited(const QString& Search);

		void buttonClicked(QAbstractButton* Button);

		void addClicked(void);

	public slots:

		virtual void accept(void) override;

		void setAvailableFields(const QList<QPair<QString, QString>>& Fields,
						    const QHash<QString, QHash<int, QString>>& Dictionary = QHash<QString, QHash<int, QString>>());

	signals:

		void onFiltersUpdate(const QString&);

};

#endif // FILTERDIALOG_HPP
