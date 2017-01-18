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

#ifndef CONNECTDIALOG_HPP
#define CONNECTDIALOG_HPP

#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QDialog>

namespace Ui
{
	class ConnectDialog;
}

class ConnectDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::ConnectDialog* ui;

	public:

		explicit ConnectDialog(QWidget* Parent = nullptr);
		virtual ~ConnectDialog(void) override;

	private slots:

		void edited(void);

	public slots:

		virtual void accept(void) override;
		virtual void reject(void) override;

		void refused(const QString& Error);
		void connected(bool OK);

	signals:

		void onAccept(const QString&, const QString&,
				    const QString&, const QString&);

};

#endif // CONNECTDIALOG_HPP
