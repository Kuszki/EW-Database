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

#ifndef SCRIPTDIALOG_HPP
#define SCRIPTDIALOG_HPP

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QDialog>

#include "databasedriver.hpp"
#include "klhighlighter.hpp"

namespace Ui
{
	class ScriptDialog;
}

class ScriptDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::ScriptDialog* ui;

		KLHighlighter* Highlighter;

	public:

		explicit ScriptDialog(const QStringList& Fields,
						  QWidget* Parent = nullptr);
		virtual ~ScriptDialog(void) override;

	protected:

		QPair<QString, int> validateScript(const QString& Script) const;

	public slots:

		virtual void accept(void) override;

		void setFields(const QStringList& Fields);

	private slots:

		void validateButtonClicked(void);
		void scriptTextChanged(void);

		void helperIndexChanged(int Index);

		void tooltipShowRequest(QModelIndex Index);
		void variablePasteRequest(QModelIndex Index);

	signals:

		void onRunRequest(const QString&);

};

#endif // SCRIPTDIALOG_HPP
