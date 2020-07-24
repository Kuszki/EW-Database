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

#include "selectordialog.hpp"
#include "ui_selectordialog.h"
#include <QDebug>
SelectorDialog::SelectorDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::SelectorDialog)
{
	ui->setupUi(this);

	QMenu* saveMenu = new QMenu(this);

	QAction* saveNew = new QAction(tr("Load new objects list"), this);
	QAction* saveCur = new QAction(tr("Filter current selection"), this);
	QAction* saveAdd = new QAction(tr("Append to current selection"), this);
	QAction* saveSub = new QAction(tr("Subtract from current selection"), this);

	saveMode = new QActionGroup(this);

	saveNew->setCheckable(true); saveNew->setData(0);
	saveCur->setCheckable(true); saveCur->setData(1);
	saveAdd->setCheckable(true); saveAdd->setData(2);
	saveSub->setCheckable(true); saveSub->setData(3);

	saveMenu->addAction(saveNew); saveMode->addAction(saveNew);
	saveMenu->addAction(saveCur); saveMode->addAction(saveCur);
	saveMenu->addAction(saveAdd); saveMode->addAction(saveAdd);
	saveMenu->addAction(saveSub); saveMode->addAction(saveSub);

	saveMode->setExclusive(true); saveNew->setChecked(true);

	Save = new QToolButton(this);

	Save->setText(tr("Apply"));
	Save->setPopupMode(QToolButton::MenuButtonPopup);
	Save->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	Save->setMenu(saveMenu);
	Save->setEnabled(false);

	ui->buttonBox->addButton(Save, QDialogButtonBox::YesRole);

	ui->openButton->setFixedSize(ui->sourceEdit->sizeHint().height(),
						    ui->sourceEdit->sizeHint().height());
	ui->refreshButton->setFixedSize(ui->sourceEdit->sizeHint().height(),
							  ui->sourceEdit->sizeHint().height());
}

SelectorDialog::~SelectorDialog(void)
{
	delete ui;
}

void SelectorDialog::actionIndexChanged(int Index)
{
	ui->sourceEdit->setEnabled(Index == 0);
	ui->openButton->setEnabled(Index == 0);

	ui->listEdit->setReadOnly(Index != 2);

	if (Index != 2) refreshButtonClicked();
}

void SelectorDialog::openButtonClicked(void)
{
	const QString Path = QFileDialog::getOpenFileName(this, tr("Open list file"), QString(),
											tr("Text files (*.txt);;All files (*.*)"));

	if (!Path.isEmpty()) ui->sourceEdit->setText(Path);

	if (QFile::exists(Path)) refreshButtonClicked();
}

void SelectorDialog::refreshButtonClicked(void)
{
	const int Index = ui->actionBox->currentIndex();
	const QRegExp Spliter("\\s+.*");

	if (Index != 2) ui->listEdit->clear();

	if (Index == 0 && !ui->sourceEdit->text().isEmpty())
	{
		QFile File(ui->sourceEdit->text()); QTextStream Stream(&File);

		if (File.open(QFile::ReadOnly | QFile::Text)) while (!Stream.atEnd())
		{
			ui->listEdit->appendPlainText(Stream.readLine().trimmed().remove(Spliter));
		}
	}
	else if (Index == 1)
	{
		const QClipboard* Cp = QApplication::clipboard();

		if (Cp->mimeData()->hasText())
		{
			ui->listEdit->setPlainText(Cp->text().remove('\r').trimmed());
		}
	}
}

void SelectorDialog::listEditChanged(void)
{
	Save->setEnabled(!ui->listEdit->document()->toPlainText().trimmed().isEmpty());
}

void SelectorDialog::accept(void)
{
	const QStringList List = ui->listEdit->document()->toPlainText().split('\n', QString::SkipEmptyParts);

	QDialog::accept(); emit onDataAccepted(List, ui->comboBox->currentIndex(), saveMode->checkedAction()->data().toInt());
}

void SelectorDialog::open(void)
{
	refreshButtonClicked(); QDialog::open();
}
