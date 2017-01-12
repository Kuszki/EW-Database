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
		int Count = 0;

	public:

		explicit UpdateDialog(QWidget* Parent = nullptr,
						  const QList<QPair<QString, QString>>& Fields = QList<QPair<QString, QString>>(),
						  const QHash<QString, QHash<int, QString>>& Dictionary = QHash<QString, QHash<int, QString>>());
		virtual ~UpdateDialog(void) override;

		QHash<QString, QString> getUpdateRules(void);

	private slots:

		void searchEdited(const QString& Search);

		void fieldChecked(bool Enabled);

	public slots:

		virtual void accept(void) override;

		void setAvailableFields(const QList<QPair<QString, QString>>& Fields,
						    const QHash<QString, QHash<int, QString>>& Dictionary = QHash<QString, QHash<int, QString>>());

		void setFieldsData(const QHash<QString, QString>& Data);

		void setFieldsUnchecked(void);

	signals:

		void onValuesUpdate(const QHash<QString, QString>&);

};

#endif // UPDATEDIALOG_HPP
