#include <QCoreApplication>
#include <tclap/CmdLine.h>
#include <QTimer>
#include "mainclass.h"
#include <QTime>
#include <QRandomGenerator>

/*
MySQLDenormalize

Copyright (C) 2018 QLands Technology Consultants.
Author: Carlos Quiros (cquiros_at_qlands.com / c.f.quiros_at_cgiar.org)

MySQLToXLSX is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

MySQLToXLSX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with MySQLToXLSX.  If not, see <http://www.gnu.org/licenses/lgpl-3.0.html>.
*/

QString getRandomHex(const int &length)
{
    QString randomHex;
    QRandomGenerator gen;
    QTime time = QTime::currentTime();
    gen.seed((uint)time.msec());
    for(int i = 0; i < length; i++) {
        int n = gen.generate() % 16;
        randomHex.append(QString::number(n,16));
    }

    return randomHex;
}

void log_error(QString message)
{
    QString temp;
    temp = message + "\n";
    printf("%s", temp.toUtf8().data());
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QString title;
    title = title + " *********************************************************************** \n";
    title = title + " * MySQLDenormalize                                                    * \n";
    title = title + " * This tool denormalize a submission from a MySQL Database into JSON  * \n";
    title = title + " * starting from the main table. It relies on the Map XML file         * \n";
    title = title + " * created by JSONToMySQL. It is useful if you need to go back         * \n";
    title = title + " * to a JSON representation from relational data.                      * \n";
    title = title + " *********************************************************************** \n";

    TCLAP::CmdLine cmd(title.toUtf8().data(), ' ', "2.0");
    //Required arguments
    TCLAP::ValueArg<std::string> hostArg("H","host","MySQL host. Default localhost",false,"localhost","string");
    TCLAP::ValueArg<std::string> portArg("P","port","MySQL port. Default 3306.",false,"3306","string");
    TCLAP::ValueArg<std::string> userArg("u","user","User to connect to MySQL",true,"","string");
    TCLAP::ValueArg<std::string> passArg("p","password","Password to connect to MySQL",true,"","string");
    TCLAP::ValueArg<std::string> schemaArg("s","schema","Schema in MySQL",true,"","string");
    TCLAP::ValueArg<std::string> mapArg("m","mapfile","XML map file to use",true,"","string");
    TCLAP::ValueArg<std::string> outArg("o","output","Output JSON file",true,"","string");
    TCLAP::ValueArg<std::string> createArg("c","create","Create XML file",true,"","string");



    cmd.add(hostArg);
    cmd.add(portArg);
    cmd.add(userArg);
    cmd.add(passArg);
    cmd.add(schemaArg);    
    cmd.add(createArg);        
    cmd.add(mapArg);
    cmd.add(outArg);


    //Parsing the command lines
    cmd.parse( argc, argv );

    QString host = QString::fromUtf8(hostArg.getValue().c_str());
    QString port = QString::fromUtf8(portArg.getValue().c_str());
    QString user = QString::fromUtf8(userArg.getValue().c_str());
    QString pass = QString::fromUtf8(passArg.getValue().c_str());
    QString schema = QString::fromUtf8(schemaArg.getValue().c_str());

    QString mapFile = QString::fromUtf8(mapArg.getValue().c_str());
    QString createFile = QString::fromUtf8(createArg.getValue().c_str());
    QString outputFile = QString::fromUtf8(outArg.getValue().c_str());



    mainClass *task = new mainClass(&app);
    task->setParameters(host,port,user,pass,schema,mapFile,outputFile,createFile);
    QObject::connect(task, SIGNAL(finished()), &app, SLOT(quit()));
    QTimer::singleShot(0, task, SLOT(run()));
    app.exec();
    return task->returnCode;
}
