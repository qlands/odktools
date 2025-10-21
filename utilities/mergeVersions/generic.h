#ifndef GENERIC_H
#define GENERIC_H
#include <QString>

struct tableDiff
{
  QString table;
  QStringList diff;
  bool parsed = false;
};
typedef tableDiff TtableDiff;

struct compError
{
  QString code;
  QString desc;
  QString table;
  QString field;
  QString value;
  QString from;
  QString to;
};
typedef compError TcompError;

struct ignoreTableValues
{
  QString table;
  QStringList values;
};
typedef ignoreTableValues TignoreTableValues;

#endif // GENERIC_H
