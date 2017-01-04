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

#ifndef RECORDMODEL_HPP
#define RECORDMODEL_HPP

#include <QFutureSynchronizer>
#include <QAbstractItemModel>
#include <QVariant>
#include <QList>
#include <QHash>

#include <QtConcurrent>

class RecordModel : public QAbstractItemModel
{

		Q_OBJECT

	public: class RecordObject
	{

		protected:

			QList<QPair<int, QVariant>> Attributes;

		public:

			RecordObject(const QList<QPair<int, QVariant>>& Fields);
			virtual ~RecordObject(void);

			QList<QPair<int, QVariant> > getFields(void) const;

			void setFields(const QList<QPair<int, QVariant>>& Fields);
			void setField(int Role, const QVariant& Value);
			QVariant getField(int Role) const;

			bool contain(const QList<QPair<int, QVariant>>& Fields) const;

	};

	private: class SortObject
	{
		private:

			const int Index;
			const bool Mode;

		public:

			SortObject(int Column, bool Ascending = true);

			bool operator() (RecordObject* First, RecordObject* Second) const;

	};

	private: class GroupObject : public RecordObject
	{

		protected:

			QList<RecordObject*> Childs;

			GroupObject* Root;
			const int Column;

		public:

			GroupObject(int Level = -1, const QList<QPair<int, QVariant>>& Fields = QList<QPair<int, QVariant>>());
			virtual ~GroupObject(void) override;

			void addChild(RecordObject* Object);

			bool removeChild(const QList<QPair<int, QVariant>>& Fields);
			bool removeChild(RecordObject* Object);
			bool removeChild(int Index);

			RecordObject* takeChild(const QList<QPair<int, QVariant>>& Fields);
			RecordObject* takeChild(RecordObject* Object);
			RecordObject* takeChild(int Index);

			QList<RecordObject*> getChilds(void);

			GroupObject* getParent(void) const;
			RecordObject* getChild(int Index);

			QVariant getData(void) const;

			int childrenCount(void) const;
			bool hasChids(void) const;

			int getColumn(void) const;
			int getIndex(void) const;

			int childIndex(RecordObject* Object) const;

			void sortChilds(const SortObject& Functor, QFutureSynchronizer<void>& Synchronizer);

	};

	private:

		QHash<RecordObject*, GroupObject*> Parents;
		QList<QPair<QString, QString>> Header;
		QList<RecordObject*> Objects;

		GroupObject* Root = nullptr;

		QStringList Groups;

		GroupObject* createGroups(QList<QPair<int, QList<QVariant>>>::ConstIterator From,
							 QList<QPair<int, QList<QVariant>>>::ConstIterator To,
							 GroupObject* Parent = nullptr);

		GroupObject* appendItem(RecordObject* Object);

		int getIndex(const QString& Field) const;

		void removeEmpty(GroupObject* Parent = nullptr, bool Emit = true);

		void groupItems(void);

	public:

		explicit RecordModel(const QList<QPair<QString, QString>>& Head, QObject* Parent = nullptr);
		virtual ~RecordModel(void) override;

		virtual QModelIndex index(int Row, int Col, const QModelIndex& Parent = QModelIndex()) const override;
		virtual QModelIndex parent(const QModelIndex& Index) const override;

		virtual bool hasChildren(const QModelIndex& Parent) const override;

		virtual int rowCount(const QModelIndex& Parent = QModelIndex()) const override;
		virtual int columnCount(const QModelIndex& Parent = QModelIndex()) const override;

		virtual QVariant headerData(int Section, Qt::Orientation Orientation, int Role) const override;

		virtual QVariant data(const QModelIndex& Index, int Role = Qt::DisplayRole) const override;

		virtual Qt::ItemFlags flags(const QModelIndex& Index) const override;

		virtual bool setData(const QModelIndex& Index, const QVariant& Value, int Role = Qt::EditRole) override;

		virtual void sort(int Column, Qt::SortOrder Order) override;

		bool setData(const QModelIndex& Index, const QList<QPair<int, QVariant>>& Data);

		QList<QPair<int, QVariant>> fullData(const QModelIndex& Index) const;

		QVariant fieldData(const QModelIndex& Index, int Col) const;

		QModelIndexList getIndexes(const QModelIndex& Parent = QModelIndex());

		int totalCount(void) const;

	public slots:

		void groupBy(const QStringList& Groupby);

		void addItem(const QList<QPair<int, QVariant>>& Attributes);

		void addItems(const QList<QList<QPair<int, QVariant>>>& Attributes);

	signals:

		void onGroupComplete(void);


};

#endif // RECORDMODEL_HPP