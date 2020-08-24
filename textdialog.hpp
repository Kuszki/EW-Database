/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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

#ifndef TEXTDIALOG_HPP
#define TEXTDIALOG_HPP

#include <QPushButton>
#include <QDialog>

namespace Ui
{
	class TextDialog;
}

class TextDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::TextDialog* ui;

	public:

		explicit TextDialog(QWidget* Parent = nullptr);
		virtual ~TextDialog(void) override;

	private slots:

		void CheckStatusChanged(void);

	public slots:

		virtual void accept(void) override;

	signals:

		void onEditRequest(bool, int, bool, bool, double);

};

#endif // TEXTDIALOG_HPP
