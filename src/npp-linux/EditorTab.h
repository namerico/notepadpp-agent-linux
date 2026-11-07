#pragma once

#include <QWidget>
#include <QString>
#include <memory>

class ScintillaEditBase;

class EditorTab : public QWidget {
    Q_OBJECT

public:
    explicit EditorTab(QWidget *parent = nullptr, const QString& filePath = "");
    virtual ~EditorTab();

    bool loadFromFile(const QString& path);
    bool saveToFile(const QString& path = "");

    QString filePath() const { return m_filePath; }
    QString title() const;
    void setTitle(const QString& title);
    bool isModified() const { return m_isModified; }

    QString text() const;
    void setText(const QString& newText);
    void insertText(const QString& textToInsert);
    void replaceSelection(const QString& replacement);
    void applyDiff(int startPos, int length, const QString& replacement);

    int currentLine() const;
    int currentColumn() const;
    int lineCount() const;
    void getSelection(int& start, int& end) const;
    void setSelection(int start, int end);
    void gotoLine(int line);
    void reloadTheme();

    ScintillaEditBase* editor() const { return m_editor; }

signals:
    void modificationStateChanged(bool modified);
    void cursorChanged(int line, int col);
    void titleChanged(const QString& newTitle);

private slots:
    void onEditorModified();
    void onSavePointChanged(bool dirty);
    void onCharAdded(int ch);

private:
    void setupEditor();

    ScintillaEditBase* m_editor = nullptr;
    QString m_filePath;
    QString m_customTitle;
    bool m_isModified = false;
    static int s_unnamedCount;
    int m_unnamedIndex = 0;
};
