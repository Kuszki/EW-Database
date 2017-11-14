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

#ifndef UPDATEDIALOG_HPP
#define UPDATEDIALOG_HPP

#include <QAbstractButton>
#include <QPushButton>
#include <QDialog>
#include <QHash>

#include "updatewidget.hpp"

namespace Ui
{
	class UpdateDialog;
}

class UpdateDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::UpdateDialog* ui;

		QList<QHash<int, QVariant>> Values;
		QSet<UpdateWidget*> Status;
		QList<int> Active;

		int Index = 0;
		int Count = 0;

	public:

		explicit UpdateDialog(QWidget* Parent = nullptr, const QList<DatabaseDriver::FIELD>& Fields = QList<DatabaseDriver::FIELD>(), bool Singletons = false);
		virtual ~UpdateDialog(void) override;

		QHash<int, QVariant> getUpdatedValues(void) const;
		QHash<int, int> getNullReasons(void) const;

		bool isDataValid(void) const;

	private slots:

		void searchBoxEdited(const QString& Search);

		void fieldButtonChecked(bool Enabled);
		void allButtonChecked(bool Enabled);

		void clearButtonClicked(void);
		void prevButtonClicked(void);
		void nextButtonClicked(void);

		void dataCheckProgress(bool OK);

	public slots:

		virtual void accept(void) override;

		void setFields(const QList<DatabaseDriver::FIELD>& Fields, bool Singletons = false);

		void setPrepared(const QList<QHash<int, QVariant>>& Data, const QList<int>& Indexes);

		void setData(const QList<QHash<int, QVariant>> &Data);
		void setData(const QHash<int, QVariant>& Data);

		void setActive(const QList<int>& Indexes);

		void setUnchecked(void);

	signals:

		void onValuesUpdate(const QHash<int, QVariant>&,
						const QHash<int, int>&);

};

#endif // UPDATEDIALOG_HPP
