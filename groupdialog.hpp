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

#ifndef GROUPDIALOG_HPP
#define GROUPDIALOG_HPP

#include <QSettings>
#include <QDialog>

namespace Ui
{
	class GroupDialog;
}

class GroupDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::GroupDialog* ui;

	public:

		explicit GroupDialog(QWidget* Parent = nullptr, const QStringList& Attributes = QStringList());
		virtual ~GroupDialog(void) override;

		QList<int> getEnabledGroupsIndexes(void);
		QStringList getEnabledGroupsNames(void);

	private slots:

		void searchEdited(const QString& Search);

	public slots:

		virtual void accept(void) override;

		void setAttributes(QStringList Attributes);

	signals:

		void onGroupsUpdate(const QList<int>);

};

#endif // GROUPDIALOG_HPP
