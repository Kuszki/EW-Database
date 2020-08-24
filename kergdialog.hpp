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

#ifndef KERGDIALOG_HPP
#define KERGDIALOG_HPP

#include <QStandardItemModel>
#include <QDialogButtonBox>
#include <QIntValidator>
#include <QFileDialog>
#include <QPushButton>
#include <QDialog>
#include <QPair>
#include <QMap>

namespace Ui
{
	class KergDialog;
}

class KergDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::KergDialog* ui;

		int Checked = 0;

	public:

		explicit KergDialog(QWidget* Parent = nullptr);
		virtual ~KergDialog(void) override;

	public slots:

		virtual void accept(void) override;

	private slots:

		void elementsActionsChanged(QStandardItem* Item);

		void actionIndexChanged(int Index);

		void dialogParamsChanged(void);
		void openButtonClicked(void);

	signals:

		void onUpdateRequest(const QString&, int, int);

};

#endif // KERGDIALOG_HPP
