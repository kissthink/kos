#include "types.h"

// forward decl
struct sk_buff;
struct net_device;
struct napi_struct;
struct Qdist;
struct delayed_work;
struct work_struct;
struct timer_list;
struct mutex;
struct lock_class_key;
struct skb_shared_hwtstamps;
struct page;
struct device;
struct pci_dev;
struct dql;
unsigned long jiffies;
unsigned long phys_base;

struct dma_map_ops;
struct dma_map_ops *dma_ops = 0;
struct in6_addr;
typedef struct raw_spinlock raw_spinlock_t;
unsigned long kernel_stack;
typedef unsigned short u16;
typedef u16 word;
struct pci_bus;
typedef unsigned long long u64;
struct device {};
struct device x86_dma_fallback_dev;
int cpu_number = 0;
struct pci_driver;
struct module;
struct ethtool_ts_info;

// kernel/panic.c
void warn_slowpath_null(const char *file, int line) {}

// kernel/printk.c
int printk(const char *fmt, ...) { return 0; }

// lib/find_next_bit.c
unsigned long find_first_bit(const unsigned long *addr, unsigned long size) { return 0; }
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
          unsigned long offset) { return 0; }

// net/ethernet/eth.c
__be16 eth_type_trans(struct sk_buff *skb, struct net_device *dev) { return 0; }
struct net_device *alloc_etherdev_mqs(int sizeof_priv, unsigned int txqs,
              unsigned int rxqs) { return 0; }
int eth_validate_addr(struct net_device *dev) { return 0; }

// net/core/dev.c
enum gro_result {
  GRO_MERGED,
  GRO_MERGED_FREE,
  GRO_HELD,
  GRO_NORMAL,
  GRO_DROP,
};
typedef enum gro_result gro_result_t;
gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb) { return GRO_MERGED; }
void __napi_schedule(struct napi_struct *n) {}
void __netif_schedule(struct Qdist *q) {}
void dev_kfree_skb_any(struct sk_buff *skb) {}
void unregister_netdev(struct net_device *dev) {}
void free_netdev(struct net_device *dev) {}
void napi_complete(struct napi_struct *n) {}
void netif_napi_add(struct net_device *dev, struct napi_struct *napi,
        int (*poll)(struct napi_struct *, int), int weight) {}
int register_netdev(struct net_device *dev) { return 0; }
void netif_device_detach(struct net_device *dev) {}
void netif_device_attach(struct net_device *dev) {}
int dev_close(struct net_device *dev) { return 0; }
int dev_open(struct net_device *dev) { return 0; }

// kernel/workqueue.c
typedef _Bool bool;
bool schedule_delayed_work(struct delayed_work *dwork, unsigned long delay) { return 0; }
bool cancel_work_sync(struct work_struct *work) { return 0; }
bool cancel_delayed_work_sync(struct delayed_work *dwork) { return 0; }
bool schedule_work(struct work_struct *work) { return 0; }
void delayed_work_timer_fn(unsigned long __data) {}

// kernel/irq/manage.c
void synchronize_irq(unsigned int irq) {}
void disable_irq(unsigned int irq) {}
void enable_irq(unsigned int irq) {}
enum irqreturn {
  IRQ_NONE        = (0 << 0),
  IRQ_HANDLED     = (1 << 0),
  IRQ_WAKE_THREAD = (1 << 1),
};
typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (*irq_handler_t)(int, void *);
int request_threaded_irq(unsigned int irq, irq_handler_t handler,
        irq_handler_t thread_fn, unsigned long irqflags,
        const char *devname, void *dev_id) { return 0; }
void free_irq(unsigned int irq, void *dev_id) {}

// kernel/timer.c
void msleep(unsigned int msecs) {}
void init_timer_key(struct timer_list *timer, unsigned int flags,
      const char *name, struct lock_class_key *key) {}
unsigned long msleep_interruptible(unsigned int msecs) { return 0; }

// kernel/mutex.c
void mutex_lock(struct mutex *lock) {}
void mutex_unlock(struct mutex *lock) {}
void __mutex_init(struct mutex *lock, const char *name, struct lock_class_key *key) {}

// include/linux/netdevice.h ??
int netdev_err(const struct net_device *dev, const char *format, ...) { return 0; }
int netdev_warn(const struct net_device *dev, const char *format, ...) { return 0; }
int netdev_info(const struct net_device *dev, const char *format, ...) { return 0; }

// net/core/skbuff.c
void skb_trim(struct sk_buff *skb, unsigned int len) {}
typedef unsigned __bitwise__ gfp_t;
struct sk_buff * __netdev_alloc_skb(struct net_device *dev,
          unsigned int length, gfp_t gfp_mask) { return 0; }
void consume_skb(struct sk_buff *skb) {}
int pskb_expand_head(struct sk_buff *skb, int nhead, int ntail,
          gfp_t gfp_mak) { return 0; }
void skb_tstamp_tx(struct sk_buff *orig_skb,
    struct skb_shared_hwtstamps *hwtstamps) {}
int skb_pad(struct sk_buff *skb, int pad) { return 0; }
unsigned char *__pskb_pull_tail(struct sk_buff *skb, int delta) { return 0; }
unsigned char *skb_put(struct sk_buff *skb, unsigned int len) { return 0; }
int ___pskb_trim(struct sk_buff *skb, unsigned int len) { return 0; }
struct sk_buff *__alloc_skb(unsigned int size, gfp_t gfp_mask,
          int flags, int node) { return 0; }

// include/linux/gfp.h
struct page *alloc_pages_current(gfp_t gfp_mask, unsigned order) { return 0; }

// mm/swap.c
void put_page(struct page *page) {}

// drivers/base/dd.c
void *dev_get_drvdata(const struct device *dev) { return 0; }
int dev_set_drvdata(struct device *dev, void *data) { return 0; }

// mm/slab.c or mm/slob.c or mm/slub.c
void kfree(const void *x) {}
typedef unsigned long size_t;
void *__kmalloc(size_t size, gfp_t flags) { return 0; }

// arch/x86/mm/ioremap.c
#ifndef __iomem
#define __iomem
#endif
void iounmap(volatile void __iomem *addr) {}
typedef u64 phys_addr_t;
typedef phys_addr_t resource_size_t;
void __iomem *ioremap_nocache(resource_size_t phys_addr, unsigned long size) { return 0; }

// drivers/pic/pic.c
void pci_release_selected_regions(struct pci_dev *pdev, int bars) {}
void pci_disable_device(struct pci_dev *dev) {}

// lib/dynamic_queue_limits.c
void dql_reset(struct dql *dql) {}
void dql_completed(struct dql *dql, unsigned int count) {}

// include/linux/device.h
int dev_err(const struct device *dev, const char *fmt, ...) { return 0; }
int _dev_info(const struct device *dev, const char *fmt, ...) { return 0; }
int dev_warn(const struct device *dev, const char *fmt, ...) { return 0; }

// net/core/utils.c
int net_ratelimit(void) { return 0; }

// arch/x86/lib/csum-wrappers_64.c or net/ipv6/ip6_checksum.c
__sum16 csum_ipv6_magic(const struct in6_addr *saddr,
      const struct in6_addr *daddr,
      __u32 len, unsigned short proto, __wsum sum) { return 0; }

// kernel/spinlock.c
unsigned long _raw_spin_lock_irqsave(raw_spinlock_t *lock) { return 0; }
void _raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags) {}
void _raw_spin_lock(raw_spinlock_t *lock) {}

// mm/vmalloc.c or mm/nommu.c
void vfree(const void *addr) {}
void *vzalloc(unsigned long size) { return 0; }

// drivers/pci/pci.c
int pci_enable_device(struct pci_dev *dev) { return 0; }
void pci_set_master(struct pci_dev *dev) {}
typedef int __bitwise pci_power_t;
int __pci_enable_wake(struct pci_dev *dev, pci_power_t state,
            bool runtime, bool enable) { return 0; }
int pci_enable_device_mem(struct pci_dev *dev) { return 0; }
int pci_select_bars(struct pci_dev *dev, unsigned long flags) { return 0; }
int pci_request_selected_regions(struct pci_dev *pdev, int bars,
          const char *res_name) { return 0; }
int pci_save_state(struct pci_dev *dev) { return 0; }
void __iomem *pci_ioremap_bar(struct pci_dev *pdev, int bar) { return 0; }
int pci_set_mwi(struct pci_dev *dev) { return 0; }
void pci_clear_mwi(struct pci_dev *dev) {}
int pci_wake_from_d3(struct pci_dev *dev, bool enable) { return 0; }
typedef int __bitwise pci_power_t;
int pci_set_power_state(struct pci_dev *dev, pci_power_t state) { return 0; }
int pci_prepare_to_sleep(struct pci_dev *dev) { return 0; }
void pci_restore_state(struct pci_dev *dev) {}
int pcix_get_mmrbc(struct pci_dev *dev) { return 0; }
int pcix_set_mmrbc(struct pci_dev *dev, int mmrbc) { return 0; }

// drivers/pci/access.c
int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int pos, word *value) { return 0; }

// arch/x86/kernel/pci-dma.c
int dma_set_mask(struct device *dev, u64 mask) { return 0; }
int dma_supported(struct device *dev, u64 mask) { return 0; }

// drivers/base/power/wakeup.c
int device_set_wakeup_enable(struct device *dev, bool enable) { return 0; }



// net/sched/sch_generic.c
void netif_carrier_off(struct net_device *dev) {}
void netif_carrier_on(struct net_device *dev) {}


// lib/hexdump.c
void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
        int rowsize, int groupsize,
        const void *buf, size_t len, bool ascii) {}

// kernel/softirq.c
void local_bh_disable(void) {}
void local_bh_enable(void) {}

// arch/x86/um/delay.c or arch/x86/lib/delay.c
void __const_udelay(unsigned long xloops) {}
void __udelay(unsigned long usecs) {}

// include/linux/kernel.h
enum system_states {
  SYSTEM_BOOTING,
  SYSTEM_RUNNING,
  SYSTEM_HALT,
  SYSTEM_POWER_OFF,
  SYSTEM_RESTART,
} system_state;

// net/core/rtnetlink.c
int rtnl_is_locked(void) { return 0; }

// lib/dump_stack.c
void dump_stack(void) {}

// lib/iomap.c
void ioread16_rep(void __iomem *addr, void *dst, unsigned long count) {}
void iowrite16_rep(void __iomem *addr, const void *src, unsigned long count) {}

// drivers/pci/pci-driver.c
void pci_unregister_driver(struct pci_driver *drv) {}
void __pci_register_driver(struct pci_driver *drv, struct module *owner,
        const char *mod_name) {}

// net/core/ethtool.c
int ethtool_op_get_ts_info(struct net_device *dev, struct ethtool_ts_info *info) { return 0; }

// kernel/params.c
struct kernel_param;
struct kernel_param_ops {};
struct kernel_param_ops param_ops_int = {};
struct kernel_param_ops param_ops_uint = {};
struct kernel_param_ops param_array_ops = {};
struct module_attribute;
struct module_kobject;
typedef long ssize_t;
ssize_t __modver_version_show(struct module_attribute *mattr,
          struct module_kobject *mk, char *buf) { return 0; }
