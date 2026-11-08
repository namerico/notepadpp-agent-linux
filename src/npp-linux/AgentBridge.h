#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

class MainWindow;

class AgentBridge : public QObject {
    Q_OBJECT

public:
    explicit AgentBridge(MainWindow* mainWindow, QObject *parent = nullptr);
    virtual ~AgentBridge();

    bool startServer(const QString& socketName = "/tmp/npp_antigravity.sock");
    void stopServer();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();

private:
    void handleSingleMessage(QLocalSocket* socket, const QByteArray& data);
    QJsonObject processJsonRpc(const QJsonObject& req);

    // Métodos RPC
    QJsonObject rpcPing(const QJsonObject& params);
    QJsonObject rpcListTabs(const QJsonObject& params);
    QJsonObject rpcSwitchTab(const QJsonObject& params);
    QJsonObject rpcNewTab(const QJsonObject& params);
    QJsonObject rpcOpenFile(const QJsonObject& params);
    QJsonObject rpcGetActiveBuffer(const QJsonObject& params);
    QJsonObject rpcSetActiveBuffer(const QJsonObject& params);
    QJsonObject rpcInsertText(const QJsonObject& params);
    QJsonObject rpcReplaceSelection(const QJsonObject& params);
    QJsonObject rpcApplyDiff(const QJsonObject& params);
    QJsonObject rpcSetSelection(const QJsonObject& params);
    QJsonObject rpcSaveActiveFile(const QJsonObject& params);
    QJsonObject rpcCloseTab(const QJsonObject& params);
    QJsonObject rpcRenameTab(const QJsonObject& params);
    QJsonObject rpcFocus(const QJsonObject& params);
    QJsonObject rpcListThemes(const QJsonObject& params);
    QJsonObject rpcSetTheme(const QJsonObject& params);
    QJsonObject rpcToggleFunctionList(const QJsonObject& params);
    QJsonObject rpcShowNotification(const QJsonObject& params);
    QJsonObject rpcSaveSession(const QJsonObject& params);
    QJsonObject rpcLoadSession(const QJsonObject& params);
    QJsonObject rpcSetPadding(const QJsonObject& params);
    QJsonObject rpcGetPadding(const QJsonObject& params);

    MainWindow* m_mainWindow;
    QLocalServer* m_server;
    QString m_socketPath;
    QMap<QLocalSocket*, QByteArray> m_socketBuffers;
};
