#include "LexerManager.h"
#include "ScintillaEditBase.h"
#include "SciLexer.h"
#include "Scintilla.h"

#include <QCoreApplication>
#include <QLibrary>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>

static int parseHexColor(const QString& hex) {
    if (hex.isEmpty()) return -1;
    bool ok = false;
    unsigned int val = hex.toUInt(&ok, 16);
    if (!ok) return -1;
    int r = (val >> 16) & 0xFF;
    int g = (val >> 8) & 0xFF;
    int b = val & 0xFF;
    return r | (g << 8) | (b << 16); // Formato BGR / COLORREF Scintilla
}

LexerManager& LexerManager::instance() {
    static LexerManager s_instance;
    return s_instance;
}

LexerManager::LexerManager() {
    m_extensionToLexer["cpp"] = "cpp";
    m_extensionToLexer["cxx"] = "cpp";
    m_extensionToLexer["cc"] = "cpp";
    m_extensionToLexer["c"] = "cpp";
    m_extensionToLexer["h"] = "cpp";
    m_extensionToLexer["hpp"] = "cpp";
    m_extensionToLexer["py"] = "python";
    m_extensionToLexer["pyw"] = "python";
    m_extensionToLexer["js"] = "cpp";
    m_extensionToLexer["ts"] = "cpp";
    m_extensionToLexer["jsx"] = "cpp";
    m_extensionToLexer["tsx"] = "cpp";
    m_extensionToLexer["json"] = "json";
    m_extensionToLexer["html"] = "hypertext";
    m_extensionToLexer["htm"] = "hypertext";
    m_extensionToLexer["xml"] = "xml";
    m_extensionToLexer["css"] = "css";
    m_extensionToLexer["sql"] = "sql";
    m_extensionToLexer["sh"] = "bash";
    m_extensionToLexer["bash"] = "bash";
    m_extensionToLexer["rs"] = "rust";
    m_extensionToLexer["lua"] = "lua";
    m_extensionToLexer["md"] = "markdown";
    m_extensionToLexer["yaml"] = "yaml";
    m_extensionToLexer["yml"] = "yaml";
    m_extensionToLexer["ini"] = "props";
    m_extensionToLexer["conf"] = "props";

    loadDefaultStyles();
}

LexerManager::~LexerManager() {}

void LexerManager::loadDefaultStyles() {
    m_defaultBg = 0xFFFFFF;
    m_defaultFg = 0x000000;
    m_lineMarginBg = 0xF0F0F0;
    m_lineMarginFg = 0x707070;
    m_caretColor = 0x000000;
    m_currentLineBg = 0xE8F2FE;
    m_selectionBg = 0xD4A070;
    m_globalStyles.clear();
    m_lexerStyles.clear();
}

bool LexerManager::initialize() {
    if (m_initialized) return true;

    QString appDir = QCoreApplication::applicationDirPath();
    QString localPath = appDir + "/liblexilla.so";

    QLibrary lexillaLib(localPath);
    if (!lexillaLib.load()) {
        lexillaLib.setFileName("lexilla");
        if (!lexillaLib.load()) {
            lexillaLib.setFileName("../notepad-plus-plus-upstream/lexilla/bin/liblexilla.so");
            if (!lexillaLib.load()) {
                qWarning() << "Falha ao carregar liblexilla.so:" << lexillaLib.errorString();
                return false;
            }
        }
    }

    m_createLexerFn = (CreateLexerFn)lexillaLib.resolve("CreateLexer");
    if (!m_createLexerFn) {
        qWarning() << "Símbolo CreateLexer não encontrado na liblexilla.so!";
        return false;
    }

    m_initialized = true;
    qInfo() << "LexerManager inicializado com sucesso via liblexilla.so.";
    return true;
}

QStringList LexerManager::availableThemes() const {
    QStringList themes;
    themes << "Padrão (Claro)" << "Dark Mode Padrão";

    QString themesDir = QCoreApplication::applicationDirPath() + "/assets/themes";
    if (!QDir(themesDir).exists()) {
        themesDir = QCoreApplication::applicationDirPath() + "/../assets/themes";
    }

    QDir dir(themesDir);
    QStringList xmlFiles = dir.entryList(QStringList() << "*.xml", QDir::Files, QDir::Name);
    for (const QString& file : xmlFiles) {
        QString name = QFileInfo(file).completeBaseName();
        if (!themes.contains(name)) {
            themes.append(name);
        }
    }
    return themes;
}

bool LexerManager::setTheme(const QString& themeName) {
    if (themeName == "Padrão" || themeName == "Padrão (Claro)") {
        m_currentThemeName = "Padrão";
        loadDefaultStyles();
        return true;
    }

    QString themesDir = QCoreApplication::applicationDirPath() + "/assets/themes";
    if (!QDir(themesDir).exists()) {
        themesDir = QCoreApplication::applicationDirPath() + "/../assets/themes";
    }

    QString themeFile = themesDir + "/" + themeName + ".xml";
    if (!QFile::exists(themeFile)) {
        themeFile = themesDir + "/" + themeName;
    }

    if (!QFile::exists(themeFile)) {
        qWarning() << "Tema não encontrado:" << themeFile;
        return false;
    }

    if (parseThemeXml(themeFile)) {
        m_currentThemeName = themeName;
        return true;
    }
    return false;
}

void LexerManager::setPadding(int left, int right, int topAscent, int bottomDescent) {
    if (left >= 0) m_paddingLeft = left;
    if (right >= 0) m_paddingRight = right;
    if (topAscent >= 0) m_extraAscent = topAscent;
    if (bottomDescent >= 0) m_extraDescent = bottomDescent;
}

bool LexerManager::parseThemeXml(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    m_globalStyles.clear();
    m_lexerStyles.clear();

    QXmlStreamReader xml(&file);
    QString currentLexerName;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QStringRef name = xml.name();
            QXmlStreamAttributes attrs = xml.attributes();

            if (name == "WidgetStyle") {
                QString styleName = attrs.value("name").toString();
                int styleId = attrs.value("styleID").toInt();
                QString fgStr = attrs.value("fgColor").toString();
                QString bgStr = attrs.value("bgColor").toString();

                int fg = parseHexColor(fgStr);
                int bg = parseHexColor(bgStr);

                if (styleId == 32) { // STYLE_DEFAULT
                    if (fg != -1) m_defaultFg = fg;
                    if (bg != -1) m_defaultBg = bg;
                } else if (styleId == 33) { // STYLE_LINENUMBER
                    if (fg != -1) m_lineMarginFg = fg;
                    if (bg != -1) m_lineMarginBg = bg;
                } else if (styleName.contains("Current line", Qt::CaseInsensitive) && bg != -1) {
                    m_currentLineBg = bg;
                } else if (styleName.contains("Selected text", Qt::CaseInsensitive) && bg != -1) {
                    m_selectionBg = bg;
                } else if (styleName.contains("Caret", Qt::CaseInsensitive) && fg != -1) {
                    m_caretColor = fg;
                }

                StyleRule rule;
                rule.styleId = styleId;
                rule.fgColor = fg;
                rule.bgColor = bg;
                m_globalStyles.append(rule);
            } else if (name == "LexerType") {
                currentLexerName = attrs.value("name").toString().toLower();
            } else if (name == "WordsStyle") {
                int styleId = attrs.value("styleID").toInt();
                int fg = parseHexColor(attrs.value("fgColor").toString());
                int bg = parseHexColor(attrs.value("bgColor").toString());
                int bold = attrs.value("fontStyle").toInt();

                StyleRule rule;
                rule.styleId = styleId;
                rule.fgColor = fg;
                rule.bgColor = bg;
                rule.bold = (bold == 1) ? 1 : 0;

                if (!currentLexerName.isEmpty()) {
                    m_lexerStyles[currentLexerName].append(rule);
                }
            }
        }
    }

    file.close();
    return true;
}

void LexerManager::applyLexerForFile(ScintillaEditBase* editor, const QString& filePath) {
    if (!editor) return;

    if (!m_initialized) {
        initialize();
    }

    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    QString lexerName = m_extensionToLexer.value(ext, "");

    if (lexerName.isEmpty() || !m_createLexerFn) {
        editor->send(SCI_SETILEXER, 0, 0);
        applyTheme(editor);
        return;
    }

    void* pLexer = m_createLexerFn(lexerName.toUtf8().constData());
    if (pLexer) {
        editor->send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(pLexer));
        applyTheme(editor);
    } else {
        editor->send(SCI_SETILEXER, 0, 0);
        applyTheme(editor);
    }
}

void LexerManager::applyTheme(ScintillaEditBase* editor) {
    if (!editor) return;

    // Aplica estilo base padrão
    editor->send(SCI_STYLESETBACK, STYLE_DEFAULT, m_defaultBg);
    editor->send(SCI_STYLESETFORE, STYLE_DEFAULT, m_defaultFg);
    editor->send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Monospace"));
    editor->send(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
    editor->send(SCI_STYLECLEARALL);

    // Margem de numeração de linhas (Margem 0)
    editor->send(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
    editor->send(SCI_SETMARGINWIDTHN, 0, 50);
    editor->send(SCI_SETMARGINSENSITIVEN, 0, 0);
    editor->send(SCI_STYLESETBACK, STYLE_LINENUMBER, m_lineMarginBg);
    editor->send(SCI_STYLESETFORE, STYLE_LINENUMBER, m_lineMarginFg);

    // Margem de folding (Margem 2)
    editor->send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
    editor->send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
    editor->send(SCI_SETMARGINWIDTHN, 2, 16);
    editor->send(SCI_SETMARGINSENSITIVEN, 2, 1);
    editor->send(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CLICK);

    // Símbolos clássicos de folding com caixas +/- do Notepad++
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
    editor->send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
    editor->send(SCI_SETFOLDMARGINCOLOUR, 1, m_lineMarginBg);
    editor->send(SCI_SETFOLDMARGINHICOLOUR, 1, m_lineMarginBg);

    // Espaçamento e respiro das margens laterais e verticais (Padding)
    editor->send(SCI_SETMARGINLEFT, 0, m_paddingLeft);   // Afasta o código da margem esquerda
    editor->send(SCI_SETMARGINRIGHT, 0, m_paddingRight); // Afasta o código da margem direita
    editor->send(SCI_SETEXTRAASCENT, m_extraAscent);    // Afasta a 1ª linha do topo e dá respiro vertical
    editor->send(SCI_SETEXTRADESCENT, m_extraDescent);  // Respiro na base de cada linha
    editor->send(SCI_SETSCROLLWIDTHTRACKING, 1);

    // Caret e linha corrente
    editor->send(SCI_SETCARETLINEVISIBLE, 1);
    editor->send(SCI_SETCARETLINEBACK, m_currentLineBg);
    editor->send(SCI_SETCARETFORE, m_caretColor);
    editor->send(SCI_SETSELBACK, 1, m_selectionBg);

    // Se temos regras específicas do tema para este lexer
    for (auto it = m_lexerStyles.begin(); it != m_lexerStyles.end(); ++it) {
        const QVector<StyleRule>& rules = it.value();
        for (const StyleRule& r : rules) {
            if (r.styleId >= 0) {
                if (r.fgColor != -1) editor->send(SCI_STYLESETFORE, r.styleId, r.fgColor);
                if (r.bgColor != -1) editor->send(SCI_STYLESETBACK, r.styleId, r.bgColor);
                if (r.bold != -1) editor->send(SCI_STYLESETBOLD, r.styleId, r.bold);
            }
        }
    }

    // Se o tema for padrão clássico, garante cores vivas essenciais
    if (m_currentThemeName == "Padrão") {
        editor->send(SCI_STYLESETFORE, SCE_C_COMMENT, 0x008000); // Verde
        editor->send(SCI_STYLESETFORE, SCE_C_COMMENTLINE, 0x008000);
        editor->send(SCI_STYLESETFORE, SCE_C_WORD, 0xFF0000); // Azul em BGR (0x0000FF em RGB)
        editor->send(SCI_STYLESETBOLD, SCE_C_WORD, 1);
        editor->send(SCI_STYLESETFORE, SCE_C_STRING, 0x808080);
        editor->send(SCI_STYLESETFORE, SCE_C_NUMBER, 0x0080FF); // Laranja em BGR
    }
}

QString LexerManager::getAutoCompletionWords(const QString& filePath) const {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    if (ext.isEmpty()) ext = "cpp";

    if (m_autoCompletionKeywords.contains(ext)) {
        return m_autoCompletionKeywords[ext].join(" ");
    }

    // Tenta carregar do XML em assets/autoCompletion/<ext>.xml
    QString autoDir = QCoreApplication::applicationDirPath() + "/assets/autoCompletion";
    if (!QDir(autoDir).exists()) {
        autoDir = QCoreApplication::applicationDirPath() + "/../assets/autoCompletion";
    }

    QString xmlPath = autoDir + "/" + ext + ".xml";
    if (!QFile::exists(xmlPath)) {
        // Tenta mapeamentos clássicos (ex: js -> javascript.xml, py -> python.xml)
        if (ext == "py") xmlPath = autoDir + "/python.xml";
        else if (ext == "js" || ext == "ts") xmlPath = autoDir + "/javascript.xml";
        else if (ext == "html" || ext == "htm") xmlPath = autoDir + "/html.xml";
        else if (ext == "sql") xmlPath = autoDir + "/sql.xml";
        else xmlPath = autoDir + "/cpp.xml";
    }

    QStringList words;
    QFile file(xmlPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QXmlStreamReader xml(&file);
        while (!xml.atEnd()) {
            if (xml.readNextStartElement() && xml.name() == "KeyWord") {
                QString w = xml.attributes().value("name").toString();
                if (!w.isEmpty() && !words.contains(w)) {
                    words.append(w);
                }
            }
        }
        file.close();
    }

    words.sort(Qt::CaseInsensitive);
    const_cast<LexerManager*>(this)->m_autoCompletionKeywords[ext] = words;
    return words.join(" ");
}
