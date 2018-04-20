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

#ifndef COPYFIELDSDIALOG_HPP
#define COPYFIELDSDIALOG_HPP

#include <QDialogButtonBox>
#include <QPushButton>
#include <QDialog>

#include "copyfieldswidget.hpp"

namespace Ui
{
	class CopyfieldsDialog;
}

class CopyfieldsDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::CopyfieldsDialog* ui;

		QStringList List;
		int Count = 0;

	public:

		explicit CopyfieldsDialog(const QStringList& Fields,
							 QWidget* Parent = nullptr);
		virtual ~CopyfieldsDialog(void) override;

		QList<CopyfieldsWidget::RECORD> getFunctions(void) const;

	private slots:

		void newButtonClicked(void);
		void copyWidgetDeleted(void);

	public slots:

		virtual void accept(void) override;

		void setFields(const QStringList& Fields);

	signals:

		onCopyRequest(const QList<CopyfieldsWidget::RECORD>&, bool);

};

#endif // COPYFIELDSDIALOG_HPP
