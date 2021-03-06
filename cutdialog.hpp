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

#ifndef CUTDIALOG_HPP
#define CUTDIALOG_HPP

#include <QAbstractButton>
#include <QPushButton>
#include <QCheckBox>
#include <QDialog>
#include <QHash>

#include "databasedriver.hpp"

namespace Ui
{
	class CutDialog;
}

class CutDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::CutDialog* ui;

		int Count = 0;

	public:

		explicit CutDialog(QWidget* Parent = nullptr,
					    const QList<DatabaseDriver::TABLE>& Tables =  QList<DatabaseDriver::TABLE>());
		virtual ~CutDialog(void) override;

		QStringList getSelectedClasses(void) const;
		int isEndingsChecked(void) const;

	private slots:

		void searchBoxEdited(const QString& Search);

		void fieldButtonChecked(bool Enabled);

		void lineIndexChanged(int Index);

	public slots:

		virtual void accept(void) override;

		void setFields(const QList<DatabaseDriver::TABLE>& Tables);

		void setUnchecked(void);

	signals:

		void onClassesUpdate(const QStringList&, int);


};

#endif // CUTDIALOG_HPP
