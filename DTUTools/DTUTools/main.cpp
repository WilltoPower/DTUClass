#include "dtutools.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DTUTools w;
    w.show();
    return a.exec();
}
