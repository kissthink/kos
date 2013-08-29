#include "cdi.h"
#include "cdi/net.h"
#include "cdi/printf.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"
#include "net/tcp.h"

// KOS
#include "kern/Debug.h"
#include "util/basics.h"

static unsigned long netcard_highest_id = 0;
static cdi_list_t netcard_list = nullptr;
//static cdi_list_t ethernet_packet_receivers;

void cdi_net_driver_init(struct cdi_net_driver* driver) {
  driver->drv.type = CDI_NETWORK;
  cdi_driver_init(reinterpret_cast<cdi_driver*>(driver));
  if (netcard_list == nullptr) {
    netcard_list = cdi_list_create();
  }
//  ethernet_packet_receivers = cdi_list_create();
}

void cdi_net_driver_destroy(struct cdi_net_driver* driver) {
  cdi_driver_destroy(reinterpret_cast<cdi_driver*>(driver));
}

void cdi_net_device_init(struct cdi_net_device* device) {
  device->number = netcard_highest_id;
  ++netcard_highest_id;
  cdi_list_push(netcard_list, device);
}

cdi_net_device* cdi_net_get_device(int num) {
  for (size_t i = 0; i < cdi_list_size(netcard_list); i++) {
    cdi_net_device* dev = (cdi_net_device *) cdi_list_get(netcard_list, i);
    if (dev->number == num) return dev;
  }
  return nullptr;
}

void printPayload(ptr_t buffer, size_t size) {
  ptr_t payload = (char *)buffer + sizeof(tcp_header);
  size -= sizeof(tcp_header);
  CdiPrintf("Payload: ");
  for (size_t i = 0; i < size; i++) {
    CdiPrintf("%02x", ((unsigned char *)payload)[i]);
  }
  CdiPrintf("\n");
}

void parseIpPacket(cdi_net_device* device, ptr_t buffer, size_t size) {
  ip_header* header = (ip_header *) buffer;
  unsigned char bytes[4];
  bytes[0] = header->source_ip & 0xff;
  bytes[1] = (header->source_ip >> 8) & 0xff;
  bytes[2] = (header->source_ip >> 16) & 0xff;
  bytes[3] = (header->source_ip >> 24) & 0xff;
  CdiPrintf("Source IP: %d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);
  bytes[0] = header->dest_ip & 0xff;
  bytes[1] = (header->dest_ip >> 8) & 0xff;
  bytes[2] = (header->dest_ip >> 16) & 0xff;
  bytes[3] = (header->dest_ip >> 24) & 0xff;
  CdiPrintf("Dest IP: %d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);

//  printPayload((char *)buffer + sizeof(ip_header), size - sizeof(ip_header));
}

void parseArpPacket(cdi_net_device* device, ptr_t buffer, size_t size) {
  DBG::outln(DBG::CDI, "Received ARP Packet");
}

void cdi_net_receive(cdi_net_device* device, ptr_t buffer, size_t size) {
  CdiPrintf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x received\n",
      device->mac & 0xff,
      (device->mac & 0xff00) >> 8,
      (device->mac & 0xff0000) >> 16,
      (device->mac & 0xff000000) >> 24,
      (device->mac & 0xff00000000) >> 32,
      (device->mac & 0xff0000000000) >> 40);
  eth_packet_header* eth = (eth_packet_header *) buffer;
  ptr_t subpacket = (void *)((char *)eth + sizeof(eth_packet_header));
  size_t subsize = size - sizeof(eth_packet_header);
  switch (eth->type) {
    case ETH_TYPE_IP:
      parseIpPacket(device, subpacket, subsize); break;
    case ETH_TYPE_ARP:
      parseArpPacket(device, subpacket, subsize); break;
  }
}

void cdi_net_send(int num, ptr_t data, size_t size) {
  cdi_net_device* dev = cdi_net_get_device(num);
  if (!dev) return;
  cdi_net_driver* driver = (cdi_net_driver *) dev->dev.driver;
  driver->send_packet(dev, data, size);
}

void cdi_net_send(ptr_t data, size_t size) {
  cdi_net_device* dev = nullptr;
  for (size_t i = 0; i < cdi_list_size(netcard_list); i++) {
    dev = (cdi_net_device *) cdi_list_get(netcard_list, i);
    if (dev) break;
  }
  if (!dev) return;
  cdi_net_driver* driver = (cdi_net_driver *) dev->dev.driver;
  driver->send_packet(dev, data, size);
}
