/*
 * Copyright (c) 2009 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_OSDEP_H_
#define _CDI_OSDEP_H_

struct cdi_driver;

/**
 * \german
 * Muss fuer jeden CDI-Treiber genau einmal verwendet werden, um ihn bei der
 * CDI-Bibliothek zu registrieren
 *
 * @param name Name des Treibers
 * @param drv Treiberbeschreibung (struct cdi_driver)
 * @param deps Liste von Namen anderer Treiber, von denen der Treiber abhaengt
 * \endgerman
 *
 * \english
 * CDI_DRIVER shall be used exactly once for each CDI driver. It registers
 * the driver with the CDI library.
 *
 * @param name Name of the driver
 * @param drv A driver description (struct cdi_driver)
 * @param deps List of names of other drivers on which this driver depends
 * \endenglish
 */
#define CDI_DRIVER(name, drvobj, deps...) /*\
  static void entry() {\
    (drvobj).drv.init();\
    cdi_kos_walk_dev_list_init((struct cdi_driver*)&(drvobj));\
  }\
  static void exit() {\
    cdi_kos_walk_dev_list_destroy((struct cdi_driver*)&(drvobj));\
    (drvobj).drv.destroy();\
  }\
  MODULE_INFO(name, &entry, &exit, deps)*/

/**
 * \english
 * This function walks the ilst of devices stored by a driver and initializes
 * each one. It is called when the module is executed, directly after driver
 * initialization.
 *
 * @param driver Driver struct containing the list of devices and function pointers.
 */
void cdi_kos_walk_dev_list_init(struct cdi_driver* dev);

/**
 * \english
 * This function walks the list of devices stored by a driver and destroys
 * each one. It is called when the module is unloaded, right before driver
 * destruction.
 *
 * @param driver Driver struct containing the list of devices and function pointers.
 */
void cdi_kos_walk_dev_list_destroy(struct cdi_driver* dev);

/**
 * \german
 * OS-spezifische Daten zu PCI-Geraeten
 * \endgerman
 * \english
 * OS-specific PCI data.
 * \endenglish
 */
typedef struct
{
} cdi_pci_device_osdep;

/**
 * \german
 * OS-spezifische Daten fuer einen ISA-DMA-Zugriff
 * \endgerman
 * \english
 * OS-specific DMA data.
 * \endenglish
 */
typedef struct
{
} cdi_dma_osdep;

/**
 * \german
 * OS-spezifische Daten fuer Speicherbereiche
 * \endgerman
 * \english
 * OS-specific data for memory areas.
 * \endenglish
 */
typedef struct {
} cdi_mem_osdep;

/**
 * \german
 * OS-spezifische Daten fuer Dateisysteme
 * \endgerman
 * \english
 * OS-specific data for file systems
 * \endenglish
 */
typedef struct {
} cdi_fs_osdep;

#endif
