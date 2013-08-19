/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _ServiceManager_h_
#define _ServiceManager_h_ 1

#include "util/RadixTree.h"
#include "kern/Service.h"
#include "kern/ServiceFeatures.h"

/// \todo Integrate with the Event system somehow

/** Service Manager
 *
 *  The service manager controls Services in a central location, by allowing
 *  them to be referred to by name.
 *
 *  It also provides services such as enumeration of Service operations.
 */
class ServiceManager {
  ServiceManager() = delete;
  ServiceManager(const ServiceManager&) = delete;
  ServiceManager& operator=(const ServiceManager&) = delete;

  /** Internal representation of a Service */
  struct InternalService {
    /// The Service itself
    Service *pService;

    /// Service operation enumeration
    ServiceFeatures *pFeatures;
  };

  /** Services we know about */
  static RadixTree<InternalService *> m_Services;

public:
  /**
   *  Enumerates all possible operations that can be performed for a
   *  given Service
   */
  static ServiceFeatures *enumerateOperations(String serviceName);

  /** Adds a service to the manager */
  static void addService(String serviceName, Service *s, ServiceFeatures *feats);

  /** Removes a service from the manager */
  static void removeService(String serviceName);

  /** Gets the Service object for a service */
  static Service *getService(String serviceName);

};

#endif /*_ServiceManager_h_ */
