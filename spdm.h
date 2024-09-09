#ifndef SPDM_H
#define SPDM_H

int spdm_socket_connect(struct tcmu_device *, uint16_t port);
void spdm_socket_close(struct tcmu_device *dev, const int socket,
                       uint32_t transport_type);
uint32_t spdm_socket_receive(struct tcmu_device *dev, const int socket,
                       uint32_t trasnport_type, void *rsp, uint32_t rsp_len);
int spdm_socket_send(struct tcmu_device *dev, const int socket,
                     uint32_t socket_cmd, uint32_t transport_type,
                     void *req, uint32_t req_len);
/*
 * Defines the storage transport encoding for SPDM, this information shall be
 * passed down to the SPDM server, when conforming to the SPDM over Storage
 * standard as defined by DSP0286.
 */
struct storage_spdm_transport_hdr {
    uint8_t security_protocol;
    uint16_t security_protocol_specific;
    bool inc_512;
    uint32_t length;
} __attribute__((__packed__));

#define SPDM_SOCKET_COMMAND_NORMAL                0x0001
#define SPDM_SOCKET_STORAGE_CMD_IF_SEND           0x0002
#define SPDM_SOCKET_STORAGE_CMD_IF_RECV           0x0003
#define SOCKET_SPDM_STORAGE_ACK_STATUS            0x0004
#define SPDM_SOCKET_COMMAND_OOB_ENCAP_KEY_UPDATE  0x8001
#define SPDM_SOCKET_COMMAND_CONTINUE              0xFFFD
#define SPDM_SOCKET_COMMAND_SHUTDOWN              0xFFFE
#define SPDM_SOCKET_COMMAND_UNKOWN                0xFFFF
#define SPDM_SOCKET_COMMAND_TEST                  0xDEAD

#define SPDM_SOCKET_TRANSPORT_TYPE_SCSI           0x03

#define SPDM_SOCKET_MAX_MESSAGE_BUFFER_SIZE       0x2000
#define SPDM_SOCKET_MAX_MSG_STATUS_LEN            0x02

#endif
