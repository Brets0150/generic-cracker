#ifndef RUNTHREAD_H
#define RUNTHREAD_H

#include <QObject>
#include <iostream>
#include <QThread>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QCryptographicHash>
#include <QDebug>
#include <QRegularExpression>
#include <cmath>

class RunThread : public QThread
{
    Q_OBJECT
public:
    RunThread(int type, QString attack, QString hashlist, long long int skip, long long int length, int timeout, QString hashType = "ALL,!user,salt", int iterations = 10);

private:
    int type;
    QString attack;
    long long int skip;
    long long int length;
    QString hashlist;
    int timeout;
    QString hashType;
    int iterations;

    QFile *wordlistFile;
    QTextStream *inputStream;

    QList<QString> hashes;

    // MDXfind specific
    QString mdxfindPath;
    long long int totalKeyspace;
    QString hashFile;
    QString saltFile;
    bool hasSalts;

    void run() override;
    void runMDXfind();
    void parseMDXfindOutput(const QString &line, long long int &crackedCounter);
    QString getMDXfindExecutable();
    bool parseHashlistWithSalts();
    int gotoSkipFile();
    bool getNext(QString &combo, long long pos);
};

#endif // RUNTHREAD_H
