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

#include "geometrywidget.hpp"
#include "ui_geometrywidget.h"

const QVector<int> GeometryWidget::Numbers = { 0, 1, 2, 3 };
const QVector<int> GeometryWidget::Points = { 6, 7, 8, 9 };
const QVector<int> GeometryWidget::Classes = { 4, 5, 10, 11, 12, 13, 14, 15 };
const QVector<int> GeometryWidget::Geometries = { 16, 17, 18, 19 };

GeometryWidget::GeometryWidget(const QHash<QString, QString>& Classes, const QHash<QString, QString>& Points, QWidget* Parent)
: QWidget(Parent), ui(new Ui::GeometryWidget)
{
	ui->setupUi(this); setParameters(Classes, Points); typeChanged(ui->typeCombo->currentIndex());

	ui->removeButton->setFixedSize(ui->typeCombo->sizeHint().height(),
							 ui->typeCombo->sizeHint().height());
}

GeometryWidget::~GeometryWidget(void)
{
	delete ui;
}

QPair<int, QVariant> GeometryWidget::getCondition(void) const
{
	static const QHash<int, int> Map = { { 16, 4 }, { 17, 2 }, { 18, 3 }, { 19, 100 } };

	const int Index = ui->typeCombo->currentIndex();

	if (Numbers.contains(Index)) return qMakePair(Index, ui->sizeSpin->value());
	if (Points.contains(Index)) return qMakePair(Index, ui->pointCombo->currentData());
	if (Classes.contains(Index)) return qMakePair(Index, ui->classCombo->currentData());

	if (Geometries.contains(Index)) return qMakePair(100, Map.value(Index));

	return qMakePair(Index, QVariant());
}

void GeometryWidget::typeChanged(int Index)
{
	ui->sizeSpin->setVisible(Numbers.contains(Index));
	ui->pointCombo->setVisible(Points.contains(Index));
	ui->classCombo->setVisible(Classes.contains(Index));
}

void GeometryWidget::editFinished(void)
{
	emit onValueUpdate(getCondition());
}

void GeometryWidget::setParameters(const QHash<QString, QString>& Classes, const QHash<QString, QString>& Points)
{
	ui->classCombo->clear(); ui->pointCombo->clear();

	for (auto i = Classes.constBegin(); i != Classes.constEnd(); ++i)
	{
		ui->classCombo->addItem(i.value(), i.key());
	}

	for (auto i = Points.constBegin(); i != Points.constEnd(); ++i)
	{
		ui->pointCombo->addItem(i.value(), i.key());
	}

	ui->classCombo->model()->sort(0);
	ui->pointCombo->model()->sort(0);

	ui->classCombo->insertItem(0, tr("Any object"), "*");
	ui->pointCombo->insertItem(0, tr("Any object"), "*");

	ui->classCombo->setCurrentIndex(0);
	ui->pointCombo->setCurrentIndex(0);
}

