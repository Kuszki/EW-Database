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

#ifndef KLSCRIPTEDITOR_HPP
#define KLSCRIPTEDITOR_HPP

#include <QPlainTextEdit>
#include <QTextBlock>
#include <QPainter>
#include <QObject>

#include "queryhlighter.hpp"

class QueryEditor : public QPlainTextEdit
{

		Q_OBJECT

	protected: class QueryNumberarea final : public QWidget
	{

		protected:

			virtual void paintEvent(QPaintEvent* Event) override;

		public:

			explicit QueryNumberarea(QueryEditor* Editor);

			QSize sizeHint(void) const override;

	};

	protected:

		QueryNumberarea* NumberArea;

		QueryHighlighter* SyntaxHighlighter;

		virtual void resizeEvent(QResizeEvent *Event) override;

	public:

		explicit QueryEditor(QWidget* Parent = nullptr);

		virtual ~QueryEditor(void) override;

		int numberareaWidth(void) const;

		QueryHighlighter& Highlighter(void);

	private slots:

		void numberareaPaintEvent(QPaintEvent *event);

		void updateNumberareaWidth(int newBlockCount);
		void updateNumberareaView(const QRect& Rect, int dY);

		void highlightCurrentLine(void);

};

#endif // KLSCRIPTEDITOR_HPP
