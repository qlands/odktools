#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>
#include "generic.h"

class mainClass : public QObject
{
    Q_OBJECT
public:
    explicit mainClass(QObject *parent = nullptr);
    int returnCode;
signals:
    void finished();
public slots:
    void run();
    void setParameters(QString createA, QString createB, QString insertA, QString insertB, QString createC, QString insertC, QString diffCreate, QString diffInsert, QString outputType, QList<TignoreTableValues> toIgnore, bool saveToFile, QString errorFile, QStringList properties);
private:
    QString a_createXML;
    QString b_createXML ;
    QString a_insertXML;
    QString b_insertXML;
    QString c_createXML;
    QString c_insertXML ;
    QString d_createSQL;
    QString d_insertSQL;
    QString output_type;
    QString error_file;
    QStringList properties;
    bool save_to_file;
    void log(QString message);
    QList<TignoreTableValues> valuesToIgnore;
};

#endif // MAINCLASS_H
