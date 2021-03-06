﻿/*
MySQLDenormalize

Copyright (C) 2018 QLands Technology Consultants.
Author: Carlos Quiros (cquiros_at_qlands.com / c.f.quiros_at_cgiar.org)

MySQLDenormalize is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

MySQLDenormalize is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with MySQLDenormalize.  If not, see <http://www.gnu.org/licenses/lgpl-3.0.html>.
*/

#include "mainclass.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QFileInfoList>
#include <QVariant>
#include <QProcess>
#include <QSqlRecord>
#include <QList>
#include <QTime>
#include <QDomNodeList>
#include <QDomNode>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/find.hpp>
#include <bsoncxx/stdx/string_view.hpp>
#include <QDebug>

#include <iostream>

using bsoncxx::builder::basic::kvp;

namespace pt = boost::property_tree;

mainClass::mainClass(QObject *parent) : QObject(parent)
{
    returnCode = 0;
}

void mainClass::log(QString message)
{
    QString temp;
    temp = message + "\n";
    printf("%s",temp.toLocal8Bit().data());
}

void mainClass::setParameters(QString host, QString port, QString user, QString pass, QString schema, QString table, QString mapDir, QString output, bool separate, QString tempDir, QString createFileName)
{
    this->host = host;
    this->port = port;
    this->user = user;
    this->pass = pass;
    this->schema = schema;
    this->table = table;
    this->mapDir = mapDir;
    this->output = output;
    this->separateSelects = separate;
    this->tempDir = tempDir;
    this->createFile = createFileName;
}

//This function goes trough a series of tables and their childs
//to obtain the child tables from the main table (start table)
void mainClass::processChilds(QDomDocument doc, QDomElement &parent, QString table, QList <TtableDef> tables, QStringList &tablesUsed)
{
    tablesUsed.append(table);
    QDomElement etable;
    etable = doc.createElement(table);
    parent.appendChild(etable);
    for (int pos = 0; pos <= tables.count()-1;pos++)
    {
        if (tables[pos].parent == table)
            processChilds(doc,etable,tables[pos].name,tables,tablesUsed);
    }
}

//QList<TfieldDef> mainClass::getDataByRowUUID3(QSqlDatabase db, QString tableToSearch, QString UUIDToSearch)
//{
//    QList<TfieldDef> records;
//    QSqlQuery qry(db);
//    QString sql;
//    sql = "SELECT * FROM " + tableToSearch + " WHERE rowuuid = '" + UUIDToSearch + "' limit 1";
//    qry.exec(sql);
//    if (qry.first())
//    {
//        QSqlRecord record = qry.record();
//        for (int pos = 0; pos <= record.count()-1; pos++)
//        {
//            QString fieldName;
//            fieldName = record.fieldName(pos);
//            TfieldDef aRecord;
//            aRecord.name = fieldName;
//            aRecord.value = qry.value(fieldName).toString();
//            records.append(aRecord);
//        }
//    }
//    if (records.count() == 0)
//    {
//        log("Empty result for " + tableToSearch + " with UUID " + UUIDToSearch);
//    }
//    return records;
//}

//This function uses mongo to search for the data associated with a UUID in a table.
//This function return a list of fields (Field name, Field value)
//QList<TfieldDef> mainClass::getDataByRowUUID2(mongocxx::collection collection, QString tableToSearch, QString UUIDToSearch)
//{
//    QList<TfieldDef> records;
//    bsoncxx::stdx::optional<bsoncxx::document::value> query_result =
//    collection.find_one(bsoncxx::builder::stream::document{} << "_tablename" << tableToSearch.toUtf8().constData() << "rowuuid" << UUIDToSearch.toUtf8().constData() << bsoncxx::builder::stream::finalize);
//    if(query_result)
//    {
//        std::string res = bsoncxx::to_json(*query_result);
//        QString result = QString::fromStdString(res);
//        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
//        QJsonObject obj = doc.object();
//        QStringList keys = obj.keys();
//        for (int pos = 0; pos <= keys.count()-1; pos++)
//        {
//            QJsonValue value = obj.value(keys[pos]);
//            TfieldDef aValue;
//            aValue.name = keys[pos];
//            aValue.value = value.toString();
//            records.append(aValue);
//        }
//    }
//    if (records.count() == 0)
//    {
//        log("Empty result for " + tableToSearch + " with UUID " + UUIDToSearch);
//    }
//    return records;
//}

// Check wether a field is multiselect. Returns true or false. If true it also returns teh multiselect table and the field holding the values
bool mainClass::isFieldMultiSelect(QString table, QString field, QString &msel_table, QString &lookup_field, bool &isSensitive)
{
    msel_table = "";
    bool result = false;
    isSensitive = false;
    for (int pos = 0; pos < tables_in_create.count(); pos++)
    {
        if (tables_in_create.item(pos).toElement().attribute("name") == table)
        {
            QDomNode field_node = tables_in_create.item(pos).firstChild();
            while(!field_node.isNull())
            {
                if (field_node.toElement().attribute("name") == field)
                {
                    if ((field_node.toElement().attribute("isMultiSelect","false") == "true"))
                    {
                        msel_table = field_node.toElement().attribute("multiSelectTable");
                        if (field_node.toElement().attribute("sensitive","false") == "true")
                            isSensitive = true;
                        result = true;
                    }
                }
                field_node = field_node.nextSibling();
            }
        }
    }
    if (result)
    {
        for (int pos = 0; pos < tables_in_create.count(); pos++)
        {
            if (tables_in_create.item(pos).toElement().attribute("name") == msel_table)
            {
                QDomNode field_node = tables_in_create.item(pos).firstChild();
                while(!field_node.isNull())
                {
                    if (field_node.toElement().attribute("rlookup","false") == "true")
                    {
                        lookup_field = field_node.toElement().attribute("name");
                    }
                    field_node = field_node.nextSibling();
                }
            }
        }
    }

    return result;
}

//Pulls the multiselect values from the multiselect tables into an string separated by space
QStringList mainClass::getMultiSelectValuesAsArray(QList<TUUIDDef> dataList, QString msel_table, QString lookup_field, QDomNode current_node, QString field_name)
{
    QStringList result;
    QDomNode start_node = current_node.firstChild();
    while (!start_node.isNull())
    {
        if (start_node.toElement().attribute("table") == msel_table)
        {
            QString UUIDToSearch = start_node.toElement().attribute("uuid");
            for (int pos = 0; pos <= dataList.count()-1;pos++)
            {
                if ((dataList[pos].table == msel_table) && (dataList[pos].UUID == UUIDToSearch))
                {
                    if (alreadyPulled2.indexOf(UUIDToSearch + "@" + msel_table) == -1)
                    {
                        for (int fld = 0; fld < dataList[pos].fields.count(); fld++ )
                        {
                            if (dataList[pos].fields[fld].name == lookup_field)
                                result.append(field_name + "/" + dataList[pos].fields[fld].value);
                        }
                        alreadyPulled2.append(UUIDToSearch + "@" + msel_table);
                    }
                }
            }
        }
        start_node = start_node.nextSibling();
    }
    return result;
}

//Pulls the multiselect values from the multiselect tables into an array
QString mainClass::getMultiSelectValuesAsString(QList<TUUIDDef> dataList, QString msel_table, QString lookup_field, QDomNode current_node)
{
    QStringList result;
    QDomNode start_node = current_node.firstChild();
    while (!start_node.isNull())
    {
        if (start_node.toElement().attribute("table") == msel_table)
        {
            QString UUIDToSearch = start_node.toElement().attribute("uuid");
            for (int pos = 0; pos <= dataList.count()-1;pos++)
            {
                if ((dataList[pos].table == msel_table) && (dataList[pos].UUID == UUIDToSearch))
                {
                    if (alreadyPulled2.indexOf(UUIDToSearch + "@" + msel_table) == -1)
                    {
                        for (int fld = 0; fld < dataList[pos].fields.count(); fld++ )
                        {
                            if (dataList[pos].fields[fld].name == lookup_field)
                                result.append(dataList[pos].fields[fld].value);
                        }
                        alreadyPulled2.append(UUIDToSearch + "@" + msel_table);
                    }
                }
            }
        }
        start_node = start_node.nextSibling();
    }
    if (result.length() > 0)
        return result.join(" ");
    else
        return "";
}

//Pulls data from the memory repository by table and uuid. Properly pull the data of multiselect tables from the memory repository and separates them if asked
QList<TfieldDef> mainClass::getDataByRowUUID4(QList<TUUIDDef> dataList, QString tableToSearch, QString UUIDToSearch, QDomNode current_node)
{    
    QList<TfieldDef> records;

    for (int pos = 0; pos <= dataList.count()-1;pos++)
    {
        if ((dataList[pos].table == tableToSearch) && (dataList[pos].UUID == UUIDToSearch))
        {
            if (alreadyPulled.indexOf(UUIDToSearch + "@" + tableToSearch) == -1)
            {
                QList<TfieldDef> records_found;
                records_found = dataList[pos].fields;
                for (int rec = 0; rec < records_found.count(); rec++)
                {
                    QString field_name = records_found[rec].name;
                    QString msel_table;
                    QString msel_field;
                    bool isSensitive;
                    if (isFieldMultiSelect(tableToSearch,field_name,msel_table,msel_field, isSensitive))
                    {
                        //log("Field: " + field_name + " in table: " + tableToSearch + " is multiselect with UUID: " + UUIDToSearch + " with field: " + msel_field);
                        if (!separateSelects)
                        {
                            records_found[rec].value = getMultiSelectValuesAsString(dataList, msel_table, msel_field, current_node);
                        }
                        else
                        {
                            QStringList items = getMultiSelectValuesAsArray(dataList, msel_table, msel_field, current_node,field_name);
                            records_found.removeAt(rec);
                            for (int item = 0; item < items.count(); item++)
                            {
                                TfieldDef a_record;
                                a_record.name = items[item];
                                if (!isSensitive)
                                    a_record.value = "true";
                                else
                                    a_record.value = "not available";
                                records_found.append(a_record);
                            }
                            rec = 0;
                        }
                    }
                }
                records.append(records_found);
                alreadyPulled.append(UUIDToSearch + "@" + tableToSearch);
            }
        }
    }
    if (records.count() == 0)
    {
        log("Empty result for " + tableToSearch + " with UUID " + UUIDToSearch);
    }

    return records;
}

//This function uses xmllint to search for the data associated with a UUID in a table.
//It has been replaced by directly searching the database using an index on rowuuid
//This function return a list of fields (Field name, Field value)
//QList<TfieldDef> mainClass::getDataByRowUUID3(QString tableToSearch, QString UUIDToSearch)
//{
//    QList<TfieldDef> records;
//    QString program = "xmllint";
//    QStringList arguments;
//    arguments << "--xpath";
//    arguments << "//table_data[@name='" + tableToSearch + "']//row[./field[@name='rowuuid' and . = '" + UUIDToSearch + "']]";
//    arguments << "./tmp/" + tableToSearch + ".xml";

//    //log("Running xmllint please wait....");
//    QProcess *myProcess = new QProcess();
//    myProcess->start(program, arguments);
//    myProcess->waitForFinished(-1);
//    QString result = myProcess->readAllStandardOutput();
//    if (result != "XPath set is empty")
//    {
//        QDomDocument doc("dataSection");
//        doc.setContent(result);
//        QDomNodeList fields;
//        fields = doc.elementsByTagName("field");
//        for (int pos = 0; pos <= fields.count()-1;pos++)
//        {
//            QString fieldName;
//            fieldName = fields.item(pos).toElement().attribute("name");
//            QString fieldValue;
//            fieldValue = fields.item(pos).firstChild().nodeValue();
//            //if (fieldName != "rowuuid")
//            //{
//            TfieldDef aRecord;
//            aRecord.name = fieldName;
//            aRecord.value = fieldValue;
//            records.append(aRecord);
//            //}
//        }
//    }
//    if (records.count() == 0)
//    {
//        log("Empty result for " + tableToSearch + " with UUID " + UUIDToSearch);
//    }
//    return records;
//}


// Parse the mao file using JSON Boost. We use boost so the order of the fields in the resulting JSON is the same of the database. QT JSON does not do this
void mainClass::parseMapFileWithBoost(QList <TUUIDDef> dataList, QDomNode node, pt::ptree &json, QString currentTable, pt::ptree &parent)
{
    QDomElement elem;
    elem = node.toElement();
    QString tableName;
    QString UUID;
    tableName = elem.attribute("table");
    UUID = elem.attribute("uuid");
    //Get the data for a UUID in a table and add it to the JSON object
    QList<TfieldDef> records;
    records = getDataByRowUUID4(dataList,tableName,UUID, node);
    for (int pos = 0; pos <= records.count()-1; pos++)
    {
        json.put(records[pos].name.toStdString(), records[pos].value.toStdString()); //json[records[pos].name] = records[pos].value;
    }
    //If the current node has a child record then process the child
    //by recursively call this process. The subtable is a JSON array
    if (!node.firstChild().isNull())
    {
        elem = node.firstChild().toElement();
        tableName = elem.attribute("table");
        //json[tableName] = QJsonArray();
        pt::ptree childObject; //QJsonObject childObject;
        parseMapFileWithBoost(dataList,node.firstChild(),childObject,tableName,json);                                       //RECURSIVE!!!
        pt::ptree array;
        pt::ptree::const_assoc_iterator it;
        it = json.find(tableName.toStdString());
        if (it != json.not_found())
            array = json.get_child(tableName.toStdString()); //QJsonArray array = json[tableName].toArray();
        array.push_back(std::make_pair("", childObject)); //array.append(childObject);
        if (tableName.indexOf("_msel_") == -1)
            json.put_child(tableName.toStdString(), array); //json[tableName] = array;

    }
    //If the current node has siblings.
    if (!node.nextSibling().isNull())
    {
        QDomNode nextSibling;
        nextSibling = node.nextSibling();
        QString currTable;
        currTable = currentTable;
        //Go trhough each sibbling
        while (!nextSibling.isNull())
        {
            elem = nextSibling.toElement();
            tableName = elem.attribute("table");
            UUID = elem.attribute("uuid");
            //New siblings usually refer to records in the same table
            //We only create an JSON Array if the table changes from
            //one sibling to another
            if (currTable != tableName)
            {
                //Each sibling table is stored as a JSON Array
                pt::ptree array2;

                pt::ptree::const_assoc_iterator it2;
                it2= parent.find(tableName.toStdString());
                if (it2 != parent.not_found())
                    array2 = parent.get_child(tableName.toStdString()); //QJsonArray array2 = parent[tableName].toArray();
                pt::ptree childObject2; //QJsonObject childObject2;
                QList<TfieldDef> records2;
                records2 = getDataByRowUUID4(dataList,tableName,UUID, nextSibling);
                for (int pos = 0; pos <= records2.count()-1; pos++)
                {
                    childObject2.put(records2[pos].name.toStdString(),records2[pos].value.toStdString()); //childObject2[records2[pos].name] = records2[pos].value;
                }
                //If the sibling has a child then recursively call
                //this function.
                if (!nextSibling.firstChild().isNull())
                {
                    QString tableName2;
                    QDomElement elem2;
                    elem2 = nextSibling.firstChild().toElement();
                    tableName2 = elem2.attribute("table");
                    //childObject2[tableName2] = QJsonArray();
                    pt::ptree childObject3; //QJsonObject childObject3;
                    parseMapFileWithBoost(dataList,nextSibling.firstChild(),childObject3,currTable,childObject2);                 //!!RECURSIVE
                    pt::ptree array3; //QJsonArray array3;

                    pt::ptree::const_assoc_iterator it3;
                    it3 = childObject2.find(tableName2.toStdString());
                    if (it3 != childObject2.not_found())
                        array3 = childObject2.get_child(tableName2.toStdString()); //childObject2[tableName2].toArray();
                    array3.push_back(std::make_pair("", childObject3)); //array3.append(childObject3);
                    if (tableName2.indexOf("_msel_") == -1)
                        childObject2.put_child(tableName2.toStdString(), array3); //childObject2[tableName2] = array3;
                }
                array2.push_back(std::make_pair("", childObject2)); //array2.append(childObject2);
                if (tableName.indexOf("_msel_") == -1)
                    parent.put_child(tableName.toStdString(), array2); //parent[tableName] = array2;
                currTable = tableName;
            }
            else
            {
                //This happend when the sibling has the same table which
                //means is a sibling record in a table
                pt::ptree childObject2; //QJsonObject childObject2;
                QList<TfieldDef> records2;
                records2 = getDataByRowUUID4(dataList,tableName,UUID,nextSibling);
                for (int pos = 0; pos <= records2.count()-1; pos++)
                {
                    childObject2.put(records2[pos].name.toStdString(), records2[pos].value.toStdString()); //childObject2[records2[pos].name] = records2[pos].value;
                }
                pt::ptree currArray;

                pt::ptree::const_assoc_iterator it4;
                it4 = parent.find(tableName.toStdString());
                if (it4 != parent.not_found())
                    currArray= parent.get_child(tableName.toStdString());  //QJsonArray currArray = parent[tableName].toArray();
                //If the sibling has a child then recursively call
                //this function.
                if (!nextSibling.firstChild().isNull())
                {
                    QString tableName2;
                    QDomElement elem2;
                    elem2 = nextSibling.firstChild().toElement();
                    tableName2 = elem2.attribute("table");
                    //childObject2[tableName2] = QJsonArray();
                    pt::ptree childObject3;//QJsonObject childObject3;
                    parseMapFileWithBoost(dataList,nextSibling.firstChild(),childObject3,currTable,childObject2);              //RECURSIVE!!!
                    pt::ptree array3;//QJsonArray array3;

                    pt::ptree::const_assoc_iterator it5;
                    it5 = childObject2.find(tableName2.toStdString());
                    if (it5 != childObject2.not_found())
                        array3 = childObject2.get_child(tableName2.toStdString()); //array3 = childObject2[tableName2].toArray();
                    array3.push_back(std::make_pair("", childObject3)); //array3.append(childObject3);
                    if (tableName2.indexOf("_msel_") == -1)
                        childObject2.put_child(tableName2.toStdString(), array3); //childObject2[tableName2] = array3;
                }
                currArray.push_back(std::make_pair("", childObject2)); //currArray.append(childObject2);
                if (tableName.indexOf("_msel_") == -1)
                    parent.put_child(tableName.toStdString(), currArray); //parent[tableName] = currArray;
            }
            nextSibling = nextSibling.nextSibling();
        }
    }
}


//This function parse a Map File and returs the data tree in JSON form
//This is a recursive fuction
//mongocxx::collection collection
//void mainClass::parseMapFile(QList <TUUIDDef> dataList, QDomNode node, QJsonObject &json, QString currentTable, QJsonObject &parent)
//{
//    QDomElement elem;
//    elem = node.toElement();
//    QString tableName;
//    QString UUID;
//    tableName = elem.attribute("table");
//    UUID = elem.attribute("uuid");

//    //Get the data for a UUID in a table and add it to the JSON object
//    QList<TfieldDef> records;
//    records = getDataByRowUUID4(dataList,tableName,UUID);
//    for (int pos = 0; pos <= records.count()-1; pos++)
//    {
//        json[records[pos].name] = records[pos].value;
//    }
//    //If the current node has a child record then process the child
//    //by recursively call this process. The subtable is a JSON array
//    if (!node.firstChild().isNull())
//    {
//        elem = node.firstChild().toElement();
//        tableName = elem.attribute("table");
//        json[tableName] = QJsonArray();
//        QJsonObject childObject;
//        parseMapFile(dataList,node.firstChild(),childObject,tableName,json);
//        QJsonArray array = json[tableName].toArray();
//        array.append(childObject);
//        json[tableName] = array;
//    }
//    //If the current node has siblings.
//    if (!node.nextSibling().isNull())
//    {
//        QDomNode nextSibling;
//        nextSibling = node.nextSibling();
//        QString currTable;
//        currTable = currentTable;
//        //Go trhough each sibbling
//        while (!nextSibling.isNull())
//        {
//            elem = nextSibling.toElement();
//            tableName = elem.attribute("table");
//            UUID = elem.attribute("uuid");
//            //New siblings usually refer to records in the same table
//            //We only create an JSON Array if the table changes from
//            //one sibling to another
//            if (currTable != tableName)
//            {
//                //Each sibling table is stored as a JSON Array
//                QJsonArray array2 = parent[tableName].toArray();
//                QJsonObject childObject2;
//                QList<TfieldDef> records2;
//                records2 = getDataByRowUUID4(dataList,tableName,UUID);
//                for (int pos = 0; pos <= records2.count()-1; pos++)
//                {
//                    childObject2[records2[pos].name] = records2[pos].value;
//                }
//                //If the sibling has a child then recursively call
//                //this function.
//                if (!nextSibling.firstChild().isNull())
//                {
//                    QString tableName2;
//                    QDomElement elem2;
//                    elem2 = nextSibling.firstChild().toElement();
//                    tableName2 = elem2.attribute("table");
//                    childObject2[tableName2] = QJsonArray();
//                    QJsonObject childObject3;
//                    parseMapFile(dataList,nextSibling.firstChild(),childObject3,currTable,childObject2);
//                    QJsonArray array3;
//                    array3 = childObject2[tableName2].toArray();
//                    array3.append(childObject3);
//                    childObject2[tableName2] = array3;
//                }
//                array2.append(childObject2);
//                parent[tableName] = array2;
//                currTable = tableName;
//            }
//            else
//            {
//                //This happend when the sibling has the same table which
//                //means is a sibling record in a table
//                QJsonObject childObject2;
//                QList<TfieldDef> records2;
//                records2 = getDataByRowUUID4(dataList,tableName,UUID);
//                for (int pos = 0; pos <= records2.count()-1; pos++)
//                {
//                    childObject2[records2[pos].name] = records2[pos].value;
//                }
//                QJsonArray currArray = parent[tableName].toArray();
//                //If the sibling has a child then recursively call
//                //this function.
//                if (!nextSibling.firstChild().isNull())
//                {
//                    QString tableName2;
//                    QDomElement elem2;
//                    elem2 = nextSibling.firstChild().toElement();
//                    tableName2 = elem2.attribute("table");
//                    childObject2[tableName2] = QJsonArray();
//                    QJsonObject childObject3;
//                    parseMapFile(dataList,nextSibling.firstChild(),childObject3,currTable,childObject2);
//                    QJsonArray array3;
//                    array3 = childObject2[tableName2].toArray();
//                    array3.append(childObject3);
//                    childObject2[tableName2] = array3;
//                }
//                currArray.append(childObject2);
//                parent[tableName] = currArray;
//            }

//            nextSibling = nextSibling.nextSibling();
//        }
//    }
//}

void mainClass::getAllUUIDs(QDomNode node,QStringList &UUIDs)
{
    QDomNode sibling;
    sibling = node;
    while (!sibling.isNull())
    {
        QDomElement eSibling;
        eSibling = sibling.toElement();
        QString UUID;
        UUID = eSibling.attribute("table","None") + "~" + eSibling.attribute("uuid","None");
        UUIDs.append(UUID);
        if (!sibling.firstChild().isNull())
            getAllUUIDs(sibling.firstChild(),UUIDs);
        sibling = sibling.nextSibling();
    }
}

void mainClass::remove_msels(QDomNode node)
{
    while (!node.isNull())
    {
        QDomElement eSibling;
        eSibling = node.toElement();
        QString tableName;
        tableName = eSibling.attribute("table","None");
        if (tableName.indexOf("_msel_") >= 0)
        {
            QDomNode parent = node.parentNode();
            node.parentNode().removeChild(node);
            node = parent;
        }
        if (!node.firstChild().isNull())
        {
            remove_msels(node.firstChild());
        }
        node = node.nextSibling();
    }
}

//This function receives a Map File and construct a JSON file with the data
//The JSON file is stored in the ouput directy and has the same name
//of the Map File
void mainClass::processMapFile(mongocxx::collection collection, QString fileName)
{
    QDir mapPath(mapDir);
    QString mapFile;
    mapFile = mapPath.absolutePath() + mapPath.separator() + fileName + ".xml";
    QDomDocument doc("mapfile");
    QFile file(mapFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        log("Map file " + mapFile + " not found");
        return;
    }
    if (!doc.setContent(&file))
    {
        file.close();
        log("Cannot parse map file " + mapFile);
        return;
    }
    file.close();
    QDomNode root;
    root = doc.firstChild().nextSibling().firstChild();
    //remove_msels(root);

    //We here search in Mongo for the data of all UUDIDs and store the
    //data in an array called dataList. By doing this we only go once
    //to mongo. However this make this tool really memory intensive.
    //We need to find a ultra fast mechanism to do this make this tool
    //usable with millions of records.
    QStringList UUIDs;
    getAllUUIDs(root,UUIDs);
    QString mongoQry;
    mongoQry = "{ \"$or\" :[";
    for (int pos = 0; pos <= UUIDs.count()-1;pos++)
    {
        QStringList parts;
        parts = UUIDs[pos].split('~');
//        if (parts[0].indexOf("_msel_") < 0 )
        mongoQry = mongoQry + "{\"_tablename\":\"" + parts[0] + "\",\"rowuuid\":\"" + parts[1] + "\"},";
    }
    mongoQry = mongoQry.left(mongoQry.length()-1);
    mongoQry = mongoQry + "]}";
    std::string utf8_text = mongoQry.toUtf8().constData();
    bsoncxx::string::view_or_value qry(utf8_text);
    bsoncxx::document::value bsondoc = bsoncxx::from_json(qry.view());
    auto cursor = collection.find(bsondoc.view());
    QList <TUUIDDef> dataList;
    //qDebug() << "***********************77";
    for (const bsoncxx::document::view& doc : cursor)
    {
        TUUIDDef aTable;
        for (bsoncxx::document::element ele : doc)
        {
            if (ele.type() == bsoncxx::type::k_utf8)
            {
                mongocxx::stdx::string_view field_key{ele.key()};
                QString key = QString::fromUtf8(field_key.data());
                QString value;
                value = QString::fromUtf8(ele.get_utf8().value.data());
                if ((key != "_tablename") && (key != "rowuuid"))
                {
                    //qDebug() << key;
                    TfieldDef aField;
                    aField.name = key;
                    aField.value = value;
                    aTable.fields.append(aField);
                }
                else
                {
                    if (key == "_tablename")
                        aTable.table = value;
                    else
                    {
                        TfieldDef rowUUIDField;
                        rowUUIDField.name = key;
                        rowUUIDField.value = value;
                        aTable.fields.append(rowUUIDField);
                        aTable.UUID = value;
                    }
                }
            }
        }
        dataList.append(aTable);
    }
    //exit(0);
    //Now that we have the data contruct the tree with the data
    //comming from the database
    QDomElement elem;
    elem = root.toElement();
    QString tableName;
    tableName = elem.attribute("table");

    //QJsonObject JSONRoot;
    //parseMapFile(dataList,root,JSONRoot,tableName,JSONRoot);

    pt::ptree JSONRootBoost;
    parseMapFileWithBoost(dataList,root,JSONRootBoost,tableName,JSONRootBoost);


    //Finally save the JSON document
    //QJsonDocument saveDoc(JSONRoot);
    QDir outputPath(output);
    //QString JSONFile;
    //JSONFile = outputPath.absolutePath() + mapPath.separator() + fileName + ".json";

    QString JSONFileBoost;
    JSONFileBoost = outputPath.absolutePath() + mapPath.separator() + fileName + ".json";
    pt::write_json(JSONFileBoost.toStdString(),JSONRootBoost);

//    QFile saveFile(JSONFile);
//    if (!saveFile.open(QIODevice::WriteOnly)) {
//        log("Couldn't open output JSON file.");
//        return;
//    }
//    saveFile.write(saveDoc.toJson());
//    saveFile.close();
}

//This uses insert many
//void mainClass::parseDataToMongo(mongocxx::collection collection, QString table, QString fileName)
//{
//    pt::ptree tree;
//    pt::read_xml(fileName.toUtf8().constData(), tree);
//    BOOST_FOREACH(boost::property_tree::ptree::value_type const&db, tree.get_child("mysqldump") )
//    {
//        const boost::property_tree::ptree & aDatabase = db.second; // value (or a subnode)
//        BOOST_FOREACH(boost::property_tree::ptree::value_type const&ctable, aDatabase.get_child("") )
//        {
//            const std::string & key = ctable.first.data();
//            if (key == "table_data")
//            {
//                const boost::property_tree::ptree & aTable = ctable.second;
//                int rowNumber;
//                rowNumber = 1;
//                std::vector<bsoncxx::document::value> documents;
//                BOOST_FOREACH(boost::property_tree::ptree::value_type const&row, aTable.get_child("") )
//                {
//                    const boost::property_tree::ptree & aRow = row.second;
//                    if (rowNumber <= 100000)
//                    {
//                        auto doc = bsoncxx::builder::basic::document{};
////                        using bsoncxx::builder::basic::kvp;
//                        doc.append(kvp("_tablename", table.toUtf8().constData()));
//                        BOOST_FOREACH(boost::property_tree::ptree::value_type const&field, aRow.get_child("") )
//                        {
//                            const std::string & fkey = field.first.data();
//                            if (fkey == "field")
//                            {
//                                const boost::property_tree::ptree & aField = field.second;
//                                std::string fname = aField.get<std::string>("<xmlattr>.name");
//                                std::string fvalue = aField.data();
//                                doc.append(kvp(fname,fvalue));
//                            }
//                            //QJsonDocument doc(JSONRow);
//                            //QString JSONString(doc.toJson(QJsonDocument::Compact));
//                        }
//                        documents.push_back(doc.extract());
//                        rowNumber++;
//                    }
//                    else
//                    {
//                        collection.insert_many(documents);
//                        documents.clear();
//                        auto doc = bsoncxx::builder::basic::document{};
//                        using bsoncxx::builder::basic::kvp;
//                        doc.append(kvp("_tablename", table.toUtf8().constData()));
//                        BOOST_FOREACH(boost::property_tree::ptree::value_type const&field, aRow.get_child("") )
//                        {
//                            const std::string & fkey = field.first.data();
//                            if (fkey == "field")
//                            {
//                                const boost::property_tree::ptree & aField = field.second;
//                                std::string fname = aField.get<std::string>("<xmlattr>.name");
//                                std::string fvalue = aField.data();
//                                doc.append(kvp(fname,fvalue));
//                            }
//                            //QJsonDocument doc(JSONRow);
//                            //QString JSONString(doc.toJson(QJsonDocument::Compact));
//                        }
//                        documents.push_back(doc.extract());
//                        rowNumber=1;
//                    }
//                    //collection.insert_one(doc.view());
//                }
//                if (documents.size() > 0)
//                {
//                    collection.insert_many(documents);
//                }
//            }
//        }
//    }
//}

//This function creates a bulk insert of the XML data into Mongo
void mainClass::parseDataToMongo(mongocxx::collection collection, QString table, QString fileName)
{
    pt::ptree tree;
    pt::read_xml(fileName.toUtf8().constData(), tree);
    BOOST_FOREACH(boost::property_tree::ptree::value_type const&db, tree.get_child("mysqldump") )
    {
        const boost::property_tree::ptree & aDatabase = db.second; // value (or a subnode)
        BOOST_FOREACH(boost::property_tree::ptree::value_type const&ctable, aDatabase.get_child("") )
        {
            const std::string & key = ctable.first.data();
            if (key == "table_data")
            {
                const boost::property_tree::ptree & aTable = ctable.second;
                mongocxx::bulk_write bulk{};
                BOOST_FOREACH(boost::property_tree::ptree::value_type const&row, aTable.get_child("") )
                {
                    const boost::property_tree::ptree & aRow = row.second;

                    auto doc = bsoncxx::builder::basic::document{};
                    //                        using bsoncxx::builder::basic::kvp;
                    doc.append(kvp("_tablename", table.toUtf8().constData()));
                    BOOST_FOREACH(boost::property_tree::ptree::value_type const&field, aRow.get_child("") )
                    {
                        const std::string & fkey = field.first.data();
                        if (fkey == "field")
                        {
                            const boost::property_tree::ptree & aField = field.second;
                            std::string fname = aField.get<std::string>("<xmlattr>.name");
                            std::string fvalue = aField.data();
                            doc.append(kvp(fname,fvalue));
                        }
                        //QJsonDocument doc(JSONRow);
                        //QString JSONString(doc.toJson(QJsonDocument::Compact));
                    }
                    mongocxx::model::insert_one insert_op{doc.view()};
                    bulk.append(insert_op);
                }
                collection.bulk_write(bulk);
            }
        }
    }
}


//This is the starter function. It queries the database for the relationships
//and get the child tables of the starting table (main table)
//then each table is exported in XML form using mysqldump into a temporary directory
//then for each surveyid in the main table process the respective Map File.
int mainClass::generateJSONs(QSqlDatabase db)
{
    QString sql;
    sql = "SELECT table_name,column_name,REFERENCED_TABLE_NAME,REFERENCED_COLUMN_NAME FROM information_schema.KEY_COLUMN_USAGE WHERE table_schema = '" + db.databaseName() + "' GROUP BY table_name,column_name,REFERENCED_TABLE_NAME,REFERENCED_COLUMN_NAME";
    //log(sql);
    //Here we get the tables and their relationships from the schema
    QList <TtableDef> tables;
    QSqlQuery rels(db);
    rels.exec(sql);

    while (rels.next())
    {
        TtableDef aTable;
        aTable.name = rels.value("table_name").toString();
        aTable.parent = rels.value("REFERENCED_TABLE_NAME").toString();
        tables.append(aTable);
    }
    if (tables.count() > 0)
    {
        //We create an XML document to store the relationships
        QDomDocument outputdoc;
        outputdoc = QDomDocument("DBStructFile");
        QDomElement root;
        root = outputdoc.createElement("tables");
        root.setAttribute("version", "1.0");
        outputdoc.appendChild(root);
        //We get the tables used from the starting table
        QStringList tablesUsed;
        processChilds(outputdoc,root,table,tables,tablesUsed);
        if (tablesUsed.count() > 0)
        {
            //We moved all the neccesary data into a Mongo Collection
            //The data is exported first to XML using MySQLDump
            //then using Boost to parse the XML and then do a bulk insert
            //into mongo.

            //This is the most efficient way to do this at this moment
            //because querying MySQL to extract data will affect
            //the performace if the server is heavily loaded by other processes

            //Connects to MONGO
            mongocxx::instance instance{}; // This should be done only once.
            mongocxx::uri uri("mongodb://localhost:27017");
            mongocxx::client client(uri);
            mongocxx::database mongoDB = client["mysqldenormalize"];
            //We create a collection based on the last 12 digits of a UUID
            QUuid collectionUUID=QUuid::createUuid();
            QString strCollectionUUID=collectionUUID.toString().replace("{","").replace("}","");
            strCollectionUUID = "C" + strCollectionUUID.right(12);
            mongocxx::collection coll = mongoDB[strCollectionUUID.toUtf8().constData()];

            auto index_specification = bsoncxx::builder::stream::document{} << "_tablename" << 1 << "rowuuid" << 1 << bsoncxx::builder::stream::finalize;
            coll.create_index(std::move(index_specification));
            //Call MySQLDump to export each table as XML
            //We use MySQLDump because it very very fast

            bool maria_bd = false;
            QStringList version_arguments;
            version_arguments.clear();
            version_arguments << "--version";
            QString version_info;
            QProcess *version_process = new QProcess();
            version_process->start("mariadb_config", version_arguments);
            version_process->waitForFinished(-1);            
            if (version_process->exitCode() == 0)
            {
                maria_bd = true;
                version_info = version_process->readAllStandardOutput().replace("\n","");
            }
            else
            {
                version_process->start("mysql_config", version_arguments);
                version_process->waitForFinished(-1);
                if (version_process->exitCode() == 0)
                {
                    version_info = version_process->readAllStandardOutput().replace("\n","");

                }
            }
            if (version_info == "")
            {
                log("Unable to detect the version of MySQL");
                exit(1);
            }
            QStringList version_parts = version_info.split(".");
            int version_number = version_parts[0].toInt();

            QDir currDir(tempDir);
            QString program = "mysqldump";
            QStringList arguments;
            QProcess *mySQLDumpProcess = new QProcess();
            log("Exporting data to Mongo");
            QTime procTime;
            procTime.start();
            for (int pos = 0; pos <= tablesUsed.count()-1; pos++)
            {
                arguments.clear();
                arguments << "--single-transaction";
                if (maria_bd == false && version_number >= 8)
                {
                    arguments << "--skip-column-statistics";
                    arguments << "--ssl-mode=DISABLED";
                }
                arguments << "-h" << host;
                arguments << "-u" << user;
                arguments << "--password=" + pass;
                arguments << "--skip-triggers";
                arguments << "--xml";
                arguments << "--no-create-info";
                arguments << schema;
                arguments << tablesUsed[pos];
                mySQLDumpProcess->setStandardOutputFile(currDir.absolutePath() + currDir.separator() + tablesUsed[pos] + ".xml");
                mySQLDumpProcess->start(program, arguments);
                mySQLDumpProcess->waitForFinished(-1);
                if ((mySQLDumpProcess->exitCode() > 0) || (mySQLDumpProcess->error() == QProcess::FailedToStart))
                {
                    if (mySQLDumpProcess->error() == QProcess::FailedToStart)
                    {
                        log("Error: Command " +  program + " not found");
                    }
                    else
                    {
                        log("Running mysqldump returned error");
                        QString serror = mySQLDumpProcess->readAllStandardError();
                        log(serror);
                        log("Running paremeters:" + arguments.join(" "));
                    }
                    return 1;
                }
                else
                    parseDataToMongo(coll,tablesUsed[pos],currDir.absolutePath() + currDir.separator() + tablesUsed[pos] + ".xml");
            }
            delete mySQLDumpProcess;

            //Select the Survey IDs that will be exported
            sql = "SELECT surveyid FROM " + table;
            //sql = "SELECT surveyid FROM " + table + " WHERE surveyid = '1c481d25-205f-42f7-a9c0-68fffde211bb'";
            QStringList lstIds;
            QSqlQuery qryIds(db);
            qryIds.exec(sql);
            while (qryIds.next())
                lstIds << qryIds.value(0).toString();

            log("Generating trees");
            for (int pos = 0; pos <= lstIds.count()-1; pos++)
            {
                processMapFile(coll,lstIds[pos]);
            }
            coll.drop();
            int Hours;
            int Minutes;
            int Seconds;
            int Milliseconds;

            Milliseconds = procTime.elapsed();
            Hours = Milliseconds / (1000*60*60);
            Minutes = (Milliseconds % (1000*60*60)) / (1000*60);
            Seconds = ((Milliseconds % (1000*60*60)) % (1000*60)) / 1000;
            log("Finished in " + QString::number(Hours) + " Hours," + QString::number(Minutes) + " Minutes and " + QString::number(Seconds) + " Seconds.");

            return 0;
        }
        else
        {
            log("No tables are in use. Check the main table");
            return 1;
        }
    }
    else
    {
        log("Can't query relationships");
        return 1;
    }
}

void mainClass::run()
{
    QDir dir(mapDir);
    if (dir.exists(mapDir))
    {
        QFileInfoList lstFiles;
        QStringList filters;
        filters << "*.xml";
        lstFiles = dir.entryInfoList(filters);
        if (lstFiles.count() > 0)
        {
            QDir outDir(output);
            if (outDir.exists())
            {
                QDir tDir;
                if (!tDir.exists(tempDir))
                {
                    if (!tDir.mkdir(tempDir))
                    {
                        log("Cannot create temporary directory");
                        returnCode = -1;
                        emit finished();
                    }
                }

                QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
                db.setHostName(host);
                db.setPort(port.toInt());
                db.setDatabaseName(schema);
                db.setUserName(user);
                db.setPassword(pass);
                db.setConnectOptions("MYSQL_OPT_SSL_MODE=SSL_MODE_DISABLED");
                if (db.open())
                {
                    //Openning and parsing the Create XML file
                    QDomDocument xmlCreateDocument;
                    QFile fileCreate(createFile);
                    if (!fileCreate.open(QIODevice::ReadOnly))
                    {
                        log("Cannot open create xml file");
                        returnCode = -1;
                        emit finished();
                    }
                    if (!xmlCreateDocument.setContent(&fileCreate))
                    {
                        log("Cannot parse create xml file");
                        fileCreate.close();
                        returnCode = -1;
                        emit finished();
                    }
                    fileCreate.close();

                    QDomElement createTablesNode = xmlCreateDocument.documentElement();
                    QDomNode start_node = createTablesNode.firstChild().nextSibling();
                    tables_in_create = start_node.toElement().elementsByTagName("table");

                    if (generateJSONs(db) == 0)
                    {
                        db.close();
                        emit finished();
                    }
                    else
                    {
                        db.close();
                        emit finished();
                    }
                }
                else
                {
                    log("Cannot connect to the database");
                    returnCode = -1;
                    emit finished();
                }
            }
            else
            {
                log("Output directory does not exists");
                returnCode = -1;
                emit finished();
            }
        }
        else
        {
            log("Map directory does not contains XML files");
            returnCode = -1;
            emit finished();
        }
    }
    else
    {
        log("Map directory does not exists");
        returnCode = -1;
        emit finished();
    }
}
