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

#include "queryhlighter.hpp"

QueryHighlighter::QueryHighlighter(QTextDocument* Parent)
: QSyntaxHighlighter(Parent)
{
	KLHighlighterRule Rule;

	Rule.Format.setForeground(Qt::darkMagenta);
	Rule.Format.setFontWeight(QFont::Normal);

	Rule.Expresion = QRegExp("\\b[0-9]+\\b");

	Rules.insert(NUMBERS, Rule);

	Rule.Format.setForeground(Qt::darkRed);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp("\\b(?:AND|OR|NOT|LIKE|IN|IS|NULL)\\b");

	Rules.insert(KEYWORDS, Rule);

	Rule.Format.setForeground(Qt::darkBlue);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp("(?:\\+|\\-|\\*|\\/|\\=|\\<\\>|\\>|\\<|\\>\\=|\\<\\=|\\(|\\)|\\,|\\')");

	Rules.insert(OPERATORS, Rule);

	Rule.Format.setForeground(Qt::darkGreen);
	Rule.Format.setFontWeight(QFont::Normal);
	Rule.Format.setFontItalic(true);

	Rule.Expresion = QRegExp("(?:#|--)[^\n]*");

	Rules.insert(COMMENTS, Rule);
}

QueryHighlighter::~QueryHighlighter(void) {}

void QueryHighlighter::highlightBlock(const QString& Text)
{
	for (const auto& Rule: Rules)
	{
		int Index = Rule.Expresion.indexIn(Text);

		while (Index >= 0)
		{
			int Length = Rule.Expresion.matchedLength();
			setFormat(Index, Length, Rule.Format);
			Index = Rule.Expresion.indexIn(Text, Index + Length);
		}
	}
}

void QueryHighlighter::SetFormat(STYLE Style, const QTextCharFormat& Format)
{
	Rules[Style].Format = Format;
}

QTextCharFormat QueryHighlighter::GetFormat(STYLE Style) const
{
	return Rules[Style].Format;
}
