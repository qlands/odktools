TEMPLATE = subdirs

CONFIG += debug

SUBDIRS = createFromXML \
    insertFromXML \
    MySQLToXLSX \
    MySQLToCSV \
    MySQLToJSON \
    MySQLToSTATA \
    createAuditTriggers \
    createDummyJSON \
    MySQLToSQLite \
    mergeVersions \
    createTemporaryTable \
    DCFToODK
