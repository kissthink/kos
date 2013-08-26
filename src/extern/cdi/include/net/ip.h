/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IP_H
#define IP_H

#include <stdint.h>

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

struct routing_entry {
    uint32_t   target;
    uint32_t   subnet;
    uint32_t   gateway;
    struct device *device;
};

struct ip_header {
    // Nibbles vertauscht, weil Big Endian
    uint8_t     ihl : 4;
    uint8_t     version : 4;

    uint8_t     type_of_service;
    uint16_t    total_length;
    uint16_t    id;

    uint16_t    fragment_offset;

    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    checksum;
    uint32_t    source_ip;
    uint32_t    dest_ip;
} __attribute__ ((packed));

#define IP_FLAG_MORE_FRAGMENTS (1 << 13)
#define IP_FLAG_DONT_FRAGMENT (1 << 14)

void init_ip(void);

void add_routing_entry(struct routing_entry* entry);
struct routing_entry* get_routing_entry(uint32_t ip);
struct routing_entry* enum_routing_entry(size_t i);
size_t routing_get_entry_count(void);
void remove_device_routes(struct device* device);

bool ip_send(uint32_t dest_ip, uint8_t proto, void* data, size_t data_size);
bool ip_send_route(uint32_t dest_ip, uint8_t proto, void* data,
    size_t data_size, struct routing_entry* route);

bool ip_send_packet(struct ip_header* header, void* data, uint32_t size);
bool ip_send_packet_route(struct ip_header* header, void* data, uint32_t size,
    struct routing_entry* route);

/**
 * IP-Paket versenden. Dabei wird das Routing umgangen und direkt die angegebene
 * Netzwerkkarte benutzt.
 */
bool ip_send_direct(struct device* device, uint32_t dest_ip, uint8_t proto,
    void* data, size_t data_size);


void ip_receive(struct device *device, void* data, size_t data_size);

uint16_t ip_checksum(void* data, size_t size);
void ip_update_checksum(struct ip_header* header);

#endif
