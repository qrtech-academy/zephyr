/*
 * SPDX-License-Identifier: MIT
 *
 * A client for the echo server in main.c, run in the same program when CONFIG_APP_SELF_TEST=y.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(self_test, LOG_LEVEL_INF);

#define CLIENT_STACK_SIZE 2048
#define CLIENT_PRIORITY   8
#define CLIENT_DELAY_MS   100

static const char message[] = "Hello, echo server!";

static void client(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_APP_PORT),
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	char reply[sizeof(message)] = {0};
	size_t received = 0;
	int sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);

	if ((sock < 0) || (zsock_connect(sock, (struct net_sockaddr *)&addr, sizeof(addr)) < 0)) {
		LOG_ERR("Cannot connect to port %d: %d", CONFIG_APP_PORT, errno);
		goto out;
	}

	if (zsock_send(sock, message, strlen(message), 0) != (ssize_t)strlen(message)) {
		LOG_ERR("send() failed: %d", errno);
		goto out;
	}

	/* The echo may arrive in pieces; a stream socket keeps no message boundaries. */
	while (received < strlen(message)) {
		ssize_t n = zsock_recv(sock, reply + received, strlen(message) - received, 0);

		if (n <= 0) {
			LOG_ERR("recv() failed after %zu bytes: %d", received, errno);
			goto out;
		}
		received += n;
	}

	if (memcmp(reply, message, strlen(message)) == 0) {
		LOG_INF("Self-test passed: the server echoed \"%s\"", reply);
	} else {
		LOG_ERR("Self-test failed: sent \"%s\", got \"%s\"", message, reply);
	}

out:
	if (sock >= 0) {
		zsock_close(sock);
	}
}

/* Start after a delay, so that the server is listening by the time the client connects. */
K_THREAD_DEFINE(client_thread, CLIENT_STACK_SIZE, client, NULL, NULL, NULL, CLIENT_PRIORITY, 0,
		CLIENT_DELAY_MS);
