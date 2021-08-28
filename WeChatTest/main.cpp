#include "wechat.h"
#include <QApplication>
#include "ckernel.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    CKernel::GetInstance();

    return a.exec();
}
