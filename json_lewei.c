#include "json.h"

#define SEND_BUFSZ	1024

#define lewei_sensor_prefix 		"{\"successful\":true,\"message\":null,"
#define lewei_sensor_postifx 		"}"

#define lewei_data					"\"data\":"
#define lewei_data_array_prefix	 	"\"data\":["
#define lewei_data_array_postfix 	"]"

#define lewei_sensor_status 		"{\"id\":\"%s\",\"type\":\"%s\",\"name\":\"%s\",\"value\":\"%s\",\"status\":\"ok\"}"

void json_lewei_all_sensor(int clnt)
{
	char *buffer, *end_buffer, *ptr;
	int length;
	
	buffer = (char*)rt_malloc(SEND_BUFSZ);
	if (buffer == RT_NULL) return;

	ptr = buffer; end_buffer = buffer + SEND_BUFSZ;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_prefix);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_data_array_prefix);
	ptr += length;
	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_status, 
		"1",  	/* id */
		"jdq", 	/* type */
		"s1",  	/* name */
		"1"		/* value */);
	ptr += length;
	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_data_array_postfix);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_postifx);
	ptr += length;

	json_set_header(clnt, 200, "Ok", rt_strlen(buffer));
	lwip_write(clnt, buffer, rt_strlen(buffer));

	rt_free(buffer);
}

void json_lewei_get_sensor(int clnt, const char* sensor)
{
	char *buffer, *end_buffer, *ptr;
	int length;
	
	buffer = (char*)rt_malloc(SEND_BUFSZ);
	if (buffer == RT_NULL) return;

	ptr = buffer; end_buffer = buffer + SEND_BUFSZ;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_prefix);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_data);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_status, 
		"1",  	/* id */
		"jdq", 	/* type */
		"s1",  	/* name */
		"1"		/* value */);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_postifx);
	ptr += length;

	json_set_header(clnt, 200, "Ok", rt_strlen(buffer));
	lwip_write(clnt, buffer, rt_strlen(buffer));

	rt_free(buffer);
}

void json_lewei_update_sensor(int clnt, const char* sensor, const char* value)
{
	char *buffer, *end_buffer, *ptr;
	int length;
	
	buffer = (char*)rt_malloc(SEND_BUFSZ);
	if (buffer == RT_NULL) return;

	ptr = buffer; end_buffer = buffer + SEND_BUFSZ;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_prefix);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_data);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_status, 
		"1",  	/* id */
		"jdq", 	/* type */
		"s1",  	/* name */
		"1"		/* value */);
	ptr += length;

	length = rt_snprintf(ptr, (rt_uint32_t)end_buffer - (rt_uint32_t)ptr, lewei_sensor_postifx);
	ptr += length;

	json_set_header(clnt, 200, "Ok", rt_strlen(buffer));
	lwip_write(clnt, buffer, rt_strlen(buffer));

	rt_free(buffer);
}

void json_lewei_handle(int clnt, char* buffer, int length)
{
	/* parse json */
	struct json_tree* tree;

	tree = json_tree_parse(buffer, length);
	if (tree != RT_NULL)
	{
		const char *userkey;
		const char *method;

		int status = -1;

		userkey = json_tree_get_string(tree, "userkey", RT_NULL);
		method = json_tree_get_string(tree, "f", RT_NULL);

		rt_kprintf("usekey = %s\n", userkey);
		rt_kprintf("method = %s\n", method);

		/* we do not check the userkey right now */
		// if (strcmp(userkey, "4c2a67e84aac4dbfb") == 0) 
		{
			if (strcmp(method, "getAllSensors") == 0)
			{
				json_lewei_all_sensor(clnt);
			}
			else if (strcmp(method, "updateSensor") == 0)
			{
				const char *sensor;
				const char *value;

				sensor = json_tree_get_string(tree, "p", "name", RT_NULL);
				value = json_tree_get_string(tree, "p", "value", RT_NULL);

				rt_kprintf("update %s ==> %s\n", sensor, value);
				json_lewei_update_sensor(clnt, sensor, value);
			}
			else if (strcmp(method, "getSensor") == 0)
			{
				const char* sensor;

				sensor = json_tree_get_string(tree, "p", "name", RT_NULL);

				json_lewei_get_sensor(clnt, sensor);
			}
		}

		json_tree_destroy(tree);
	}
}

