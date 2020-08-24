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

#ifndef FILTERDIALOG_HPP
#define FILTERDIALOG_HPP

#include <QAbstractButton>
#include <QFileDialog>
#include <QPushButton>
#include <QMessageBox>
#include <QClipboard>
#include <QCheckBox>
#include <QDialog>
#include <QHash>
#include <QMenu>

#include "databasedriver.hpp"
#include "geometrywidget.hpp"
#include "redactionwidget.hpp"
#include "filterwidget.hpp"
#include "klhighlighter.hpp"

namespace Ui
{
	class FilterDialog;
}

class FilterDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::FilterDialog* ui;

		KLHighlighter* Highlighter;

		QActionGroup* saveMode;

		QAction* resetClass;
		QAction* resetFields;
		QAction* resetGeometry;
		QAction* resetRedaction;
		QAction* resetAdvanced;

		QHash<QString, QString> Classes;
		QHash<QString, QString> Points;
		QHash<QString, QString> Lines;
		QHash<QString, QString> Surfaces;

		QList<QSet<int>> Attributes;

		unsigned Above = 0;
		QString Limiter;

	public:

		explicit FilterDialog(QWidget* Parent = nullptr, const QStringList& Variables = QStringList(),
						  const QList<DatabaseDriver::FIELD>& Fields = QList<DatabaseDriver::FIELD>(),
						  const QList<DatabaseDriver::TABLE>& Tables = QList<DatabaseDriver::TABLE>(),
						  unsigned Common = 0, bool Singletons = false);
		virtual ~FilterDialog(void) override;

		QString getLimiterFile(void) const;
		QString getFilterRules(void) const;
		QString getAdvancedRules(void) const;

		QList<int> getUsedFields(void) const;
		QHash<int, QVariant> getGeometryRules(void) const;
		QHash<int, QVariant> getRedactionRules(void) const;
		QHash<int, QVariant> getFieldsRules(void) const;

		double getRadius(void) const;

	protected:

		QPair<QString, int> validateScript(const QString& Script) const;

	private slots:

		void classSearchEdited(const QString& Search);
		void simpleSearchEdited(const QString& Search);

		void resetButtonClicked(void);

		void limiterBoxChecked(bool Checked);

		void classBoxChecked(void);

		void filterRulesChanged(void);

		void newButtonClicked(void);
		void validateButtonClicked(void);
		void selectButtonClicked(void);
		void unselectButtonClicked(void);

		void helperIndexChanged(int Index);

		void tooltipShowRequest(QModelIndex Index);
		void variablePasteRequest(QModelIndex Index);

	public slots:

		virtual void accept(void) override;

		void setFields(const QStringList& Variables,
					const QList<DatabaseDriver::FIELD>& Fields,
					const QList<DatabaseDriver::TABLE>& Tables,
					unsigned Common = 0, bool Singletons = false);

	signals:

		void onFiltersUpdate(const QString&, const QString&, const QList<int>&,
						 const QHash<int, QVariant>&, const QHash<int, QVariant>&,
						 const QString&, double, int);

};

#endif // FILTERDIALOG_HPP
