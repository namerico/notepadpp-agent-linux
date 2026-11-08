#include "EditorTab.h"
#include "ScintillaEditBase.h"
#include "LexerManager.h"

#include <QVBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

int EditorTab::s_unnamedCount = 0;

EditorTab::EditorTab(QWidget *parent, const QString& filePath)
    : QWidget(parent)
    , m_filePath(filePath)
{
    if (m_filePath.isEmpty()) {
        s_unnamedCount++;
        m_unnamedIndex = s_unnamedCount;
    }

    m_editor = new ScintillaEditBase(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_editor);
    setLayout(layout);

    setupEditor();

    if (!m_filePath.isEmpty()) {
        loadFromFile(m_filePath);
    } else {
        LexerManager::instance().applyLexerForFile(m_editor, "");
    }

    connect(m_editor, &ScintillaEditBase::notifyChange, this, &EditorTab::onEditorModified);
    connect(m_editor, &ScintillaEditBase::savePointChanged, this, &EditorTab::onSavePointChanged);
    connect(m_editor, &ScintillaEditBase::charAdded, this, &EditorTab::onCharAdded);
}

EditorTab::~EditorTab() {}

void EditorTab::setupEditor() {
    // Configurações padrão do Notepad++
    m_editor->send(SCI_SETCODEPAGE, SC_CP_UTF8);
    m_editor->send(SCI_SETTABWIDTH, 4);
    m_editor->send(SCI_SETUSETABS, 0); // Espaços para tabs ou configurável
    m_editor->send(SCI_SETINDENT, 4);
    m_editor->send(SCI_SETTABINDENTS, 1);
    m_editor->send(SCI_SETBACKSPACEUNINDENTS, 1);
    m_editor->send(SCI_SETVIEWEOL, 0);
    m_editor->send(SCI_SETVIEWWS, SCWS_INVISIBLE);
    m_editor->send(SCI_SETMOUSEDWELLTIME, 500);

    // Margem de seleção
    m_editor->send(SCI_SETSELBACK, 1, 0xD4A070); // Cor de seleção clássica

    // Padding e respiro de margens
    m_editor->send(SCI_SETMARGINLEFT, 0, LexerManager::instance().paddingLeft());
    m_editor->send(SCI_SETMARGINRIGHT, 0, LexerManager::instance().paddingRight());
    m_editor->send(SCI_SETEXTRAASCENT, LexerManager::instance().extraAscent());
    m_editor->send(SCI_SETEXTRADESCENT, LexerManager::instance().extraDescent());
    m_editor->send(SCI_SETSCROLLWIDTHTRACKING, 1);
}

QString EditorTab::title() const {
    if (!m_customTitle.isEmpty()) {
        return m_customTitle;
    }
    if (m_filePath.isEmpty()) {
        return QString("new %1").arg(m_unnamedIndex);
    }
    QFileInfo fi(m_filePath);
    return fi.fileName();
}

void EditorTab::setTitle(const QString& newTitle) {
    m_customTitle = newTitle;
    emit titleChanged(newTitle);
}

bool EditorTab::loadFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Falha ao abrir arquivo:" << path;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    m_filePath = path;
    m_editor->sends(SCI_SETTEXT, 0, data.constData());
    m_editor->send(SCI_SETSAVEPOINT);
    m_editor->send(SCI_EMPTYUNDOBUFFER);

    m_isModified = false;
    LexerManager::instance().applyLexerForFile(m_editor, m_filePath);
    emit modificationStateChanged(false);
    emit titleChanged(title());
    return true;
}

bool EditorTab::saveToFile(const QString& path) {
    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Falha ao salvar no arquivo:" << targetPath;
        return false;
    }

    QString currentText = text();
    QByteArray utf8Data = currentText.toUtf8();
    file.write(utf8Data);
    file.close();

    m_filePath = targetPath;
    m_editor->send(SCI_SETSAVEPOINT);
    m_isModified = false;

    LexerManager::instance().applyLexerForFile(m_editor, m_filePath);
    emit modificationStateChanged(false);
    emit titleChanged(title());
    return true;
}

QString EditorTab::text() const {
    int length = m_editor->send(SCI_GETTEXTLENGTH);
    if (length <= 0) return QString();

    QByteArray buf(length + 1, 0);
    m_editor->send(SCI_GETTEXT, length + 1, reinterpret_cast<sptr_t>(buf.data()));
    return QString::fromUtf8(buf.constData(), length);
}

void EditorTab::setText(const QString& newText) {
    QByteArray utf8 = newText.toUtf8();
    m_editor->sends(SCI_SETTEXT, 0, utf8.constData());
}

void EditorTab::insertText(const QString& textToInsert) {
    QByteArray utf8 = textToInsert.toUtf8();
    int currentPos = m_editor->send(SCI_GETCURRENTPOS);
    m_editor->send(SCI_INSERTTEXT, currentPos, reinterpret_cast<sptr_t>(utf8.constData()));
}

void EditorTab::replaceSelection(const QString& replacement) {
    QByteArray utf8 = replacement.toUtf8();
    m_editor->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(utf8.constData()));
}

void EditorTab::applyDiff(int startPos, int length, const QString& replacement) {
    m_editor->send(SCI_SETTARGETSTART, startPos);
    m_editor->send(SCI_SETTARGETEND, startPos + length);
    QByteArray utf8 = replacement.toUtf8();
    m_editor->send(SCI_REPLACETARGET, utf8.length(), reinterpret_cast<sptr_t>(utf8.constData()));
}

int EditorTab::currentLine() const {
    int pos = m_editor->send(SCI_GETCURRENTPOS);
    return m_editor->send(SCI_LINEFROMPOSITION, pos) + 1;
}

int EditorTab::currentColumn() const {
    int pos = m_editor->send(SCI_GETCURRENTPOS);
    return m_editor->send(SCI_GETCOLUMN, pos) + 1;
}

int EditorTab::lineCount() const {
    return m_editor->send(SCI_GETLINECOUNT);
}

void EditorTab::getSelection(int& start, int& end) const {
    start = m_editor->send(SCI_GETSELECTIONSTART);
    end = m_editor->send(SCI_GETSELECTIONEND);
}

void EditorTab::setSelection(int start, int end) {
    m_editor->send(SCI_SETSEL, start, end);
}

void EditorTab::gotoLine(int line) {
    if (line > 0) line--;
    m_editor->send(SCI_GOTOLINE, line);
    m_editor->send(SCI_SCROLLCARET);
}

void EditorTab::reloadTheme() {
    LexerManager::instance().applyLexerForFile(m_editor, m_filePath);
}

void EditorTab::onCharAdded(int ch) {
    if (!QChar(ch).isLetterOrNumber() && ch != '_') return;

    int curPos = m_editor->send(SCI_GETCURRENTPOS);
    int startWord = m_editor->send(SCI_WORDSTARTPOSITION, curPos, 1);
    int lenEntered = curPos - startWord;

    if (lenEntered >= 2) {
        QString wordList = LexerManager::instance().getAutoCompletionWords(m_filePath);
        if (!wordList.isEmpty()) {
            QByteArray utf8Words = wordList.toUtf8();
            m_editor->sends(SCI_AUTOCSHOW, lenEntered, utf8Words.constData());
        }
    }
}

void EditorTab::onEditorModified() {
    bool dirty = m_editor->send(SCI_GETMODIFY) != 0;
    if (dirty != m_isModified) {
        m_isModified = dirty;
        emit modificationStateChanged(dirty);
    }
    emit cursorChanged(currentLine(), currentColumn());
}

void EditorTab::onSavePointChanged(bool dirty) {
    m_isModified = dirty;
    emit modificationStateChanged(dirty);
}
