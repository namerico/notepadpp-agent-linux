#include "AgentBridge.h"
#include "MainWindow.h"
#include "EditorTab.h"
#include "LexerManager.h"
#include "FunctionListDock.h"

#include <QLocalSocket>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

AgentBridge::AgentBridge(MainWindow* mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &AgentBridge::onNewConnection);
}

AgentBridge::~AgentBridge() {
    stopServer();
}

bool AgentBridge::startServer(const QString& socketName) {
    m_socketPath = socketName;

    // Remove socket anterior se estiver pendente
    QLocalServer::removeServer(m_socketPath);

    if (!m_server->listen(m_socketPath)) {
        qWarning() << "Falha ao iniciar servidor AgentBridge em" << m_socketPath << ":" << m_server->errorString();
        return false;
    }

    qInfo() << "Servidor AgentBridge ouvindo em" << m_socketPath;
    return true;
}

void AgentBridge::stopServer() {
    if (m_server && m_server->isListening()) {
        m_server->close();
        QLocalServer::removeServer(m_socketPath);
    }
}

void AgentBridge::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QLocalSocket* socket = m_server->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, &AgentBridge::onReadyRead);
        connect(socket, &QLocalSocket::disconnected, this, &AgentBridge::onSocketDisconnected);
    }
}

void AgentBridge::onReadyRead() {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    m_socketBuffers[socket].append(socket->readAll());
    QByteArray& buffer = m_socketBuffers[socket];

    int newlineIndex = -1;
    while ((newlineIndex = buffer.indexOf('\n')) != -1) {
        QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty()) {
            handleSingleMessage(socket, line);
        }
    }
}

void AgentBridge::onSocketDisconnected() {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        m_socketBuffers.remove(socket);
        socket->deleteLater();
    }
}

void AgentBridge::handleSingleMessage(QLocalSocket* socket, const QByteArray& data) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject errResp;
        errResp["jsonrpc"] = "2.0";
        errResp["id"] = QJsonValue::Null;
        QJsonObject errObj;
        errObj["code"] = -32700;
        errObj["message"] = QString("Parse error: %1").arg(parseError.errorString());
        errResp["error"] = errObj;
        socket->write(QJsonDocument(errResp).toJson(QJsonDocument::Compact) + "\n");
        socket->flush();
        return;
    }

    QJsonObject req = doc.object();
    QJsonObject resp = processJsonRpc(req);
    socket->write(QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n");
    socket->flush();
}

QJsonObject AgentBridge::processJsonRpc(const QJsonObject& req) {
    QJsonObject resp;
    resp["jsonrpc"] = "2.0";
    QJsonValue id = req.value("id");
    resp["id"] = id;

    QString method = req.value("method").toString();
    QJsonObject params = req.value("params").toObject();

    try {
        QJsonObject result;
        if (method == "ping") {
            result = rpcPing(params);
        } else if (method == "listTabs") {
            result = rpcListTabs(params);
        } else if (method == "switchTab") {
            result = rpcSwitchTab(params);
        } else if (method == "newTab") {
            result = rpcNewTab(params);
        } else if (method == "openFile") {
            result = rpcOpenFile(params);
        } else if (method == "getActiveBuffer") {
            result = rpcGetActiveBuffer(params);
        } else if (method == "setActiveBuffer") {
            result = rpcSetActiveBuffer(params);
        } else if (method == "insertText") {
            result = rpcInsertText(params);
        } else if (method == "replaceSelection") {
            result = rpcReplaceSelection(params);
        } else if (method == "applyDiff") {
            result = rpcApplyDiff(params);
        } else if (method == "setSelection") {
            result = rpcSetSelection(params);
        } else if (method == "saveActiveFile") {
            result = rpcSaveActiveFile(params);
        } else if (method == "closeTab") {
            result = rpcCloseTab(params);
        } else if (method == "renameTab") {
            result = rpcRenameTab(params);
        } else if (method == "focus") {
            result = rpcFocus(params);
        } else if (method == "listThemes") {
            result = rpcListThemes(params);
        } else if (method == "setTheme") {
            result = rpcSetTheme(params);
        } else if (method == "toggleFunctionList") {
            result = rpcToggleFunctionList(params);
        } else if (method == "showNotification") {
            result = rpcShowNotification(params);
        } else if (method == "saveSession") {
            result = rpcSaveSession(params);
        } else if (method == "loadSession") {
            result = rpcLoadSession(params);
        } else {
            QJsonObject err;
            err["code"] = -32601;
            err["message"] = QString("Method not found: %1").arg(method);
            resp["error"] = err;
            return resp;
        }

        resp["result"] = result;
    } catch (const std::exception& e) {
        QJsonObject err;
        err["code"] = -32000;
        err["message"] = QString("Internal error: %1").arg(e.what());
        resp["error"] = err;
    }

    return resp;
}

QJsonObject AgentBridge::rpcPing(const QJsonObject& params) {
    Q_UNUSED(params);
    QJsonObject res;
    res["status"] = "ok";
    res["application"] = "Notepad++ Agent Linux";
    res["version"] = "8.9.8.1-agent";
    res["pid"] = static_cast<qint64>(QCoreApplication::applicationPid());
    res["tabCount"] = m_mainWindow->tabCount();
    res["currentTab"] = m_mainWindow->currentTabIndex();
    return res;
}

QJsonObject AgentBridge::rpcListTabs(const QJsonObject& params) {
    Q_UNUSED(params);
    QJsonObject res;
    QJsonArray tabsArr;

    int count = m_mainWindow->tabCount();
    for (int i = 0; i < count; ++i) {
        EditorTab* tab = m_mainWindow->tabAt(i);
        if (!tab) continue;

        QJsonObject tabObj;
        tabObj["index"] = i;
        tabObj["title"] = tab->title();
        tabObj["filePath"] = tab->filePath();
        tabObj["isModified"] = tab->isModified();
        tabObj["lineCount"] = tab->lineCount();
        tabObj["isActive"] = (i == m_mainWindow->currentTabIndex());
        tabsArr.append(tabObj);
    }

    res["tabs"] = tabsArr;
    res["total"] = count;
    return res;
}

QJsonObject AgentBridge::rpcSwitchTab(const QJsonObject& params) {
    int index = params.value("index").toInt(-1);
    int total = m_mainWindow->tabCount();

    if (index < 0 || index >= total) {
        QJsonObject res;
        res["success"] = false;
        res["error"] = "Invalid tab index";
        return res;
    }

    m_mainWindow->setCurrentTabIndex(index);
    QJsonObject res;
    res["success"] = true;
    res["currentTab"] = index;
    return res;
}

QJsonObject AgentBridge::rpcNewTab(const QJsonObject& params) {
    QString initialText = params.value("text").toString();
    EditorTab* tab = m_mainWindow->newTab(initialText);

    QJsonObject res;
    res["success"] = (tab != nullptr);
    res["tabIndex"] = m_mainWindow->currentTabIndex();
    if (tab) {
        res["title"] = tab->title();
    }
    return res;
}

QJsonObject AgentBridge::rpcOpenFile(const QJsonObject& params) {
    QString path = params.value("path").toString();
    int line = params.value("line").toInt(0);

    QJsonObject res;
    if (path.isEmpty()) {
        res["success"] = false;
        res["error"] = "Path cannot be empty";
        return res;
    }

    EditorTab* tab = m_mainWindow->openFile(path, line);
    if (!tab) {
        res["success"] = false;
        res["error"] = "Failed to open file";
        return res;
    }

    res["success"] = true;
    res["tabIndex"] = m_mainWindow->currentTabIndex();
    res["filePath"] = tab->filePath();
    res["title"] = tab->title();
    res["lines"] = tab->lineCount();
    return res;
}

QJsonObject AgentBridge::rpcGetActiveBuffer(const QJsonObject& params) {
    Q_UNUSED(params);
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    int selStart = 0, selEnd = 0;
    tab->getSelection(selStart, selEnd);

    res["success"] = true;
    res["tabIndex"] = m_mainWindow->currentTabIndex();
    res["filePath"] = tab->filePath();
    res["title"] = tab->title();
    res["isModified"] = tab->isModified();
    res["lineCount"] = tab->lineCount();
    res["currentLine"] = tab->currentLine();
    res["currentColumn"] = tab->currentColumn();
    res["selectionStart"] = selStart;
    res["selectionEnd"] = selEnd;
    res["text"] = tab->text();
    return res;
}

QJsonObject AgentBridge::rpcSetActiveBuffer(const QJsonObject& params) {
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    QString text = params.value("text").toString();
    tab->setText(text);
    res["success"] = true;
    res["lineCount"] = tab->lineCount();
    return res;
}

QJsonObject AgentBridge::rpcInsertText(const QJsonObject& params) {
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    QString text = params.value("text").toString();
    tab->insertText(text);
    res["success"] = true;
    res["lineCount"] = tab->lineCount();
    return res;
}

QJsonObject AgentBridge::rpcReplaceSelection(const QJsonObject& params) {
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    QString text = params.value("text").toString();
    tab->replaceSelection(text);
    res["success"] = true;
    res["lineCount"] = tab->lineCount();
    return res;
}

QJsonObject AgentBridge::rpcApplyDiff(const QJsonObject& params) {
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    int startPos = params.value("startPos").toInt(0);
    int length = params.value("length").toInt(0);
    QString replacement = params.value("replacement").toString();

    tab->applyDiff(startPos, length, replacement);
    res["success"] = true;
    res["lineCount"] = tab->lineCount();
    return res;
}

QJsonObject AgentBridge::rpcSetSelection(const QJsonObject& params) {
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    int start = params.value("start").toInt(0);
    int end = params.value("end").toInt(0);
    int line = params.value("line").toInt(-1);

    if (line > 0) {
        tab->gotoLine(line);
    } else {
        tab->setSelection(start, end);
    }

    res["success"] = true;
    res["currentLine"] = tab->currentLine();
    res["currentColumn"] = tab->currentColumn();
    return res;
}

QJsonObject AgentBridge::rpcSaveActiveFile(const QJsonObject& params) {
    Q_UNUSED(params);
    EditorTab* tab = m_mainWindow->currentTab();
    QJsonObject res;
    if (!tab) {
        res["success"] = false;
        res["error"] = "No active tab";
        return res;
    }

    bool ok = m_mainWindow->saveActiveFile();
    res["success"] = ok;
    res["filePath"] = tab->filePath();
    return res;
}

QJsonObject AgentBridge::rpcCloseTab(const QJsonObject& params) {
    int index = params.value("index").toInt(m_mainWindow->currentTabIndex());
    m_mainWindow->closeTab(index);
    QJsonObject res;
    res["success"] = true;
    res["remainingTabs"] = m_mainWindow->tabCount();
    return res;
}

QJsonObject AgentBridge::rpcRenameTab(const QJsonObject& params) {
    int index = params.value("index").toInt(m_mainWindow->currentTabIndex());
    QString title = params.value("title").toString();
    QJsonObject res;
    if (title.isEmpty()) {
        res["success"] = false;
        res["error"] = "Title cannot be empty";
        return res;
    }
    m_mainWindow->setTabTitle(index, title);
    res["success"] = true;
    res["tabIndex"] = index;
    res["title"] = title;
    return res;
}

QJsonObject AgentBridge::rpcFocus(const QJsonObject& params) {
    Q_UNUSED(params);
    m_mainWindow->showNormal();
    m_mainWindow->raise();
    m_mainWindow->activateWindow();
    QJsonObject res;
    res["success"] = true;
    return res;
}

QJsonObject AgentBridge::rpcListThemes(const QJsonObject& params) {
    Q_UNUSED(params);
    QJsonObject res;
    res["current"] = LexerManager::instance().currentTheme();
    QJsonArray arr;
    for (const QString& th : LexerManager::instance().availableThemes()) {
        arr.append(th);
    }
    res["themes"] = arr;
    return res;
}

QJsonObject AgentBridge::rpcSetTheme(const QJsonObject& params) {
    QString name = params.value("theme").toString();
    QJsonObject res;
    if (name.isEmpty()) {
        res["success"] = false;
        res["error"] = "Theme name is required";
        return res;
    }
    m_mainWindow->applyTheme(name);
    res["success"] = true;
    res["current"] = LexerManager::instance().currentTheme();
    return res;
}

QJsonObject AgentBridge::rpcToggleFunctionList(const QJsonObject& params) {
    bool visible = params.value("visible").toBool(!m_mainWindow->functionListDock()->isVisible());
    m_mainWindow->toggleFunctionList(visible);
    QJsonObject res;
    res["success"] = true;
    res["visible"] = visible;
    return res;
}

QJsonObject AgentBridge::rpcShowNotification(const QJsonObject& params) {
    QString msg = params.value("message").toString();
    int timeout = params.value("timeout").toInt(4000);
    m_mainWindow->showStatusMessage(QString("[Antigravity Agent] %1").arg(msg), timeout);
    QJsonObject res;
    res["success"] = true;
    return res;
}

QJsonObject AgentBridge::rpcSaveSession(const QJsonObject& params) {
    QString path = params.value("path").toString();
    m_mainWindow->saveSession(path);
    QJsonObject res;
    res["success"] = true;
    res["sessionPath"] = path.isEmpty() ? m_mainWindow->defaultSessionPath() : path;
    return res;
}

QJsonObject AgentBridge::rpcLoadSession(const QJsonObject& params) {
    QString path = params.value("path").toString();
    bool ok = m_mainWindow->loadSession(path);
    QJsonObject res;
    res["success"] = ok;
    res["tabCount"] = m_mainWindow->tabCount();
    res["sessionPath"] = path.isEmpty() ? m_mainWindow->defaultSessionPath() : path;
    return res;
}
