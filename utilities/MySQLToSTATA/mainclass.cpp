#include "mainclass.h"
#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QProcess>
#include <QChar>
#include <QElapsedTimer>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QUuid>
#include "jsonworker.h"
#include "listmutex.h"
#include <QSqlQuery>

namespace pt = boost::property_tree;

mainClass::mainClass(QObject *parent) : QObject(parent)
{
    returnCode = 0;
    letterIndex = 1;
}

void mainClass::log(QString message)
{
    QString temp;
    temp = message + "\n";
    printf("%s", temp.toUtf8().data());
}

void mainClass::setParameters(QString host, QString port, QString user, QString pass, QString schema, QString createXML, QString outputDir, bool protectSensitive, QString tempDir, QString encryption_key, int num_workers)
{
    this->host = host;
    this->port = port;
    this->user = user;
    this->pass = pass;
    this->num_workers = num_workers;
    this->schema = schema;
    this->outputDirectory = outputDir;
    this->protectSensitive = protectSensitive;
    this->tempDir = tempDir;
    this->createXML = createXML;
    this->encryption_key = encryption_key;
    this->db = QSqlDatabase::addDatabase("QMYSQL","repository");
    db.setHostName(host);
    db.setPort(port.toInt());
    db.setDatabaseName(schema);
    db.setUserName(user);
    db.setPassword(pass);
    if (!db.open())
    {
        log("Error while conneting to MySQL");
        exit(1);
    }
}

QString mainClass::getSheetDescription(QString name)
{
    QString truncated;
    truncated = name.left(25);
    truncated = truncated.replace("[","");
    truncated = truncated.replace("]","");
    truncated = truncated.replace(":","");
    truncated = truncated.replace("*","");
    truncated = truncated.replace("?","");
    truncated = truncated.replace("/","");
    truncated = truncated.replace("\\","");
    if (tableNames.indexOf(truncated) == -1)
    {
        tableNames.append(truncated);
        return truncated;
    }
    else
    {
        truncated = truncated + "_" + QString::number(letterIndex);
        letterIndex++;
        tableNames.append(truncated);
        return truncated;
    }
}

void mainClass::getMultiSelectInfo(QDomNode table, QString table_name, QString &multiSelect_field, QStringList &keys, QString &rel_table, QString &rel_field)
{
    QDomNode child = table.firstChild();
    while (!child.isNull())
    {
        if (child.toElement().tagName() == "table")
        {
            if (child.toElement().attribute("name") == table_name)
            {
                QDomNode field = child.firstChild();
                while (!field.isNull())
                {
                    if (field.toElement().attribute("rlookup","false") == "true")
                    {
                        multiSelect_field = field.toElement().attribute("name");
                        rel_table = field.toElement().attribute("rtable");
                        rel_field = field.toElement().attribute("rfield");
                    }
                    else
                    {
                        if (field.toElement().attribute("key","false") == "true")
                        {
                            keys.append(field.toElement().attribute("name"));
                        }
                    }
                    field = field.nextSibling();
                }
            }
        }
        child = child.nextSibling();
    }
}

void mainClass::loadTable(QDomNode table)
{
    QDomElement eTable;
    eTable = table.toElement();

    TtableDef aTable;
    aTable.islookup = false;
    aTable.name = eTable.attribute("name","");
    aTable.desc = eTable.attribute("name","");
    if (aTable.name.indexOf("_msel_") >= 0)
        aTable.ismultiselect = true;
    else
        aTable.ismultiselect = false;

    QDomNode field = table.firstChild();
    while (!field.isNull())
    {
        QDomElement eField;
        eField = field.toElement();
        if (eField.tagName() == "field")
        {
            TfieldDef aField;
            aField.name = eField.attribute("name","");
            aField.desc = eField.attribute("desc","");
            aField.type = eField.attribute("type","");
            aField.size = eField.attribute("size","").toInt();
            aField.decSize = eField.attribute("decsize","").toInt();
            if (eField.attribute("rlookup","false") == "true")
            {
                aField.isLookUp = true;
                aField.lookupRelTable = eField.attribute("rtable");
                aField.lookupRelField = eField.attribute("rfield");
            }
            if (eField.attribute("sensitive","false") == "true")
            {
                aField.sensitive = true;
                aField.protection = eField.attribute("protection","exclude");
            }
            else
                aField.sensitive = false;
            if (eField.attribute("key","false") == "true")
            {
                TfieldDef keyField;
                keyField.name = aField.name;
                keyField.replace_value = "";
                aField.isKey = true;
                if (aField.sensitive == true)
                {
                    if (protectedKeys.indexOf(aField.name) < 0)
                        protectedKeys.append(aField.name);
                }
            }
            else
                aField.isKey = false;
            // NOTE ON Rank. Rank is basically a multiselect with order and handled as a multiselect by ODK Tools. However
            // we cannot pull the data from the database because the records may not be stored in the same order the user placed them in Collect
            if ((eField.attribute("isMultiSelect","false") == "true") && (eField.attribute("odktype","") != "rank"))
            {
                aField.isMultiSelect = true;
                aField.multiSelectTable = eField.attribute("multiSelectTable");
                QString multiSelect_field;
                QStringList keys;
                QString multiSelectRelTable;
                QString multiSelectRelField;
                getMultiSelectInfo(table, aField.multiSelectTable, multiSelect_field, keys, multiSelectRelTable, multiSelectRelField);
                aField.multiSelectField = multiSelect_field;
                aField.multiSelectRelTable = multiSelectRelTable;
                aField.multiSelectRelField = multiSelectRelField;
                aField.multiSelectKeys.append(keys);
            }
            aTable.fields.append(aField);
        }
        else
        {
            loadTable(field);
        }
        field = field.nextSibling();
    }
    mainTables.append(aTable);

}

int  mainClass::processTasks(QDir currDir)
{
    ListMutex *mutex = new ListMutex(this);

    QList< JSONWorker*> workers;
    for (int w=1; w <= num_workers; w++)
    {
        JSONWorker *a_worker = new JSONWorker(this);
        a_worker->setName("Worker" + QString::number(w));
        a_worker->setParameters(host, port, user, pass, schema, currDir, outputDirectory);
        workers.append(a_worker);
    }

    //qDebug() << "Working on separation: " + QString::number(separate_task_list.count());
    //Work on separation
    mutex->set_total(separate_task_list.count());
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->setTasks(separate_task_list);
        workers[w]->setMutex(mutex);
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->start();
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->wait();
    }
    for (int w=0; w < workers.count(); w++)
    {
        if (workers[w]->status != 0)
            return -1;
    }

    //qDebug() << "Working on applying updates: " + QString::number(update_task_list.count());
    //Work on the updates
    mutex->set_total(update_task_list.count());
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->setTasks(update_task_list);
        workers[w]->setMutex(mutex);
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->start();
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->wait();
    }
    for (int w=0; w < workers.count(); w++)
    {
        if (workers[w]->status != 0)
            return -1;
    }

    //qDebug() << "Working on generating JSON files";
    //Work on the JSON extract
    mutex->set_total(json_task_list.count());
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->setTasks(json_task_list);
        workers[w]->setMutex(mutex);
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->start();
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->wait();
    }
    for (int w=0; w < workers.count(); w++)
    {
        if (workers[w]->status != 0)
            return -1;
    }

    //qDebug() << "Working on merging the final files";
    //Work in the merging
    mutex->set_total(merge_task_list.count());
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->setTasks(merge_task_list);
        workers[w]->setMutex(mutex);
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->start();
    }
    for (int w=0; w < workers.count(); w++)
    {
        workers[w]->wait();
    }
    for (int w=0; w < workers.count(); w++)
    {
        if (workers[w]->status != 0)
            return -1;
    }
    return 0;
}

QStringList mainClass::get_parts(int total, int parts)
{
    QStringList res;
    int div = total / parts;
    int start = 1;
    int end = div;
    if (parts >= total)
    {
        res.append("1|" + QString::number(total));
        return res;
    }
    if (parts == 1)
    {
        res.append("1|" + QString::number(total));
        return res;
    }
    if (total >= 500)
    {
        for (int pos = 1; pos < parts; pos++)
        {
            res.append(QString::number(start) + "|" + QString::number(end));
            start = end + 1 ;
            end = start + div;
        }
        res.append(QString::number(start) + "|" + QString::number(total));
    }
    else
    {
        res.append("1|" + QString::number(total));
    }
    return res;
}

bool mainClass::is_lookup_int(QString table)
{
    for (int pos=0; pos < lookupTables.count(); pos++)
    {
        if (lookupTables[pos].name == table)
            return lookupTables[pos].is_numeric;
    }
    return false;
}

int mainClass::generateXLSX()
{

    QDomDocument docA("input");
    QFile fileA(createXML);
    if (!fileA.open(QIODevice::ReadOnly))
    {
        log("Cannot open input create XML file");
        returnCode = 1;
        return returnCode;
    }
    if (!docA.setContent(&fileA))
    {
        log("Cannot parse input create XML file");
        fileA.close();
        returnCode = 1;
        return returnCode;
    }
    fileA.close();


    QDomElement rootA = docA.documentElement();

    //Load the lookup tables
    QDomNode lkpTable = rootA.firstChild().firstChild();
    //Getting the fields to export from Lookup tables
    while (!lkpTable.isNull())
    {
        QDomElement eTable;
        eTable = lkpTable.toElement();

        TtableDef aTable;
        aTable.islookup = true;
        aTable.name = eTable.attribute("name","");
        aTable.desc = eTable.attribute("name","");

        QDomNode field = lkpTable.firstChild();
        while (!field.isNull())
        {
            QDomElement eField;
            eField = field.toElement();

            TfieldDef aField;
            if (eField.attribute("name","").right(4) == "_cod")
                aTable.lkp_code_field = eField.attribute("name","");
            if (eField.attribute("name","").right(4) == "_des")
                aTable.lkp_desc_field = eField.attribute("name","");
            if (eField.attribute("type","").right(4) == "int")
                aTable.is_numeric = true;

            field = field.nextSibling();
        }
        lookupTables.append(aTable);

        lkpTable = lkpTable.nextSibling();
    }

    //Getting the fields to export from tables
    QDomNode table = rootA.firstChild().nextSibling().firstChild();

    //Load the data tables recursively
    loadTable(table);
    for (int nt =mainTables.count()-1; nt >= 0;nt--)
    {
        tables.append(mainTables[nt]);
    }

    if (rootA.tagName() == "XMLSchemaStructure")
    {

        QDir currDir(tempDir);

        QElapsedTimer procTime;
        procTime.start();
        QString sql;
        QStringList fields;
        QStringList fields_for_select;
        QStringList descfields;
        QStringList fields_for_create;       
        QStringList drops;

        QDir finalDir(outputDirectory);

        if (!finalDir.exists(outputDirectory))
        {
            if (!finalDir.mkdir(outputDirectory))
            {
                log("Cannot create temporary output directory");
                returnCode = 1;
                emit finished();
            }
        }

        for (int pos = 0; pos < tables.count(); pos++)
        {
            TtaskItem a_merge_task;
            a_merge_task.task_type = 4;
            a_merge_task.table = tables[pos].name;
            a_merge_task.final_file = finalDir.absolutePath() + currDir.separator() + tables[pos].name + ".json";


            TtaskItem a_json_task;
            a_json_task.task_type = 2;
            a_json_task.table = tables[pos].name;

            a_json_task.sql_file = currDir.absolutePath() + currDir.separator() + tables[pos].name + "_query.sql";
            a_json_task.json_file = currDir.absolutePath() + currDir.separator() + tables[pos].name + ".json";

            a_merge_task.json_files.append(currDir.absolutePath() + currDir.separator() + tables[pos].name + ".json");


            fields.clear();
            descfields.clear();
            for (int fld = 0; fld < tables[pos].fields.count(); fld++)
            {
                if (this->protectSensitive)
                {
                    if (tables[pos].fields[fld].sensitive == false)
                    {
                        fields.append(tables[pos].fields[fld].name);
                    }
                    else
                    {
                        if (tables[pos].fields[fld].protection != "exclude")
                            fields.append("HEX(AES_ENCRYPT(" + tables[pos].name + "." + tables[pos].fields[fld].name + ",UNHEX('" + this->encryption_key + "'))) as " + tables[pos].fields[fld].name);
                    }
                }
                else
                    fields.append(tables[pos].fields[fld].name);
            }

            QStringList fields_to_select;
            for (int fld =0; fld < fields.count(); fld++)
            {
                if (fields[fld].indexOf(" as ") < 0)
                    fields_to_select.append("ifnull(`" + fields[fld] + "`,'') as `" + fields[fld] + "`");
                else
                    fields_to_select.append(fields[fld]);
            }

            sql = "SELECT " + fields_to_select.join(",") + " FROM " + tables[pos].name + ";\n";
            QFile sqlfile(currDir.absolutePath() + currDir.separator() + tables[pos].name + "_query.sql");
            if (!sqlfile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                return 1;
            }
            QTextStream out(&sqlfile);
            out << sql;
            sqlfile.close();

            json_task_list.append(a_json_task);
            merge_task_list.append(a_merge_task);

        }
        int result = processTasks(currDir);

        // Generating DO files for each table
        for (int pos = 0; pos < tables.count(); pos++)
        {
            QString do_file = finalDir.absolutePath() + currDir.separator() + tables[pos].name + ".do";

            QFile dofile(do_file);
            if (!dofile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                log("Cannot open DO file for writing");
                return 1;
            }
            QTextStream do_out(&dofile);
            do_out << "/* This script will generate a STATA " + tables[pos].name + ".dta file from a " + tables[pos].name + ".raw tab delimited file*/\n";
            do_out << "/* You only need to run this once.*/\n";
            do_out << "/* Change c:\\my_working_dir to the directory holding this file.*/\n";
            do_out << "cd \"c:\\my_working_dir\"\n";
            do_out << "import delimited \"" + tables[pos].name + ".raw\", delimiter(tab) varnames(1) encoding(UTF-8) stringcols(_all)\n";

            for (int fld=0; fld < tables[pos].fields.count(); fld++)
            {
                QString line_data;

                if (tables[pos].fields[fld].type == "double")
                    line_data = "destring " + tables[pos].fields[fld].name + ", replace float force\n";
                if (tables[pos].fields[fld].type.indexOf("decimal") >= 0)
                    line_data = "destring " + tables[pos].fields[fld].name + ", replace float force\n";
                if (tables[pos].fields[fld].type.indexOf("int") >= 0)
                    line_data =  "destring " + tables[pos].fields[fld].name + ", replace force\n";
                if (tables[pos].fields[fld].type.indexOf("varchar") >= 0)
                {
                    QString zize = QString::number(tables[pos].fields[fld].size);
                    line_data =  "recast str"+ zize + " " + tables[pos].fields[fld].name + ", force\n";
                }
                if (line_data != "")
                    do_out << line_data;
            }
            do_out << "save " + tables[pos].name + "\n\n";
            for (int fld=0; fld < tables[pos].fields.count(); fld++)
            {
                do_out << "label variable " + tables[pos].fields[fld].name + " \"" + tables[pos].fields[fld].desc.replace("\"","") + "\"\n";
            }
            do_out << "\n";

            for (int fld=0; fld < tables[pos].fields.count(); fld++)
            {
                if (tables[pos].fields[fld].isLookUp == true)
                {
                    if (is_lookup_int(tables[pos].fields[fld].lookupRelTable))
                        do_out << "label values " + tables[pos].fields[fld].name + " " + tables[pos].fields[fld].lookupRelTable + "\n";
                    else
                        do_out << "* label values " + tables[pos].fields[fld].name + " " + tables[pos].fields[fld].lookupRelTable + "\n";
                }
            }

            dofile.close();
        }

        QString do_file_lbl = finalDir.absolutePath() + currDir.separator() + "lookups.do";
        QFile dofile_lbl(do_file_lbl);
        if (!dofile_lbl.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            log("Cannot open DO file for writing");
            return 1;
        }
        QTextStream do_out_lbl(&dofile_lbl);

        QSqlQuery lkps(db);
        do_out_lbl << "#delimit ;\n";

        for (int pos = 0; pos < lookupTables.count(); pos++)
        {
            if (lookupTables[pos].is_numeric)
            {
                do_out_lbl << "label define " + lookupTables[pos].name + "\n";
                QString sql = "SELECT " + lookupTables[pos].lkp_code_field + "," + lookupTables[pos].lkp_desc_field + " FROM " + lookupTables[pos].name;
                //qDebug() << sql;
                lkps.exec(sql);
                while(lkps.next())
                {
                    do_out_lbl << lkps.value(0).toString() + " \"" + lkps.value(1).toString() + "\"\n";
                }
                do_out_lbl << ";\n";
            }
            else
            {
                do_out_lbl << "* label define " + lookupTables[pos].name + "\n";
                QString sql = "SELECT " + lookupTables[pos].lkp_code_field + "," + lookupTables[pos].lkp_desc_field + " FROM " + lookupTables[pos].name;
                //qDebug() << sql;
                lkps.exec(sql);
                while(lkps.next())
                {
                    do_out_lbl << "* " + lkps.value(0).toString() + " \"" + lkps.value(1).toString() + "\"\n";
                }
                do_out_lbl << "* ;\n";
            }
        }
        do_out_lbl << "#delimit cr\n";
        //exit(1);


        QSqlQuery drop(db);
        for (int d=0; d < drops.count(); d++)
        {
            drop.exec(drops[d]);
        }
        db.close();
        QDir tdir(tempDir);
        tdir.removeRecursively();

        if (result != 0)
        {
            returnCode = 1;
            return returnCode;
        }

        int Hours;
        int Minutes;
        int Seconds;
        int Milliseconds;

        if (returnCode == 0)
        {
            Milliseconds = procTime.elapsed();
            Hours = Milliseconds / (1000*60*60);
            Minutes = (Milliseconds % (1000*60*60)) / (1000*60);
            Seconds = ((Milliseconds % (1000*60*60)) % (1000*60)) / 1000;
            log("Finished in " + QString::number(Hours) + " Hours," + QString::number(Minutes) + " Minutes and " + QString::number(Seconds) + " Seconds.");
        }
        return returnCode;
    }
    else
    {
        log("The input create XML file is not valid");
        returnCode = 1;
        return returnCode;
    }
}

void mainClass::run()
{
    if (QFile::exists(createXML))
    {
        QDir tDir;
        if (!tDir.exists(tempDir))
        {
            if (!tDir.mkdir(tempDir))
            {
                log("Cannot create temporary directory");
                returnCode = 1;
                emit finished();
            }
        }

        if (generateXLSX() == 0)
        {
            returnCode = 0;
            emit finished();
        }
        else
        {
            returnCode = 1;
            emit finished();
        }
    }
    else
    {
        log("The create XML file does not exists");
        returnCode = 1;
        emit finished();
    }
}
