#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("Notepad++ Agent Linux");
    app.setApplicationVersion("8.9.8.1-agent");
    app.setOrganizationName("Antigravity");

    // Procura o ícone autêntico do camaleão do Notepad++
    QStringList iconCandidates = {
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
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args.at(i);
        if (!arg.startsWith("-")) {
            window.openFile(QFileInfo(arg).absoluteFilePath());
        }
    }

    return app.exec();
}
