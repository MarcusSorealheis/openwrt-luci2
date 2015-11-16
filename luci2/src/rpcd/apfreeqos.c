/*
 */

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <net/if.h>
#include <libubus.h>

#include <rpcd/plugin.h>

#define	RPC_LUCI2_APFREE_BL_INIT_PATH		"/etc/init.d/apfreebl"
#define	RPC_LUCI2_APFREE_IP_INIT_PATH		"/etc/init.d/apfreeip"
#define	RPC_LUCI2_APFREE_VIP_INIT_PATH		"/etc/init.d/apfreevip"

static const struct rpc_daemon_ops *ops;
static struct blob_buf buf;

static int
rpc_errno_status(void)
{
	switch (errno)
	{
	case EACCES:
		return UBUS_STATUS_PERMISSION_DENIED;

	case ENOTDIR:
		return UBUS_STATUS_INVALID_ARGUMENT;

	case ENOENT:
		return UBUS_STATUS_NOT_FOUND;

	case EINVAL:
		return UBUS_STATUS_INVALID_ARGUMENT;

	default:
		return UBUS_STATUS_UNKNOWN_ERROR;
	}
}

enum
{
	APFREE_F_IP,
	APFREE_F_VIP,
	APFREE_F_BL,
	APFREE_F_MAX
};

static const char * get_apfree_init_path(int which)
{
	switch(which)
	{
	case APFREE_F_IP:
		return RPC_LUCI2_APFREE_IP_INIT_PATH;
	case APFREE_F_VIP:
		return RPC_LUCI2_APFREE_VIP_INIT_PATH;
	case APFREE_F_BL:
		return RPC_LUCI2_APFREE_BL_INIT_PATH;
	default:
		return NULL;
	}
};

static int 
rpc_apfree_qos_set_rule(int which, struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_attr *msg)
{
	pid_t 				pid;
	int					fd;
	struct stat 		s;
	const char			*path = NULL;
	
	path = get_apfree_init_path(which);

	if (stat(path, &s))
		return rpc_errno_status();

	if (!(s.st_mode & S_IXUSR))
		return UBUS_STATUS_PERMISSION_DENIED;

	switch ((pid = fork()))
	{
	case -1:
		return rpc_errno_status();

	case 0:
		uloop_done();

		if ((fd = open("/dev/null", O_RDWR)) > -1)
		{
			dup2(fd, 0);
			dup2(fd, 1);
			dup2(fd, 2);

			close(fd);
		}

		chdir("/");

		if (execl(path, path, "start", NULL))
			return rpc_errno_status();

	default:
		return 0;
	}
};

static int 
rpc_apfree_qos_set_ip_rule(struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_attr *msg)
{
	return rpc_apfree_qos_set_rule(APFREE_F_IP, ctx, obj, req, method, msg);
};

static int 
rpc_apfree_qos_set_vip_rule(struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_attr *msg)
{
	return rpc_apfree_qos_set_rule(APFREE_F_VIP, ctx, obj, req, method, msg);
};

static int 
rpc_apfree_qos_set_bl_rule(struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_attr *msg)
{
	return rpc_apfree_qos_set_rule(APFREE_F_BL, ctx, obj, req, method, msg);
};

static int
rpc_apfree_qos_ip_stat(struct ubus_context *ctx, struct ubus_object *obj,
                  struct ubus_request_data *req, const char *method,
                  struct blob_attr *msg)
{
	FILE *f;
	void *c, *d;
	char line[256] = {0};

	blob_buf_init(&buf, 0);
	c = blobmsg_open_array(&buf, "entries");

	if ((f = fopen("/proc/net/tiger_ip_stat", "r")) != NULL)
	{
		while (fgets(line, sizeof(line) - 1, f)) {
			unsigned int upload = 0, download = 0;
			char ip[16] = {0};
			int rc = sscanf(line, "IP=%s   %*s  %*s  %*s  %*s %*s  upload=%u  download=%u",
			            ip, &upload, &download);
			if(rc == 3) {
				d = blobmsg_open_table(&buf, NULL);
				blobmsg_add_string(&buf, "ip", ip);
				blobmsg_add_u32(&buf, "upload", upload);
				blobmsg_add_u32(&buf, "download", download);
				blobmsg_close_table(&buf, d);
			}
			memset(line, 0, 256);
		}
		fclose(f);
	}
	
	blobmsg_close_array(&buf, c);
	ubus_send_reply(ctx, req, buf.head);
   
	
	return 0;
}

static int
rpc_apfree_qos_api_init(const struct rpc_daemon_ops *o, struct ubus_context *ctx)
{
	static const struct ubus_method apfree_qos_methods[] = {
		UBUS_METHOD_NOARG("ip_stat", 		rpc_apfree_qos_ip_stat),

		UBUS_METHOD_NOARG("set_ip_rule", 	rpc_apfree_qos_set_ip_rule),

		UBUS_METHOD_NOARG("set_vip_rule", 	rpc_apfree_qos_set_vip_rule),

		UBUS_METHOD_NOARG("set_bl_rule", 	rpc_apfree_qos_set_bl_rule)
	};

	static struct ubus_object_type apfree_qos_type =
		UBUS_OBJECT_TYPE("luci-rpc-apfree-qos", apfree_qos_methods);

	static struct ubus_object apfree_qos_obj = {
		.name = "luci2.network.apfreeqos",
		.type = &apfree_qos_type,
		.methods = apfree_qos_methods,
		.n_methods = ARRAY_SIZE(apfree_qos_methods),
	};
	
	ops = o;

	return ubus_add_object(ctx, &apfree_qos_obj);
}

struct rpc_plugin rpc_plugin = {
	.init = rpc_apfree_qos_api_init
};
