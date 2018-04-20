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

#ifndef COPYFIELDSWIDGET_HPP
#define COPYFIELDSWIDGET_HPP

#include <QWidget>

namespace Ui
{
	class CopyfieldsWidget;
}

class CopyfieldsWidget : public QWidget
{

		Q_OBJECT

	public: enum FUNCTION
	{
		WHERE,
		UPDATE
	};

	public:

		using RECORD = QPair<FUNCTION, QPair<int, int>>;

	private:

		Ui::CopyfieldsWidget* ui;

	public:

		explicit CopyfieldsWidget(const QStringList& Fields,
							 QWidget* Parent = nullptr);
		virtual ~CopyfieldsWidget(void) override;

		RECORD getFunction(void) const;
		FUNCTION getAction(void) const;

		int getSrc(void) const;
		int getDest(void) const;

		void setData(const QStringList& Fields);

};

#endif // COPYFIELDSWIDGET_HPP
