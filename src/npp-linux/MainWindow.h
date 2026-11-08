#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QAction>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QVector>

class EditorTab;
class AgentBridge;
class FunctionListDock;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow();

    // Métodos expostos para UI e Agent Bridge
    EditorTab* newTab(const QString& initialText = "");
    EditorTab* openFile(const QString& path, int line = 0);
    bool saveActiveFile();
    bool saveFileAs();
    bool saveAllFiles();
    void closeTab(int index);
    void closeAllTabs();

    int tabCount() const;
    int currentTabIndex() const;
    void setCurrentTabIndex(int index);
    void setTabTitle(int index, const QString& title);
    EditorTab* tabAt(int index) const;
    EditorTab* currentTab() const;

    void showStatusMessage(const QString& msg, int timeout = 4000);
    void applyTheme(const QString& themeName);
    void toggleFunctionList(bool visible);
    FunctionListDock* functionListDock() const { return m_functionListDock; }

    void saveSession(const QString& path = "");
    bool loadSession(const QString& path = "");
    QString defaultSessionPath() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onCurrentTabChanged(int index);
    void onTabCloseRequested(int index);
    void onTabModificationChanged(bool modified);
    void onCursorChanged(int line, int col);
    void onTabTitleChanged(const QString& title);

    // Ações de Menu e Toolbar
    void actionNew();
    void actionOpen();
    void actionSave();
    void actionSaveAs();
    void actionSaveAll();
    void actionClose();
    void actionCloseAll();
    void actionUndo();
    void actionRedo();
    void actionCut();
    void actionCopy();
    void actionPaste();
    void actionFind();
    void actionReplace();
    void actionZoomIn();
    void actionZoomOut();
    void actionAbout();

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void updateTabIcon(int index, bool modified);
    void updateStatusBarInfo();

    QTabWidget* m_tabWidget;
    AgentBridge* m_agentBridge;
    FunctionListDock* m_functionListDock;

    // Status bar labels
    QLabel* m_statusMessageLabel;
    QLabel* m_statusDocInfoLabel;
    QLabel* m_statusCursorLabel;
    QLabel* m_statusEolLabel;
    QLabel* m_statusEncodingLabel;
    QLabel* m_statusAgentBridgeLabel;

    // Actions
    QAction* m_actNew;
    QAction* m_actOpen;
    QAction* m_actSave;
    QAction* m_actSaveAs;
    QAction* m_actSaveAll;
    QAction* m_actClose;
    QAction* m_actCloseAll;
    QAction* m_actExit;

    QAction* m_actUndo;
    QAction* m_actRedo;
    QAction* m_actCut;
    QAction* m_actCopy;
    QAction* m_actPaste;
    QAction* m_actSelectAll;

    QAction* m_actFind;
    QAction* m_actReplace;
    QAction* m_actZoomIn;
    QAction* m_actZoomOut;
    QAction* m_actToggleFunctionList;
    QAction* m_actAbout;
};
