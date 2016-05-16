// loading_thread.h is part of Noggit3, licensed via GNU General Public License (version 3).
// Bernd Lörwald <bloerwald+noggit@googlemail.com>

#pragma once

#include <QThread>

namespace noggit
{
  namespace async
  {
    class loader;

    class loading_thread : public QThread
    {
      Q_OBJECT
    public:
      loading_thread (loader* async_loader);

    protected:
      virtual void timerEvent (QTimerEvent*);

    private:
      loader* _async_loader;
    };
  }
}
