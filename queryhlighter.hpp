/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  KLScript code highlighter for KLLibs                                   *
 *  Copyright (C) 2015  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
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

#ifndef KLHIGHLIGHTER_HPP
#define KLHIGHLIGHTER_HPP

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QueryHighlighter : public QSyntaxHighlighter
{

		Q_OBJECT

	public: enum STYLE
	{
		NUMBERS,
		KEYWORDS,
		OPERATORS,
		BREAKS,
		MATHS,
		COMMENTS
	};

	protected:

		struct KLHighlighterRule
		{
			QTextCharFormat Format;
			QRegExp Expresion;
		};

		QHash<STYLE, KLHighlighterRule> Rules;

		virtual void highlightBlock(const QString& Text) override;

	public:


		QueryHighlighter(QTextDocument* Parent);


		virtual ~QueryHighlighter(void) override;


		void SetFormat(STYLE Style, const QTextCharFormat& Format);

		QTextCharFormat GetFormat(STYLE Style) const;

};

#endif // KLHIGHLIGHTER_HPP
