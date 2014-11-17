#include <stddef.h>

#include "config.h"
#include "node.h"

CMARK_EXPORT cmark_node_type
cmark_node_get_type(cmark_node *node)
{
	return node->type;
}

cmark_node*
cmark_node_next(cmark_node *node)
{
	return node->next;
}

cmark_node*
cmark_node_previous(cmark_node *node)
{
	return node->prev;
}

cmark_node*
cmark_node_parent(cmark_node *node)
{
	return node->parent;
}

cmark_node*
cmark_node_first_child(cmark_node *node)
{
	return node->first_child;
}

cmark_node*
cmark_node_last_child(cmark_node *node)
{
	return node->last_child;
}

static inline bool
S_is_block(cmark_node *node) {
	return node->type >= CMARK_NODE_FIRST_BLOCK
	       && node->type <= CMARK_NODE_LAST_BLOCK;
}

static inline bool
S_is_inline(cmark_node *node) {
	return node->type >= CMARK_NODE_FIRST_INLINE
	       && node->type <= CMARK_NODE_LAST_INLINE;
}

static bool
S_can_contain(cmark_node *node, cmark_node *child)
{
	if (child->type == CMARK_NODE_DOCUMENT) {
		return false;
	}

	switch (node->type) {
	case CMARK_NODE_DOCUMENT:
	case CMARK_NODE_BQUOTE:
	case CMARK_NODE_LIST_ITEM:
		return S_is_block(child)
		       && child->type != CMARK_NODE_LIST_ITEM;

	case CMARK_NODE_LIST:
		return child->type == CMARK_NODE_LIST_ITEM;

	case CMARK_NODE_PARAGRAPH:
	case CMARK_NODE_ATX_HEADER:
	case CMARK_NODE_SETEXT_HEADER:
	case CMARK_NODE_EMPH:
	case CMARK_NODE_STRONG:
	case CMARK_NODE_LINK:
	case CMARK_NODE_IMAGE:
	case CMARK_NODE_LINK_LABEL:
		return S_is_inline(child);

	default:
		break;
	}

	return false;
}

// Unlink a node without adjusting its next, prev, and parent pointers.
static void
S_node_unlink(cmark_node *node)
{
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}

	// Adjust first_child and last_child of parent.
	cmark_node *parent = node->parent;
	if (parent) {
		if (parent->first_child == node) {
			parent->first_child = node->next;
		}
		if (parent->last_child == node) {
			parent->last_child = node->prev;
		}
	}
}

void
cmark_node_unlink(cmark_node *node) {
	S_node_unlink(node);

	node->next   = NULL;
	node->prev   = NULL;
	node->parent = NULL;
}

int
cmark_node_insert_before(cmark_node *node, cmark_node *sibling)
{
	if (!S_can_contain(node->parent, sibling)) {
		return 0;
	}

	S_node_unlink(sibling);

	cmark_node *old_prev = node->prev;

	// Insert 'sibling' between 'old_prev' and 'node'.
	if (old_prev) {
		old_prev->next = sibling;
	}
	sibling->prev = old_prev;
	sibling->next = node;
	node->prev    = sibling;

	// Set new parent.
	cmark_node *parent = node->parent;
	sibling->parent = parent;

	// Adjust first_child of parent if inserted as first child.
	if (parent && !old_prev) {
		parent->first_child = sibling;
	}

	return 1;
}

int
cmark_node_insert_after(cmark_node *node, cmark_node *sibling)
{
	if (!S_can_contain(node->parent, sibling)) {
		return 0;
	}

	S_node_unlink(sibling);

	cmark_node *old_next = node->next;

	// Insert 'sibling' between 'node' and 'old_next'.
	if (old_next) {
		old_next->prev = sibling;
	}
	sibling->next = old_next;
	sibling->prev = node;
	node->next    = sibling;

	// Set new parent.
	cmark_node *parent = node->parent;
	sibling->parent = parent;

	// Adjust last_child of parent if inserted as last child.
	if (parent && !old_next) {
		parent->last_child = sibling;
	}

	return 1;
}

int
cmark_node_prepend_child(cmark_node *node, cmark_node *child)
{
	if (!S_can_contain(node, child)) {
		return 0;
	}

	S_node_unlink(child);

	cmark_node *old_first_child = node->first_child;

	child->next       = old_first_child;
	child->prev       = NULL;
	child->parent     = node;
	node->first_child = child;

	if (old_first_child) {
		old_first_child->prev = child;
	}
	else {
		// Also set last_child if node previously had no children.
		node->last_child = child;
	}

	return 1;
}

int
cmark_node_append_child(cmark_node *node, cmark_node *child)
{
	if (!S_can_contain(node, child)) {
		return 0;
	}

	S_node_unlink(child);

	cmark_node *old_last_child = node->last_child;

	child->next      = NULL;
	child->prev      = old_last_child;
	child->parent    = node;
	node->last_child = child;

	if (old_last_child) {
		old_last_child->next = child;
	}
	else {
		// Also set first_child if node previously had no children.
		node->first_child = child;
	}

	return 1;
}


