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

#ifndef LABELDIALOG_HPP
#define LABELDIALOG_HPP

#include <QDialogButtonBox>
#include <QPushButton>
#include <QDialog>

namespace Ui
{
	class LabelDialog;
}

class LabelDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::LabelDialog* ui;

	public:

		explicit LabelDialog(QWidget* Parent = nullptr);
		virtual ~LabelDialog(void) override;

	private slots:

		void labelTextChanged(const QString& Text);

	public slots:

		virtual void accept(void) override;

	signals:

		void onLabelRequest(const QString&, int,
						double, double, bool);

};

#endif // LABELDIALOG_HPP
