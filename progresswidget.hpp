/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Firebird database editor                                               *
 *  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  lukasz.kuszki@gmail.com    *
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

#ifndef PROGRESSWIDGET_HPP
#define PROGRESSWIDGET_HPP

#include <QProgressBar>
#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QTime>

class ProgressWidget : public QProgressBar
{

		Q_OBJECT

	protected:

		QTimer etaTimer;
		QTimer trgTimer;

		QTime startTime;

		QString fmt;

	public:

		explicit ProgressWidget(QWidget* parent = nullptr);
		virtual ~ProgressWidget(void) override;

	public slots:

		void setFormat(const QString& format);
		void setRange(int min, int max);
		void setValue(int val);

		void hide(void);

	private slots:

		void updateEta(void);

};

#endif // PROGRESSWIDGET_HPP
