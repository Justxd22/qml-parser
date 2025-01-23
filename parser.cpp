#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>
#include <QJsonObject>
#include <QJsonDocument>


#ifdef _WIN32
#include <io.h>
#define STDIN_FILENO 0
#else
#include <sys/select.h>
#include <unistd.h>
#endif

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qv4value_p.h>

#include <3rdparty/lz-string/src/lzstring.h>

#include "AstGenerator.h"
#include "parser.h"

using namespace std;

bool Foobar::debug = true;

void Foobar::setDebug(bool debug_) { Foobar::debug = debug_; }

int Foobar::InternalRun(const QString &code) {
  QQmlJS::Engine engine;
  QQmlJS::Lexer lexer(&engine);

  lexer.setCode(code, 1, true);
  QQmlJS::Parser parser(&engine);
  bool success = parser.parse();

  if (!success) {
    const auto diagnosticMessages = parser.diagnosticMessages();

    for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
      // qWarning("Qml Parsing Error %d: %s", m.loc.startLine, qPrintable(m.message));
      QJsonObject errorObject;
      errorObject["line"] = static_cast<int>(m.loc.startLine);
      errorObject["message"] = m.message;
      errorObject["error"] = "Qml Parsing Error";

      QJsonDocument jsonDoc(errorObject);
      QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
      std::cout << jsonString.toStdString();
    }

    return 0;
  }

  const bool debug = this->m_options.testFlag(Option::Debug);
  setDebug(debug);

  AstGenerator generator(&engine, 0);
  const json ast = generator(parser.ast());

  if (debug) {
    ofstream myfile;
    myfile.open("sandbox/test.json");
    myfile << ast.dump(2);
    myfile.close();
  } else {
    cout << ast.dump(2);
  }

  return 0;
}

Foobar::Foobar(Options options) { this->m_options = options; }

int Foobar::Run(QStringList args) {
    QString code;
try
{

    if (args.count() > 1) {
        QTextStream(stderr) << "Please provide only one path or one QML text or use stdin.\n";
        return 1;
    }

    if (args.count() == 1) {
        const QString pathOrText = args[0];
        QFileInfo fileInfo(pathOrText);

        if (fileInfo.isDir()) {
            QTextStream(stderr) << "qml-parser doesn't handle paths to directories.\n";
            return 1;
        }

        if (fileInfo.isFile()) {
            QFile file(pathOrText);
            if (!file.open(QFile::ReadOnly | QFile::Text)) {
                QTextStream(stderr) << "Error opening file: " << pathOrText << "\n";
                return 1;
            }
            code = QString::fromUtf8(file.readAll());
            file.close();
        } else {
            code = pathOrText;
            if (!pathOrText.trimmed().startsWith("import")) {
                code = LZString::decompressFromBase64(code);
            }
        }
    } else { // No arguments provided, read from stdin if no stdin exit and report no input was provided 
    #ifdef _WIN32
    if (_isatty(_fileno(stdin))) {
        QTextStream(stderr) << "Error: No input provided. Please provide input via stdin or command line arguments.\n";
        return 1;
    }
    #else
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) {
        QTextStream(stderr) << "Error: No input provided. Please provide input via stdin or command line arguments.\n";
        return 1;
    }
    #endif
    
    QTextStream in(stdin);
    code = in.readAll();
    }

    return InternalRun(code);
} catch (const std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
} catch (...) {
    std::cerr << "Unknown exception occurred." << std::endl;
    return 1;
}

}
