static void configure_first(void *);
static void configure(void *);
static void configure_final(void *);

SYSINIT(configure1, SI_SUB_CONFIGURE, SI_ORDER_FIRST, configure_first, NULL);
SYSINIT(configure2, SI_SUB_CONFIGURE, SI_ORDER_THIRD, configure, NULL);
SYSINIT(configure3, SI_SUB_CONFIGURE, SI_ORDER_ANY, configure_final, NULL);

/**
 * Determine I/O configuration for a machine.
 */
static void configure_first(void *dummy) {
  /* nexus0 is the top of the amd64 device tree */
//  device_add_child(root_bus, "nexus", 0);
}

static void configure(void *dummy) {
  /**
   * Enable interrupts on the processor. The interrupts are still
   * disabled in the interrupt controllers until interrupt handlers
   * are registered.
   */
  //enable_intr();

  /* initialize newbus architecture */
//  root_bus_configure();
}

static void configure_final(void *dummy) {
  // cninit_finish();
  // if (bootverbose)
  //  printf("Device configuration finished.\n");
  // cold = 0;
}
