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

#include "sqlhighlighter.hpp"

const QStringList SqlHighlighter::SqlKeywords =
{
	"ADD", "ALL", "ALTER", "AND", "ANY", "AS", "ASC",
	"BACKUP", "BETWEEN", "BY", "CASE", "CHECK", "COLUMN",
	"CONSTRAINT", "CREATE", "DATABASE", "DEFAULT", "DELETE",
	"DESC", "DISTINCT", "DROP", "EXEC", "EXISTS", "FOREIGN",
	"FROM", "FULL", "GROUP", "HAVING", "IN", "INDEX", "INNER",
	"INSERT", "INTO", "IS", "JOIN", "KEY", "LEFT", "LIKE",
	"LIMIT", "NOT", "NULL", "OR", "ORDER", "OUTER", "PRIMARY",
	"PROCEDURE", "REPLACE", "RIGHT", "ROWNUM", "SELECT",
	"SET", "TABLE", "TOP", "TRUNCATE", "UNION", "UNIQUE",
	"UPDATE", "VALUES", "VIEW", "WHERE", "ON"

};

const QStringList SqlHighlighter::SqlOperators =
{
	"\\=", "\\+", "\\-", "\\*", "\\/", "\\%",
	"\\&", "\\|", "\\^", "\\<\\>",
	"\\>", "\\<", "\\>\\=", "\\<\\=",
	"\\.", "\\,", "\\(", "\\)",
};

const QStringList SqlHighlighter::SqlBuiltin =
{
	"ASCII", "CHAR", "CHARINDEX", "CONCAT", "CONCAT_WS", "DATALENGTH",
	"DIFFERENCE", "FORMAT", "LEFT", "LEN", "LOWER", "LTRIM", "NCHAR",
	"PATINDEX", "QUOTENAME", "REPLACE", "REPLICATE", "REVERSE", "RIGHT",
	"RTRIM", "SOUNDEX", "SPACE", "STR", "STUFF", "SUBSTRING", "TRANSLATE",
	"TRIM", "UNICODE", "UPPER", "ABS", "ACOS", "ASIN", "ATAN", "ATN2",
	"AVG", "CEILING", "COUNT", "COS", "COT", "DEGREES", "EXP", "FLOOR",
	"LOG", "LOG10", "MAX", "MIN", "PI", "POWER", "RADIANS", "RAND",
	"SIGN", "SIN", "SQRT", "SQUARE", "SUM", "TAN", "CURRENT_TIMESTAMP",
	"DATEADD", "DATEDIFF", "DATEFROMPARTS", "DATENAME", "DATEPART",
	"DAY", "GETDATE", "GETUTCDATE", "ISDATE", "MONTH", "SYSDATETIME",
	"YEAR", "CAST", "COALESCE", "CONVERT", "CURRENT_USER", "IIF",
	"ISNULL", "ISNUMERIC", "NULLIF", "SESSION_USER", "SESSIONPROPERTY",
	"SYSTEM_USER", "USER_NAME", "ROUND"
};

SqlHighlighter::SqlHighlighter(QTextDocument* Parent)
: QSyntaxHighlighter(Parent)
{
	SqlHighlighterRule Rule;

	Rule.Format.setForeground(Qt::darkMagenta);
	Rule.Format.setFontWeight(QFont::Normal);

	Rule.Expresion = QRegExp("\\b[0-9]+\\b");

	Rules.insert(NUMBERS, Rule);

	Rule.Format.setForeground(Qt::darkBlue);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp(QString("\\b(?:%1)\\b").arg(SqlKeywords.join('|')), Qt::CaseInsensitive);

	Rules.insert(KEYWORDS, Rule);

	Rule.Format.setForeground(Qt::darkBlue);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp(QString("(?:%1)").arg(SqlOperators.join('|')), Qt::CaseInsensitive);

	Rules.insert(OPERATORS, Rule);

	Rule.Format.setForeground(Qt::darkCyan);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp(QString("\\b(?:%1)\\b").arg(SqlBuiltin.join('|')), Qt::CaseInsensitive);

	Rules.insert(MATHS, Rule);

	Rule.Format.setForeground(Qt::darkRed);
	Rule.Format.setFontWeight(QFont::Bold);

	Rule.Expresion = QRegExp("\'(?:\\.|(\\\\\\\')|[^\\\'\'\\n])*\'");

	Rules.insert(STRINGS, Rule);

	Rule.Format.setForeground(Qt::darkGreen);
	Rule.Format.setFontWeight(QFont::Normal);
	Rule.Format.setFontItalic(true);

	Rule.Expresion = QRegExp("--.*");

	Rules.insert(COMMENTS, Rule);
}

SqlHighlighter::~SqlHighlighter(void) {}

void SqlHighlighter::highlightBlock(const QString& Text)
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

void SqlHighlighter::SetFormat(STYLE Style, const QTextCharFormat& Format)
{
	Rules[Style].Format = Format;
}

QTextCharFormat SqlHighlighter::GetFormat(STYLE Style) const
{
	return Rules[Style].Format;
}

QStandardItemModel* SqlHighlighter::getSqlHelperModel(QObject* Parent, const QVariantList& Variables)
{
	static const QStringList Res =
	{
		":/sql/operators",
		":/sql/keywords",
		":/sql/strings",
		":/sql/numeric",
		":/sql/datetime",
		":/sql/advanced",
	};

	QStringList Groups =
	{
		tr("Variables"),
		tr("Operators"),
		tr("Keywords"),
		tr("String functions"),
		tr("Numeric functions"),
		tr("Datetime functions"),
		tr("Advanced functions")
	};

	QStandardItemModel* Model = new QStandardItemModel(Parent);
	QStandardItem* Root = Model->invisibleRootItem();

	if (Variables.isEmpty()) Groups.removeFirst();

	for (const auto& Group : Groups)
	{
		Root->appendRow(new QStandardItem(Group));
	}

	for (const auto& Var : Variables)
	{
		Root->child(0)->appendRow(new QStandardItem(Var.toMap().value("name").toString()));
	}

	for (int i = 0; i < Res.size(); ++i)
	{
		QStandardItem* Up = Root->child(i + !Variables.isEmpty());

		QFile File(Res[i]); File.open(QFile::ReadOnly | QFile::Text);

		while (!File.atEnd())
		{
			QStringList Row = QString::fromUtf8(File.readLine().trimmed()).split('\t');

			if (Row.size() == 2)
			{
				QStandardItem* Item = new QStandardItem();

				Item->setData(Row.first(), Qt::DisplayRole);
				Item->setData(Row.last(), Qt::ToolTipRole);

				Up->appendRow(Item);
			}
		}
	}

	return Model;
}
