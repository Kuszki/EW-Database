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

#ifndef SELECTORDIALOG_HPP
#define SELECTORDIALOG_HPP

#include <QDialogButtonBox>
#include <QActionGroup>
#include <QToolButton>
#include <QFileDialog>
#include <QPushButton>
#include <QClipboard>
#include <QMimeData>
#include <QDialog>
#include <QMenu>

namespace Ui
{
	class SelectorDialog;
}

class SelectorDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::SelectorDialog* ui;

		QActionGroup* saveMode;
		QToolButton* Save;

	public:

		explicit SelectorDialog(QWidget* Parent = nullptr);
		virtual ~SelectorDialog(void) override;

	private slots:

		void actionIndexChanged(int Index);

		void openButtonClicked(void);
		void refreshButtonClicked(void);

		void listEditChanged(void);

	public slots:

		virtual void accept(void) override;
		virtual void open(void) override;

	signals:

		void onDataAccepted(const QStringList&, int, int);

};

#endif // SELECTORDIALOG_HPP
