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

RecordModel::RecordObject::RecordObject(int Uid, const QHash<int, QVariant>& Fields)
: Attributes(Fields), Index(Uid) {}

RecordModel::RecordObject::~RecordObject(void) {}

QHash<int, QVariant> RecordModel::RecordObject::getFields(void) const
{
	return Attributes;
}

void RecordModel::RecordObject::setFields(const QHash<int, QVariant>& Fields, bool Replace)
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

bool RecordModel::RecordObject::contain(const QHash<int, QVariant>& Fields) const
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

RecordModel::GroupObject::GroupObject(int Level, const QHash<int, QVariant>& Fields)
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

bool RecordModel::GroupObject::removeChild(const QHash<int, QVariant>& Fields)
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

RecordModel::RecordObject* RecordModel::GroupObject::takeChild(const QHash<int, QVariant>& Fields)
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

QVector<RecordModel::GroupObject*> RecordModel::GroupObject::allGroups(void)
{
	QVector<GroupObject*> List;

	for (const auto Child : Childs) if (auto C = dynamic_cast<GroupObject*>(Child))
	{
		List.append(C); List.append(C->allGroups());
	}

	return List;
}

QVector<RecordModel::RecordObject*> RecordModel::GroupObject::getChilds(void)
{
	return Childs;
}

QSet<int> RecordModel::GroupObject::allUids(void)
{
	QSet<int> List;

	for (const auto Child : Childs)
		if (auto C = dynamic_cast<GroupObject*>(Child))
		{
			List |= C->allUids();
		}
		else
		{
			List.insert(Child->getUid());
		}

	return List;
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
	static auto Sort = std::stable_sort<QVector<RecordObject*>::iterator, SortObject>;

	for (auto& Child : Childs) if (auto Group = dynamic_cast<GroupObject*>(Child)) Group->sortChilds(Functor, Synchronizer);
	Synchronizer.addFuture(QtConcurrent::run(Sort, Childs.begin(), Childs.end(), Functor));
}

RecordModel::SortObject::SortObject(int Column, bool Ascending) : Index(Column), Mode(Ascending) {}

bool RecordModel::SortObject::operator () (RecordModel::RecordObject* First, RecordModel::RecordObject* Second) const
{
	const auto One = First->getField(Index);
	const auto Two = Second->getField(Index);

	if (One == Two) return false;

	bool Compare = One < Two;

	return Mode ? Compare : !Compare;
}

RecordModel::RecordModel(const QStringList& Head, QObject* Parent)
: QAbstractItemModel(Parent), Header(Head), Locker(QMutex::Recursive) {}

RecordModel::~RecordModel(void)
{
	QMutexLocker Synchronizer(&Locker);

	for (auto& Object : Objects) delete Object;
}

QModelIndex RecordModel::index(int Row, int Col, const QModelIndex& Parent) const
{
	QMutexLocker Synchronizer(&Locker);

	if (!hasIndex(Row, Col, Parent)) return QModelIndex();

	if (!Root) return createIndex(Row, Col, Objects.value(Row));

	GroupObject* parentObject = Parent.isValid() ? (GroupObject*) Parent.internalPointer() : Root;

	if (auto Child = parentObject->getChild(Row))
	{
		return createIndex(Row, Col, Child);
	}
	else return QModelIndex();
}

QModelIndex RecordModel::parent(const QModelIndex& Index) const
{
	QMutexLocker Synchronizer(&Locker);

	if (!Root || !Index.isValid()) return QModelIndex();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (Roots.contains((GroupObject*) Object))
	{
		if (auto Group = dynamic_cast<GroupObject*>(Object))
		{
			if (auto Parent = Group->getParent()) if (Parent != Root)
			{
				return createIndex(Parent->getIndex(), 0, Parent);
			}
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
	QMutexLocker Synchronizer(&Locker);

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
	QMutexLocker Synchronizer(&Locker);

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
	QMutexLocker Synchronizer(&Locker); return Header.size();
}

QVariant RecordModel::headerData(int Section, Qt::Orientation Orientation, int Role) const
{
	QMutexLocker Synchronizer(&Locker);

	if (Orientation != Qt::Horizontal || Section > Header.size()) return QVariant();

	if (Role != Qt::DisplayRole) return QVariant();
	else return Header[Section];
}

QVariant RecordModel::data(const QModelIndex &Index, int Role) const
{
	QMutexLocker Synchronizer(&Locker);

	if (!Index.isValid() || !(Role == Qt::DisplayRole || Role == Qt::EditRole)) return QVariant();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (Object) return Object->getField(Index.column());
	else return QVariant();
}

Qt::ItemFlags RecordModel::flags(const QModelIndex& Index) const
{	
	QMutexLocker Synchronizer(&Locker);

	if (!Index.isValid()) return Qt::ItemFlags(0);

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	const auto groupFlag = selectGroups ?  Qt::ItemIsSelectable : Qt::NoItemFlags;

	if (dynamic_cast<GroupObject*>(Object)) return Qt::ItemIsEnabled | groupFlag;
	else return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool RecordModel::setData(const QModelIndex& Index, const QVariant& Value, int Role)
{
	QMutexLocker Synchronizer(&Locker);

	if (!Index.isValid() || !(Role == Qt::DisplayRole || Role == Qt::EditRole)) return false;

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return false;
	if (Root && !Parents.contains(Object)) return false;

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

		Roots = Parents.values().toSet();

		endRemoveRows();

		removeEmpty(Parent->getParent());
		appendItem(Object);
	}
	else	emit dataChanged(Index, Index);

	return true;
}

void RecordModel::sort(int Column, Qt::SortOrder Order)
{
	QMutexLocker Synchronizer(&Locker);

	const SortObject Sort(Column, Order == Qt::AscendingOrder);

	beginResetModel();

	if (!Root) __gnu_parallel::sort(Objects.begin(), Objects.end(), Sort);
	else
	{
		QFutureSynchronizer<void> Synchronizer;
		Root->sortChilds(Sort, Synchronizer);
		Synchronizer.waitForFinished();
	}

	endResetModel();
}

bool RecordModel::setData(int Index, const QHash<int, QVariant>& Data, bool Replace)
{
	QMutexLocker Synchronizer(&Locker);

	for (const auto& Item : Objects) if (Item->getUid() == Index)
	{
		if (Root) return setData(createIndex(Parents[Item]->getIndex(Item), 0, Item), Data, Replace);
		else return setData(createIndex(Objects.indexOf(Item), 0, Item), Data, Replace);
	}

	return false;
}

bool RecordModel::setData(const QModelIndex& Index, const QHash<int, QVariant>& Data, bool Replace)
{
	QMutexLocker Synchronizer(&Locker);

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

		Roots = Parents.values().toSet();

		endRemoveRows();

		removeEmpty(Parent->getParent());
		appendItem(Object);
	}
	else	emit dataChanged(Index, Index);

	return true;
}

QHash<int, QVariant> RecordModel::fullData(const QModelIndex& Index) const
{
	QMutexLocker Synchronizer(&Locker);

	if (!Index.isValid()) return QHash<int, QVariant>();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	return Object->getFields();
}

QVariant RecordModel::fieldData(const QModelIndex& Index, int Col) const
{
	QMutexLocker Synchronizer(&Locker);

	if (!Index.isValid()) return QVariant();

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	return Object->getField(Col);
}

QModelIndexList RecordModel::getIndexes(const QModelIndex& Parent) const
{
	QMutexLocker Synchronizer(&Locker);

	if (!hasChildren(Parent)) return QModelIndexList();

	QModelIndexList List; int Count = rowCount(Parent);

	for (int i = 0; i < Count; ++i) List.append(index(i, 0, Parent));

	return List;
}

QSet<int> RecordModel::getUids(const QModelIndexList& Selection) const
{
	QMutexLocker Synchronizer(&Locker); QSet<int> List;

	for (const auto& Index : Selection)
	{
		RecordObject* Object = (RecordObject*) Index.internalPointer();

		if (auto G = dynamic_cast<GroupObject*>(Object))
		{
			List |= G->allUids();
		}
		else List.insert(Object->getUid());
	}

	return List;
}

QSet<int> RecordModel::getUids(void) const
{
	QMutexLocker Synchronizer(&Locker);

	QSet<int> Uids; Uids.reserve(Objects.size());

	for (const auto& O : Objects) Uids.insert(O->getUid());

	return Uids;
}

int RecordModel::getUid(const QModelIndex& Index) const
{
	QMutexLocker Synchronizer(&Locker);

	RecordObject* Object = (RecordObject*) Index.internalPointer();

	if (dynamic_cast<GroupObject*>(Object)) return -1;

	return Object->getUid();
}

bool RecordModel::saveToFile(const QString& Path, const QList<int>& Columns, const QModelIndexList& List, bool Names) const
{
	QMutexLocker Synchronizer(&Locker);

	for (const auto& Index : Columns) if (Header.size() <= Index) return false;

	QFile File(Path); if (!File.open(QFile::WriteOnly | QFile::Text)) return false;

	QTextStream Stream(&File);

	if (Names) for (const auto& ID : Columns)
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
	QMutexLocker Synchronizer(&Locker);

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

		beginRemoveRows(QModelIndex(), Row, Row);

		Objects.removeOne(Object); delete Object;

		endRemoveRows();
	}

	return true;
}

bool RecordModel::removeItem(int Index)
{
	QMutexLocker Synchronizer(&Locker);

	for (const auto& Item : Objects) if (Item->getUid() == Index)
	{
		if (Root) return removeItem(createIndex(Parents[Item]->getIndex(Item), 0, Item));
		else return removeItem(createIndex(Objects.indexOf(Item), 0, Item));
	}

	return false;
}

int RecordModel::totalCount(void) const
{
	QMutexLocker Synchronizer(&Locker); return Objects.count();
}

bool RecordModel::isGrouped(void) const
{
	QMutexLocker Synchronizer(&Locker); return Root;
}

bool RecordModel::exists(int Index) const
{
	QMutexLocker Synchronizer(&Locker);

	for (const auto& Item : Objects)
	{
		if (Item->getUid() == Index) return true;
	}

	return false;
}

QModelIndex RecordModel::index(int Index) const
{
	QMutexLocker Synchronizer(&Locker);

	for (const auto& Item : Objects) if (Item->getUid() == Index)
	{
		if (Root)
		{
			const auto Parent = Parents.value(Item, nullptr);

			if (Parent) return createIndex(Parent->getIndex(Item), 0, Item);
		}
		else return createIndex(Objects.indexOf(Item), 0, Item);
	}

	return QModelIndex();
}

QModelIndex RecordModel::find(int Index, QVariant Data) const
{
	QMutexLocker Synchronizer(&Locker);

	if (Objects.isEmpty() || Header.size() <= Index) return QModelIndex();

	for (const auto& Object : Objects) if (Object->getField(Index) == Data)
	{
		return Root ? createIndex(Parents[Object]->getIndex(Object), 0, Object) :
				    createIndex(Objects.indexOf(Object), 0, Object);
	}

	return QModelIndex();
}

void RecordModel::setGroupsSelectable(bool Selectable)
{
	selectGroups = Selectable;
}

RecordModel::GroupObject* RecordModel::createGroups(QList<QPair<int, QList<QVariant>>>::ConstIterator From, QList<QPair<int, QList<QVariant>>>::ConstIterator To, RecordModel::GroupObject* Parent)
{
	QMutexLocker Synchronizer(&Locker);

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
				Roots.insert(Child);
			}
		}
	}

	return Parent;
}

RecordModel::GroupObject* RecordModel::appendItem(RecordModel::RecordObject* Object)
{
	QMutexLocker Synchronizer(&Locker);

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
	Roots.insert(Current);

	endInsertRows();

	return Current;
}

int RecordModel::getIndex(const QString& Field) const
{
	QMutexLocker Synchronizer(&Locker);

	int i = 0; for (const auto& Item : Header)
	{
		if (Item == Field) return i; ++i;
	}

	return -1;
}

void RecordModel::removeEmpty(RecordModel::GroupObject* Parent, bool Emit)
{
	QMutexLocker Synchronizer(&Locker);

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

			while (P && P != Root && !P->hasChids())
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
	QMutexLocker Synchronizer(&Locker);

	QList<QPair<int, QList<QVariant>>> Indexes;

	if (Root) delete Root;

	Parents.clear();
	Roots.clear();

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
	QMutexLocker Synchronizer(&Locker); QStringList Groupby;

	for (const auto& I : Levels) if (I < Header.size()) Groupby.append(Header[I]);

	groupByStr(Groupby);
}

void RecordModel::groupByStr(const QStringList& Groupby)
{
	QMutexLocker Synchronizer(&Locker);

	if (Groups == Groupby) { emit onGroupComplete(); return; } Groups = Groupby;

	beginResetModel();

	if (Root) { Roots.clear(); Parents.clear(); delete Root; Root = nullptr; }

	if (!Groups.isEmpty()) { groupItems(); removeEmpty(Root, false); }

	endResetModel();

	emit onGroupComplete();
}

void RecordModel::addItem(int ID, const QHash<int, QVariant>& Attributes)
{
	QMutexLocker Synchronizer(&Locker);

	auto Object = new RecordObject(ID, Attributes);

	if (Root) appendItem(Object);
	else
	{
		const int Count = Objects.size() + 1;

		beginInsertRows(QModelIndex(), Count, Count);
		Objects.append(Object);
		endInsertRows();
	}

}

void RecordModel::addItems(const QHash<int, QHash<int, QVariant>>& Items)
{
	QMutexLocker Synchronizer(&Locker);

	Objects.reserve(Objects.size() + Items.size());

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
			Objects.append(new RecordObject(i.key(), i.value()));
		}

		endInsertRows();
	}
}
