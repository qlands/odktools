#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>

class mainClass : public QObject
{
    Q_OBJECT
public:
    explicit mainClass(QObject *parent = nullptr);
    void setParameters(QString host,QString port,QString user,QString pass,QString schema,QString mapFile,QString outputFile,QString createFile);
    int returnCode;
signals:
    void finished();
public slots:
    void run();
private:
    void log(QString message);
    QString host;
    QString port;
    QString user;
    QString pass;
    QString schema;
    QString mapFile;
    QString outputFile;
    QString createFile;
};

#endif // MAINCLASS_H
