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

#ifndef BREAKSDIALOG_HPP
#define BREAKSDIALOG_HPP

#include <QDialogButtonBox>
#include <QPushButton>
#include <QDialog>

namespace Ui
{
	class BreaksDialog;
}

class BreaksDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::BreaksDialog* ui;

	public:

		explicit BreaksDialog(QWidget* Parent = nullptr);
		virtual ~BreaksDialog(void) override;

	public slots:

		virtual void accept(void) override;

	private slots:

		void dialogParamsChanged(void);

	signals:

		void onReduceRequest(int, double, double, bool);

};

#endif // BREAKSDIALOG_HPP
