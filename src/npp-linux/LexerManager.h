#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>

class ScintillaEditBase;

struct StyleRule {
    int styleId = -1;
    int fgColor = -1;
    int bgColor = -1;
    int bold = -1;
    int italic = -1;
    QString fontName;
    int fontSize = -1;
};

class LexerManager {
public:
    static LexerManager& instance();

    bool initialize();
    void applyLexerForFile(ScintillaEditBase* editor, const QString& filePath);
    void applyTheme(ScintillaEditBase* editor);

    QStringList availableThemes() const;
    QString currentTheme() const { return m_currentThemeName; }
    bool setTheme(const QString& themeName);

    void setPadding(int left, int right, int topAscent, int bottomDescent);
    int paddingLeft() const { return m_paddingLeft; }
    int paddingRight() const { return m_paddingRight; }
    int extraAscent() const { return m_extraAscent; }
    int extraDescent() const { return m_extraDescent; }

    QString getAutoCompletionWords(const QString& filePath) const;

private:
    LexerManager();
    ~LexerManager();

    void loadDefaultStyles();
    bool parseThemeXml(const QString& filePath);
    void parseAutoCompletionXml(const QString& filePath, const QString& ext);

    typedef void* (*CreateLexerFn)(const char* name);
    CreateLexerFn m_createLexerFn = nullptr;
    QMap<QString, QString> m_extensionToLexer;
    bool m_initialized = false;

    QString m_currentThemeName = "Padrão";
    QVector<StyleRule> m_globalStyles;
    QMap<QString, QVector<StyleRule>> m_lexerStyles; // Nome do lexer -> regras
    QMap<QString, QStringList> m_autoCompletionKeywords; // ext -> lista de keywords

    int m_defaultBg = 0xFFFFFF;
    int m_defaultFg = 0x000000;
    int m_lineMarginBg = 0xF0F0F0;
    int m_lineMarginFg = 0x707070;
    int m_caretColor = 0x000000;
    int m_currentLineBg = 0xE8F2FE;
    int m_selectionBg = 0xD4A070;

    int m_paddingLeft = 16;
    int m_paddingRight = 16;
    int m_extraAscent = 6;
    int m_extraDescent = 4;
};
