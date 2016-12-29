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

#include "recordmodel.hpp"

RecordModel::RecordObject::RecordObject(const QMap<int, QVariant>& Fields)
: Attributes(Fields) {}

RecordModel::RecordObject::~RecordObject(void) {}

QMap<int, QVariant> RecordModel::RecordObject::getFields(void) const
{
	return Attributes;
}

void RecordModel::RecordObject::setFields(const QMap<int, QVariant>& Fields)
{
	Attributes = Fields;
}

void RecordModel::RecordObject::setField(int Role, const QVariant& Value)
{
	Attributes[Role] = Value;
}

QVariant RecordModel::RecordObject::getField(int Role) const
{
	return Attributes.value(Role);
}

bool RecordModel::RecordObject::contain(const QMap<int, QVariant>& Fields) const
{
	for (auto i = Fields.constBegin(); i != Fields.constEnd(); ++i)
	{
		if (Attributes.value(i.key()) != i.value()) return false;
	}

	return true;
}

RecordModel::GroupObject::GroupObject(int Level, const QMap<int, QVariant>& Fields)
: RecordObject(Fields), Root(nullptr), Column(Level) {}

RecordModel::GroupObject::~GroupObject(void)
{
	for (auto Child : Childs) delete dynamic_cast<GroupObject*>(Child);
}

void RecordModel::GroupObject::addChild(RecordModel::RecordObject* Object)
{
	if (auto Group = dynamic_cast<GroupObject*>(Object))
	{
		Group->Root = this;
	}

	Childs.append(Object);
}

bool RecordModel::GroupObject::removeChild(const QMap<int, QVariant>& Fields)
{
	for (auto& Child : Childs) if (Child->contain(Fields))
	{
		delete Child; Child = nullptr;
	}

	return Childs.removeAll(nullptr);
}

bool RecordModel::GroupObject::removeChild(RecordModel::RecordObject* Object)
{
	if (Childs.removeOne(Object))
	{
		delete Object;
		return true;
	}
	else return false;
}

bool RecordModel::GroupObject::removeChild(int Index)
{
	if (Childs.size() > Index)
	{
		delete Childs.takeAt(Index);
		return true;
	}
	else return false;
}

RecordModel::RecordObject* RecordModel::GroupObject::takeChild(const QMap<int, QVariant>& Fields)
{
	for (auto& Child : Childs) if (Child->contain(Fields)) return Child;
}

RecordModel::RecordObject* RecordModel::GroupObject::takeChild(RecordModel::RecordObject* Object)
{
	if (Childs.contains(Object))
	{
		Childs.removeOne(Object);

		return Object;
	}
	else return nullptr;
}

RecordModel::RecordObject* RecordModel::GroupObject::takeChild(int Index)
{
	if (Childs.size() > Index) return Childs.takeAt(Index);
}

QList<RecordModel::RecordObject*> RecordModel::GroupObject::getChilds(void)
{
	return Childs;
}

RecordModel::GroupObject* RecordModel::GroupObject::getParent(void) const
{
	return Root;
}

RecordModel::RecordObject* RecordModel::GroupObject::getChild(int Index)
{
	return Childs.value(Index);
}

QVariant RecordModel::GroupObject::getData(void) const
{
	return Attributes[Column];
}

int RecordModel::GroupObject::childrenCount(void) const
{
	return Childs.size();
}

bool RecordModel::GroupObject::hasChids(void) const
{
	return !Childs.isEmpty();
}

int RecordModel::GroupObject::getColumn(void) const
{
	return Column;
}

int RecordModel::GroupObject::getIndex(void) const
{
	if (Root) return Root->Childs.indexOf((GroupObject*) this); else	return 0;
}

int RecordModel::GroupObject::childIndex(RecordObject* Object) const
{
	return Childs.indexOf(Object);
}

RecordModel::GroupObject* RecordModel::createGroups(QMap<int, QList<QVariant>>::ConstIterator From, QMap<int, QList<QVariant>>::ConstIterator To, RecordModel::GroupObject* Parent)
{
	if (!Parent) Parent = new GroupObject();

	for (const auto& Field : From.value())
	{
		GroupObject* Child = new GroupObject(From.key(), Parent->getFields());

		Child->setField(From.key(), Field);
		Parent->addChild(Child);

		if (From + 1 != To) createGroups(From + 1, To, Child);
		else
		{
			const auto Fields = Child->getFields();

			for (auto Object : Objects) if (Object->contain(Fields))
			{
				Parents.insert(Object, Child);
				Child->addChild(Object);
			}
		}
	}

	return Parent;
}

RecordModel::GroupObject* RecordModel::appendItem(RecordModel::RecordObject* Object)
{
	if (!Root) return nullptr;

	GroupObject* Current = Root;
	GroupObject* Result = nullptr;

	for (int i = 0; i < Groups.size(); ++i)
	{
		for (auto Group : Current->getChilds())
		{
			auto G = dynamic_cast<GroupObject*>(Group);
			if (G->getData() == Object->getField(G->getColumn()))
			{
				Result = G;
			}
		}

		if (!Result)
		{
			auto Index = createIndex(Current->getIndex(), 0, Current);
			int Column = Header.keys().indexOf(Groups[i]);
			const int Count = Current->childrenCount() + 1;

			beginInsertRows(Current == Root ? QModelIndex() : Index, Count, Count);

			Result = new GroupObject(Column, Current->getFields());
			Result->setField(Column, Object->getField(Column));
			Current->addChild(Result);

			endInsertRows();
		}

		Current = Result;
		Result = nullptr;
	}

	auto Index = createIndex(Current->getIndex(), 0, Current);
	const int Count = Current->childrenCount() + 1;

	beginInsertRows(Index, Count, Count);

	Parents.insert(Object, Current);
	Objects.append(Object);
	Current->addChild(Object);

	endInsertRows();

	return Current;
}

void RecordModel::removeEmpty(RecordModel::GroupObject* Parent)
{
	if (Parent)
	{
		if (Parent->hasChids()) for (auto Child : Parent->getChilds())
		{
			if (GroupObject* Group = dynamic_cast<GroupObject*>(Child))
			{
				removeEmpty(Group);
			}
		}
		else if (auto P = Parent->getParent())
		{
			auto From = createIndex(P->getIndex(), 0, P);
			int Row = Parent->getIndex();

			beginRemoveRows(From, Row, Row);
			P->removeChild(Parent);
			endRemoveRows();

			removeEmpty(P->getParent());
		}
	}
}

void RecordModel::groupItems(void)
{
	const QList<QString> Keys = Header.keys();
	QMap<int, QList<QVariant>> Indexes;

	if (Root) delete Root; Parents.clear();

	for (const auto& Group : Groups)
	{
		const int Index = Keys.indexOf(Group);
		if (Index == -1) continue;

		Indexes.insert(Index, QList<QVariant>());

		for (const auto& Object : Objects)
		{
			QVariant Value = Object->getField(Index);

			if (!Indexes[Index].contains(Value))
			{
				Indexes[Index].append(Value);
			}
		}
	}

	Root = createGroups(Indexes.constBegin(), Indexes.constEnd(), Root);
}

RecordModel::RecordModel(const QMap<QString, QString>& Head, QObject* Parent, const QStringList& Groupby)
: QAbstractItemModel(Parent), Header(Head), Groups(Groupby)
{
	if (Groups.size()) groupItems();
}

RecordModel::~RecordModel(void)
{
	for (const auto& Object : Objects) delete Object;
}

QModelIndex RecordModel::index(int Row, int Col, const QModelIndex& Parent) const
{
	if (!hasIndex(Row, Col, Parent)) return QModelIndex();
	if (!Root) return createIndex(Row, Col, Objects[Row]);

	GroupObject* parentObject = Parent.isValid() ? (GroupObject*) Parent.internalPointer() : Root;

	if (auto Child = parentObject->getChild(Row))
	{
		return createIndex(Row, Col, Child);
	}
	else return QModelIndex();
}

QModelIndex RecordModel::parent(const QModelIndex& Index) const
{
	if (!Index.isValid()) return QModelIndex();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (auto Group = dynamic_cast<GroupObject*>(Object))
	{
		if (auto Parent = Group->getParent())
		{
			return createIndex(Parent->getIndex(), 0, Parent);
		}
		else return QModelIndex();
	}
	else
	{
		GroupObject* Parent = Parents.value(Object);

		if (Parent)
		{
			return createIndex(Parent->getIndex(), 0, Parent);
		}
		else return QModelIndex();
	}
}

bool RecordModel::hasChildren(const QModelIndex& Parent) const
{
	if (Parent == QModelIndex()) return true;

	RecordObject* Object = (RecordObject*) Parent.internalPointer();

	if (auto Group = dynamic_cast<GroupObject*>(Object))
	{
		return Group->hasChids();
	}
	else return false;
}

int RecordModel::rowCount(const QModelIndex& Parent) const
{
	if (!Parent.isValid())
	{
		if (Root) return Root->childrenCount();
		else return Objects.size();
	}

	RecordObject* Object = (RecordObject*) Parent.internalPointer();

	if (auto Group = dynamic_cast<GroupObject*>(Object))
	{
		return Group->childrenCount();
	}
	else return 0;
}

int RecordModel::columnCount(const QModelIndex &Parent) const
{
	return Header.size();
}

QVariant RecordModel::headerData(int Section, Qt::Orientation Orientation, int Role) const
{
	if (Orientation != Qt::Horizontal) return QVariant();

	if (Role == Qt::DisplayRole) return Header.values().value(Section);
	else if (Role == Qt::UserRole) return Header.keys().value(Section);

	return QVariant();
}

QVariant RecordModel::data(const QModelIndex &Index, int Role) const
{
	if (!Index.isValid() || !(Role == Qt::DisplayRole || Role == Qt::EditRole)) return QVariant();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	return Object->getField(Index.column());
}

Qt::ItemFlags RecordModel::flags(const QModelIndex& Index) const
{
	if (!Index.isValid()) return Qt::ItemFlags(0);

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return Qt::ItemIsEnabled;
	else return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool RecordModel::setData(const QModelIndex& Index, const QVariant& Value, int Role)
{
	if (!Index.isValid() || !(Role == Qt::DisplayRole || Role == Qt::EditRole)) return false;

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return false;

	Object->setField(Index.column(), Value);

	if (Root && !Object->contain(Parents[Object]->getFields()))
	{
		auto Parent = Parents[Object];
		auto From = createIndex(Parent->getIndex(), 0, Parent);
		int Row = Parent->childIndex(Object);

		beginRemoveRows(From, Row, Row);

		Parent->takeChild(Object);
		Parents.remove(Object);
		Objects.removeOne(Object);

		endRemoveRows();

		removeEmpty(Parent->getParent());
		appendItem(Object);
	}
	else	emit dataChanged(Index, Index);
}

bool RecordModel::setData(const QModelIndex& Index, const QMap<int, QVariant>& Data)
{
	if (!Index.isValid()) return false;

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return false;

	Object->setFields(Data);

	if (Root && !Object->contain(Parents[Object]->getFields()))
	{
		auto Parent = Parents[Object];
		auto From = createIndex(Parent->getIndex(), 0, Parent);
		int Row = Parent->childIndex(Object);

		beginRemoveRows(From, Row, Row);

		Parent->takeChild(Object);
		Parents.remove(Object);
		Objects.removeOne(Object);

		endRemoveRows();

		removeEmpty(Parent->getParent());
		appendItem(Object);
	}
	else	emit dataChanged(Index, Index);
}

QMap<int, QVariant> RecordModel::fullData(const QModelIndex& Index) const
{
	if (!Index.isValid()) return QMap<int, QVariant>();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	return Object->getFields();
}

QVariant RecordModel::fieldData(const QModelIndex& Index, int Col) const
{
	if (!Index.isValid()) return QVariant();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	return Object->getField(Col);
}

void RecordModel::groupBy(const QStringList& Groupby)
{
	if (Groups == Groupby)
	{
		emit onGroupComplete(); return;
	}
	else Groups = Groupby;

	beginResetModel();

	Parents.clear();
	delete Root;
	Root = nullptr;

	if (!Groups.isEmpty()) groupItems();

	endResetModel();
	removeEmpty(Root);

	emit onGroupComplete();
}

void RecordModel::addItem(const QMap<int, QVariant>& Attributes)
{
	auto Object = new RecordObject(Attributes);

	if (Root) appendItem(Object);
	else
	{
		const int Count = Objects.size() + 1;

		beginInsertRows(QModelIndex(), Count, Count);
		Objects.append(Object);
		endInsertRows();
	}

}

void RecordModel::addItems(const QList<QMap<int, QVariant>>& Attributes)
{
	if (Root) for (const auto& Item : Attributes) appendItem(new RecordObject(Item));
	else
	{
		beginInsertRows(QModelIndex(), Objects.size() + 1, Objects.size() + Attributes.size());

		for (const auto& Item : Attributes) Objects.append(new RecordObject(Item));

		endInsertRows();
	}
}

void RecordModel::setItems(const QList<QMap<int, QVariant>>& Attributes)
{
	beginResetModel();

	while (!Objects.isEmpty()) delete Objects.takeLast();
	if (Root) { delete Root; Root = new GroupObject(); }

	endResetModel();

	for (const auto& Item : Attributes)
	{
		Objects.append(new RecordObject(Item));
	}

	if (!Header.isEmpty()) groupItems();
}

