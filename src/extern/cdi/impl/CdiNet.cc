#include "cdi/net.h"
#include "cdi.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"

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
  for (int i = 0; i < cdi_list_size(netcard_list); i++) {
    cdi_net_device* dev = (cdi_net_device *) cdi_list_get(netcard_list, i);
    if (dev->number == num) return dev;
  }
  return nullptr;
}

void parseIpPacket(cdi_net_device* device, ptr_t buffer, size_t size) {
  ip_header* header = (ip_header *) buffer;
  DBG::outln(DBG::CDI, "Source IP: ", FmtHex(header->source_ip));
  DBG::outln(DBG::CDI, "Dest IP: ", FmtHex(header->dest_ip));
}

void parseArpPacket(cdi_net_device* device, ptr_t buffer, size_t size) {
  arp* arp_header = (arp *) buffer;
  DBG::outln(DBG::CDI, "Received ARP Packet");
}

void cdi_net_receive(cdi_net_device* device, ptr_t buffer, size_t size) {
  DBG::outln(DBG::CDI, "MAC: ", FmtHex(device->mac), " received:");
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
