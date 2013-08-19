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

#include "cdi.h"


typedef struct {
        struct list_node* anchor;
            unsigned long size;
} list_t;

extern "C"
{
list_t* list_create(void);
void list_destroy(list_t* list);
list_t* list_push(list_t* list, void* value);
void* list_pop(list_t* list);
int list_is_empty(list_t* list);
void* list_get_element_at(list_t* list, int index);
list_t* list_insert(list_t* list, int index, void* value);
void* list_remove(list_t* list, int index);
unsigned long list_size(list_t* list);
};

struct list_node {
    struct list_node* next;
    void* value;
};

typedef unsigned long dword;

/* 
 * Die folgenden Variablen werden f�r ein minimales Caching benutzt 
 * (beschleunigt sequentiellen Zugriff)
 */
static list_t* last_list = NULL;
static int last_index = 0;
static struct list_node* last_node = NULL;
    

/**
 * Erzeugt eine neue Liste.
 *
 * @return Pointer auf die neue Liste
 */
list_t* list_create() 
{
    list_t* list = new list_t;
    list->anchor = NULL;
    list->size   = 0;

    return list;
}

/**
 * Zerstoert eine Liste
 */
void list_destroy(list_t* list)
{
    // Alle Elemente loeschen
    while (list_pop(list) != NULL);

    // Listenhandle freigeben
    delete list;
}

/**
 * @return TRUE, wenn die Liste leer ist, FALSE sonst.
 */
int list_is_empty(list_t* list)
{
    return ((!list) || (list->anchor == NULL)) ? 1 : 0;
}

/**
 * @return Anzahl der Elemente in der Liste
 */
dword list_size(list_t* list)
{
    if (!list || !list->anchor) {
        return 0;
    } else {
        return list->size;
    }
}

/**
 * F�gt ein neues Element an den Anfang einer Liste ein.
 *
 * @param list Liste, in die das Element eingef�gt werden soll
 * @param value Einzuf�gendes Element
 *
 * @return Pointer auf die erg�nzte Liste oder NULL, wenn ein Nullpointer
 * als Liste �bergeben wird.
 */
list_t* list_push(list_t* list, void* value)
{
    if (!list) {
        return NULL;
    }

    struct list_node* element = new list_node;
    element->value = value;
    element->next = list->anchor;
    list->anchor = element;
    list->size++;

    if (last_list == list) {
        last_index++;
    }
    
    return list;
}

/**
 * Gibt das erste Element der Liste zur�ck und l�scht es aus ihr.
 *
 * @param list Liste, aus der das Element entfernt werden soll
 *
 * @return Der Wert des entfernten Elements oder NULL, wenn als Liste ein
 * Nullpointer �bergeben wurde oder die Liste leer ist.
 */
void* list_pop(list_t* list)
{
    struct list_node* old_anchor;
    void* value;
    
    if (!list) {
        return NULL;
    }
    
    if (list->anchor) {
        value = list->anchor->value;

        old_anchor = list->anchor;
        list->anchor = list->anchor->next;
        delete old_anchor;
    
        list->size--;
    } else {
        value = NULL;
    }
    
    if (last_list == list) {
        if (last_index == 0) {
            last_list = NULL;
        } else {
            last_index--;
        }
    }

    return value;
}
    
/**
 * Gibt einen Knoten aus der Liste zur�ck. Wird von allen Funktionen benutzt,
 * die �ber einen Index auf ein Listenelement zugreifen m�ssen.
 *
 * Um sequentielle Zugriffe zu beschleunigen, wird der jeweils zuletzt
 * zur�ckgegebene Knoten in einer statischen Variablen gespeichert. Wenn 
 * m�glich, wird mit der Suche dann beim n�chsten Aufruf nicht am Anker der
 * Liste, sondern am zuletzt zur�ckgegebenen Knoten begonnen.
 *
 * @param list Liste, deren Knoten zur�ckgegeben werden soll
 * @param index Index des Knotens in der Liste. Der erste Knoten besitzt den
 * Index 0.
 *
 * @return Knoten an der gegebenen Position oder NULL, wenn als Liste ein
 * Nullpointer �bergeben wurde oder kein Element mit dem �bergebenen Index
 * in der Liste existiert.
 */
static struct list_node* list_get_node_at(list_t* list, int index)
{
    if (!list || !list->anchor || (index < 0)) {
        return NULL;
    }
    //Chaching deaktivieren
    //last_list = NULL;
    struct list_node* current = list->anchor;
    int n = index;

    if (last_list && (last_list == list) && (last_index <= index) && (last_node != NULL)) {
        n -= last_index;
        current = last_node;
    }
    
    while (n--) {
        current = current->next;
        if (!current) {
            return NULL;
        }
    }

    last_list = list;
    last_index = index;
    last_node = current;

    return current;
}

/**
 * Fragt den Wert eines Listenelements ab.
 *
 * @param list Liste, deren Element abgefragt werden soll
 * @param index Index des abzufragenden Elements (das erste Element
 * hat den Index 0)
 * 
 * @return Der Wert des Elements oder NULL, wenn als Liste ein
 * Nullpointer �bergeben wurde oder in der Liste kein Element mit dem
 * �bergebenen Index existiert.
 */
void* list_get_element_at(list_t* list, int index)
{
    struct list_node* node = list_get_node_at(list, index);
    if (node) {
        return node->value;
    } else {
        return NULL;
    }
}

/**
 * F�gt ein neues Listenelement an eine gegebene Position ein. Die
 * Indizes der nachfolgenden Elemente werden beim einf�gen des neues
 * Elements um eins verschoben.
 *
 * @param list Liste, in die das Element eingef�gt werden soll
 * @param index Index, den das neu eingef�gte Element haben soll
 * @param value Wert des einzuf�genden Elements
 *
 * @return Liste mit dem eingef�gten Element oder NULL, wenn als Liste
 * ein Nullpointer �bergeben wurde oder der Index gr��er als die Anzahl
 * der Elemente in der Liste ist
 */
list_t* list_insert(list_t* list, int index, void* value)
{
    struct list_node* new_node = new list_node;
    new_node->value = value;

    if (!list) {
        return NULL;
    }

    if (index) {
        struct list_node* prev_node = list_get_node_at(list, index - 1);

        if (!prev_node) {
            return NULL;
        }

        new_node->next = prev_node->next;
        prev_node->next = new_node;
    } else {
        new_node->next = list->anchor;
        list->anchor = new_node;
    }
    
    list->size++;
    
    if (last_list == list) {
        if (last_index >= index) {
            last_index++;
        }
    }

    return list;
}

/**
 * L�scht ein Listenelement und gibt seinen Wert zur�ck.
 *
 * @param list Liste, aus der ein Element gel�scht werden soll
 * @param index Index des zu l�schenden Elements
 *
 * @return Der Wert des gel�schten Elements oder NULL, wenn als List ein
 * Nullpointer �bergeben wurde oder in der Liste kein Element mit dem
 * �bergebenen Index existiert
 */
void* list_remove(list_t* list, int index)
{
    void* value;

    if (!list) {
        return NULL;
    }
    
    struct list_node* node;
    if (index) {
        struct list_node* prev_node = list_get_node_at(list, index - 1);

        if ((!prev_node) || (!prev_node->next)) {
            return NULL;
        }

        node = prev_node->next;
        prev_node->next = node->next;
        value = node->value;
        delete node;
    } else {
        if (!list->anchor) {
            return NULL;
        }

        node = list->anchor;
        list->anchor = node->next;;
        value = node->value;
        delete node;
    }
    
    list->size--;
    
    if (last_list == list) {
        if (last_index > index) {
            last_index--;
        } else if (last_index == index) {
            last_list = NULL;
        }
    }

    return value;
}
