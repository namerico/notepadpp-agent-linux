#include "FunctionListDock.h"
#include "EditorTab.h"

#include <QHeaderView>
#include <QRegularExpression>
#include <QFileInfo>
#include <QStyle>

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

void FunctionListDock::parseContent(const QString& text, const QString& filePath) {
    m_treeWidget->setUpdatesEnabled(false);
    clear();

    QString ext = QFileInfo(filePath).suffix().toLower();
    QStringList lines = text.split('\n');

    QTreeWidgetItem *classesGroup = new QTreeWidgetItem(m_treeWidget, QStringList() << "Classes / Interfaces" << "");
    classesGroup->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));

    QTreeWidgetItem *functionsGroup = new QTreeWidgetItem(m_treeWidget, QStringList() << "Funções / Métodos" << "");
    functionsGroup->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));

    // Expressões regulares para detecção universal
    QRegularExpression classRegex(R"(^\s*(?:class|struct|interface)\s+([A-Za-z0-9_]+))");
    QRegularExpression pyFuncRegex(R"(^\s*def\s+([A-Za-z0-9_]+)\s*\()");
    QRegularExpression jsFuncRegex(R"(^\s*(?:async\s+)?(?:function\s+([A-Za-z0-9_]+)|(?:const|let|var)\s+([A-Za-z0-9_]+)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>))");
    QRegularExpression cppFuncRegex(R"(^\s*(?:[A-Za-z0-9_<:>]+[\s*&]+)+([A-Za-z0-9_~]+)\s*\([^)]*\)\s*(?:const)?\s*\{?)");

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        int lineNum = i + 1;

        // Classes
        QRegularExpressionMatch classMatch = classRegex.match(line);
        if (classMatch.hasMatch()) {
            QString name = classMatch.captured(1);
            QTreeWidgetItem *item = new QTreeWidgetItem(classesGroup, QStringList() << name << QString::number(lineNum));
            item->setData(1, Qt::UserRole, lineNum);
            continue;
        }

        // Funções Python
        QRegularExpressionMatch pyMatch = pyFuncRegex.match(line);
        if (pyMatch.hasMatch()) {
            QString name = pyMatch.captured(1) + "()";
            QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << name << QString::number(lineNum));
            item->setData(1, Qt::UserRole, lineNum);
            continue;
        }

        // Funções JS/TS
        QRegularExpressionMatch jsMatch = jsFuncRegex.match(line);
        if (jsMatch.hasMatch()) {
            QString name = jsMatch.captured(1).isEmpty() ? jsMatch.captured(2) : jsMatch.captured(1);
            if (!name.isEmpty()) {
                QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (name + "()") << QString::number(lineNum));
                item->setData(1, Qt::UserRole, lineNum);
                continue;
            }
        }

        // Funções C/C++ (se não for if/while/for/switch)
        if (!line.trimmed().startsWith("if") && !line.trimmed().startsWith("for") && 
            !line.trimmed().startsWith("while") && !line.trimmed().startsWith("switch") &&
            !line.trimmed().startsWith("catch")) {
            QRegularExpressionMatch cppMatch = cppFuncRegex.match(line);
            if (cppMatch.hasMatch()) {
                QString name = cppMatch.captured(1);
                if (!name.isEmpty() && name != "return") {
                    QTreeWidgetItem *item = new QTreeWidgetItem(functionsGroup, QStringList() << (name + "()") << QString::number(lineNum));
                    item->setData(1, Qt::UserRole, lineNum);
                }
            }
        }
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
