#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "libtcmu_log.h"
#include "libtcmu.h"
#include "spdm.h"

static bool read_bytes(const int socket, uint8_t *buffer,
                       size_t number_of_bytes)
{
    ssize_t number_received = 0;
    ssize_t result;

    while (number_received < number_of_bytes) {
        result = recv(socket, buffer + number_received,
                      number_of_bytes - number_received, 0);
        if (result <= 0) {
            return false;
        }
        number_received += result;
    }
    return true;
}

static bool read_data32(const int socket, uint32_t *data)
{
    bool result;

    result = read_bytes(socket, (uint8_t *)data, sizeof(uint32_t));
    if (!result) {
        return result;
    }
    *data = ntohl(*data);
    return true;
}

static bool read_multiple_bytes(const int socket, uint8_t *buffer,
                                uint32_t *bytes_received,
                                uint32_t max_buffer_length)
{
    uint32_t length;
    bool result;

    result = read_data32(socket, &length);
    if (!result) {
        return result;
    }

    if (length > max_buffer_length) {
        return false;
    }

    if (bytes_received) {
        *bytes_received = length;
    }

    if (length == 0) {
        return true;
    }

    return read_bytes(socket, buffer, length);
}

static bool receive_platform_data(const int socket,
                                  uint32_t transport_type,
                                  uint32_t *command,
                                  uint8_t *receive_buffer,
                                  uint32_t *bytes_to_receive)
{
    bool result;
    uint32_t response;
    uint32_t bytes_received;

    result = read_data32(socket, &response);
    if (!result) {
        return result;
    }
    *command = response;

    result = read_data32(socket, &transport_type);
    if (!result) {
        return result;
    }

    bytes_received = 0;
    result = read_multiple_bytes(socket, receive_buffer, &bytes_received,
                                 *bytes_to_receive);
    if (!result) {
        return result;
    }
    *bytes_to_receive = bytes_received;

    return result;
}

static bool write_bytes(const int socket, const uint8_t *buffer,
                        uint32_t number_of_bytes)
{
    ssize_t number_sent = 0;
    ssize_t result;

    while (number_sent < number_of_bytes) {
        result = send(socket, buffer + number_sent,
                      number_of_bytes - number_sent, 0);
        if (result == -1) {
            return false;
        }
        number_sent += result;
    }
    return true;
}

static bool write_data32(const int socket, uint32_t data)
{
    data = htonl(data);
    return write_bytes(socket, (uint8_t *)&data, sizeof(uint32_t));
}

static bool write_multiple_bytes(const int socket, const uint8_t *buffer,
                                 uint32_t bytes_to_send)
{
    bool result;

    result = write_data32(socket, bytes_to_send);
    if (!result) {
        return result;
    }

    return write_bytes(socket, buffer, bytes_to_send);
}

static bool send_platform_data(const int socket,
                               uint32_t transport_type, uint32_t command,
                               const uint8_t *send_buffer, size_t bytes_to_send)
{
    bool result;

    result = write_data32(socket, command);
    if (!result) {
        return result;
    }

    result = write_data32(socket, transport_type);
    if (!result) {
        return result;
    }

    return write_multiple_bytes(socket, send_buffer, bytes_to_send);
}

int spdm_socket_connect(struct tcmu_device *dev, uint16_t port)
{
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0) {
        tcmu_dev_err(dev, "cannot create socket: %s\n", strerror(errno));
        return -1;
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(port);


    if (connect(client_socket, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        tcmu_dev_err(dev, "cannot connect: %s\n", strerror(errno));
        close(client_socket);
        return -1;
    }

    return client_socket;
}

int spdm_send_request(struct tcmu_device *dev, const int socket,
                       uint32_t transport_type, void *req, uint32_t req_len)
{
    bool result;

    if (socket < 0) {
        tcmu_dev_err(dev, "invalid socket: %d\n", socket);
        return -1;
    }

    result = send_platform_data(socket, transport_type,
                                SPDM_SOCKET_COMMAND_NORMAL,
                                req, req_len);
    if (!result) {
        tcmu_dev_err(dev, "failed to send platform data\n");
        return -1;
    }

    return result;
}

uint32_t spdm_recv_response(struct tcmu_device *dev, const int socket,
                       uint32_t transport_type, void *rsp, uint32_t rsp_len)
{
    uint32_t command;
    bool result;

    result = receive_platform_data(socket, transport_type, &command,
                                   (uint8_t *)rsp, &rsp_len);
    if (!result) {
        tcmu_dev_err(dev, "failed to receive platform data\n");
        return 0;
    }

    assert(command != 0);

    return rsp_len;
}

void spdm_socket_close(struct tcmu_device *dev, const int socket,
                       uint32_t transport_type)
{
    if (socket < 0) {
        tcmu_dev_err(dev, "invalid socket: %d\n", socket);
    }

    send_platform_data(socket, transport_type,
                       SPDM_SOCKET_COMMAND_SHUTDOWN, NULL, 0);
}
