#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdlib.h>

#define QUEUE(Name, prefix, Element)                                                     \
  struct Name##Node {                                                                    \
    Element element;                                                                     \
    struct Name##Node* next;                                                             \
  };                                                                                     \
                                                                                         \
  typedef struct {                                                                       \
    struct Name##Node *front, *back;                                                     \
  } Name;                                                                                \
                                                                                         \
  Name new##Name() {                                                                     \
    Name queue = {.front = NULL, .back = NULL};                                          \
    return queue;                                                                        \
  }                                                                                      \
                                                                                         \
  bool prefix##IsEmpty(Name self) {                                                      \
    return self.front == NULL;                                                           \
  }                                                                                      \
                                                                                         \
  void prefix##Push(Name* self, Element element) {                                       \
    struct Name##Node* newNode = (struct Name##Node*) malloc(sizeof(struct Name##Node)); \
    newNode->element = element;                                                          \
    newNode->next = NULL;                                                                \
    if (self->front == NULL)                                                             \
      self->front = self->back = newNode;                                                \
    else {                                                                               \
      self->back->next = newNode;                                                        \
      self->back = newNode;                                                              \
    }                                                                                    \
  }                                                                                      \
                                                                                         \
  Element prefix##Pop(Name* self) {                                                      \
    Element element = self->front->element;                                              \
    struct Name##Node* front = self->front;                                              \
    self->front = front->next;                                                           \
    free(front);                                                                         \
    return element;                                                                      \
  }

#endif
