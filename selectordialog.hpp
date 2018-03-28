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

#ifndef SELECTORDIALOG_HPP
#define SELECTORDIALOG_HPP

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QPushButton>
#include <QDialog>

namespace Ui
{
	class SelectorDialog;
}

class SelectorDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::SelectorDialog* ui;

	public:

		explicit SelectorDialog(QWidget* Parent = nullptr);
		virtual ~SelectorDialog(void) override;

	private slots:

		void openButtonClicked(void);

	public slots:

		virtual void accept(void) override;

	signals:

		void onDataAccepted(const QString&, int);

};

#endif // SELECTORDIALOG_HPP
