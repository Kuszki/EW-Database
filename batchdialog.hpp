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

#ifndef BATCHDIALOG_HPP
#define BATCHDIALOG_HPP

#include <QDialog>

#include "batchwidget.hpp"

namespace Ui
{
	class BatchDialog;
}

class BatchDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::BatchDialog* ui;

	public:

		explicit BatchDialog(const QStringList& Fields,
						 const QStringList& First,
						 QWidget* Parent = nullptr);
		virtual ~BatchDialog(void) override;

		QMap<int, BatchWidget::FUNCTION> getFunctions(void) const;

	public slots:

		virtual void accept(void) override;

		void setParameters(const QStringList& Fields,
					    const QStringList& First);

	signals:

		void onBatchRequest(const QMap<int, BatchWidget::FUNCTION>&);

};

#endif // BATCHDIALOG_HPP