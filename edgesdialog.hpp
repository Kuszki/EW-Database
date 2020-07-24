/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@o2.pl             *
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

#ifndef EdgesDialog_HPP
#define EdgesDialog_HPP

#include <QStandardItemModel>
#include <QAbstractButton>
#include <QPushButton>
#include <QCheckBox>
#include <QDialog>
#include <QHash>

#include "databasedriver.hpp"

namespace Ui
{
	class EdgesDialog;
}

class EdgesDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::EdgesDialog* ui;

		QList<int> Active;

	public:

		explicit EdgesDialog(QWidget* Parent = nullptr,
						 const QList<DatabaseDriver::FIELD>& Fields = QList<DatabaseDriver::FIELD>());
		virtual ~EdgesDialog(void) override;

		QList<int> getSelectedFields(void) const;

	private slots:

		void searchBoxEdited(const QString& Search);

		void allButtonChecked(bool Enabled);

	public slots:

		virtual void accept(void) override;

		void setFields(const QList<DatabaseDriver::FIELD>& Fields);

		void setActive(const QList<int>& Indexes);

		void setUnchecked(void);

	signals:

		void onEdgeRequest(const QList<int>&);


};

#endif // EdDGESDIALOG_HPP
