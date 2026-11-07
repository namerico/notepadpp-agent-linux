#include "LexerManager.h"
#include "ScintillaEditBase.h"
#include <QCoreApplication>
#include <QLibrary>
#include <QFileInfo>
#include <QDebug>
#include <QColor>

LexerManager& LexerManager::instance() {
    static LexerManager s_instance;
    return s_instance;
}

LexerManager::LexerManager() {
    // Mapeamento clássico do Notepad++ de extensões para lexers Scintilla/Lexilla
    m_extensionToLexer["cpp"] = "cpp";
    m_extensionToLexer["cxx"] = "cpp";
    m_extensionToLexer["cc"] = "cpp";
    m_extensionToLexer["c"] = "cpp";
    m_extensionToLexer["h"] = "cpp";
    m_extensionToLexer["hpp"] = "cpp";
    m_extensionToLexer["py"] = "python";
    m_extensionToLexer["pyw"] = "python";
    m_extensionToLexer["js"] = "cpp"; // cpp lexer lida muito bem com JS/TS em modo simples ou ecmascript
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
}

LexerManager::~LexerManager() {}

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

void LexerManager::applyLexerForFile(ScintillaEditBase* editor, const QString& filePath) {
    if (!editor) return;

    if (!m_initialized) {
        initialize();
    }

    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    QString lexerName = m_extensionToLexer.value(ext, "");

    if (lexerName.isEmpty() || !m_createLexerFn) {
        // Texto plano
        editor->send(SCI_SETILEXER, 0, 0);
        applyTheme(editor, false);
        return;
    }

    void* pLexer = m_createLexerFn(lexerName.toUtf8().constData());
    if (pLexer) {
        editor->send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(pLexer));
        applyTheme(editor, false);
    } else {
        editor->send(SCI_SETILEXER, 0, 0);
        applyTheme(editor, false);
    }
}

void LexerManager::applyTheme(ScintillaEditBase* editor, bool darkMode) {
    if (!editor) return;

    // Notepad++ cores clássicas
    // Fundo padrão: branco #FFFFFF (ou #202020 em dark mode)
    int bg = darkMode ? 0x202020 : 0xFFFFFF;
    int fg = darkMode ? 0xD0D0D0 : 0x000000;

    editor->send(SCI_STYLESETBACK, STYLE_DEFAULT, bg);
    editor->send(SCI_STYLESETFORE, STYLE_DEFAULT, fg);
    editor->send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Monospace"));
    editor->send(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
    editor->send(SCI_STYLECLEARALL);

    // Margem de numeração de linhas (Margem 0)
    int marginBg = darkMode ? 0x2b2b2b : 0xF0F0F0;
    int marginFg = darkMode ? 0x808080 : 0x707070;
    editor->send(SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
    editor->send(SCI_SETMARGINWIDTHN, 0, 48); // largura suficiente para números de linhas
    editor->send(SCI_SETMARGINSENSITIVEN, 0, 0);
    editor->send(SCI_STYLESETBACK, STYLE_LINENUMBER, marginBg);
    editor->send(SCI_STYLESETFORE, STYLE_LINENUMBER, marginFg);

    // Margem de folding (Margem 2)
    editor->send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
    editor->send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
    editor->send(SCI_SETMARGINWIDTHN, 2, 16);
    editor->send(SCI_SETMARGINSENSITIVEN, 2, 1);
    editor->send(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_CLICK);

    // Cores de destaque padrão Notepad++
    // Comentários: Verde escuro (0x008000)
    int commentColor = darkMode ? 0x6A9955 : 0x008000;
    editor->send(SCI_STYLESETFORE, SCE_C_COMMENT, commentColor);
    editor->send(SCI_STYLESETFORE, SCE_C_COMMENTLINE, commentColor);
    editor->send(SCI_STYLESETFORE, SCE_C_COMMENTDOC, commentColor);

    // Palavras-chave: Azul clássico Notepad++ (0x0000FF), negrito
    int keywordColor = darkMode ? 0x569CD6 : 0x0000FF;
    editor->send(SCI_STYLESETFORE, SCE_C_WORD, keywordColor);
    editor->send(SCI_STYLESETBOLD, SCE_C_WORD, 1);

    // Strings: Marrom/Laranja (0x800080 ou 0xA31515)
    int stringColor = darkMode ? 0xCE9178 : 0x808080;
    editor->send(SCI_STYLESETFORE, SCE_C_STRING, stringColor);
    editor->send(SCI_STYLESETFORE, SCE_C_CHARACTER, stringColor);

    // Números: Laranja (0xFF8000)
    int numberColor = darkMode ? 0xB5CEA8 : 0xFF8000;
    editor->send(SCI_STYLESETFORE, SCE_C_NUMBER, numberColor);

    // Operadores
    int opColor = darkMode ? 0xD4D4D4 : 0x000080;
    editor->send(SCI_STYLESETFORE, SCE_C_OPERATOR, opColor);

    // Indent guides e caret
    editor->send(SCI_SETINDENTATIONGUIDES, SC_IV_LOOKBOTH);
    editor->send(SCI_SETCARETLINEVISIBLE, 1);
    editor->send(SCI_SETCARETLINEBACK, darkMode ? 0x2A2A2A : 0xE8F2FE);
    editor->send(SCI_SETCARETFORE, darkMode ? 0xFFFFFF : 0x000000);
}
