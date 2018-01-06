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

#ifndef REDACTIONWIDGET_HPP
#define REDACTIONWIDGET_HPP

#include <QVariant>
#include <QVector>
#include <QWidget>
#include <QPair>

#include <QtCore>

namespace Ui
{
	class RedactionWidget;
}

class RedactionWidget : public QWidget
{

		Q_OBJECT

	private:

		static const QVector<int> Numbers;
		static const QVector<int> Texts;
		static const QVector<int> Combos;

		Ui::RedactionWidget* ui;

	public:

		explicit RedactionWidget(QWidget* Parent = nullptr);
		virtual ~RedactionWidget(void) override;

		QPair<int, QVariant> getCondition(void) const;

	private slots:

		void typeChanged(int Index);
		void editFinished(void);

	signals:

		void onValueUpdate(const QPair<int, QVariant>&);

};

#endif // REDACTIONWIDGET_HPP
