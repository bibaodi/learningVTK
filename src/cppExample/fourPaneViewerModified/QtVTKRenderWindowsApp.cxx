#include <QApplication>
#include <QSurfaceFormat>

#include "QVTKRenderWidget.h"
#include "QtVTKRenderWindows.h"

int main(int argc, char **argv) {
    // needed to ensure appropriate OpenGL context is created for VTK rendering.
    QSurfaceFormat::setDefaultFormat(QVTKRenderWidget::defaultFormat());

    // QT Stuff
    QApplication app(argc, argv);

    QtVTKRenderWindows myQtVTKRenderWindows(argc, argv);
    myQtVTKRenderWindows.show();
    // after show will make vtk got real window size.
    myQtVTKRenderWindows.initVtkAfterInitialization(argv);
    myQtVTKRenderWindows.Render();
    return app.exec();
}
