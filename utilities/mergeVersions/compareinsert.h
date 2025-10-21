#ifndef COMPAREINSERT_H
#define COMPAREINSERT_H

#include <QObject>
#include <QtCore>
#include <QDomElement>
#include "generic.h"
#include <QDomDocument>

class compareInsert : public QObject
{    
public:
    explicit compareInsert(QObject *parent = nullptr);
    void setFiles(QString insertA, QString insertB, QString insertC, QString diffSQL, QString outputType, QList<TignoreTableValues> toIgnore);
    int compare();
    int createCFile();
    int createDiffFile();
    void setAsParsed(QString table);
    QList<TtableDiff> getDiffs();
    QList<TcompError> getErrorList();
private:
    QString inputA;
    QString inputB;
    QString outputC;
    QString outputD;
    QString outputType;
    QList<TignoreTableValues> valuesToIgnore;
    bool fatalError;
    QDomDocument docB;
    QList<TtableDiff> diff;
    QList<TcompError> errorList;
    void log(QString message);
    void fatal(QString message);
    QDomNode findTable(QDomDocument docB,QString tableName);
    QDomNode findValue(QDomNode table,QString code);
    void addValueToDiff(QDomElement table, QDomElement field);
    void UpdateValue(QDomElement table, QDomElement field);
    void addTableToDiff(QDomElement table);
    void changeValueInC(QDomNode table, QString code, QString newDescription);
    void compareLKPTables(QDomNode table,QDomDocument &docB);
    void addDiffToTable(QString table, QString sql);
    bool ignoreChange(QString table, QString value);
    void UpdateProperty(QDomElement table, QDomElement field, QString property);
    void changePropertyInC(QDomNode table, QString code, QString property, QString newpropertyValue);
    void changePropertiesInC(QDomNode table, QString properties);
};

#endif // COMPAREINSERT_H
