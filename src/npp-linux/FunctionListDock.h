#pragma once

#include <QDockWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class EditorTab;

class FunctionListDock : public QDockWidget {
    Q_OBJECT

public:
    explicit FunctionListDock(QWidget *parent = nullptr);
    virtual ~FunctionListDock();

    void updateSymbols(EditorTab* tab);
    void clear();

signals:
    void lineSelected(int line);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onFilterChanged(const QString &text);

private:
    void parseContent(const QString& text, const QString& filePath);

    QLineEdit *m_filterEdit;
    QTreeWidget *m_treeWidget;
};
