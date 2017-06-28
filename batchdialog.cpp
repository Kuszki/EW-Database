#include "batchdialog.hpp"
#include "ui_batchdialog.h"

BatchDialog::BatchDialog(const QStringList& Fields, const QStringList& First, QWidget* Parent)
: QDialog(Parent), ui(new Ui::BatchDialog)
{
	ui->setupUi(this); setParameters(Fields, First);

	ui->fieldsLayout->setAlignment(Qt::AlignTop);
}

BatchDialog::~BatchDialog(void)
{
	delete ui;
}

QMap<int, BatchWidget::FUNCTION> BatchDialog::getFunctions(void) const
{
	QList<BatchWidget::FUNCTION> List;

	for (int i = 0; i < ui->fieldsLayout->count(); ++i)
		if (auto W = qobject_cast<BatchWidget*>(ui->fieldsLayout->itemAt(i)->widget()))
			List.insert(W->getField(), W->getFunction());
}

void BatchDialog::accept(void)
{
	emit onBatchRequest(getFunctions()); QDialog::accept();
}

void BatchDialog::setParameters(const QStringList& Fields, const QStringList& First)
{
	while (auto I = ui->fieldsLayout->takeAt(0)) if (auto W = I->widget()) W->deleteLater();

	for (int i = 0; i < First.size(); ++i)
	{
		auto Widget = new BatchWidget(i, First[i], Fields, this);

		ui->fieldsLayout->addWidget(Widget);
	}
}
