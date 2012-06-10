// MainWindow.cpp is part of Noggit3, licensed via GNU General Public License (version 3).
// Benedikt Kleiner <benedikt.kleiner@googlemail.com>
// Glararan <glararan@glararan.eu>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>
#include <QDockWidget>

#include <noggit/Menu.h>
#include <noggit/MapView.h>

namespace noggit
{
  namespace ui
  {
    class settingsDialog;
    class about_widget;
    class help_widget;

    class MainWindow : public QMainWindow
    {
      Q_OBJECT

    public:
      MainWindow(QWidget* parent = 0);

    public slots:
      void create_world_view (World*);

    private slots:
      void settingsClicked();

      void maps();
      void projectExplorerOpen();

      void help_mapEditor();
      void about();

    private:
      void initialize_video();
      void createDockWidgets();

      QGLWidget* _dummy_gl_widget;
      QToolBar* currentToolBar;

      noggit::ui::settingsDialog* _settings;
      noggit::ui::about_widget* _about;
      noggit::ui::help_widget* _help;
      World* _world;
    };
  }
}

#endif // MAINWINDOW_H
