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

#include "queryeditor.hpp"

QueryEditor::QueryNumberarea::QueryNumberarea(QueryEditor* Editor)
: QWidget(Editor) {}

void QueryEditor::QueryNumberarea::paintEvent(QPaintEvent* Event)
{
	auto Parent = qobject_cast<QueryEditor*>(parent());

	Parent->numberareaPaintEvent(Event);
}

QSize QueryEditor::QueryNumberarea::sizeHint(void) const
{
	auto Parent = qobject_cast<QueryEditor*>(parent());

	return QSize(Parent->numberareaWidth(), 0);
}

QueryEditor::QueryEditor(QWidget* Parent)
: QPlainTextEdit(Parent)
{
	SyntaxHighlighter = new QueryHighlighter(this->document());
	NumberArea = new QueryNumberarea(this);

	connect(this, &QueryEditor::blockCountChanged,
		   this, &QueryEditor::updateNumberareaWidth);

	connect(this, &QueryEditor::updateRequest,
		   this, &QueryEditor::updateNumberareaView);

	connect(this, &QueryEditor::cursorPositionChanged,
		   this, &QueryEditor::highlightCurrentLine);

	updateNumberareaWidth(0);
	highlightCurrentLine();

	setFont(QFont("monospace"));
	setTabStopWidth(25);
}

QueryEditor::~QueryEditor(void) {}

int QueryEditor::numberareaWidth(void) const
{
	int digits = 1;
	int max = qMax(1, blockCount());

	while (max >= 10)
	{
		max /= 10;
		++digits;
	}

	int space = 6 + fontMetrics().width(QLatin1Char('9')) * digits;

	return space;
}

QueryHighlighter& QueryEditor::Highlighter(void)
{
	return *SyntaxHighlighter;
}

void QueryEditor::numberareaPaintEvent(QPaintEvent* Event)
{
	QTextBlock block = firstVisibleBlock();
	QPainter painter(NumberArea);

	painter.fillRect(Event->rect(), QColor(235, 235, 235));
	painter.setPen(Qt::black);

	int blockNumber = block.blockNumber();
	int top = blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + blockBoundingRect(block).height();

	while (block.isValid() && top <= Event->rect().bottom())
	{
		if (block.isVisible() && bottom >= Event->rect().top())
		{
			painter.drawText(0,
						  top,
						  NumberArea->width() - 3,
						  fontMetrics().height(),
						  Qt::AlignRight,
						  QString::number(blockNumber + 1));
		}

		block = block.next();
		top = bottom;
		bottom = top + blockBoundingRect(block).height();

		++blockNumber;
	}
}

void QueryEditor::updateNumberareaWidth(int)
{
	setViewportMargins(numberareaWidth(), 0, 0, 0);
}

void QueryEditor::updateNumberareaView(const QRect& Rect, int dY)
{
	if (dY)
		NumberArea->scroll(0, dY);
	else
		NumberArea->update(0,
					    Rect.y(),
					    NumberArea->width(),
					    Rect.height());

	if (Rect.contains(viewport()->rect())) updateNumberareaWidth(0);
}

void QueryEditor::resizeEvent(QResizeEvent* Event)
{
	QPlainTextEdit::resizeEvent(Event);

	QRect cr = contentsRect();

	NumberArea->setGeometry(QRect(cr.left(),
							cr.top(),
							numberareaWidth(),
							cr.height()));
}

void QueryEditor::highlightCurrentLine(void)
{
	QList<QTextEdit::ExtraSelection> extraSelections;

	if (!isReadOnly())
	{
		QTextEdit::ExtraSelection selection;

		selection.format.setBackground(QColor(255, 255, 205));
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = textCursor();
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}

	setExtraSelections(extraSelections);
}
