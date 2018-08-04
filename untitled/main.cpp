#include "mainwindow.h"
#include <QApplication>
#include <QPainter>

class Widget : public QWidget {
protected:
  void paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.drawPixmap(0, 0, QPixmap("1.png"));
  }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();



    return a.exec();
}
