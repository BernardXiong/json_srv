#include "json.h"
#include <lwip/sockets.h>

#define JSON_SRV_STACK_SIZE	1024
#define JSON_SRV_PRI		16
#define JSON_SRV_PORT		80
#define JSON_SESSION_BUFSZ	4096

#define JSON_STR_BEGIN_WITH(str, prefix)	(strncasecmp((str), (prefix), strlen(prefix)) == 0)
#define JSON_STR_404		"404"

void json_set_header(int clnt, int code, const char* title, int length)
{
	static const char* fmt = "HTTP/1.1 %d %s\r\nJSON_SRV";

	static const char* content = "Content-Type: application/json\r\nContent-Length: %ld\r\n\r\n";
	static const char* content_nolength = "Content-Type: application/json\r\n\r\n";

	int offset;
	char *buffer, *ptr, *end_buffer;

	buffer = (char*) rt_malloc(512);
	if (buffer == RT_NULL) return ;

	ptr = buffer;
	end_buffer = (char*)buffer + 512;

	offset = rt_snprintf(ptr, end_buffer - ptr, fmt, code, title);
	ptr += offset;

	if (length != -1)
		offset = rt_snprintf(ptr, end_buffer - ptr, content_nolength);
	else 
		offset = rt_snprintf(ptr, end_buffer - ptr, content, length);
	ptr += offset;

	lwip_write(clnt, buffer, ptr - (char*)buffer);

	/* release buffer */
	rt_free(buffer);
}

static int json_handle_request(int clnt, rt_uint8_t* buf, rt_size_t len)
{
#define JSON_GET_METHOD		0x00
#define JSON_POST_METHOD	0x01

	rt_uint8_t* ch;
	rt_uint8_t* request_ptr;
	rt_uint8_t* host_path, path[64];
	int content_length;
	int method;

	request_ptr = buf;
	if (JSON_STR_BEGIN_WITH(request_ptr, "GET "))
	{
		/* GET method */
		request_ptr += 4;
		method = JSON_GET_METHOD;
	}
	else if (JSON_STR_BEGIN_WITH(request_ptr, "POST "))
	{
		/* POST method */
		request_ptr += 5;
		method = JSON_POST_METHOD;
	}
	else
	{
		/* response with 404 */
		return 404;
	}

	/* handle the path of request */
	host_path = request_ptr;
	ch = strchr(request_ptr, ' ');
	if (ch == RT_NULL) 
	{
		/* Bad Request */
		return 404;
	}
	*ch++ = '\0';
	request_ptr = ch;

	/* check path, whether there is a query */
	ch = strchr(host_path, '?');
	if (ch != RT_NULL)
	{
		/* Bad Request */
		return 404;
	}

	/* check protocol for HTTP/1.0 or HTTP/1.1 */
	if (!JSON_STR_BEGIN_WITH(request_ptr, "HTTP/1"))
	{
		return 404;
	}

	ch = strstr(request_ptr, "\r\n");
	if (ch == RT_NULL)
	{
		return 404;
	}
	*ch ++ = '\0'; *ch ++ = '\0';
	request_ptr = ch;

	/* copy host path */
	strncpy(path, host_path, sizeof(path) - 1);

	while (1)
	{
		if (*request_ptr == '\0')
		{
			/* end of request */
			return 404;
		}

		if (JSON_STR_BEGIN_WITH(request_ptr, "\r\n"))
		{
			/* end of get request */
			request_ptr += 2;

			if (method == JSON_GET_METHOD)
			{
				/* get method */
				if (host_path[0] == '/' && host_path[1] == '\0')
				{
					char* default_index = "JSON Server/ART\n";
					json_set_header(clnt, 200, "OK", -1);
					lwip_write(clnt, default_index, rt_strlen(default_index));
					lwip_close(clnt);

					return 200;
				}

				return 404;
			}
			else if (method == JSON_POST_METHOD)
			{
				int offset;

				/* post method */

				/* read the rest request */
				offset = (rt_uint16_t)(len - ((rt_uint32_t)request_ptr - (rt_uint32_t)buf));
				if (content_length > 0)
				{
					memcpy(buf, request_ptr, offset);
				}

				/* to read the rest content */
				while (offset < content_length)
				{
					int read_bytes;

					/* read POST content */
					read_bytes = lwip_recv(clnt, (char*)&buf[offset], content_length - offset, 0);
					if (read_bytes <= 0) break;

					offset += read_bytes;
				}

				if (offset == content_length)
				{
					buf[offset] = '\0';

					/* handle the post json script */
					// rt_kprintf("%s\n", buf);

					if (strcmp(path, "/rpc") == 0)
					{
					}
					else if (strcmp(path, "/api") == 0)
					{
						json_lewei_handle(clnt, buf, content_length);
					}
				}
			}

			return 200;
		}

		if (JSON_STR_BEGIN_WITH(request_ptr, "Content-Length:"))
		{
			rt_uint8_t *length_str;

			/* get content length */
			request_ptr += 15;
			while (*request_ptr == ' ') request_ptr ++;
			length_str = request_ptr;

			ch = strstr(request_ptr, "\r\n");
			if (ch == RT_NULL) 
			{
				/* Bad Request */
				return 404;
			}

			/* terminal field */
			*ch ++ = '\0';
			*ch ++ = '\0';
			request_ptr = ch;

			/* get content length */
			content_length = atoi(length_str);
		}
		else if (JSON_STR_BEGIN_WITH(request_ptr, "Content-Type:"))
		{
			rt_uint8_t *content;
			/* get content type */
			request_ptr += 13;
			while (*request_ptr == ' ') request_ptr ++;
			content = request_ptr;

			ch = strstr(request_ptr, "\r\n");
			if (ch == RT_NULL) 
			{
				/* Bad Request */
				return 404;
			}

			/* terminal field */
			*ch ++ = '\0';
			*ch ++ = '\0';
			request_ptr = ch;

			/* check content */
			if (!JSON_STR_BEGIN_WITH(content, "application/json"))
				return 404;
		}
		else
		{
			ch = strstr(request_ptr, "\r\n");
			if (ch == RT_NULL) 
			{
				/* Bad Request */
				return 404;
			}

			/* terminal field */
			*ch ++ = '\0';
			*ch ++ = '\0';
			request_ptr = ch;
		}
	}

	return 0;
}

static void json_srv(void* parameter)
{
	int ret; /* return result */
	int sockaddr_len;
	int sock = -1, connected;
	rt_uint8_t *buf;
	struct sockaddr_in server_addr, client_addr;

	buf = (rt_uint8_t*) rt_malloc (JSON_SESSION_BUFSZ);
	if (buf == RT_NULL) goto __exit;

	sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) goto __exit;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(JSON_SRV_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	rt_memset(&(server_addr.sin_zero), 0x00, sizeof(server_addr.sin_zero));

	ret = bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	if (ret < 0) goto __exit;
	/* listen */

	ret = listen(sock, 2);
	if (ret < 0) goto __exit;

	while (1)
	{
		connected = lwip_accept(sock, (struct sockaddr *)&client_addr, (socklen_t*)&sockaddr_len);
		if (connected >= 0)
		{
			while (1)
			{
				int recv_len;
				int ret;

				recv_len = lwip_recv(connected, buf, JSON_SESSION_BUFSZ - 1, 0);
				if (recv_len > 0) 
				{
					buf[recv_len] = 0;
				}
				else
				{
					lwip_close(connected);
					break;
				}

				/* handle web json request */
				ret = json_handle_request(connected, buf, recv_len);
				if (ret == 404)
				{
					lwip_send(connected, JSON_STR_404, strlen(JSON_STR_404), 0);
				}

				lwip_close(connected);
				break;
			}
		}
	}

__exit:
	if (sock >= 0) lwip_close(sock);

	return;
}

rt_err_t json_srv_init(void)
{
	rt_thread_t tid;

	tid = rt_thread_find("jsrv");
	if (tid != RT_NULL) /* json server thread already startup. */
		return -RT_EBUSY;

	tid = rt_thread_create("jsrv", json_srv, RT_NULL, 
		JSON_SRV_STACK_SIZE, JSON_SRV_PRI, 10);
	if (tid != RT_NULL)
		rt_thread_startup(tid);

	return RT_EOK;
}
#include <finsh.h>
FINSH_FUNCTION_EXPORT(json_srv_init, start json server);

