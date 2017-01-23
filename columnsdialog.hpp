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

#ifndef COLUMNSDIALOG_HPP
#define COLUMNSDIALOG_HPP

#include <QSettings>
#include <QCheckBox>
#include <QDialog>
#include <QHash>

namespace Ui
{
	class ColumnsDialog;
}

class ColumnsDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::ColumnsDialog* ui;

	public:

		explicit ColumnsDialog(QWidget* Parent = nullptr, const QStringList Headers = QStringList(), int Common = 0);
		virtual ~ColumnsDialog(void) override;

		QList<int> getEnabledColumnsIndexes(void);
		QStringList getEnabledColumnsNames(void);

	private slots:

		void selectButtonClicked(void);
		void unselectButtonClicked(void);

		void searchTextEdited(const QString& Search);

	public slots:

		virtual void accept(void) override;

		void setAttributes(const QStringList Headers, int Common = 0);

	signals:

		void onColumnsUpdate(const QList<int>&);

};

#endif // COLUMNSDIALOG_HPP
