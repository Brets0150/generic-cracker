#include "runthread.h"
#include <utility>
#include <QCoreApplication>
#include <QDir>
using namespace std;

RunThread::RunThread(int type, QString attack, QString hashlist, long long skip, long long length, int timeout, QString hashType, int iterations){
    this->type = type;
    this->attack = std::move(attack);
    this->skip = skip;
    this->length = length;
    this->hashlist = std::move(hashlist);
    this->timeout = timeout;
    this->hashType = std::move(hashType);
    this->iterations = iterations;
    this->totalKeyspace = 0;
    this->mdxfindPath = getMDXfindExecutable();
    this->hasSalts = false;
}

void RunThread::run(){
    // MDXfind doesn't require an attack type (mask/wordlist)
    // Type 0 means hash identification only
    if(this->type != 0 && this->type != 1 && this->type != 2){
        cerr << "Invalid attack type!" << endl;
        return;
    }
    else if(this->type != 0 && this->attack.length() == 0){
        cerr << "Attack type with invalid input!" << endl;
        return;
    }
    else if(this->hashlist.length() == 0){
        cerr << "No hashlist provided!" << endl;
        return;
    }

    // Verify hashlist exists
    QFile file(this->hashlist);
    if(!file.exists()){
        cerr << "Hashlist file does not exist: " << this->hashlist.toStdString() << endl;
        return;
    }

    // Verify wordlist exists (type 2 = wordlist attack)
    // Skip this check for type 0 (MDXfind hash identification only)
    if(this->type == 2){
        QFile wordlist(this->attack);
        if(!wordlist.exists()){
            cerr << "Wordlist file does not exist: " << this->attack.toStdString() << endl;
            return;
        }
    }

    // Check if MDXfind executable exists
    if(this->mdxfindPath.isEmpty()){
        cerr << "MDXfind executable not found!" << endl;
        return;
    }

    // Parse hashlist and extract salts if present
    if(!parseHashlistWithSalts()){
        cerr << "Failed to parse hashlist!" << endl;
        return;
    }

    // Run MDXfind
    runMDXfind();
}

int RunThread::gotoSkipFile(){
    for(long long int i=0;i<this->skip;i++){
        if(this->inputStream->atEnd()){
            cerr << "Skip value too large for wordlist!" << endl;
            return -1;
        }
        this->inputStream->readLine();
    }
    return 0;
}

bool RunThread::getNext(QString &combo, long long int pos){
    if(this->type == 1){
        // TODO mask combinations
        return false;
    }
    else{
        if(this->inputStream->atEnd()){
            cerr << "Wordlist not large enough to satisfy length!" << endl;
            return false;
        }
        combo = this->inputStream->readLine();
        return true;
    }
}

QString RunThread::getMDXfindExecutable(){
    // Get the application directory
    QString appDir = QCoreApplication::applicationDirPath();

    // Check for mdxfind in mdx_bin directory relative to the application
    QStringList possiblePaths;

#ifdef Q_OS_WIN
    possiblePaths << appDir + "/../mdx_bin/mdxfind.exe";
    possiblePaths << appDir + "/mdx_bin/mdxfind.exe";
    possiblePaths << "./mdx_bin/mdxfind.exe";
#else
    possiblePaths << appDir + "/../mdx_bin/mdxfind";
    possiblePaths << appDir + "/mdx_bin/mdxfind";
    possiblePaths << "./mdx_bin/mdxfind";
#endif

    for(const QString &path : possiblePaths){
        QFile file(path);
        if(file.exists()){
            return QDir::cleanPath(path);
        }
    }

    return QString(); // Return empty string if not found
}

bool RunThread::parseHashlistWithSalts(){
    // Open the original hashlist file
    QFile inputFile(this->hashlist);
    if(!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        cerr << "Failed to open hashlist file: " << this->hashlist.toStdString() << endl;
        return false;
    }

    // Create temporary files for hashes and salts
    // Use the hashlist filename as base for temp files
    QString basePath = this->hashlist;
    this->hashFile = basePath + ".hashes";
    this->saltFile = basePath + ".salts";

    QFile hashFileObj(this->hashFile);
    QFile saltFileObj(this->saltFile);

    if(!hashFileObj.open(QIODevice::WriteOnly | QIODevice::Text)){
        cerr << "Failed to create hash file: " << this->hashFile.toStdString() << endl;
        return false;
    }

    if(!saltFileObj.open(QIODevice::WriteOnly | QIODevice::Text)){
        cerr << "Failed to create salt file: " << this->saltFile.toStdString() << endl;
        return false;
    }

    QTextStream hashStream(&hashFileObj);
    QTextStream saltStream(&saltFileObj);
    QTextStream in(&inputFile);

    int lineNumber = 0;
    bool foundSalts = false;

    while(!in.atEnd()){
        QString line = in.readLine().trimmed();
        lineNumber++;

        if(line.isEmpty()){
            continue;
        }

        // Split by colon
        QStringList parts = line.split(':');

        if(parts.size() == 1){
            // Just a hash, no salt
            hashStream << parts[0] << endl;
            saltStream << endl; // Empty salt line
        }
        else if(parts.size() == 2){
            // Could be hash:salt or hash:plaintext
            // We need to determine which based on the content
            // For Hashtopolis, the format should be hash:salt when there's a salt
            // If second part looks like plaintext (contains =, letters, etc), treat as hash only

            // Heuristic: if the second part is short (< 64 chars) and looks hex-ish or is very short,
            // it's likely a salt. Otherwise, it's plaintext and we have hash-only format.
            QString firstPart = parts[0];
            QString secondPart = parts[1];

            // Check if second part looks like a salt (short hex, or special salt markers)
            bool looksLikeSalt = false;
            if(secondPart.length() <= 64){
                // Check if it's hex or a short identifier
                QRegularExpression hexPattern("^[a-fA-F0-9]+$");
                if(hexPattern.match(secondPart).hasMatch() || secondPart.length() <= 16){
                    looksLikeSalt = true;
                }
            }

            if(looksLikeSalt){
                // hash:salt format
                hashStream << firstPart << endl;
                saltStream << secondPart << endl;
                foundSalts = true;
            }
            else{
                // hash:plaintext format (no salt)
                hashStream << firstPart << endl;
                saltStream << endl; // Empty salt line
            }
        }
        else if(parts.size() >= 3){
            // hash:salt:plaintext format
            hashStream << parts[0] << endl;
            saltStream << parts[1] << endl;
            foundSalts = true;
        }
    }

    inputFile.close();
    hashFileObj.close();
    saltFileObj.close();

    this->hasSalts = foundSalts;

    return true;
}

void RunThread::runMDXfind(){
    QProcess process;
    QStringList arguments;

    // Add hash type parameter (from your bash script: hash_parm='ALL,!user,salt')
    if(!this->hashType.isEmpty()){
        arguments << "-h" << this->hashType;
    }

    // Add iterations parameter (from your bash script: rounds=10)
    if(this->iterations > 0){
        arguments << "-i" << QString::number(this->iterations);
        arguments << "-q" << QString::number(this->iterations); // -q is also used for internal iteration counts
    }

    // Add hashlist file (use parsed hash file)
    arguments << "-f" << this->hashFile;

    // Add salt file (always required by MDXfind, even if empty)
    arguments << "-s" << this->saltFile;

    // Add extended search option (from your bash script)
    arguments << "-e"; // Extended search for truncated hashes

    // Add wordlist (for type 2 = wordlist attack)
    if(this->type == 2){
        arguments << this->attack;
    }

    // Add skip option if specified
    if(this->skip > 0){
        arguments << "-w" << QString::number(this->skip);
    }

    // Set up the process
    process.setProgram(this->mdxfindPath);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::MergedChannels);

    // Start MDXfind
    process.start();
    if(!process.waitForStarted()){
        cerr << "Failed to start MDXfind: " << process.errorString().toStdString() << endl;
        return;
    }

    long long int crackedCounter = 0;
    long long int linesProcessed = 0;
    time_t lastUpdate = time(NULL);
    time_t startTime = time(NULL);
    long long int lastCounter = 0;

    // Read output line by line
    while(process.state() == QProcess::Running || process.canReadLine()){
        if(process.canReadLine()){
            QString line = QString::fromUtf8(process.readLine()).trimmed();

            // Parse MDXfind output for found hashes
            parseMDXfindOutput(line, crackedCounter);

            linesProcessed++;

            // Update status every 5 seconds
            if(time(NULL) - lastUpdate >= 5){
                // Calculate progress based on length if specified
                int progress = 0;
                int speed = 0;

                if(this->length > 0){
                    progress = (int)floor((double)linesProcessed / this->length * 10000);
                    speed = (int)(((double)(linesProcessed - lastCounter)) / (time(NULL) - lastUpdate));
                }
                else{
                    // If no length specified, show lines processed as speed only
                    speed = (int)(((double)(linesProcessed - lastCounter)) / (time(NULL) - lastUpdate));
                }

                cout << "STATUS " << progress << " " << speed << endl;
                lastCounter = linesProcessed;
                lastUpdate = time(NULL);
            }

            // Check timeout
            if(this->timeout > 0 && (time(NULL) - startTime) >= this->timeout){
                process.terminate();
                if(!process.waitForFinished(5000)){
                    process.kill();
                }
                cout << "STATUS " << (int)floor((double)linesProcessed/this->length*10000) << " " << (int)(((double)(linesProcessed - lastCounter))/(time(NULL) - lastUpdate)) << endl;
                return;
            }

            // Note: Length limit is handled by MDXfind via -w (skip) parameter
            // We don't terminate based on output lines, as MDXfind outputs many diagnostic lines
        }
        else{
            process.waitForReadyRead(100);
        }
    }

    process.waitForFinished();

    // Read ALL remaining output from both channels
    QByteArray allOutput = process.readAll();
    if(!allOutput.isEmpty()){
        QString output = QString::fromUtf8(allOutput);
        QStringList lines = output.split('\n');
        for(const QString &line : lines){
            QString trimmed = line.trimmed();
            if(!trimmed.isEmpty()){
                parseMDXfindOutput(trimmed, crackedCounter);
            }
        }
    }

    // Final status update
    cout << "STATUS 10000 0" << endl;
}

void RunThread::parseMDXfindOutput(const QString &line, long long int &crackedCounter){
    // MDXfind output formats:
    // Without salt: HASHTYPE hash:plaintext
    //   Example: MD5x01 5f4dcc3b5aa765d61d8327deb882cf99:password
    // With salt: HASHTYPE hash:salt:plaintext
    //   Example: MD5SALT 5192d8813bbef9b620dd91a757834dc2:vl1A*):zhurA123

    // Skip empty lines and lines that don't contain hash results
    if(line.isEmpty() || !line.contains(":")){
        return;
    }

    // Use regex to parse the output: HASHTYPE hash:...
    // The pattern matches: [HASHTYPE] [space] [hash]:[rest]
    QRegularExpression regex("^([A-Z0-9\\-x]+)\\s+([a-fA-F0-9]+):(.+)$");
    QRegularExpressionMatch match = regex.match(line);

    if(match.hasMatch()){
        QString hashType = match.captured(1);
        QString hash = match.captured(2);
        QString rest = match.captured(3); // Could be "plaintext" or "salt:plaintext"

        QString plaintext;
        QString salt;

        // Check if rest contains another colon (salt:plaintext format)
        int colonPos = rest.indexOf(':');
        if(colonPos > 0){
            // Format: hash:salt:plaintext
            salt = rest.left(colonPos);
            plaintext = rest.mid(colonPos + 1);
        }
        else{
            // Format: hash:plaintext (no salt)
            plaintext = rest;
        }

        // Output format for Hashtopolis: hash:plaintext:hashtype
        // This allows Hashtopolis to know both the plaintext AND which algorithm was used
        // Note: We output just the plaintext, not the salt, since Hashtopolis already has the salt
        cout << hash.toStdString() << ":" << plaintext.toStdString() << ":" << hashType.toStdString() << endl;

        crackedCounter++;
    }
}
