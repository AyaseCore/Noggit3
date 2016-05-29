// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#pragma once

#include <QObject>
#include <QLinkedList>
#include <QMutex>
#include <QSet>

namespace noggit
{
  namespace async
  {
    class object;

    class loader : public QObject
    {
      Q_OBJECT

    public:
      loader (size_t thread_count, QObject* parent = nullptr);
      ~loader();

      void add_object (object*);

    private:
      object* next_object_to_load();
      QList<object*> _objects_to_load;
      QMutex _loading_mutex;

      friend class loading_thread;
    };
  }
}
