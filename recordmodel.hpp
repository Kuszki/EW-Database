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

#include <QAbstractItemModel>
#include <QVariant>
#include <QList>
#include <QMap>

class RecordModel : public QAbstractItemModel
{

		Q_OBJECT

	public: class RecordObject
	{

		protected:

			QMap<int, QVariant> Attributes;

		public:

			RecordObject(const QMap<int, QVariant>& Fields);
			virtual ~RecordObject(void);

			QMap<int, QVariant> getFields(void) const;

			void setFields(const QMap<int, QVariant>& Fields);
			void setField(int Role, const QVariant& Value);
			QVariant getField(int Role) const;

			bool contain(const QMap<int, QVariant>& Fields) const;

	};

	private: class GroupObject : public RecordObject
	{

		protected:

			QList<RecordObject*> Childs;

			GroupObject* Root;
			const int Column;

		public:

			GroupObject(int Level = -1, const QMap<int, QVariant>& Fields = QMap<int, QVariant>());
			virtual ~GroupObject(void) override;

			void addChild(RecordObject* Object);

			bool removeChild(const QMap<int, QVariant>& Fields);
			bool removeChild(RecordObject* Object);
			bool removeChild(int Index);

			RecordObject* takeChild(const QMap<int, QVariant>& Fields);
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

	};

	private:

		QMap<RecordObject*, GroupObject*> Parents;
		QVector<RecordObject*> Objects;
		QMap<QString, QString> Header;

		GroupObject* Root = nullptr;

		QStringList Groups;

		GroupObject* createGroups(QMap<int, QList<QVariant>>::ConstIterator From,
							 QMap<int, QList<QVariant>>::ConstIterator To,
							 GroupObject* Parent = nullptr);

		GroupObject* appendItem(RecordObject* Object);

		int removeEmpty(GroupObject* Parent = nullptr);

		void groupItems(void);

	public:

		explicit RecordModel(const QMap<QString, QString>& Head,
						 QObject* Parent = nullptr,
						 const QStringList& Groupby = QStringList());
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

		bool setData(const QModelIndex& Index, const QMap<int, QVariant>& Data);

		QMap<int, QVariant> fullData(const QModelIndex& Index) const;

		QVariant fieldData(const QModelIndex& Index, int Col) const;

		void groupBy(const QStringList& Groupby);

		void addItem(const QMap<int, QVariant>& Attributes);

		void addItems(const QList<QMap<int, QVariant>>& Attributes);

		void setItems(const QList<QMap<int, QVariant>>& Attributes);


};

#endif // RECORDMODEL_HPP
