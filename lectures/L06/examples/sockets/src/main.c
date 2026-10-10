/*
 * SPDX-License-Identifier: MIT
 *
 * A TCP echo server, reachable from the host through native_sim's offloaded sockets.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define SERVER_STACK_SIZE 2048
#define SERVER_PRIORITY   7

/* Send all of a buffer. A stream socket may accept fewer bytes than asked, so send until done. */
static int send_all(int sock, const char *buf, size_t len)
{
	while (len > 0) {
		ssize_t sent = zsock_send(sock, buf, len, 0);

		if (sent < 0) {
			return -errno;
		}
		buf += sent;
		len -= sent;
	}
	return 0;
}

/* Echo everything a client sends until it closes the connection. */
static void serve_client(int client)
{
	char buf[128];

	for (;;) {
		/* Blocks this thread only: the other threads keep running while it waits. */
		ssize_t received = zsock_recv(client, buf, sizeof(buf), 0);

		if (received == 0) {
			return; /* The client closed the connection. */
		}
		if (received < 0) {
			LOG_ERR("recv() failed: %d", errno);
			return;
		}
		if (send_all(client, buf, received) < 0) {
			LOG_ERR("send() failed: %d", errno);
			return;
		}
		LOG_INF("Echoed %zd bytes", received);
	}
}

static void server(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_APP_PORT),
		.sin_addr.s_addr = net_htonl(NET_INADDR_ANY),
	};
	int one = 1;
	int listener = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);

	if (listener < 0) {
		LOG_ERR("socket() failed: %d", errno);
		return;
	}

	/* Allow a restarted program to listen on the port again at once, as on any Linux server. */
	zsock_setsockopt(listener, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &one, sizeof(one));

	if ((zsock_bind(listener, (struct net_sockaddr *)&addr, sizeof(addr)) < 0) ||
	    (zsock_listen(listener, 1) < 0)) {
		LOG_ERR("Cannot listen on port %d: %d", CONFIG_APP_PORT, errno);
		zsock_close(listener);
		return;
	}
	LOG_INF("Listening on TCP port %d", CONFIG_APP_PORT);

	/* One client at a time: enough for an echo server, and it keeps the example to one thread.
	 */
	for (;;) {
		int client = zsock_accept(listener, NULL, NULL);

		if (client < 0) {
			LOG_ERR("accept() failed: %d", errno);
			continue;
		}
		LOG_INF("Client connected");

		/*
		 * Workaround for native_sim in Zephyr 4.4: the offloaded-sockets driver does not
		 * initialize the timeouts of a socket returned by accept(). A zero timeout there
		 * makes a blocking recv() call straight into the host's recv(), which blocks the
		 * whole process, simulated time included, instead of just this thread. Setting both
		 * timeouts explicitly to zero, which means "wait forever", initializes them. On a
		 * board, and with Zephyr's own IP stack, this is harmless and changes nothing.
		 */
		struct zsock_timeval forever = {0};

		zsock_setsockopt(client, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &forever,
				 sizeof(forever));
		zsock_setsockopt(client, ZSOCK_SOL_SOCKET, ZSOCK_SO_SNDTIMEO, &forever,
				 sizeof(forever));

		serve_client(client);
		zsock_close(client);
		LOG_INF("Client disconnected");
	}
}

K_THREAD_DEFINE(server_thread, SERVER_STACK_SIZE, server, NULL, NULL, NULL, SERVER_PRIORITY, 0, 0);
