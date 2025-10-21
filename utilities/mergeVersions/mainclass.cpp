#include "mainclass.h"
#include "mergecreate.h"
#include "compareinsert.h"

mainClass::mainClass(QObject *parent) : QObject(parent)
{
    returnCode = 0;
}

void mainClass::log(QString message)
{
    QString temp;
    temp = message + "\n";
    printf("%s",temp.toUtf8().data());
}

void mainClass::run()
{
    QDomDocument XMLResult;
    QDomElement XMLRoot;
    QDomElement eErrors;
    if (output_type != "h")
    {
        XMLResult = QDomDocument("XMLResult");
        XMLRoot = XMLResult.createElement("XMLResult");
        XMLResult.appendChild(XMLRoot);
        eErrors = XMLResult.createElement("errors");
        XMLRoot.appendChild(eErrors);
    }

    compareInsert insert;
    insert.setFiles(a_insertXML,b_insertXML,c_insertXML,d_insertSQL,output_type,valuesToIgnore);
    returnCode = insert.compare();
    if (output_type != "h")
    {
        QList<TcompError> errorList = insert.getErrorList();
        for (int pos = 0; pos < errorList.count(); pos++)
        {
            QDomElement anError;
            anError = XMLResult.createElement("error");
            anError.setAttribute("table",errorList[pos].table);
            anError.setAttribute("field",errorList[pos].field);
            anError.setAttribute("code",errorList[pos].code);
            anError.setAttribute("value",errorList[pos].value);
            anError.setAttribute("from",errorList[pos].from);
            anError.setAttribute("to",errorList[pos].to);
            eErrors.appendChild(anError);
            if (errorList[pos].code == "RNS")
                returnCode = 1;
        }
    }
//    qDebug() << "------------------------";
//    qDebug() << returnCode;
//    qDebug() << "------------------------";
    //if (returnCode == 0)
    //{

        int loaded = insert.createCFile();

//        qDebug() << "------------------------0";
//        qDebug() << loaded;
//        qDebug() << "------------------------0";

        if (loaded == 0)
        {
            mergeCreate create;
            create.setFiles(a_createXML,b_createXML,c_createXML,d_createSQL,output_type);
            create.setInsertDiff(insert.getDiffs());
            create.properties = this->properties;
            returnCode = create.compare();
            if (output_type != "h")
            {
                QList<TcompError> errorList = create.getErrorList();
                for (int pos = 0; pos < errorList.count(); pos++)
                {
                    QDomElement anError;
                    anError = XMLResult.createElement("error");
                    anError.setAttribute("table",errorList[pos].table);
                    anError.setAttribute("field",errorList[pos].field);
                    anError.setAttribute("code",errorList[pos].code);
                    anError.setAttribute("value",errorList[pos].value);
                    anError.setAttribute("from",errorList[pos].from);
                    anError.setAttribute("to",errorList[pos].to);
                    eErrors.appendChild(anError);
                    if (errorList[pos].code == "TNS" || errorList[pos].code == "TWP" || errorList[pos].code == "FNS")
                        returnCode = 1;
                }
            }
            if (returnCode == 0)
            {
                QStringList insertsUsed = create.getInsertTablesUsed();
                for (int pos = 0; pos < insertsUsed.count(); pos++)
                {
                    insert.setAsParsed(insertsUsed[pos]);
                }
            }
        }
    //}
    if (returnCode == 0)
    {
        returnCode = insert.createDiffFile();
    }
    if (output_type != "h")
    {
        log(XMLResult.toString());
        if (save_to_file)
        {
            if (QFile::exists(error_file))
                QFile::remove(error_file);
            QFile file(error_file);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                out.setCodec("UTF-8");
#endif
                XMLResult.save(out,1,QDomNode::EncodingFromTextStream);
                file.close();
            }
            else
                log("Error: Cannot create xml error file");
        }
    }
    emit finished();
}
void mainClass::setParameters(QString createA, QString createB, QString insertA, QString insertB, QString createC, QString insertC, QString diffCreate, QString diffInsert, QString outputType, QList<TignoreTableValues> toIgnore, bool saveToFile, QString errorFile, QStringList properties)
{
    a_createXML = createA;
    b_createXML = createB;
    a_insertXML = insertA;
    b_insertXML = insertB;
    c_createXML = createC;
    c_insertXML = insertC;
    d_createSQL = diffCreate;
    d_insertSQL = diffInsert;
    output_type = outputType;
    valuesToIgnore = toIgnore;
    error_file = errorFile;
    save_to_file = saveToFile;
    this->properties = properties;
}
