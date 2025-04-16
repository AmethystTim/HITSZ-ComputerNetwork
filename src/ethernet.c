#include "ethernet.h"

#include "arp.h"
#include "driver.h"
#include "ip.h"
#include "utils.h"
/**
 * @brief 处理一个收到的数据包
 *
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf) {
    // TO-DO
    if (buf->len < sizeof(ether_hdr_t)) {
        return;
    }
    // 获取以太网包头
    ether_hdr_t *hdr = (ether_hdr_t *)buf->data;
    net_protocol_t protocol = hdr->protocol16;
    protocol = swap16(protocol);
    // 移除以太网包头
    buf_remove_header(buf, sizeof(ether_hdr_t));
    net_in(buf, protocol, hdr->src);
}
/**
 * @brief 处理一个要发送的数据包
 *
 * @param buf 要处理的数据包
 * @param mac 目标MAC地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol) {
    // TO-DO
    if (buf->len < ETHERNET_MIN_TRANSPORT_UNIT) {
        // 填充以太网头部
        if (buf_add_padding(buf, ETHERNET_MIN_TRANSPORT_UNIT - buf->len) != 0){
            printf("buf_add_padding error\n");
            return;
        }
    }
    // 添加以太网包头
    if (buf_add_header(buf, sizeof(ether_hdr_t)) != 0){
        printf("buf_add_header error\n");
        return;
    }
    ether_hdr_t *hdr = (ether_hdr_t *)buf->data;
    // 设置目标MAC地址
    memcpy(hdr->dst, mac, NET_MAC_LEN);
    // 设置源MAC地址
    memcpy(hdr->src, net_if_mac, NET_MAC_LEN);
    // 填写协议类型
    hdr->protocol16 = swap16(protocol);
    // 发送数据帧
    driver_send(buf);
}
/**
 * @brief 初始化以太网协议
 *
 */
void ethernet_init() {
    buf_init(&rxbuf, ETHERNET_MAX_TRANSPORT_UNIT + sizeof(ether_hdr_t));
}

/**
 * @brief 一次以太网轮询
 *
 */
void ethernet_poll() {
    if (driver_recv(&rxbuf) > 0)
        ethernet_in(&rxbuf);
}
