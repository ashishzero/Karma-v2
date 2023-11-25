#pragma once
#include "kCommon.h"

template <typename LinkedNode>
void kDListInit(LinkedNode *first)
{
	first->Prev = first;
	first->Next = first;
}

template <typename LinkedNode>
bool kDListIsEmpty(LinkedNode *first)
{
	return first->Next == first;
}

template <typename LinkedNode>
void kDListPushFront(LinkedNode *first, LinkedNode *node)
{
	LinkedNode *next = first->Next;
	LinkedNode *prev = first;

	node->Next       = next;
	node->Prev       = prev;

	prev->Next       = node;
	next->Prev       = node;
}

template <typename LinkedNode>
void kDListPushBack(LinkedNode *first, LinkedNode *node)
{
	kDListPushFront(first->Prev, node);
}

template <typename LinkedNode>
LinkedNode *kDListPopFront(LinkedNode *first)
{
	LinkedNode *node = first->Next;
	LinkedNode *next = node->Next;
	LinkedNode *prev = node->Prev;

	kAssert(node != next && node != prev);

	prev->Next = next;
	next->Prev = prev;

	return node;
}

template <typename LinkedNode>
LinkedNode *kDListPopBack(LinkedNode *first)
{
	return kDListPopFront(first->Prev->Prev);
}

template <typename LinkedNode>
void kDListRemove(LinkedNode *r)
{
	LinkedNode *prev = r->Prev;
	LinkedNode *next = r->Next;
	prev->Next       = next;
	next->Prev       = prev;
}
