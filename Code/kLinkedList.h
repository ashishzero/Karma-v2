#pragma once
#include "kCommon.h"

template <typename LinkedNode> void kInitDoublyLinkedList(LinkedNode *first)
{
	first->prev = first;
	first->next = first;
}

template <typename LinkedNode> bool kDoublyLinkedListEmpty(LinkedNode *first)
{
	return first->next == first;
}

template <typename LinkedNode> void kDoublyLinkedListPushFront(LinkedNode *first, LinkedNode *node)
{
	LinkedNode *next = first->next;
	LinkedNode *prev = first;

	node->next		 = next;
	node->prev		 = prev;

	prev->next		 = node;
	next->prev		 = node;
}

template <typename LinkedNode> void kDoublyLinkedListPushBack(LinkedNode *first, LinkedNode *node)
{
	kDoublyLinkedListPushFront(first->prev, node);
}

template <typename LinkedNode> LinkedNode *kDoublyLinkedListPopFront(LinkedNode *first)
{
	LinkedNode *node = first->next;
	LinkedNode *next = node->next;
	LinkedNode *prev = node->prev;

	kAssert(node != next && node != prev);

	prev->next = next;
	next->prev = prev;

	return node;
}

template <typename LinkedNode> LinkedNode *kDoublyLinkedListPopBack(LinkedNode *first)
{
	return kDoublyLinkedListPopFront(first->prev->prev);
}

template <typename LinkedNode> void kDoublyLinkedListRemove(LinkedNode *r)
{
	LinkedNode *prev = r->prev;
	LinkedNode *next = r->next;
	prev->next		 = next;
	next->prev		 = prev;
}
