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

#include <QStandardItemModel>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>

#include "databasedriver_v2.hpp"

namespace Ui
{
	class FilterWidget;
}

class FilterWidget : public QWidget
{

		Q_OBJECT

	private:

		QLineEdit* Simple = nullptr;
		QWidget* Widget = nullptr;
		Ui::FilterWidget* ui;

		int Index = 0;

	public:

		explicit FilterWidget(int ID, const DatabaseDriver_v2::FIELD& Field, QWidget* Parent = nullptr);
		virtual ~FilterWidget(void) override;

		QString getCondition(void) const;
		QVariant getValue(void) const;
		QString getLabel(void) const;

		int getIndex(void) const;

	private slots:

		void operatorChanged(const QString& Name);

		void editFinished(void);
		void resetIndex(void);

	public slots:

		void setParameters(int ID, const DatabaseDriver_v2::FIELD& Field);
		void setValue(const QVariant& Value);
		void setChecked(bool Checked);

		bool isChecked(void) const;

		void reset(void);

	signals:

		void onValueUpdate(const QString&, const QVariant&);

		void onStatusChanged(bool);

};

#endif // FILTERWIDGET_HPP
