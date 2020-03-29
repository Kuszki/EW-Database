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

#ifndef JOINDIALOG_HPP
#define JOINDIALOG_HPP

#include <QStandardItemModel>
#include <QAbstractButton>
#include <QMessageBox>
#include <QPushButton>
#include <QListView>
#include <QDialog>
#include <QSet>

namespace Ui
{
	class JoinDialog;
}

class JoinDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::JoinDialog *ui;

	public:

		explicit JoinDialog(const QHash<QString, QString>& Points,
						const QHash<QString, QString>& Lines,
						const QHash<QString, QString>& Circles,
						QWidget* Parent = nullptr);
		virtual ~JoinDialog(void) override;

		const QHash<QString, QString> P, L, C;

	private slots:

		void buttonBoxClicked(QAbstractButton* Button);

		void typeIndexChanged(int Index);

		void targetNameChanged(void);

	public slots:

		void completeActions(int Count);

	signals:

		void onCreateRequest(const QString&, const QString&, bool, int, double);
		void onDeleteRequest(const QString&, const QString&, int);

};

#endif // JOINDIALOG_HPP
