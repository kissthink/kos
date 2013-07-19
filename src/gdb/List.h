#ifndef _Gdb_List_h_
#define _Gdb_List_h_ 1

template<typename T> class Node {
  T element;
  Node<T>* prev;
  Node<T>* next;
public:
  Node(): prev(nullptr), next(nullptr) {}
  void operator delete(ptr_t ptr) { globaldelete(ptr, sizeof(Node<T>)); }
  inline T getElement() const { return element; }
  inline void setElement(T elem) { element = elem; }
  inline Node<T>* getPrev() const { return prev; }
  inline Node<T>* getNext() const { return next; }
  inline void setPrev(Node<T>* node) {
    prev = node;
    prev->next = this;
  }
  inline void setNext(Node<T>* node) {
    next = node;
    next->prev = this;
  }
};

template<typename T> class List {
  Node<T>* head;
  Node<T>* tail;
public:
  List(): head(nullptr), tail(nullptr) {}
  void insertEnd(T item, bool replace = false) {
    if (replace) {      // replace if already exist
      Node<T>* node = search(item);
      if (node) {
        node->setElement(item);
        return;
      }
    }
    Node<T>* newNode = new Node<T>;
    newNode->setElement(item);
    if (head == nullptr) head = tail = newNode;
    else {
      tail->setNext(newNode);
      tail = tail->getNext();
    }
  }
  Node<T>* search(T elem) {
    Node<T>* next = head;
    bool found = false;
    while (!found && next != nullptr) {
      if (next->getElement() == elem) found = true;
      else next = next->getNext();
    }
    return found ? next : nullptr;
  }
  void remove(T elem) {
    Node<T>* nodeToRemove = search(elem);
    if (nodeToRemove) {
      if (nodeToRemove == head) head = nodeToRemove->getNext();
      else nodeToRemove->getPrev()->setNext(nodeToRemove->getNext());
      delete nodeToRemove;
    }
  }
};

#endif /* _Gdb_List_h_ */
