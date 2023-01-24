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

#include "progresswidget.hpp"

ProgressWidget::ProgressWidget(QWidget* parent)
: QProgressBar(parent)
, fmt(format())
{
	etaTimer.setInterval(1000);
	etaTimer.setSingleShot(false);

	trgTimer.setInterval(10000);
	trgTimer.setSingleShot(true);

	connect(&etaTimer, &QTimer::timeout,
		   this, &ProgressWidget::updateEta);

	connect(&trgTimer, &QTimer::timeout,
		   &etaTimer, qOverload<>(&QTimer::start));
}

ProgressWidget::~ProgressWidget(void) {}

void ProgressWidget::setFormat(const QString& format)
{
	QProgressBar::setFormat(fmt = format);
}

void ProgressWidget::setRange(int min, int max)
{
	QProgressBar::setRange(min, max);

	startTime = QTime::currentTime();

	if (min != max) trgTimer.start();
	else etaTimer.stop();
}

void ProgressWidget::setValue(int val)
{
	if (val == value()) return;
	else QProgressBar::setValue(val);

	if (val == maximum())
	{
		etaTimer.stop();
		trgTimer.stop();

		setFormat(fmt);
	}
}

void ProgressWidget::hide(void)
{
	QProgressBar::hide();

	etaTimer.stop();
	trgTimer.stop();

	setFormat(fmt);
}

void ProgressWidget::updateEta(void)
{
	const int span = maximum() - minimum();
	const int prog = value() - minimum();

	if (span <= 0 || prog <= 0) return;
	else if (prog == span) QProgressBar::setFormat(fmt);
	else
	{
		const double diff = startTime.msecsTo(QTime::currentTime()) / 1000.0;
		const double full = double(diff * span) / double(prog);

		const auto time = QTime(0, 0).addSecs(full - diff).toString();

		QProgressBar::setFormat(QString("%1 (%2)").arg(fmt, time));
	}
}
