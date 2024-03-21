#include "EasyChat.h"
#include "login.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
Widget *pW;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "EChat_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    login l;
    l.show();
    Widget w;
    pW = &w;

    QObject::connect(&l,SIGNAL(Jump_to_Chat()),&w,SLOT(reciveLogin()));

    return a.exec();
}
