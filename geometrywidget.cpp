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

#include "geometrywidget.hpp"
#include "ui_geometrywidget.h"

const QVector<int> GeometryWidget::Numbers = { 0, 1 };
const QVector<int> GeometryWidget::Points = { 2, 3, 4, 5, 6, 7 };
const QVector<int> GeometryWidget::Classes = { 8, 9 };
const QVector<int> GeometryWidget::Lines = { 10, 11 };

GeometryWidget::GeometryWidget(const QHash<QString, QString>& Classes, const QHash<QString, QString>& Points, QWidget* Parent)
: QWidget(Parent), ui(new Ui::GeometryWidget)
{
	ui->setupUi(this); setParameters(Classes, Points); typeChanged(ui->typeCombo->currentIndex());
}

GeometryWidget::~GeometryWidget(void)
{
	delete ui;
}

QPair<int, QVariant> GeometryWidget::getCondition(void) const
{
	const int Index = ui->typeCombo->currentIndex();

	if (Numbers.contains(Index)) return qMakePair(Index, ui->sizeSpin->value());
	if (Lines.contains(Index)) return qMakePair(Index, ui->lineCombo->currentData());
	if (Points.contains(Index)) return qMakePair(Index, ui->pointCombo->currentData());
	if (Classes.contains(Index)) return qMakePair(Index, ui->classCombo->currentData());

	return qMakePair(Index, QVariant());
}

void GeometryWidget::typeChanged(int Index)
{
	ui->sizeSpin->setVisible(Numbers.contains(Index));
	ui->pointCombo->setVisible(Points.contains(Index));
	ui->classCombo->setVisible(Classes.contains(Index));
	ui->lineCombo->setVisible(Lines.contains(Index));
}

void GeometryWidget::editFinished(void)
{
	emit onValueUpdate(getCondition());
}

void GeometryWidget::setParameters(const QHash<QString, QString>& Classes, const QHash<QString, QString>& Points)
{
	ui->classCombo->clear(); ui->pointCombo->clear(); ui->lineCombo->clear();

	for (auto i = Points.constBegin(); i != Points.constEnd(); ++i)
	{
		ui->classCombo->addItem(i.value(), i.key());
		ui->pointCombo->addItem(i.value(), i.key());
	}

	for (auto i = Classes.constBegin(); i != Classes.constEnd(); ++i)
	{
		ui->classCombo->addItem(i.value(), i.key());
		ui->lineCombo->addItem(i.value(), i.key());
	}

	ui->classCombo->model()->sort(0);
	ui->pointCombo->model()->sort(0);
	ui->lineCombo->model()->sort(0);

	ui->classCombo->insertItem(0, tr("Any object"), "*");
	ui->pointCombo->insertItem(0, tr("Any object"), "*");
	ui->lineCombo->insertItem(0, tr("Any object"), "*");

	ui->classCombo->setCurrentIndex(0);
	ui->pointCombo->setCurrentIndex(0);
	ui->lineCombo->setCurrentIndex(0);
}

