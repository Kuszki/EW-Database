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

#ifndef VARIABLESDIALOG_HPP
#define VARIABLESDIALOG_HPP

#include <QDialogButtonBox>
#include <QPushButton>
#include <QDialog>

namespace Ui
{
	class VariablesDialog;
}

class VariablesDialog : public QDialog
{

		Q_OBJECT

	private:

		const QHash<QString, QSet<QString>> Data;

		Ui::VariablesDialog* ui;

	public:

		explicit VariablesDialog(const QHash<QString, QSet<QString>>& Items, QWidget* Parent = nullptr);
		virtual ~VariablesDialog(void) override;

	public slots:

		virtual void accept(void) override;

	private slots:

		void variableIndexChanged(int Index);
		void dialogParamsChanged(void);

	signals:

		void onChangeRequest(const QString&, int, int, double);

};

#endif // VARIABLESDIALOG_HPP
