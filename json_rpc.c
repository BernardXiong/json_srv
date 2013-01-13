/**
 * JSON RPC 2.0 implementation on RT-Thread
 */
#include <rtthread.h>
#include <json.h>
#include <string.h>

static void json_rpc_handle(const char* json, size_t length)
{
	int status = 0;
	struct json_tree* tree = RT_NULL;

	tree = json_tree_parse(json, length);
	if (tree != RT_NULL)
	{
		const char *version;
		const char *method;
		struct json_node *params;
		int id, index, position;

		char cmd[64];

		/* check parameter */
		version = json_tree_get_string(tree, "jsonrpc", RT_NULL);
		method  = json_tree_get_string(tree, "method", RT_NULL);
		id      = json_tree_get_integer(tree, "id", RT_NULL);
		params  = json_tree_get_node(tree, "params", RT_NULL);

		/* parameter check */
		if (strcmp(version, "2.0") != 0) goto __exit;

		position = 0;
		position += rt_sprintf(&cmd[position], "%s(", method);
		for (index = 0; index < params->count; index ++)
		{
			struct json_node *node;

			node = params->vu.array[index];
			
			if (index != 0) 
				position += rt_sprintf(&cmd[position], ",");

			if (node->type == JSON_NODE_TYPE_STR)
				position += rt_sprintf(&cmd[position], "\"%s\"", node->vu.str_value);
			if (node->type == JSON_NODE_TYPE_INT)
				position += rt_sprintf(&cmd[position], "%d", node->vu.int_value);
			if (node->type == JSON_NODE_TYPE_FLOAT)
				position += rt_sprintf(&cmd[position], "%f", node->vu.float_value);
		}
		position += rt_sprintf(&cmd[position], ")");

		rt_kprintf("cmd: %s\n", cmd);
	}

__exit:
	if (tree != RT_NULL) json_tree_destroy(tree);
	return;
}

#include <finsh.h>
const char jrpc[] = "{\"jsonrpc\": \"2.0\", \"method\": \"ls\", \"params\": [\"/\", 100], \"id\": 1}";
void jrpc_test(void)
{
	json_rpc_handle(jrpc, strlen(jrpc));
}
FINSH_FUNCTION_EXPORT(jrpc_test, json rpc test);
