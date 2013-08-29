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

#ifndef _TCP_H_
#define _TCP_H_

#include <stdint.h>

#define TCPS_WAIT_FOR_SYN_ACK 1
#define TCPS_CONNECTED 2
#define TCPS_WAIT_FOR_LAST_ACK 3
#define TCPS_CONNECTED_FIN 4
#define TCPS_CLOSED 5
#define TCPS_WAIT_FOR_ACCEPT 6
#define TCPS_WAIT_FOR_FIN_ACK 7

#define TCPF_FIN    (1 << 0)
#define TCPF_SYN    (1 << 1)
#define TCPF_RST    (1 << 2)
#define TCPF_PSH    (1 << 3)
#define TCPF_ACK    (1 << 4)
#define TCPF_URG    (1 << 5)

struct tcp_header {
    uint16_t    source_port;
    uint16_t    dest_port;
    uint32_t    seq;
    uint32_t    ack;

    uint8_t     reserved : 4;
    uint8_t     data_offset : 4;

    uint8_t     flags;
    uint16_t    window;
    uint16_t    checksum;
    uint16_t    urgent_ptr;
} __attribute__ ((packed));

struct tcp_pseudo_header {
    uint32_t    source_ip;
    uint32_t    dest_ip;
    uint16_t    proto;
    uint16_t    length;
} __attribute__ ((packed));

struct tcp_in_buffer {
    uint32_t    seq;
    size_t      size;
    void*       data;
};

struct tcp_out_buffer {
    size_t  size;
    void*   data;
};

#endif
