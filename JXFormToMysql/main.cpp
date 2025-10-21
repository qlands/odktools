/*
JXFormToMySQL 3.0

Copyright (c) 2025 QLands Inc.

JXFormToMySQL is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

JXFormToMySQL is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with JXFormToMySQL.  If not, see <http://www.gnu.org/licenses/lgpl-3.0.html>.

*/


#include <QCoreApplication>
#include <mainclass.h>
#include <QTimer>


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    mainClass *task = new mainClass(&app);
    task->setParameters(argc, argv);

    QObject::connect(task, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, task, SLOT(run()));

    app.exec();
    return task->returnCode;

}
