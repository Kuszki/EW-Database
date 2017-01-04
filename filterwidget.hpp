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

#ifndef FILTERWIDGET_HPP
#define FILTERWIDGET_HPP

#include <QComboBox>
#include <QLineEdit>
#include <QWidget>

#include "databasedriver.hpp"

namespace Ui
{
	class FilterWidget;
}

class FilterWidget : public QWidget
{

		Q_OBJECT

	private:

		Ui::FilterWidget* ui;
		QWidget* Widget;

	public:

		explicit FilterWidget(const QString& Name, const QString& Key, QWidget* Parent = nullptr,
						  const QHash<int, QString>& Dictionary = QHash<int, QString>());
		virtual ~FilterWidget(void) override;

		QString getCondition(void) const;
		QString getValue(void) const;

	private slots:

		void editFinished(void);

	public slots:

		void setParameters(const QString& Name, const QString& Key, const QString& Value);

		void setName(const QString& Name);
		void setKey(const QString& Key);
		void setValue(const QString& Value);

		bool isChecked(void) const;

		void reset(void);

	signals:

		void onValueUpdate(const QString&, const QString&);

		void onStatusChanged(bool);

};

#endif // FILTERWIDGET_HPP
