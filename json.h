#ifndef __JSON_H__
#define __JSON_H__

#include <rtthread.h>

#define json_malloc		rt_malloc
#define json_free		rt_free
#define json_realloc	rt_realloc

#define JSON_NODE_LAYER_MAX		10

#define JSON_NODE_TYPE_UNKNOWN	0x00
#define JSON_NODE_TYPE_STR		0x01
#define JSON_NODE_TYPE_INT		0x02
#define JSON_NODE_TYPE_FLOAT	0x03
#define JSON_NODE_TYPE_BOOL		0x04
#define JSON_NODE_TYPE_ARRAY	0x05
#define JSON_NODE_TYPE_DICT		0x06

struct json_node
{
	rt_uint16_t type;
	rt_uint16_t count;
	char* key;

	union
	{
		char* str_value;
		int int_value;
		int boolean_value;
		double float_value;

		struct json_node** array;
		struct json_node** child;
	} vu;
};

struct json_tree
{
	struct json_node root;

	int layer;
	struct json_node *node_stack[JSON_NODE_LAYER_MAX];
};

struct json_tree* json_tree_parse(const char* buffer, rt_size_t length);
void json_tree_destroy(struct json_tree* tree);

struct json_node* json_tree_get_node(struct json_tree* tree, ...);
const char* json_tree_get_string(struct json_tree* tree, ...);
int json_tree_get_integer(struct json_tree* tree, ...);
float json_tree_get_float(struct json_tree* tree, ...);
struct json_node* json_tree_get_array(struct json_tree* tree, int index, ...);

#endif
