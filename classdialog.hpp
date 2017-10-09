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

#ifndef CLASSDIALOG_HPP
#define CLASSDIALOG_HPP

#include <QIntValidator>
#include <QDialog>
#include <QPair>
#include <QMap>

namespace Ui
{
	class ClassDialog;
}

class ClassDialog : public QDialog
{

		Q_OBJECT

	private:

		const QHash<QString, QHash<int, QString>> lineLayers;
		const QHash<QString, QHash<int, QString>> pointLayers;
		const QHash<QString, QHash<int, QString>> textLayers;

		Ui::ClassDialog* ui;

	public:

		explicit ClassDialog(const QHash<QString, QString>& Classes,
						 const QHash<QString, QHash<int, QString>>& Lines,
						 const QHash<QString, QHash<int, QString>>& Points,
						 const QHash<QString, QHash<int, QString>>& Texts,
						 QWidget* Parent = nullptr);
		virtual ~ClassDialog(void) override;

	public slots:

		virtual void accept(void) override;

	private slots:

		void classIndexChanged(int Index);

	signals:

		void onChangeRequest(const QString&, int, int, int,
						 const QString&, int);

};

#endif // CLASSDIALOG_HPP
