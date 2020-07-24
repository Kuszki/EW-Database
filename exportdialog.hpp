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

#ifndef EXPORTDIALOG_HPP
#define EXPORTDIALOG_HPP

#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QCheckBox>
#include <QDialog>

namespace Ui
{
	class ExportDialog;
}

class ExportDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::ExportDialog* ui;

		int Count = 0;

	public:

		explicit ExportDialog(QWidget* Parent = nullptr, const QStringList& Headers = QStringList());
		virtual ~ExportDialog(void) override;

		QList<int> getEnabledColumnsIndexes(void);
		QStringList getEnabledColumnsNames(void);

	private slots:

		void selectButtonClicked(void);
		void unselectButtonClicked(void);

		void searchTextEdited(const QString& Search);

		void typeIndexChanged(int Type);

		void itemCheckChanged(bool Checked);

	public slots:

		virtual void accept(void) override;

		void setAttributes(const QStringList& Headers);

	signals:

		void onExportRequest(const QList<int>&, int, bool);

};

#endif // EXPORTDIALOG_HPP
