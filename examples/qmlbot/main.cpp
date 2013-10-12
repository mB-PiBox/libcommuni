/*
 * Copyright (C) 2008-2013 The Communi Project
 *
 * This example is free, and not covered by the LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[])
{
    qputenv("IRC_DEBUG", "1");
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine(QUrl("qrc:/main.qml"));
    return app.exec();
}