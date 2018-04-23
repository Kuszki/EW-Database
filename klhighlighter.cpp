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

#include "klhighlighter.hpp"

const QStringList KLHighlighter::JsKeywords =
{
	"abstract", "arguments", "await", "boolean"
	"break", "byte", "case", "catch"
	"char", "class", "const", "continue"
	"debugger", "default", "delete", "do"
	"double", "else", "enum", "eval"
	"export", "extends", "false", "final"
	"finally", "float", "for", "function"
	"goto", "if", "implements", "import"
	"in", "instanceof", "int", "interface"
	"let", "long", "native", "new"
	"null", "package", "private", "protected"
	"public", "return", "short", "static"
	"super", "switch", "synchronized", "this"
	"throw", "throws", "transient", "true"
	"try", "typeof", "var", "void"
	"volatile", "while", "with", "yield"
};

const QStringList KLHighlighter::JsOperators =
{
	"\\=", "\\+", "\\-", "\\*", "\\/", "\\%",
	"\\+\\+", "\\-\\-", "\\&\\&", "\\|\\|",
	"\\!", "\\=\\=", "\\!\\=", "\\;",
	"\\>", "\\<", "\\>\\=", "\\<\\=",
	"\\.", "\\,", "\\{", "\\}",
	"\\(", "\\)", "\\[", "\\]"
};

const QStringList KLHighlighter::JsBuiltin =
{
	"Math", "Boolean", "Date", "Infinity", "NaN", "undefined",
	"E", "LN2", "LN10", "LOG2E", "LOG10E", "PI", "SQRT1_2", "SQRT2",
	"abs", "acos", "acosh", "asin", "asinh", "atan", "atan2", "atanh",
	"cbrt", "ceil", "cos", "cosh", "exp", "floor", "log", "max", "min",
	"pow", "random", "round", "sin", "sinh", "sqrt", "tan", "tanh",
	"trunc", "concat", "copyWithin", "entries", "every", "fill",
	"filter", "find", "findIndex", "forEach", "from", "includes",
	"indexOf", "isArray", "join", "keys", "lastIndexOf", "map", "pop",
	"push", "reduce", "reduceRight", "reverse", "shift", "slice", "some",
	"sort", "splice", "toString", "unshift", "valueOf", "getDate", "getDay",
	"getFullYear", "getHours", "getMilliseconds", "getMinutes", "getMonth",
	"getSeconds", "getTime", "getTimezoneOffset", "getUTCDate", "getUTCDay",
	"getUTCFullYear", "getUTCHours", "getUTCMilliseconds", "getUTCMinutes",
	"getUTCMonth", "getUTCSeconds", "getYear", "now", "parse", "setDate",
	"setFullYear", "setHours", "setMilliseconds", "setMinutes", "setMonth",
	"setSeconds", "setTime", "setUTCDate", "setUTCFullYear", "setUTCHours",
	"setUTCMilliseconds", "setUTCMinutes", "setUTCMonth", "setUTCSeconds",
	"setYear", "toDateString", "toGMTString", "toISOString", "toJSON",
	"toLocaleDateString", "toLocaleTimeString", "toLocaleString",
	"toTimeString", "toUTCString", "UTC", "decodeURI", "decodeURIComponent",
	"encodeURI", "encodeURIComponent", "escape", "eval", "isFinite", "isNaN",
	"Number", "parseFloat", "parseInt", "String", "unescape", "isInteger",
	"isSafeInteger", "toExponential", "toFixed", "toPrecision"
};

KLHighlighter::KLHighlighter(QTextDocument* Parent)
: QSyntaxHighlighter(Parent)
{
	KLHighlighterRule Rule;

	Rule.Format.setForeground(Qt::darkMagenta);
	Rule.Format.setFontWeight(QFont::Normal);

	Rule.Expresion = QRegExp("\\b[0-9]+\\b");

	Rules.insert(NUMBERS, Rule);

	Rule.Format.setForeground(Qt::darkBlue);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp(QString("\\b(?:%1)\\b").arg(JsKeywords.join('|')));

	Rules.insert(KEYWORDS, Rule);

	Rule.Format.setForeground(Qt::darkBlue);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp(QString("(?:%1)").arg(JsOperators.join('|')));

	Rules.insert(OPERATORS, Rule);

	Rule.Format.setForeground(Qt::darkCyan);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp(QString("\\b(?:%1)\\b").arg(JsBuiltin.join('|')));

	Rules.insert(MATHS, Rule);

	Rule.Format.setForeground(Qt::darkRed);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp("\"(?:\\.|(\\\\\\\")|[^\\\"\"\\n])*\"");

	Rules.insert(STRINGS, Rule);

	Rule.Format.setForeground(Qt::darkGreen);
	Rule.Format.setFontWeight(QFont::Normal);
	Rule.Format.setFontItalic(true);

	Rule.Expresion = QRegExp("\\/\\/.*");

	Rules.insert(COMMENTS, Rule);
}

KLHighlighter::~KLHighlighter(void) {}

void KLHighlighter::highlightBlock(const QString& Text)
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

void KLHighlighter::SetFormat(STYLE Style, const QTextCharFormat& Format)
{
	Rules[Style].Format = Format;
}

QTextCharFormat KLHighlighter::GetFormat(STYLE Style) const
{
	return Rules[Style].Format;
}
