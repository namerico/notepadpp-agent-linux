#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QDir>
#include <QDebug>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Notepad++ Agent Linux");
    app.setApplicationVersion("8.9.8.1-agent");
    app.setOrganizationName("Antigravity");

    QString appDir = QApplication::applicationDirPath();

    // Procura o ícone autêntico do camaleão do Notepad++ de forma dinâmica e resiliente
    QStringList iconCandidates = {
        appDir + "/assets/icons/chameleon.png",
        appDir + "/../assets/icons/chameleon.png",
        "/usr/share/icons/hicolor/256x256/apps/notepadpp.png",
        appDir + "/../../src/notepad-plus-plus-upstream/PowerEditor/misc/chameleon/chameleon-pencil-200.png",
        "/home/namerico/Documentos/Projetos Antigravity/notepadpp-agent-linux/src/notepad-plus-plus-upstream/PowerEditor/misc/chameleon/chameleon-pencil-200.png",
        "/home/namerico/Documentos/Projetos Antigravity/notepadpp-agent-linux/src/notepad-plus-plus-upstream/PowerEditor/src/icons/npp.ico"
    };

    for (const QString& iconPath : iconCandidates) {
        if (QFileInfo::exists(iconPath)) {
            app.setWindowIcon(QIcon(iconPath));
            break;
        }
    }

    MainWindow window;
    window.show();

    // Se o usuário passou arquivos pela linha de comando, abre todos eles
    QStringList args = app.arguments();
    bool noSession = false;
    QStringList filesToOpen;

    for (int i = 1; i < args.size(); ++i) {
        QString arg = args.at(i);
        if (arg == "--no-session" || arg == "-nosession") {
            noSession = true;
        } else if (!arg.startsWith("-")) {
            filesToOpen.append(QFileInfo(arg).absoluteFilePath());
        }
    }

    if (!filesToOpen.isEmpty()) {
        for (const QString& file : filesToOpen) {
            window.openFile(file);
        }
    } else if (!noSession) {
        // Carrega a sessão anterior automaticamente (como o Notepad++ oficial)
        window.loadSession();
    }

    return app.exec();
}
