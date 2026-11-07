#pragma once

#include <QString>
#include <QMap>
#include "Scintilla.h"
#include "SciLexer.h"
#include "ILexer.h"

class ScintillaEditBase;

class LexerManager {
public:
    static LexerManager& instance();

    bool initialize();
    void applyLexerForFile(ScintillaEditBase* editor, const QString& filePath);
    void applyTheme(ScintillaEditBase* editor, bool darkMode = false);

private:
    LexerManager();
    ~LexerManager();

    typedef void* (*CreateLexerFn)(const char* name);
    CreateLexerFn m_createLexerFn = nullptr;
    QMap<QString, QString> m_extensionToLexer;
    bool m_initialized = false;
};
