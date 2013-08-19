/**
 * \mainpage
 * \german
 * Die CDI-Dokumentation ist auch auf <a target="_top"
 * href="http://lpt.tyndur.org/cdi/english/">englisch</a> verfügbar.<br>
 * The CDI documentation is also available in <a target="_top"
 * href="http://lpt.tyndur.org/cdi/english/">English</a>.
 *
 * \section main_introduction Einführung
 * Das Common Driver Interface (oder kurz CDI) ist ein Standardinterface,
 * das es ermöglicht, betriebssystemunabhängige Treiber zu schreiben.
 *
 * Dies funktioniert mithilfe einer Bibliothek, die die Brücke zwischen
 * Treiber und Betriebssystem bildet. CDI definiert die Datenstrukturen und
 * Funktionsprotoypen dieser Bibliothek, während ihre Implementierung
 * OS-spezifisch ist. CDI-komforme Treiber können ohne Änderungen gegen diese
 * Bibliothek gebaut werden und stehen damit jedem Betriebssystem zur
 * Verfügung, das CDI implementiert.
 *
 * Die folgenden Seiten seien zum Einstieg in die technischen Details
 * empfohlen:
 * - \ref general_interface
 * - \ref core "Das Modul Core"
 *
 * \section main_resources Ressourcen
 * Der Quellcode der Treiber und die Headerdatei des Interfaces können aus dem
 * git-Repository heruntergeladen werden:
 * - http://git.tyndur.org/?p=cdi.git;a=tree
 * - git://git.tyndur.org/cdi.git
 *
 * Für CDI existiert eine Mailingliste, auf der die Weiterentwicklung
 * diskutiert wird. Patches können ebenfalls an diese Mailingliste gesendet
 * werden. Für die Mailingliste bitte bevorzugt Englisch verwenden.
 * - cdi-devel@tyndur.org
 * - http://list.tyndur.org/cgi-bin/mailman/listinfo/cdi-devel
 *
 * Auch im IRC sind CDI-Entwickler und -Benutzer vertreten:
 * - \#tyndur auf irc.euirc.net
 * \endgerman
 * \english
 * Die CDI-Dokumentation ist auch auf <a target="_top"
 * href="http://lpt.tyndur.org/cdi/">deutsch</a> verfügbar.<br>
 * The CDI documentation is also available in <a target="_top"
 * href="http://lpt.tyndur.org/cdi/">German</a>.
 *
 * \section main_introduction Introduction
 * The Common Driver Interface (or CDI) is a standard interface that allows to
 * write OS independent drivers.
 *
 * To achieve this it employs a library that works as a bridge between
 * drivers and the OS. CDI defines things like data structures and function
 * prototypes of this library, whereas its implementation is OS specific.
 * Drivers conforming to the CDI can be built against this library without any
 * changes, so they can be used in any operating system implementing CDI.
 *
 * The following pages are recommended for an introduction into the technical
 * details:
 * - \ref general_interface
 * - \ref core "The module Core"
 *
 * \section main_resources Resources
 * The source code of drivers and the header files of the interface can be
 * downloaded from the git repository:
 * - http://git.tyndur.org/?p=cdi.git;a=tree
 * - git://git.tyndur.org/cdi.git
 *
 * For CDI there is a mailing list on which the further development is
 * discussed. It's also the right place to send patches to.
 * - cdi-devel@tyndur.org
 * - http://list.tyndur.org/cgi-bin/mailman/listinfo/cdi-devel
 *
 * You'll also find CDI developers and users on IRC:
 * - \#tyndur on irc.euirc.net
 * \endenglish
 */

/**
 * \german
 * \page general_interface Allgemeine Schnittstellenbeschreibung
 *
 * Diese Seite beschreibt allgemeine Regeln für das Zusammenspiel von Treibern
 * und Betriebssystem, die nicht einem bestimmtem Modul oder einer bestimmten
 * Datenstruktur zugeordnet werden können.
 *
 * Die zwei wesentlichen Teile sind Bedingungen, die das Betriebssystem auf
 * Quellcode-Ebene garantieren muss (z.B. Bereitstellung von bestimmten
 * Funktionen) und Bedingungen, die zur Laufzeit gelten.
 *
 * \section gi_source Quellcode-Ebene
 * \subsection gi_source_naming Namensregeln
 * Um Namenskonflikte zu vermeiden, sind folgende Regeln zu beachten:
 * - Öffentliche Namen der CDI-Implementierung (d.h. global sichtbare Namen
 *   und in CDI-Headerdateien verwendete Namen) beginnen mit cdi_ oder CDI_.
 * - Öffentliche Namen eines CDI-Treibers beginnen mit dem Treibernamen
 *   gefolgt von einem Unterstrich (z.B. e1000_send_packet)
 *
 * \subsection gi_source_libc Funktionen der C-Standardbibliothek
 * Die CDI-Implementierung muss keine vollständige C-Standardbibliothek zur
 * Verfügung stellen. Treiber müssen sich auf folgende Teile beschränken
 * (oder die Benutzung weiterer Funktionen durch \#ifdef optional machen):
 * - Alles aus stddef.h
 * - Alles aus stdint.h
 * - Alles aus string.h
 * - Speicherverwaltungsfunktionen aus stdlib.h: malloc, calloc, realloc, free
 * - printf-Familie aus stdio.h, ohne fprintf und vfprintf. Außerdem asprintf.
 *   \todo Gebrauch von stdio.h einschränken
 *
 * \section gi_runtime Laufzeit
 * \subsection gi_runtime_intr Nebenläufigkeit und Unterbrechungen
 * Nebenläufigkeit ist das gleichzeitige oder quasi-gleichzeitige Ablaufen von
 * Code (z.B. Multithreading). Eine Unterbrechung bedeutet, dass ausgeführter
 * Code angehalten wird, um anderen Code auszuführen, und erst anschließend
 * der angehaltene Code wieder fortgesetzt wird (z.B. Unix-Signale).
 *
 * Die Nutzung beider Konzepte erfordert es, dass der Quellcode sich gegen
 * unerwünschte Effekt wie Race Conditions schützt, z.B. durch Locks. Da die
 * jeweiligen Methoden OS-abhängig sind und um Treiber einfach zu halten,
 * gelten für CDI die folgenden Einschränkungen:
 *
 * - Treibercode wird niemals unterbrochen. Die einzige Ausnahme ist die
 *   Funktion cdi_wait_irq, während der IRQ-Handler ausgeführt werden
 *   dürfen.
 * - Nebenläufigkeit ist erlaubt, sofern die CDI-Implementierung sicherstellt,
 *   dass für jedes Gerät gleichzeitig nur eine Funktion ausgeführt wird
 *   (z.B. durch ein Lock auf das cdi_device). Treiber dürfen nicht davon
 *   ausgehen, dass irgendwelche Funktionen nebenläufig ausgeführt werden.
 *   Nebenläufigkeit ist für die Implementierung optional.
 * \endgerman
 */

/**
 * \english
 * \page general_interface General interface description
 *
 * This page describes general rules for the interaction between drivers and
 * the operating system which are not related to just one specific module or
 * data structure.
 *
 * The two major parts of it are conditions that the operating systems needs to
 * guarantee on the source code level (e.g. availability of certain functions)
 * and runtime conditions.
 *
 * \section gi_source Source code
 * \subsection gi_source_naming Naming
 * In order to avoid name space conflicts, the following rules should be
 * applied:
 * - Public names of the CDI implementation (i.e. globally visible symbol names
 *   and names used in CDI header files) start with cdi_ or CDI_.
 * - Public names of a CDI driver start with the driver name followed by an
 *   underscore (e.g. e1000_send_packet)
 *
 * \subsection gi_source_libc Functions of the C standard library
 * The CDI implementation doesn't have to provide a complete C standard
 * library. Drivers are limited to following parts of the library, which the
 * CDI implementation must provide:
 * - Everything in stddef.h
 * - Everything in stdint.h
 * - Everything in string.h
 * - Memory management functions in stdlib.h: malloc, calloc, realloc, free
 * - printf family in stdio.h, without fprintf and vfprintf. Additionally
 *   asprintf.
 *   \todo Further restrictions for stdio.h
 *
 * \section gi_runtime Runtime
 * \subsection gi_runtime_intr Concurrency and interruptions
 * Concurrency means (seemingly) parallel execution of code (e.g.
 * multithreading). An interruption means that running code is stopped in order
 * to execute different code, and only afterwards the stopped code is continued
 * (e.g. Unix signals).
 *
 * Both concepts require that source code is protected against unwanted effects
 * like race conditions, e.g. by locks. The respective method is OS dependent,
 * and to keep drivers simple, for CDI the following restrictions are made:
 *
 * - Driver code is never interrupted. The only exception is the function
 *   cdi_wait_irq: While it is running, IRQ handler may be executed.
 * - The use of threading is allowed if the CDI implementation ensures that for
 *   each device only one function is executed at the same time (e.g. by
 *   putting a lock in cdi_device). Drivers may not assume that threading is
 *   used, it is optional for the CDI implementation.
 * \endenglish
 */
