#pragma once
#include "kCommon.h"

template <typename LinkedNode> void kDListInit(LinkedNode *first)
{
	first->prev = first;
	first->next = first;
}

template <typename LinkedNode> bool kDListIsEmpty(LinkedNode *first)
{
	return first->next == first;
}

template <typename LinkedNode> void kDListPushFront(LinkedNode *first, LinkedNode *node)
{
	LinkedNode *next = first->next;
	LinkedNode *prev = first;

	node->next		 = next;
	node->prev		 = prev;

	prev->next		 = node;
	next->prev		 = node;
}

template <typename LinkedNode> void kDListPushBack(LinkedNode *first, LinkedNode *node)
{
	kDListPushFront(first->prev, node);
}

template <typename LinkedNode> LinkedNode *kDListPopFront(LinkedNode *first)
{
	LinkedNode *node = first->next;
	LinkedNode *next = node->next;
	LinkedNode *prev = node->prev;

	kAssert(node != next && node != prev);

	prev->next = next;
	next->prev = prev;

	return node;
}

template <typename LinkedNode> LinkedNode *kDListPopBack(LinkedNode *first)
{
	return kDListPopFront(first->prev->prev);
}

template <typename LinkedNode> void kDListRemove(LinkedNode *r)
{
	LinkedNode *prev = r->prev;
	LinkedNode *next = r->next;
	prev->next		 = next;
	next->prev		 = prev;
}
