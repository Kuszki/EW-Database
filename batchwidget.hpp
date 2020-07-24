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

#ifndef BATCHWIDGET_HPP
#define BATCHWIDGET_HPP

#include <QWidget>

namespace Ui
{
	class BatchWidget;
}

class BatchWidget : public QWidget
{

		Q_OBJECT

	public: enum FUNCTION
	{
		IGNORE,
		WHERE,
		UPDATE
	};

	public:

		using RECORD = QPair<int, FUNCTION>;

	private:

		Ui::BatchWidget* ui;

		int Index = 0;

	public:

		explicit BatchWidget(int ID, const QString& Tip,
						 const QStringList& Fields,
						 QWidget* Parent = nullptr);
		virtual ~BatchWidget(void) override;

		RECORD getFunction(void) const;
		FUNCTION getAction(void) const;

		int getField(void) const;
		int getIndex(void) const;	

	public slots:

		void headerChecked(bool Checked);

		void setData(int ID, const QString& Tip,
				   const QStringList& Fields);

};

#endif // BATCHWIDGET_HPP
