#include "arp.h"

#include "ethernet.h"
#include "net.h"

#include <stdio.h>
#include <string.h>
/**
 * @brief 初始的arp包
 *
 */
static const arp_pkt_t arp_init_pkt = {
    .hw_type16 = swap16(ARP_HW_ETHER),
    .pro_type16 = swap16(NET_PROTOCOL_IP),
    .hw_len = NET_MAC_LEN,
    .pro_len = NET_IP_LEN,
    .sender_ip = NET_IF_IP,
    .sender_mac = NET_IF_MAC,
    .target_mac = {0}};

/**
 * @brief arp地址转换表，<ip,mac>的容器
 *
 */
map_t arp_table;

/**
 * @brief arp buffer，<ip,buf_t>的容器
 *
 */
map_t arp_buf;

/**
 * @brief 打印一条arp表项
 *
 * @param ip 表项的ip地址
 * @param mac 表项的mac地址
 * @param timestamp 表项的更新时间
 */
void arp_entry_print(void *ip, void *mac, time_t *timestamp) {
    printf("%s | %s | %s\n", iptos(ip), mactos(mac), timetos(*timestamp));
}

/**
 * @brief 打印整个arp表
 *
 */
void arp_print() {
    printf("===ARP TABLE BEGIN===\n");
    map_foreach(&arp_table, arp_entry_print);
    printf("===ARP TABLE  END ===\n");
}

/**
 * @brief 发送一个arp请求
 *
 * @param target_ip 想要知道的目标的ip地址
 */
void arp_req(uint8_t *target_ip) {
    // TO-DO
    buf_init(&txbuf, sizeof(arp_pkt_t));
    // 初始化ARP包
    arp_pkt_t* packet = (arp_pkt_t*) txbuf.data;
    // 设置opcode
    packet->opcode16 = swap16(ARP_REQUEST);
    // 填写目标IP
    memcpy(packet->target_ip, target_ip, NET_IP_LEN);
    // 设置广播mac地址
    uint8_t mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ethernet_out(&txbuf, mac, NET_PROTOCOL_ARP);
}

/**
 * @brief 发送一个arp响应
 *
 * @param target_ip 目标ip地址
 * @param target_mac 目标mac地址
 */
void arp_resp(uint8_t *target_ip, uint8_t *target_mac) {
    // TO-DO
    buf_init(&txbuf, sizeof(arp_pkt_t));
    // 初始化ARP包
    arp_pkt_t* packet = (arp_pkt_t*)txbuf.data;
    // 设置opcode
    packet->opcode16 = swap16(ARP_REPLY);
    // 填写目标IP
    memcpy(packet->target_ip, target_ip, NET_IP_LEN);
    // 填写目标MAC
    memcpy(packet->target_mac, target_mac, NET_MAC_LEN);
    ethernet_out(&txbuf, target_mac, NET_PROTOCOL_ARP);
}

/**
 * @brief 处理一个收到的数据包
 *
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void arp_in(buf_t *buf, uint8_t *src_mac) {
    // TO-DO
    if (buf->len < sizeof(arp_pkt_t)) {
        return;
    }
    // 报头检查
    arp_pkt_t *packet = (arp_pkt_t *)buf->data;
    if (packet->hw_type16 != swap16(ARP_HW_ETHER) ||
        packet->pro_type16!= swap16(NET_PROTOCOL_IP) ||
        packet->hw_len    != NET_MAC_LEN ||
        packet->pro_len   != NET_IP_LEN ||
        ((swap16(packet->opcode16) != ARP_REQUEST) && (swap16(packet->opcode16) != ARP_REPLY))) {
        return;
    }
    // 更新ARP表
    map_set(&arp_table, packet->sender_ip, packet->sender_mac);
    // 查看缓存情况
    if (map_get(&arp_buf, packet->sender_ip) == NULL) {
        if (swap16(packet->opcode16) == ARP_REQUEST && memcmp(packet->target_ip, net_if_ip, NET_IP_LEN) == 0) {
            // 回应
            arp_resp(packet->sender_ip, packet->sender_mac);
        }
    } else {
        // 缓存中有包
        ethernet_out(map_get(&arp_buf, packet->sender_ip), src_mac, NET_PROTOCOL_IP);
        // 删除缓存
        map_delete(&arp_buf, packet->sender_ip);
    }
}

/**
 * @brief 处理一个要发送的数据包
 *
 * @param buf 要处理的数据包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void arp_out(buf_t *buf, uint8_t *ip) {
    // TO-DO
    uint8_t* mac = map_get(&arp_table, ip);
    if (mac == NULL) {
        // 未找到mac地址，发送arp请求
        if (map_get(&arp_buf, ip) == NULL) {
            // 没有包，发送arp请求
            map_set(&arp_buf, ip, buf);
            arp_req(ip);
        } else {
            return;
        }
    } else {
        ethernet_out(buf, mac, NET_PROTOCOL_IP);
    }
}

/**
 * @brief 初始化arp协议
 *
 */
void arp_init() {
    map_init(&arp_table, NET_IP_LEN, NET_MAC_LEN, 0, ARP_TIMEOUT_SEC, NULL, NULL);
    map_init(&arp_buf, NET_IP_LEN, sizeof(buf_t), 0, ARP_MIN_INTERVAL, NULL, buf_copy);
    net_add_protocol(NET_PROTOCOL_ARP, arp_in);
    arp_req(net_if_ip);
}