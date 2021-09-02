/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  KLScript code highlighter for KLLibs                                   *
 *  Copyright (C) 2015  Łukasz "Kuszki" Dróżdż  l.drozdz@o2.pl             *
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

#ifndef SQLHIGHLIGHTER_HPP
#define SQLHIGHLIGHTER_HPP

#include <QtWidgets>
#include <QtCore>

/*! \file		klhighlighter.hpp
 *  \brief	Deklaracje dla klasy SqlHighlighter i jej składników.
 *
 */

/*! \file		klhighlighter.cpp
 *  \brief	Implementacja klasy SqlHighlighter i jej składników.
 *
 */

/*! \brief	Mechanizm wyróżniający elementy składni.
 *
 * Implementacja subklasy `QSyntaxHighlighter` umożliwiająca wyróżnianie składni `KLScript` w dokumentach Qt.
 *
 */
class SqlHighlighter : public QSyntaxHighlighter
{

		Q_OBJECT

	/*! \brief		Wyliczenie styli wyróżnienia.
	 *
	 * Pozwala ograniczyć pule styli do wybranych pozycji.
	 *
	 */
	public: enum STYLE
	{
		NUMBERS,		//!< Liczby.
		KEYWORDS,		//!< Słowa kluczowe.
		OPERATORS,	//!< Operatory matematyczne.
		STRINGS,		//!< Słowa kluczowe kończące program.
		MATHS,		//!< Funkcje matematyczne.
		COMMENTS		//!< Komentarze.
	};

	protected:

		/*! \brief		Reprezentacja wyróżnienia.
		 *
		 * Definiuje zasady wyróżnienia i jego styl.
		 *
		 */
		struct SqlHighlighterRule
		{
			QTextCharFormat Format;	//!< Styl tekstu.
			QRegExp Expresion;		//!< Wyrażenie reguralne.
		};

		QMap<STYLE, SqlHighlighterRule> Rules;	//!< Kontener na wyróżnienia.

		virtual void highlightBlock(const QString& Text) override;

	public:

		static const QStringList SqlKeywords;
		static const QStringList SqlOperators;
		static const QStringList SqlBuiltin;

		/*! \brief		Konstruktor domyślny.
		 *  \param [in]	Parent Dokument roboczy.
		 *
		 * Inicjuje pole rodzica i tworzy wrzystkie domyślne reguły wyróżnień.
		 *
		 */
		SqlHighlighter(QTextDocument* Parent);

		/*! \brief		Destruktor.
		 *
		 * Zwalnia wszystkie użyte zasoby.
		 *
		 */
		virtual ~SqlHighlighter(void) override;

		/*! \brief		Ustala styl.
		 *  \param [in]	Style	Indeks stylu.
		 *  \param [in]	Format	Format stylu.
		 *
		 * Ustala nowy styl dla wybranego fragmentu kodu.
		 *
		 */
		void SetFormat(STYLE Style, const QTextCharFormat& Format);

		/*! \brief		Pobiera styl.
		 *  \param [in]	Style Indeks stylu.
		 *  \return		Styl wybranego fragmentu.
		 *
		 * Pobiera styl wybranego fragmentu kodu.
		 *
		 */
		QTextCharFormat GetFormat(STYLE Style) const;

		static QStandardItemModel* getSqlHelperModel(QObject* Parent,
					const QVariantList& Variables = QVariantList());

};

#endif // SQLHIGHLIGHTER_HPP
