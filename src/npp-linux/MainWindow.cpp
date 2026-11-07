#include "MainWindow.h"
#include "EditorTab.h"
#include "AgentBridge.h"
#include "FunctionListDock.h"
#include "LexerManager.h"
#include "ScintillaEditBase.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QDebug>
#include <QIcon>

static QIcon createTabIcon(bool modified) {
    QPixmap pix(14, 14);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    if (modified) {
        // Círculo vermelho do Notepad++ para arquivo alterado
        p.setBrush(QColor(220, 50, 50));
        p.setPen(QColor(180, 20, 20));
        p.drawEllipse(1, 1, 11, 11);
    } else {
        // Disquete azul do Notepad++ para arquivo salvo
        p.setBrush(QColor(60, 120, 210));
        p.setPen(QColor(30, 80, 160));
        p.drawRoundedRect(1, 1, 11, 11, 2, 2);
        p.setBrush(Qt::white);
        p.drawRect(3, 2, 7, 4);
    }
    return QIcon(pix);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(new QTabWidget(this))
    , m_agentBridge(new AgentBridge(this, this))
    , m_functionListDock(new FunctionListDock(this))
{
    setWindowTitle("Notepad++ (Antigravity Linux Agent Edition)");
    resize(1200, 800);

    // Configuração do TabWidget
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    setCentralWidget(m_tabWidget);

    // Dock lateral de Funções
    addDockWidget(Qt::RightDockWidgetArea, m_functionListDock);
    connect(m_functionListDock, &FunctionListDock::lineSelected, this, [this](int line) {
        EditorTab* tab = currentTab();
        if (tab) {
            tab->gotoLine(line);
        }
    });

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    // Inicia com um novo documento vazio "new 1"
    newTab();

    // Inicia o servidor IPC Agent Bridge
    m_agentBridge->startServer("/tmp/npp_antigravity.sock");
    m_statusAgentBridgeLabel->setText(" Agent Bridge: /tmp/npp_antigravity.sock (Ativo) ");
}

MainWindow::~MainWindow() {}

void MainWindow::createActions() {
    // Arquivo
    m_actNew = new QAction(style()->standardIcon(QStyle::SP_FileIcon), "&Novo", this);
    m_actNew->setShortcut(QKeySequence::New);
    connect(m_actNew, &QAction::triggered, this, &MainWindow::actionNew);

    m_actOpen = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "&Abrir...", this);
    m_actOpen->setShortcut(QKeySequence::Open);
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::actionOpen);

    m_actSave = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "&Salvar", this);
    m_actSave->setShortcut(QKeySequence::Save);
    connect(m_actSave, &QAction::triggered, this, &MainWindow::actionSave);

    m_actSaveAs = new QAction("Salvar &Como...", this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(m_actSaveAs, &QAction::triggered, this, &MainWindow::actionSaveAs);

    m_actSaveAll = new QAction("Salvar &Todos", this);
    m_actSaveAll->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    connect(m_actSaveAll, &QAction::triggered, this, &MainWindow::actionSaveAll);

    m_actClose = new QAction("Fechar", this);
    m_actClose->setShortcut(QKeySequence::Close);
    connect(m_actClose, &QAction::triggered, this, &MainWindow::actionClose);

    m_actCloseAll = new QAction("Fechar Todos", this);
    connect(m_actCloseAll, &QAction::triggered, this, &MainWindow::actionCloseAll);

    m_actExit = new QAction("Sair", this);
    m_actExit->setShortcut(QKeySequence::Quit);
    connect(m_actExit, &QAction::triggered, this, &MainWindow::close);

    // Editar
    m_actUndo = new QAction(style()->standardIcon(QStyle::SP_ArrowBack), "&Desfazer", this);
    m_actUndo->setShortcut(QKeySequence::Undo);
    connect(m_actUndo, &QAction::triggered, this, &MainWindow::actionUndo);

    m_actRedo = new QAction(style()->standardIcon(QStyle::SP_ArrowForward), "&Refazer", this);
    m_actRedo->setShortcut(QKeySequence::Redo);
    connect(m_actRedo, &QAction::triggered, this, &MainWindow::actionRedo);

    m_actCut = new QAction("Recor&tar", this);
    m_actCut->setShortcut(QKeySequence::Cut);
    connect(m_actCut, &QAction::triggered, this, &MainWindow::actionCut);

    m_actCopy = new QAction("&Copiar", this);
    m_actCopy->setShortcut(QKeySequence::Copy);
    connect(m_actCopy, &QAction::triggered, this, &MainWindow::actionCopy);

    m_actPaste = new QAction("Co&lar", this);
    m_actPaste->setShortcut(QKeySequence::Paste);
    connect(m_actPaste, &QAction::triggered, this, &MainWindow::actionPaste);

    m_actSelectAll = new QAction("Selecionar &Tudo", this);
    m_actSelectAll->setShortcut(QKeySequence::SelectAll);
    connect(m_actSelectAll, &QAction::triggered, [this]() {
        EditorTab* tab = currentTab();
        if (tab && tab->editor()) {
            tab->editor()->send(SCI_SELECTALL);
        }
    });

    // Pesquisar
    m_actFind = new QAction("Localizar...", this);
    m_actFind->setShortcut(QKeySequence::Find);
    connect(m_actFind, &QAction::triggered, this, &MainWindow::actionFind);

    m_actReplace = new QAction("Substituir...", this);
    m_actReplace->setShortcut(QKeySequence::Replace);
    connect(m_actReplace, &QAction::triggered, this, &MainWindow::actionReplace);

    // Exibir
    m_actZoomIn = new QAction("Aumentar Zoom", this);
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(m_actZoomIn, &QAction::triggered, this, &MainWindow::actionZoomIn);

    m_actZoomOut = new QAction("Diminuir Zoom", this);
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(m_actZoomOut, &QAction::triggered, this, &MainWindow::actionZoomOut);

    m_actToggleFunctionList = new QAction("Lista de Funções (Function List)", this);
    m_actToggleFunctionList->setCheckable(true);
    m_actToggleFunctionList->setChecked(true);
    m_actToggleFunctionList->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
    connect(m_actToggleFunctionList, &QAction::toggled, this, &MainWindow::toggleFunctionList);
    connect(m_functionListDock, &QDockWidget::visibilityChanged, m_actToggleFunctionList, &QAction::setChecked);

    // Ajuda
    m_actAbout = new QAction("&Sobre o Notepad++ Agent Linux", this);
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::actionAbout);
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&Arquivo");
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addAction(m_actSaveAs);
    fileMenu->addAction(m_actSaveAll);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actClose);
    fileMenu->addAction(m_actCloseAll);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actExit);

    QMenu* editMenu = menuBar()->addMenu("&Editar");
    editMenu->addAction(m_actUndo);
    editMenu->addAction(m_actRedo);
    editMenu->addSeparator();
    editMenu->addAction(m_actCut);
    editMenu->addAction(m_actCopy);
    editMenu->addAction(m_actPaste);
    editMenu->addSeparator();
    editMenu->addAction(m_actSelectAll);

    QMenu* searchMenu = menuBar()->addMenu("&Pesquisar");
    searchMenu->addAction(m_actFind);
    searchMenu->addAction(m_actReplace);

    QMenu* viewMenu = menuBar()->addMenu("E&xibir");
    viewMenu->addAction(m_actZoomIn);
    viewMenu->addAction(m_actZoomOut);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actToggleFunctionList);

    QMenu* langMenu = menuBar()->addMenu("&Linguagem");
    langMenu->addAction("Detectar Automaticamente");

    QMenu* settingsMenu = menuBar()->addMenu("&Configurações");
    QMenu* themesMenu = settingsMenu->addMenu("&Temas");
    QStringList themes = LexerManager::instance().availableThemes();
    for (const QString& th : themes) {
        QAction* thAct = themesMenu->addAction(th);
        connect(thAct, &QAction::triggered, this, [this, th]() {
            applyTheme(th);
        });
    }

    QMenu* helpMenu = menuBar()->addMenu("A&juda");
    helpMenu->addAction(m_actAbout);
}

void MainWindow::createToolBars() {
    QToolBar* mainToolBar = addToolBar("Barra Principal");
    mainToolBar->setMovable(false);
    mainToolBar->addAction(m_actNew);
    mainToolBar->addAction(m_actOpen);
    mainToolBar->addAction(m_actSave);
    mainToolBar->addAction(m_actSaveAll);
    mainToolBar->addAction(m_actClose);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_actCut);
    mainToolBar->addAction(m_actCopy);
    mainToolBar->addAction(m_actPaste);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_actUndo);
    mainToolBar->addAction(m_actRedo);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_actFind);
    mainToolBar->addAction(m_actReplace);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_actZoomIn);
    mainToolBar->addAction(m_actZoomOut);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_actToggleFunctionList);
}

void MainWindow::createStatusBar() {
    QStatusBar* bar = statusBar();

    m_statusMessageLabel = new QLabel("Pronto", this);
    m_statusDocInfoLabel = new QLabel("length: 0   lines: 1", this);
    m_statusCursorLabel = new QLabel("Ln: 1   Col: 1   Sel: 0", this);
    m_statusEolLabel = new QLabel("Unix (LF)", this);
    m_statusEncodingLabel = new QLabel("UTF-8", this);
    m_statusAgentBridgeLabel = new QLabel(" Agent Bridge: Inativo ", this);

    m_statusDocInfoLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_statusCursorLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_statusEolLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_statusEncodingLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_statusAgentBridgeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    bar->addWidget(m_statusMessageLabel, 1);
    bar->addPermanentWidget(m_statusDocInfoLabel);
    bar->addPermanentWidget(m_statusCursorLabel);
    bar->addPermanentWidget(m_statusEolLabel);
    bar->addPermanentWidget(m_statusEncodingLabel);
    bar->addPermanentWidget(m_statusAgentBridgeLabel);
}

EditorTab* MainWindow::newTab(const QString& initialText) {
    EditorTab* tab = new EditorTab(m_tabWidget);
    if (!initialText.isEmpty()) {
        tab->setText(initialText);
    }

    int idx = m_tabWidget->addTab(tab, createTabIcon(false), tab->title());
    m_tabWidget->setCurrentIndex(idx);

    connect(tab, &EditorTab::modificationStateChanged, this, &MainWindow::onTabModificationChanged);
    connect(tab, &EditorTab::cursorChanged, this, &MainWindow::onCursorChanged);
    connect(tab, &EditorTab::titleChanged, this, &MainWindow::onTabTitleChanged);

    updateStatusBarInfo();
    return tab;
}

EditorTab* MainWindow::openFile(const QString& path, int line) {
    // Checa se o arquivo já está aberto em alguma aba
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab* existing = qobject_cast<EditorTab*>(m_tabWidget->widget(i));
        if (existing && existing->filePath() == path) {
            m_tabWidget->setCurrentIndex(i);
            if (line > 0) existing->gotoLine(line);
            return existing;
        }
    }

    // Se a aba atual for uma "new 1" vazia e não modificada, usa ela
    EditorTab* current = currentTab();
    if (m_tabWidget->count() == 1 && current && current->filePath().isEmpty() && !current->isModified() && current->text().isEmpty()) {
        if (current->loadFromFile(path)) {
            m_tabWidget->setTabText(0, current->title());
            updateTabIcon(0, false);
            if (line > 0) current->gotoLine(line);
            updateStatusBarInfo();
            return current;
        }
    }

    EditorTab* tab = new EditorTab(m_tabWidget, path);
    int idx = m_tabWidget->addTab(tab, createTabIcon(false), tab->title());
    m_tabWidget->setCurrentIndex(idx);

    connect(tab, &EditorTab::modificationStateChanged, this, &MainWindow::onTabModificationChanged);
    connect(tab, &EditorTab::cursorChanged, this, &MainWindow::onCursorChanged);
    connect(tab, &EditorTab::titleChanged, this, &MainWindow::onTabTitleChanged);

    if (line > 0) {
        tab->gotoLine(line);
    }

    updateStatusBarInfo();
    return tab;
}

bool MainWindow::saveActiveFile() {
    EditorTab* tab = currentTab();
    if (!tab) return false;

    if (tab->filePath().isEmpty()) {
        return saveFileAs();
    }
    return tab->saveToFile();
}

bool MainWindow::saveFileAs() {
    EditorTab* tab = currentTab();
    if (!tab) return false;

    QString fileName = QFileDialog::getSaveFileName(this, "Salvar Arquivo Como", tab->filePath().isEmpty() ? tab->title() : tab->filePath());
    if (fileName.isEmpty()) return false;

    return tab->saveToFile(fileName);
}

bool MainWindow::saveAllFiles() {
    bool ok = true;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab* tab = qobject_cast<EditorTab*>(m_tabWidget->widget(i));
        if (tab && tab->isModified()) {
            if (tab->filePath().isEmpty()) {
                m_tabWidget->setCurrentIndex(i);
                if (!saveFileAs()) ok = false;
            } else {
                if (!tab->saveToFile()) ok = false;
            }
        }
    }
    return ok;
}

void MainWindow::closeTab(int index) {
    if (index < 0 || index >= m_tabWidget->count()) return;

    EditorTab* tab = qobject_cast<EditorTab*>(m_tabWidget->widget(index));
    if (tab && tab->isModified()) {
        auto res = QMessageBox::question(this, "Notepad++",
            QString("Salvar alterações em \"%1\"?").arg(tab->title()),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (res == QMessageBox::Yes) {
            if (tab->filePath().isEmpty()) {
                m_tabWidget->setCurrentIndex(index);
                if (!saveFileAs()) return;
            } else {
                tab->saveToFile();
            }
        } else if (res == QMessageBox::Cancel) {
            return;
        }
    }

    m_tabWidget->removeTab(index);
    if (tab) tab->deleteLater();

    if (m_tabWidget->count() == 0) {
        newTab();
    }
}

void MainWindow::closeAllTabs() {
    while (m_tabWidget->count() > 0) {
        closeTab(0);
    }
}

int MainWindow::tabCount() const {
    return m_tabWidget->count();
}

int MainWindow::currentTabIndex() const {
    return m_tabWidget->currentIndex();
}

void MainWindow::setCurrentTabIndex(int index) {
    m_tabWidget->setCurrentIndex(index);
}

void MainWindow::setTabTitle(int index, const QString& title) {
    if (index >= 0 && index < m_tabWidget->count()) {
        EditorTab* tab = tabAt(index);
        if (tab) {
            tab->setTitle(title);
            m_tabWidget->setTabText(index, title);
        }
    }
}

EditorTab* MainWindow::tabAt(int index) const {
    return qobject_cast<EditorTab*>(m_tabWidget->widget(index));
}

EditorTab* MainWindow::currentTab() const {
    return qobject_cast<EditorTab*>(m_tabWidget->currentWidget());
}

void MainWindow::showStatusMessage(const QString& msg, int timeout) {
    m_statusMessageLabel->setText(msg);
    if (timeout > 0) {
        QStatusBar* bar = statusBar();
        bar->showMessage(msg, timeout);
    }
}

void MainWindow::applyTheme(const QString& themeName) {
    if (LexerManager::instance().setTheme(themeName)) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            EditorTab* t = tabAt(i);
            if (t) {
                t->reloadTheme();
            }
        }
        showStatusMessage(QString("Tema aplicado: %1").arg(themeName));
    }
}

void MainWindow::toggleFunctionList(bool visible) {
    if (m_functionListDock) {
        m_functionListDock->setVisible(visible);
        if (visible) {
            m_functionListDock->updateSymbols(currentTab());
        }
    }
}

void MainWindow::updateTabIcon(int index, bool modified) {
    m_tabWidget->setTabIcon(index, createTabIcon(modified));
}

void MainWindow::updateStatusBarInfo() {
    EditorTab* tab = currentTab();
    if (!tab) return;

    QString text = tab->text();
    int len = text.length();
    int lines = tab->lineCount();
    m_statusDocInfoLabel->setText(QString("length: %1   lines: %2").arg(len).arg(lines));

    int line = tab->currentLine();
    int col = tab->currentColumn();
    int selStart = 0, selEnd = 0;
    tab->getSelection(selStart, selEnd);
    int selLen = qAbs(selEnd - selStart);

    m_statusCursorLabel->setText(QString("Ln: %1   Col: %2   Sel: %3").arg(line).arg(col).arg(selLen));
}

void MainWindow::onCurrentTabChanged(int index) {
    Q_UNUSED(index);
    updateStatusBarInfo();
    if (m_functionListDock && m_functionListDock->isVisible()) {
        m_functionListDock->updateSymbols(currentTab());
    }
}

void MainWindow::onTabCloseRequested(int index) {
    closeTab(index);
}

void MainWindow::onTabModificationChanged(bool modified) {
    EditorTab* tab = qobject_cast<EditorTab*>(sender());
    if (!tab) return;

    int idx = m_tabWidget->indexOf(tab);
    if (idx != -1) {
        updateTabIcon(idx, modified);
    }
    updateStatusBarInfo();
}

void MainWindow::onCursorChanged(int line, int col) {
    Q_UNUSED(line);
    Q_UNUSED(col);
    updateStatusBarInfo();
}

void MainWindow::onTabTitleChanged(const QString& title) {
    EditorTab* tab = qobject_cast<EditorTab*>(sender());
    if (!tab) return;

    int idx = m_tabWidget->indexOf(tab);
    if (idx != -1) {
        m_tabWidget->setTabText(idx, title);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Confirma salvamento de arquivos modificados
    bool hasModified = false;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        EditorTab* t = tabAt(i);
        if (t && t->isModified()) {
            hasModified = true;
            break;
        }
    }

    if (hasModified) {
        auto res = QMessageBox::question(this, "Notepad++ Agent Linux",
            "Existem documentos modificados. Deseja salvar tudo antes de sair?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (res == QMessageBox::Yes) {
            if (!saveAllFiles()) {
                event->ignore();
                return;
            }
        } else if (res == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    m_agentBridge->stopServer();
    event->accept();
}

void MainWindow::actionNew() {
    newTab();
}

void MainWindow::actionOpen() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Abrir Arquivo(s)");
    for (const QString& file : files) {
        openFile(file);
    }
}

void MainWindow::actionSave() {
    saveActiveFile();
}

void MainWindow::actionSaveAs() {
    saveFileAs();
}

void MainWindow::actionSaveAll() {
    saveAllFiles();
}

void MainWindow::actionClose() {
    closeTab(currentTabIndex());
}

void MainWindow::actionCloseAll() {
    closeAllTabs();
}

void MainWindow::actionUndo() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_UNDO);
    }
}

void MainWindow::actionRedo() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_REDO);
    }
}

void MainWindow::actionCut() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_CUT);
    }
}

void MainWindow::actionCopy() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_COPY);
    }
}

void MainWindow::actionPaste() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_PASTE);
    }
}

void MainWindow::actionFind() {
    bool ok;
    QString text = QInputDialog::getText(this, "Localizar", "Texto:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        EditorTab* tab = currentTab();
        if (tab && tab->editor()) {
            QByteArray utf8 = text.toUtf8();
            int curPos = tab->editor()->send(SCI_GETCURRENTPOS);
            tab->editor()->send(SCI_SETSEARCHFLAGS, 0);
            tab->editor()->send(SCI_SETTARGETSTART, curPos);
            tab->editor()->send(SCI_SETTARGETEND, tab->editor()->send(SCI_GETTEXTLENGTH));
            int foundPos = tab->editor()->send(SCI_SEARCHINTARGET, utf8.length(), reinterpret_cast<sptr_t>(utf8.constData()));
            if (foundPos != -1) {
                tab->setSelection(foundPos, foundPos + utf8.length());
            } else {
                showStatusMessage(QString("Texto \"%1\" não encontrado.").arg(text));
            }
        }
    }
}

void MainWindow::actionReplace() {
    bool ok;
    QString findText = QInputDialog::getText(this, "Substituir", "Localizar:", QLineEdit::Normal, "", &ok);
    if (!ok || findText.isEmpty()) return;

    QString repText = QInputDialog::getText(this, "Substituir", "Substituir por:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        QByteArray fUtf8 = findText.toUtf8();
        QByteArray rUtf8 = repText.toUtf8();

        int curPos = tab->editor()->send(SCI_GETCURRENTPOS);
        tab->editor()->send(SCI_SETSEARCHFLAGS, 0);
        tab->editor()->send(SCI_SETTARGETSTART, curPos);
        tab->editor()->send(SCI_SETTARGETEND, tab->editor()->send(SCI_GETTEXTLENGTH));
        int foundPos = tab->editor()->send(SCI_SEARCHINTARGET, fUtf8.length(), reinterpret_cast<sptr_t>(fUtf8.constData()));
        if (foundPos != -1) {
            tab->editor()->send(SCI_REPLACETARGET, rUtf8.length(), reinterpret_cast<sptr_t>(rUtf8.constData()));
            tab->setSelection(foundPos, foundPos + rUtf8.length());
            showStatusMessage("Ocorrência substituída.");
        } else {
            showStatusMessage("Nenhuma ocorrência encontrada.");
        }
    }
}

void MainWindow::actionZoomIn() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_ZOOMIN);
    }
}

void MainWindow::actionZoomOut() {
    EditorTab* tab = currentTab();
    if (tab && tab->editor()) {
        tab->editor()->send(SCI_ZOOMOUT);
    }
}

void MainWindow::actionAbout() {
    QMessageBox::about(this, "Sobre o Notepad++ Agent Linux",
        "<h2>Notepad++ v8.9.8.1 (Antigravity Linux Agent Edition)</h2>"
        "<p>Port nativo de alto desempenho baseado em <b>Scintilla + Lexilla + Qt5</b> integrado ao ecossistema <b>Google Antigravity</b>.</p>"
        "<p><b>Recursos:</b></p>"
        "<ul>"
        "<li>Agent Bridge IPC ativo via Unix Domain Socket: <code>/tmp/npp_antigravity.sock</code></li>"
        "<li>Realce sintático e dobramento de código (folding) nativo</li>"
        "<li>Controle remoto e automação por agentes autônomos e Artemis Linux</li>"
        "<li>Servidor MCP nativo <code>npp-mcp</code> para pareamento de IA</li>"
        "</ul>");
}
