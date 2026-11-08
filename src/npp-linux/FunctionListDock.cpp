#include "FunctionListDock.h"
#include "EditorTab.h"

#include <QHeaderView>
#include <QRegularExpression>
#include <QFileInfo>
#include <QStyle>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>
#include <QDebug>

FunctionListDock::FunctionListDock(QWidget *parent)
    : QDockWidget("Lista de Funções (Function List)", parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);

    m_filterEdit = new QLineEdit(container);
    m_filterEdit->setPlaceholderText("Filtrar símbolos (funções, classes)...");
    m_filterEdit->setClearButtonEnabled(true);
    layout->addWidget(m_filterEdit);

    m_treeWidget = new QTreeWidget(container);
    m_treeWidget->setHeaderLabels(QStringList() << "Símbolo" << "Linha");
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->setRootIsDecorated(true);
    layout->addWidget(m_treeWidget);

    setWidget(container);

    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &FunctionListDock::onItemDoubleClicked);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &FunctionListDock::onFilterChanged);
}

FunctionListDock::~FunctionListDock() {}

void FunctionListDock::clear() {
    m_treeWidget->clear();
}

void FunctionListDock::updateSymbols(EditorTab* tab) {
    clear();
    if (!tab) return;

    QString text = tab->text();
    QString filePath = tab->filePath();
    parseContent(text, filePath);
}

QString FunctionListDock::getXmlRulePathForExtension(const QString& ext) const {
    QString appDir = QCoreApplication::applicationDirPath();
    QString baseDir = appDir + "/assets/functionList";
    if (!QDir(baseDir).exists()) {
        baseDir = appDir + "/../assets/functionList";
    }

    static const QMap<QString, QString> s_map = {
        {"py", "python.xml"}, {"pyw", "python.xml"},
        {"cpp", "cpp.xml"}, {"cxx", "cpp.xml"}, {"cc", "cpp.xml"}, {"hpp", "cpp.xml"}, {"h", "cpp.xml"},
        {"c", "c.xml"},
        {"rs", "rust.xml"},
        {"js", "javascript.js.xml"}, {"mjs", "javascript.js.xml"}, {"cjs", "javascript.js.xml"},
        {"ts", "typescript.xml"}, {"tsx", "typescript.xml"},
        {"go", "go.xml"},
        {"php", "php.xml"},
        {"sh", "bash.xml"}, {"bash", "bash.xml"},
        {"java", "java.xml"},
        {"lua", "lua.xml"},
        {"sql", "sql.xml"},
        {"cs", "cs.xml"},
        {"html", "html.xml"}, {"htm", "html.xml"},
        {"xml", "xml.xml"},
        {"ini", "ini.xml"},
        {"toml", "toml.xml"},
        {"yaml", "yaml.xml"}, {"yml", "yaml.xml"},
        {"rb", "ruby.xml"}
    };

    QString fileName = s_map.value(ext, "");
    if (!fileName.isEmpty()) {
        QString fullPath = baseDir + "/" + fileName;
        if (QFile::exists(fullPath)) {
            return fullPath;
        }
    }
    return QString();
}

bool FunctionListDock::parseFromXmlRules(const QString& text, const QString& xmlPath, QTreeWidgetItem* classesGroup, QTreeWidgetItem* functionsGroup) {
    QFile file(xmlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader xml(&file);
    QStringList classExprs;
    QStringList funcExprs;

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNextStartElement()) {
            QString name = xml.name().toString();
            if (name == "classRange") {
                QString expr = xml.attributes().value("mainExpr").toString();
                if (!expr.isEmpty()) classExprs.append(expr);
            } else if (name == "function") {
                QString expr = xml.attributes().value("mainExpr").toString();
                if (!expr.isEmpty()) funcExprs.append(expr);
            }
        }
    }
    file.close();

    int foundCount = 0;

    // Tenta aplicar as regras oficiais com QRegularExpression
    for (const QString& expr : funcExprs) {
        QRegularExpression::PatternOptions options = QRegularExpression::MultilineOption;
        QString cleanExpr = expr;
        if (cleanExpr.contains("(?x)")) {
            cleanExpr.remove("(?x)");
            options |= QRegularExpression::ExtendedPatternSyntaxOption;
        }

        QRegularExpression regex(cleanExpr, options);
        if (!regex.isValid()) continue;

        QRegularExpressionMatchIterator it = regex.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int offset = match.capturedStart();
            int lineNum = text.left(offset).count('\n') + 1;
            QString captured = match.captured(0).trimmed();

            // Limpa sufixos de bloco
            captured.remove(QRegularExpression(R"([\{\:\;\(\)].*$)"));
            captured = captured.trimmed();
            if (captured.isEmpty()) continue;

            // Extrai o nome identificador
            QStringList tokens = captured.split(QRegularExpression(R"([\s\(\:\=\>]+)"), Qt::SkipEmptyParts);
            QString symbol = tokens.isEmpty() ? captured : tokens.last();
            if (symbol == "def" || symbol == "fn" || symbol == "function" || symbol == "func") {
                if (tokens.size() >= 2) symbol = tokens.at(tokens.size() - 2);
            }

            if (!symbol.isEmpty()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (symbol + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                foundCount++;
            }
        }
    }

    return (foundCount > 0);
}

void FunctionListDock::parseWithBuiltinPatterns(const QString& text, const QString& ext, QTreeWidgetItem* classesGroup, QTreeWidgetItem* functionsGroup) {
    QStringList lines = text.split('\n');

    // Expressões regulares de alta precisão
    QRegularExpression classRegex(R"(^\s*(?:class|struct|interface|type)\s+([A-Za-z0-9_]+))");
    QRegularExpression pyFuncRegex(R"(^\s*(?:async\s+)?def\s+([A-Za-z0-9_]+)\s*\()");
    QRegularExpression jsFuncRegex(R"(^\s*(?:async\s+)?(?:function\s+([A-Za-z0-9_]+)|(?:const|let|var)\s+([A-Za-z0-9_]+)\s*=\s*(?:async\s*)?(?:\([^)]*\)|[A-Za-z0-9_]+)\s*=>))");
    QRegularExpression rustFuncRegex(R"(^\s*(?:pub(?:\([^)]*\))?\s+)?(?:async\s+)?fn\s+([A-Za-z0-9_]+)\s*(?:<[^>]*>)?\s*\()");
    QRegularExpression goFuncRegex(R"(^\s*func\s+(?:\([^)]*\)\s+)?([A-Za-z0-9_]+)\s*\()");
    QRegularExpression phpFuncRegex(R"(^\s*(?:public|private|protected|static|\s)*function\s+([A-Za-z0-9_]+)\s*\()");
    QRegularExpression bashFuncRegex(R"(^\s*(?:function\s+)?([A-Za-z0-9_]+)\s*\(\)\s*\{)");
    QRegularExpression rubyFuncRegex(R"(^\s*def\s+([A-Za-z0-9_]+))");
    QRegularExpression cppFuncRegex(R"(^\s*(?:[A-Za-z0-9_<:>]+[\s*&]+)+([A-Za-z0-9_~]+)\s*\([^)]*\)\s*(?:const)?\s*\{?)");

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        int lineNum = i + 1;
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("//") || trimmed.startsWith("#") || trimmed.startsWith("/*")) {
            continue;
        }

        // Classes / Structs
        QRegularExpressionMatch classMatch = classRegex.match(line);
        if (classMatch.hasMatch()) {
            QString name = classMatch.captured(1);
            QTreeWidgetItem *item = new QTreeWidgetItem(classesGroup, QStringList() << name << QString::number(lineNum));
            item->setData(1, Qt::UserRole, lineNum);
            continue;
        }

        // Rust
        if (ext == "rs") {
            QRegularExpressionMatch m = rustFuncRegex.match(line);
            if (m.hasMatch()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (m.captured(1) + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // Go
        if (ext == "go") {
            QRegularExpressionMatch m = goFuncRegex.match(line);
            if (m.hasMatch()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (m.captured(1) + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // PHP
        if (ext == "php") {
            QRegularExpressionMatch m = phpFuncRegex.match(line);
            if (m.hasMatch()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (m.captured(1) + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // Bash
        if (ext == "sh" || ext == "bash") {
            QRegularExpressionMatch m = bashFuncRegex.match(line);
            if (m.hasMatch()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (m.captured(1) + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // Ruby
        if (ext == "rb") {
            QRegularExpressionMatch m = rubyFuncRegex.match(line);
            if (m.hasMatch()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (m.captured(1) + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // Python
        QRegularExpressionMatch pyMatch = pyFuncRegex.match(line);
        if (pyMatch.hasMatch()) {
            QString name = pyMatch.captured(1) + "()";
            QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << name << QString::number(lineNum));
            item->setData(1, Qt::UserRole, lineNum);
            continue;
        }

        // JS / TS
        QRegularExpressionMatch jsMatch = jsFuncRegex.match(line);
        if (jsMatch.hasMatch()) {
            QString name = jsMatch.captured(1).isEmpty() ? jsMatch.captured(2) : jsMatch.captured(1);
            if (!name.isEmpty()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (name + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // C / C++
        if (!trimmed.startsWith("if") && !trimmed.startsWith("for") && 
            !trimmed.startsWith("while") && !trimmed.startsWith("switch") &&
            !trimmed.startsWith("catch")) {
            QRegularExpressionMatch cppMatch = cppFuncRegex.match(line);
            if (cppMatch.hasMatch()) {
                QString name = cppMatch.captured(1);
                if (!name.isEmpty() && name != "return" && name != "else") {
                    QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (name + "()") << QString::number(lineNum));
                    item->setData(1, Qt::UserRole, lineNum);
                }
            }
        }
    }
}

void FunctionListDock::parseContent(const QString& text, const QString& filePath) {
    m_treeWidget->setUpdatesEnabled(false);
    clear();

    QString ext = QFileInfo(filePath).suffix().toLower();

    QTreeWidgetItem *classesGroup = new QTreeWidgetItem(m_treeWidget, QStringList() << "Classes / Interfaces / Structs" << "");
    classesGroup->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

    QTreeWidgetItem *functionsGroup = new QTreeWidgetItem(m_treeWidget, QStringList() << "Funções / Métodos" << "");
    functionsGroup->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));

    QString xmlRulePath = getXmlRulePathForExtension(ext);
    bool parsedFromXml = false;
    if (!xmlRulePath.isEmpty()) {
        parsedFromXml = parseFromXmlRules(text, xmlRulePath, classesGroup, functionsGroup);
    }

    if (!parsedFromXml) {
        parseWithBuiltinPatterns(text, ext, classesGroup, functionsGroup);
    }

    if (classesGroup->childCount() == 0) {
        delete classesGroup;
    } else {
        classesGroup->setExpanded(true);
    }

    if (functionsGroup->childCount() == 0) {
        delete functionsGroup;
    } else {
        functionsGroup->setExpanded(true);
    }

    m_treeWidget->setUpdatesEnabled(true);
}

void FunctionListDock::onItemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;

    QVariant data = item->data(1, Qt::UserRole);
    if (data.isValid()) {
        int line = data.toInt();
        emit lineSelected(line);
    }
}

void FunctionListDock::onFilterChanged(const QString &text) {
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *top = m_treeWidget->topLevelItem(i);
        bool anyChildVisible = false;

        for (int j = 0; j < top->childCount(); ++j) {
            QTreeWidgetItem *child = top->child(j);
            bool match = child->text(0).contains(text, Qt::CaseInsensitive);
            child->setHidden(!match);
            if (match) anyChildVisible = true;
        }

        top->setHidden(!anyChildVisible && !text.isEmpty());
        if (anyChildVisible) top->setExpanded(true);
    }
}
