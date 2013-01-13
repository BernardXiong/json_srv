#ifndef __JSON_SRV_H__
#define __JSON_SRV_H__

void json_set_header(int clnt, int code, const char* title, int length);
rt_err_t json_srv_init(void);

#endif

