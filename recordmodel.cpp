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

#include "recordmodel.hpp"

RecordModel::RecordObject::RecordObject(int Uid, const QMap<int, QVariant>& Fields)
: Attributes(Fields), Index(Uid) {}

RecordModel::RecordObject::~RecordObject(void) {}

QMap<int, QVariant> RecordModel::RecordObject::getFields(void) const
{
	return Attributes;
}

void RecordModel::RecordObject::setFields(const QMap<int, QVariant>& Fields, bool Replace)
{
	if (!Replace) for (auto i = Fields.constBegin(); i != Fields.constEnd(); ++i)
	{
		Attributes[i.key()] = i.value();
	}
	else Attributes = Fields;
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
		if (!Attributes.contains(i.key()) || Attributes[i.key()] != i.value()) return false;
	}

	return true;
}

int RecordModel::RecordObject::getUid(void) const
{
	return Index;
}

RecordModel::GroupObject::GroupObject(int Level, const QMap<int, QVariant>& Fields)
: RecordObject(-1, Fields), Root(nullptr), Column(Level) {}

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
	for (auto& Child : Childs) if (Child->contain(Fields)) return Child; return nullptr;
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
	if (Childs.size() > Index) return Childs.takeAt(Index); else return nullptr;
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
	return getField(Column);
}

int RecordModel::GroupObject::childrenCount(void) const
{
	return Childs.size();
}

bool RecordModel::GroupObject::hasChids(void) const
{
	return !Childs.isEmpty();
}

int RecordModel::GroupObject::getIndex(RecordModel::RecordObject* Child) const
{
	return Childs.indexOf(Child);
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

void RecordModel::GroupObject::sortChilds(const RecordModel::SortObject& Functor, QFutureSynchronizer<void>& Synchronizer)
{
	static auto Sort = std::stable_sort<QList<RecordObject*>::iterator, SortObject>;

	for (auto& Child : Childs) if (auto Group = dynamic_cast<GroupObject*>(Child)) Group->sortChilds(Functor, Synchronizer);
	Synchronizer.addFuture(QtConcurrent::run(Sort, Childs.begin(), Childs.end(), Functor));
}

RecordModel::SortObject::SortObject(int Column, bool Ascending)
: Index(Column), Mode(Ascending) {}

bool RecordModel::SortObject::operator () (RecordModel::RecordObject* First, RecordModel::RecordObject* Second) const
{
	const auto One = First->getField(Index);
	const auto Two = Second->getField(Index);

	if (One == Two) return false;

	bool Compare = One < Two;

	return Mode ? Compare : !Compare;
}

RecordModel::RecordModel(const QStringList& Head, QObject* Parent)
: QAbstractItemModel(Parent), Header(Head) {}

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
	}
	else if (auto Parent = Parents.value(Object, nullptr))
	{
		return createIndex(Parent->getIndex(), 0, Parent);
	}

	return QModelIndex();
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
	if (Orientation != Qt::Horizontal || Section > Header.size()) return QVariant();

	if (Role != Qt::DisplayRole) return QVariant();
	else return Header[Section];
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
		Objects.removeOne(Object);
		Parents.remove(Object);

		endRemoveRows();

		removeEmpty(Parent->getParent());
		appendItem(Object);
	}
	else	emit dataChanged(Index, Index);

	return true;
}

void RecordModel::sort(int Column, Qt::SortOrder Order)
{
	SortObject Sort(Column, Order == Qt::AscendingOrder);

	beginResetModel();

	if (!Root) std::stable_sort(Objects.begin(), Objects.end(), Sort);
	else
	{
		QFutureSynchronizer<void> Synchronizer;
		Root->sortChilds(Sort, Synchronizer);
		Synchronizer.waitForFinished();
	}

	endResetModel();
}

bool RecordModel::setData(int Index, const QMap<int, QVariant>& Data, bool Replace)
{
	for (const auto& Item : Objects) if (Item->getUid() == Index)
	{
		if (Root) return setData(createIndex(Parents[Item]->getIndex(Item), 0, Item), Data, Replace);
		else return setData(createIndex(Objects.indexOf(Item), 0, Item), Data, Replace);
	}

	return false;
}

bool RecordModel::setData(const QModelIndex& Index, const QMap<int, QVariant>& Data, bool Replace)
{
	if (!Index.isValid()) return false;

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return false;

	Object->setFields(Data, Replace);

	if (Root && !Object->contain(Parents[Object]->getFields()))
	{
		auto Parent = Parents[Object];
		auto From = createIndex(Parent->getIndex(), 0, Parent);
		int Row = Parent->childIndex(Object);

		beginRemoveRows(From, Row, Row);

		Parent->takeChild(Object);
		Objects.removeOne(Object);
		Parents.remove(Object);

		endRemoveRows();

		removeEmpty(Parent->getParent());
		appendItem(Object);
	}
	else	emit dataChanged(Index, Index);

	return true;
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

QModelIndexList RecordModel::getIndexes(const QModelIndex& Parent) const
{
	if (!hasChildren(Parent)) return QModelIndexList();

	QModelIndexList List; int Count = rowCount(Parent);

	for (int i = 0; i < Count; ++i) List.append(index(i, 0, Parent));

	return List;
}

QList<int> RecordModel::getUids(const QModelIndexList& Selection) const
{
	QList<int> List;

	for (const auto& Index : Selection)
	{
		RecordObject* Object = (RecordObject*) Index.internalPointer();

		if (dynamic_cast<GroupObject*>(Object)) continue;

		List.append(Object->getUid());
	}

	return List;
}

bool RecordModel::saveToFile(const QString& Path, const QList<int>& Columns, const QModelIndexList& List) const
{
	for (const auto& Index : Columns) if (Header.size() <= Index) return false;

	QFile File(Path); if (!File.open(QFile::WriteOnly | QFile::Text)) return false;

	QTextStream Stream(&File);

	for (const auto& ID : Columns)
	{
		Stream << Header[ID];

		if (ID != Columns.last()) Stream << '\t';
		else Stream << '\n';
	}

	for (const auto& Index : List)
	{
		RecordObject* Object = (RecordObject*) Index.internalPointer();

		for (const auto& ID : Columns)
		{
			Stream << Object->getField(ID).toString();

			if (ID != Columns.last()) Stream << '\t';
			else Stream << '\n';
		}
	}

	return true;
}

bool RecordModel::removeItem(const QModelIndex& Index)
{
	if (!Index.isValid()) return false;

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return false;

	if (Root)
	{
		GroupObject* Parent = Parents[Object];
		const int Row = Parent->childIndex(Object);

		beginRemoveRows(parent(Index), Row, Row);

		Parent->removeChild(Object);
		Objects.removeOne(Object);

		endRemoveRows();

		removeEmpty(Parent);
	}
	else
	{
		const int Row = Objects.indexOf(Object);

		beginRemoveRows(parent(Index), Row, Row);

		Objects.removeOne(Object); delete Object;

		endRemoveRows();
	}

	return true;
}

bool RecordModel::removeItem(int Index)
{
	for (const auto& Item : Objects) if (Item->getUid() == Index)
	{
		if (Root) return removeItem(createIndex(Parents[Item]->getIndex(Item), 0, Item));
		else return removeItem(createIndex(Objects.indexOf(Item), 0, Item));
	}
}

int RecordModel::totalCount(void) const
{
	return Objects.count();
}

bool RecordModel::isGrouped(void) const
{
	return Root;
}

bool RecordModel::exists(int Index) const
{
	for (const auto& Item : Objects) if (Item->getUid() == Index) return true; return false;
}

RecordModel::GroupObject* RecordModel::createGroups(QList<QPair<int, QList<QVariant>>>::ConstIterator From, QList<QPair<int, QList<QVariant>>>::ConstIterator To, RecordModel::GroupObject* Parent)
{
	if (!Parent) Parent = new GroupObject();

	for (const auto& Field : (*From).second)
	{
		GroupObject* Child = new GroupObject((*From).first, Parent->getFields());

		Child->setField((*From).first, Field);
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
			int Column = getIndex(Groups[i]);
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
	const int Count = Current->childrenCount() ? Current->childrenCount() + 1 : 0;

	beginInsertRows(Index, Count, Count);

	Parents.insert(Object, Current);
	Objects.append(Object);
	Current->addChild(Object);

	endInsertRows();

	return Current;
}

int RecordModel::getIndex(const QString& Field) const
{
	int i = 0; for (const auto& Item : Header)
	{
		if (Item == Field) return i; ++i;
	}

	return -1;
}

void RecordModel::removeEmpty(RecordModel::GroupObject* Parent, bool Emit)
{
	if (Parent)
	{
		if (Parent->hasChids()) for (auto Child : Parent->getChilds())
		{
			if (GroupObject* Group = dynamic_cast<GroupObject*>(Child))
			{
				removeEmpty(Group, Emit);
			}
		}
		else if (auto P = Parent->getParent())
		{
			auto From = createIndex(P->getIndex(), 0, P);
			int Row = Parent->getIndex();

			if (Emit) beginRemoveRows(From, Row, Row);
			P->removeChild(Parent);
			if (Emit) endRemoveRows();

			while (P && !P->hasChids()) if (P != Root)
			{
				auto Delete = P; P = P->getParent();

				auto From = createIndex(P->getIndex(), 0, P);
				int Row = Delete->getIndex();

				if (Emit) beginRemoveRows(From, Row, Row);
				P->removeChild(Delete);
				if (Emit) endRemoveRows();
			}
		}
	}
}

void RecordModel::groupItems(void)
{
	QList<QPair<int, QList<QVariant>>> Indexes;

	if (Root) delete Root; Parents.clear();

	for (const auto& Group : Groups)
	{
		const int Index = getIndex(Group);
		if (Index == -1) continue;

		auto Current = qMakePair(Index, QList<QVariant>());

		for (const auto& Object : Objects)
		{
			const QVariant Value = Object->getField(Index);
			if (!Current.second.contains(Value)) Current.second.append(Value);
		}

		Indexes.append(Current);
	}

	Root = createGroups(Indexes.constBegin(), Indexes.constEnd(), Root);
}

void RecordModel::groupByInt(const QList<int>& Levels)
{
	QStringList Groupby;

	for (const auto& I : Levels) if (I < Header.size()) Groupby.append(Header[I]);

	groupByStr(Groupby);
}

void RecordModel::groupByStr(const QStringList& Groupby)
{
	if (Groups == Groupby) { emit onGroupComplete(); return; } Groups = Groupby;

	beginResetModel();

	if (Root) { Parents.clear(); delete Root; Root = nullptr; }

	endResetModel();

	beginResetModel();

	if (!Groups.isEmpty()) { groupItems(); removeEmpty(Root, false); }

	endResetModel();

	emit onGroupComplete();
}

void RecordModel::addItem(int ID, const QMap<int, QVariant>& Attributes)
{
	auto Object = new RecordObject(ID, Attributes);

	if (Root) appendItem(Object);
	else
	{
		const int Count = Objects.size() + 1;

		beginInsertRows(QModelIndex(), Count, Count);
		Objects.insert(ID, Object);
		endInsertRows();
	}

}

void RecordModel::addItems(const QMap<int, QMap<int, QVariant>>& Items)
{
	if (Root) for (auto i = Items.constBegin(); i != Items.constEnd(); ++i)
	{
		appendItem(new RecordObject(i.key(), i.value()));
	}
	else
	{
		const int Count = Objects.size() ? Objects.size() + 1 : 0;

		beginInsertRows(QModelIndex(), Count, Count + Items.size());

		for (auto i = Items.constBegin(); i != Items.constEnd(); ++i)
		{
			Objects.insert(i.key(), new RecordObject(i.key(), i.value()));
		}

		endInsertRows();
	}
}
