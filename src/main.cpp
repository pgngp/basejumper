#include <QApplication>

#include "mainwindow.h"
#include <QtGui>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //QPixmap pixmap(":/images/basejumper.png");
    //QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint);
    //splash.show();
    //splash.showMessage("Loading Basejumper...", Qt::AlignBottom);

    MainWindow mainWin;
    mainWin.show();

    //splash.finish(&mainWin);

    return app.exec();
}

