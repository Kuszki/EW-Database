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

#ifndef HARMONIZEDIALOG_HPP
#define HARMONIZEDIALOG_HPP

#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDialog>
#include <QSet>

namespace Ui
{
	class HarmonizeDialog;
}

class HarmonizeDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::HarmonizeDialog* ui;

		QString File;

	public:

		explicit HarmonizeDialog(QWidget* Parent = nullptr, const QString& Path = QString());
		virtual ~HarmonizeDialog(void) override;

		QString getPath(void) const;

	public slots:

		virtual void accept(void) override;

		virtual void open(const QString& Path);

		void setPath(const QString& Path);

	private slots:

		void sourceTypeChanged(int Type);

		void fitParametersChanged(void);

	signals:

		void onFitRequest(const QString&, int,
					   int, int, int, int,
					   double, double, bool);

};

#endif // HARMONIZEDIALOG_HPP
