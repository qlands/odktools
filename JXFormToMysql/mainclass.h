#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>
#include <QDomDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QDir>



struct duplicatedLookUp
{
    QStringList tables;
    QString sameas;
};
typedef duplicatedLookUp TduplicatedLookUp;

struct tblwitherror
{
    QString name;
    int num_selects = 0;
    int num_fields = 0;
    int page_size = 0;
};
typedef tblwitherror Ttblwitherror;

struct lngDesc
{
    QString langCode;
    QString desc;
};
typedef lngDesc TlngLkpDesc;

struct sepSection
{
    QString name;
    QString desc;
};
typedef sepSection TsepSection;

//Variable mapping structure between ODK and MySQL
struct fieldMap
{
    QString type;
    int size;
    int decSize;
};
typedef fieldMap TfieldMap;

//Extra columns in survey
struct extraSurveyColum
{
    QString name;
    QString value;
};
typedef extraSurveyColum TextraSurveyColum;

//Field Definition structure
struct fieldDef
{
    QString name; //Field Name
    QList<TlngLkpDesc > desc; //List of field descriptions in different languages
    QString type; //Variable type in MySQL
    QString odktype; //Variable type in MySQL
    int size; //Variable size
    int decSize; //Variable decimal size
    bool calculateWithSelect;
    QString formula;
    QString rTable; //Related table
    QString rField; //Related field
    QString rName; //Contraint name
    bool key; //Whether the field is key
    QString xmlCode; //The field XML code /xx/xx/xx/xx
    QString xmlFullPath; //The field XML code /xx/xx/xx/xx with groups
    bool isMultiSelect; //Whether the field if multiselect
    QString multiSelectTable; //Multiselect table
    QString selectSource; //The source of the select. Internal, External or Search
    QString selectListName; //The list name of the select
    bool sensitive;
    int selectType=0;
    QString externalFileName;
    QString codeColumn;
    QString descColumn;
    bool autoincrement = false;
    QList<TextraSurveyColum > extraSurveyColumns; //List of extra columns in survey
};
typedef fieldDef TfieldDef;

struct ODKGroupingDef
{
    QString varibleType; //Type
    QString varibleName; //Name
    bool isRepeat;
};
typedef ODKGroupingDef TODKGroupingDef;

//Language structure
struct langDef
{
    QString code;
    QString desc;
    bool deflang; //Wether the language is default
    bool coded;
};
typedef langDef TlangDef;


struct otherLkpValue
{
    QString column_name;
    QVariant column_value;
};
typedef otherLkpValue TotherLkpValue;



//Sirvey Variables in ODK
struct surveyVariableDef
{
    QString name;
    QString fullName;
    QString type;
    QJsonObject object;
};
typedef surveyVariableDef TsurveyVariableDef;

//Lookup value structure
struct lkpValue
{
    QString code;
    QList<TlngLkpDesc > desc; //List of lookup values in different languages
    QStringList other_cols;
    QList<TotherLkpValue > other_values;
};
typedef lkpValue TlkpValue;

//Table structure. Hold information about each table in terms of name, xmlCode, fields
//and, if its a lookuptable, the lookup values
struct tableDef
{
    QString name;
    QList<TlngLkpDesc > desc; //List of table descriptions in different languages
    QList<TfieldDef> fields; //List of fields
    QList<TlkpValue> lkpValues; //List of lookup values
    QStringList propertyList;
    QStringList propertyTypes;
    int pos; //Global position of the table
    bool islookup; //Whether the table is a lookup table
    bool isOneToOne; //Whether the table has been separated
    QString xmlCode; //The table XML code /xx/xx/xx/xx
    QString xmlFullPath; //The table XML code /xx/xx/xx/xx
    QString parentTable; //The parent of the table
    QDomElement tableElement; //Each table is an Dom Element for building the manifest XML file
    QDomElement tableCreteElement; //Each table is a second Dom Element for building the XML Create file
    QStringList loopItems;
    bool isLoop;
    bool isOSM;
    bool isGroup;
    bool hasOther = false;
    QString lookupCSV;
};
typedef tableDef TtableDef;

struct duplicatedSelectValue
{
    QString variableName;
    QString selectValue;
};
struct invalidSelectValue
{
    QString variableName;
    QString selectValue;
};
typedef duplicatedSelectValue TduplicatedSelectValue;
typedef invalidSelectValue TinvalidSelectValue;

struct duplicatedField
{
    QString table;
    QStringList fields;
};
typedef duplicatedField TduplicatedField;


class mainClass : public QObject
{
    Q_OBJECT
public:
    explicit mainClass(QObject *parent = 0);
    int returnCode;
signals:
    void finished();
    void finishedWithError(int error);
public slots:
    void run();
    void setParameters(int p_argc, char *p_argv[]);
private:
    void logLoopError();

    int argc;
    char *argv;

    bool debug;
    bool ignore_too_many_selects = false;
    QString command;
    QString outputType;
    QString default_language;
    QStringList variableStack; //This is a stack of groups or repeats for a variable. Used to get /xxx/xxx/xxx structures
    QStringList repeatStack; //This is a stack of repeats. So we know in which repeat we are
    QString prefix; //Table prefix
    int tableIndex; //Global index of a table. Used later on to sort them
    QStringList supportFiles;
    QStringList submittedFiles;
    bool primaryKeyAdded;
    int CSVRowNumber;
    bool CSVColumError;
    QStringList CSVvalues;
    QStringList CSVSQLs;
    int numColumns;
    int numColumnsInData;
    QStringList duplicatedTables;
    bool justCheck;
    QStringList requiredFiles;
    QStringList extraColumnsInSurvey;
    QStringList extraColumnsInOptions;
    QStringList ODKLanguages;
    bool hasSelects;
    bool hasOnlyExternalSelects;
    QStringList extra_survey_columns;
    QStringList extra_choices_columns;
    QStringList extra_invalid_columns;

    QDomDocument XMLResult;
    QDomElement XMLDocRoot;
    bool logXMLError=false;

    //List of languages
    QList <TlangDef> languages;

    QList<TtableDef> tables; //List of tables
    QList<TtableDef> merging_tables; //List of tables

    QList<TduplicatedSelectValue> duplicatedSelectValues;
    QList<TinvalidSelectValue> invalidSelectValues;

    QList<TduplicatedField> duplicatedFields;

    QStringList invalidFieldNames;
    QStringList invalidDataColumnName;
    QStringList invalidFields;

    QList <QJsonObject > readOnlyCalculates;

    QList< TduplicatedLookUp> duplicated_lookups;


    // Main vars

    // QString input;
    // QString ddl;
    // QString insert;
    // QString drop;
    // QString insertXML;
    // QString metadata;
    // QString xmlFile;
    // QString xmlCreateFile;
    // QString mTable;
    // QString mainVar;
    // QString lang;
    // QString defLang;
    // QString transFile;
    // QString tempDirectory;
    // QString prefixArg;
    // QString parseSurvey;
    // QString parseChoices;
    // bool displayLanguages;


    //Functions
    void isFieldValid(QString field, bool select);
    void loadInvalidFieldNames();
    void addRequiredFile(QString fileName);
    void checkFieldName(TtableDef table, QString fieldName);
    void checkTableName(QString tableName);
    int isSelect(QString variableType);
    bool isNote(QString variableType);
    bool isCalculate(QString variableType);
    void log(QString message);
    void report_file_error(QString file_name);
    QString fixColumnName(QString column);
    bool isColumnValid(QString column);
    int convertCSVToSQLite(QString fileName, QDir tempDirectory, QSqlDatabase database);
    QDomElement getTableCreateElement(QString table);
    QDomElement getTableElement(QString table);
    //bool lkpComp(TlkpValue left, TlkpValue right);
    //bool tblComp(TtableDef left, TtableDef right);
    QString getDescForLanguage(QList<TlngLkpDesc > lkpList, QString langCode);
    QString getDefLanguage();
    QString getDefLanguageCode();
    QString getLanguageCode(QString languageName);
    int getMaxDescLength(QList<TlkpValue> values, int minimum=256);
    int getMaxValueLength(QList<TlkpValue> values, QString fieldType, int minimum=128);
    bool areValuesStrings(QList<TlkpValue> values);
    QString get_related_usage(QString table);
    TtableDef checkDuplicatedLkpTable(QString table, QList<TlkpValue> thisValues, bool select_from_file, QString lookupCSV);
    QString getKeyField(QString table);
    QString getRelatedName(TtableDef table, QString relatedTable);
    QString getForeignColumns(TtableDef table, QString relatedTable);
    QString getReferencedColumns(TtableDef table, QString relatedTable);
    bool isRelatedTableLookUp(QString relatedTable);
    QString fixString(QString source);
    void generateOutputFiles(QString ddlFile,QString insFile, QString metaFile, QString xmlFile, QString transFile, QString XMLCreate, QString insertXML, QString dropSQL);
    TfieldMap mapODKFieldTypeToMySQL(QString ODKFieldType);
    int getTableIndex(QString name);
    void appendUUIDs();
    bool isDefaultLanguage(QString language);
    int getCodedLangIndexByName(QString language);
    int genLangIndexByName(QString language);
    void addToStack(QString groupOrRepeat, QString type);
    QString fixField(QString source, bool select=false);
    void addToRepeat(QString repeat);
    bool removeFromStack();
    bool removeRepeat();
    QString getVariableStack(bool full);
    QString getTopRepeat();
    TtableDef getTable(QString name);
    bool selectHasOrOther(QString variableType);
    QList <TlngLkpDesc > getLabels(QJsonValue labelValue);
    bool checkSelectValue(QString variableName, QList<TlkpValue> values, QString value, bool report=true, bool multiSelect = false);
    QList<TlkpValue> getSelectValuesFromGeoJSON(QString variableName, QString variableType, QString fileName, int &result, QDir dir, QString codeColumn, QString descColumn, QStringList &propertyList, QStringList &propertyTypes);
    QList<TlkpValue> getSelectValuesFromXML(QString variableName, QString variableType, QString fileName, bool hasOrOther, int &result, QDir dir, QString codeColumn="name", QString descColumn="label");
    QList<TlkpValue> getSelectValuesFromCSV2(QString variableName, QString variableType, QString fileName, bool hasOrOther, int &result, QDir dir, QSqlDatabase database, QString queryValue, QString codeColumn="name", QString descColumn="label");
    QList<TlkpValue> getSelectValuesFromCSV(QString searchExpresion, QJsonArray choices,QString variableName, QString variableType, bool hasOrOther, int &result, QDir dir, QSqlDatabase database, QString &file, QString &codeColumn, QString &descColumn);
    QStringList getExtraColumns(QJsonArray choices);
    QStringList getExtraColumnsTypes(QJsonArray choices, QStringList extra_colums);
    bool checkColumnName(QString name);
    QList<TlkpValue> getSelectValues(QString variableName,QString variableType,QJsonArray choices, bool hasOther, QStringList extraColumns);
    void getReferenceForSelectAt(QString calculateExpresion,QString &fieldType, int &fieldSize, int &fieldDecSize, QString &fieldRTable, QString &fieldRField);
    QString getUUIDCode(bool last = false);
    void parseOSMField(TtableDef &OSMTable, QJsonObject fieldObject);
    void parseField(QJsonObject fieldObject, QString mainTable, QString mainField, QDir dir, QSqlDatabase database, QString varXMLCode ="");
    void getJSONRootVariables(QJsonObject JSONObject, QList <TsurveyVariableDef> &surveyVariables);
    void parseTable(QJsonObject tableObject, QString tableType, bool repeatOfOne = false);
    bool isRepeatOfOne(QString repeat_control);
    void parseJSONObject(QJsonObject JSONObject, QString mainTable, QString mainField, QDir dir, QSqlDatabase database);
    void getLanguages(QJsonObject JSONObject, QStringList &languageList, int &num_labels);
    void reportDuplicatedFields();
    void reportDuplicatedTables();
    void reportSelectDuplicates();
    void reportSelectInvalid();
    QList<TlkpValue> getValuesFromInsertFile(QString tableName, QDomNode startNode, QString &clmCode, QString &cmlDesc);
    int genLangIndex(QString langCode);
    int addLanguage(QString langCode, bool defLang, bool coded);
    int addLanguage2(QString code, QString name, bool defLang);
    int processJSON(QString inputFile, QString mainTable, QString mainField, QDir dir, QSqlDatabase database);
    int getFieldIndex(TtableDef table, QString fieldName);
    bool checkTables2();
    void protect_sensitive();

};

#endif // MAINCLASS_H
