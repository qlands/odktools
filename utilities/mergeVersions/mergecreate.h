#ifndef MERGECREATE_H
#define MERGECREATE_H

#include <QObject>
#include <QtCore>
#include <QDomElement>
#include <QList>
#include <QUuid>
#include "generic.h"



class mergeCreate : public QObject
{    
    struct rfieldDef
    {
      QString name;
      QString rname;
      QString rcode;
      bool isLookUp = false;
    };
    typedef rfieldDef TrfieldDef;

    struct rtableDef
    {
      QString parentTable;
      QString name;
      bool isLookUp = false;
      QList<TrfieldDef> rfields;
    };
    typedef rtableDef TrtableDef;

    struct replaceRef
    {
        QString table_name;
        QString rel_name;
        QString field_name;
        QString rel_table;
        QString rel_field;
    };
    typedef replaceRef TreplaceRef;

public:
    explicit mergeCreate(QObject *parent = nullptr);
    int compare();
    void setFiles(QString createA, QString createB, QString createC, QString diffSQL, QString outputType);
    void setInsertDiff(QList<TtableDiff> diff);
    QStringList getInsertTablesUsed();
    QList<TcompError> getErrorList();
    QStringList properties;
private:
    QDomElement rootA;
    QDomElement rootB;
    QString inputA;
    QString inputB ;
    QString outputC;
    QString outputD;
    bool fatalError;
    QStringList diff;
    QList<TtableDiff> insert_diff;
    QString outputType;
    int idx;
    QList<TcompError> errorList;
    QList<TrtableDef> rtables;
    QStringList dropTables;
    QStringList insertTablesUsed;
    QList<TreplaceRef> create_lookup_rels;
    QStringList dropped_rels;
    QStringList newFields;
    QStringList newTables;
    void addAlterFieldToDiff(QString table, QDomElement eField, int newSize, int newDec, bool islookup);
    void ddTableToDrop(QString name);
    void changeLookupRelationship(QString table, QDomElement a, QDomElement b, bool islookup);
    void addFieldToDiff(QString table, QDomElement eField);
    void addFieldToRTables(QString parentTable, QString rTable, QString field, QString rField, QString rname, bool isLookUp);
    void addTableToSDiff(QDomNode table, bool lookUp);
    void log(QString message);
    void fatal(QString message);
    QDomNode findField(QDomNode table,QString field);
    QDomNode findTable(QDomDocument docB,QString tableName);
    QString compareFields(QDomElement a, QDomElement b, int &newSize, int &newDec);
    QString getFieldDefinition(QDomElement field);
    void checkField(QDomNode eTable, QDomElement a, QDomElement b, bool islookup);
    void compareLKPTables(QDomNode table,QDomDocument &docB);
    void compareTables(QDomNode table,QDomDocument &docB);
    void addTableToDrop(QString name);
    void replace_lookup_relationships(QString table, QString field);
    bool relation_is_dropped(QString name);
};

#endif // MERGECREATE_H
