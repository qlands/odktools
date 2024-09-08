#include "mainclass.h"


mainClass::mainClass(QObject *parent) : QObject(parent)
{
    returnCode = 0;    
}

void mainClass::log(QString message)
{
    QString temp;
    temp = message + "\n";
    printf("%s", temp.toUtf8().data());
}

void mainClass::setParameters(QString host,QString port,QString user,QString pass,QString schema,QString mapFile,QString outputFile,QString createFile)
{
    this->host = host;
    this->port = port;
    this->user = user;
    this->pass = pass;
    this->schema = schema;
    this->mapFile = mapFile;
    this->outputFile = outputFile;
    this->createFile = createFile;
}

void mainClass::run()
{
    returnCode = 0;
    emit finished();
}




