#include "include/Extensions/installextension.h"
#include "include/Extensions/extension.h"
#include "include/notepadqq.h"
#include "ui_installextension.h"
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QSettings>
#include <QDir>

namespace Extensions {

    InstallExtension::InstallExtension(const QString &extensionFilename, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::InstallExtension),
        m_extensionFilename(extensionFilename)
    {
        ui->setupUi(this);

        QFont f = ui->lblName->font();
        f.setPointSizeF(f.pointSizeF() * 1.2);
        ui->lblName->setFont(f);

        //setFixedSize(this->width(), this->height());
        setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMinMaxButtonsHint);

        QString manifestStr = readExtensionManifest(extensionFilename);
        if (!manifestStr.isNull()) {
            QJsonParseError err;
            QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestStr.toUtf8(), &err);

            if (err.error != QJsonParseError::NoError) {
                // FIXME Failed to load
                qDebug() << manifestStr;
                qCritical() << err.errorString();
            }

            QJsonObject manifest = manifestDoc.object();

            m_uniqueName = manifest.value("unique_name").toString();
            m_runtime = manifest.value("runtime").toString();
            ui->lblName->setText(manifest.value("name").toString());
            ui->lblVersionAuthor->setText(tr("Version %1, %2")
                                          .arg(manifest.value("version").toString(tr("unknown version")))
                                          .arg(manifest.value("author").toString(tr("unknown author"))));
            ui->lblDescription->setText(manifest.value("description").toString());

            // Tell the user if this is an update
            QString alreadyInstalledPath = getAbsoluteExtensionFolder(Notepadqq::extensionsPath(), m_uniqueName);
            if (!alreadyInstalledPath.isNull()) {
                QJsonObject manifest = Extension::getManifest(alreadyInstalledPath);
                if (!manifest.isEmpty()) {
                    QString currentVersion = manifest.value("version").toString(tr("unknown version"));
                    ui->lblVersionAuthor->setText(ui->lblVersionAuthor->text() + " " +
                                                  tr("(current version is %1)").arg(currentVersion));
                    ui->btnInstall->setText(tr("Update"));
                }
            }

        } else {
            // FIXME Error reading manifest from archive
        }
    }

    InstallExtension::~InstallExtension()
    {
        delete ui;
    }

    QString InstallExtension::getAbsoluteExtensionFolder(const QString &extensionsPath, const QString &extensionUniqueName)
    {
        if (extensionUniqueName.isEmpty())
            return QString();

        if (!extensionUniqueName.contains(QRegExp(R"(^[-_0-9a-z]+(\.[-_0-9a-z]+)+$)", Qt::CaseSensitive))) {
            return QString();
        }

        QDir path(extensionsPath);
        return path.absoluteFilePath(extensionUniqueName);

    }

    bool InstallExtension::installNodejsExtension(const QString &packagePath)
    {
        QProcess process;
        QByteArray output;
        QByteArray error;
        process.setWorkingDirectory(Notepadqq::extensionToolsPath());
        process.start(Notepadqq::nodejsPath(), QStringList()
                      << "install.js"
                      << packagePath
                      << Notepadqq::extensionsPath()
                      << Notepadqq::npmPath());

        if (process.waitForStarted(20000)) {
            while (process.waitForReadyRead(30000)) {
                output.append(process.readAllStandardOutput());
                error.append(process.readAllStandardError());
            }

            QString err = QString(error);
            if (err.trimmed().isEmpty()) {
                QMessageBox::information(this, tr("Extension installed"), tr("The extension has been successfully installed!"));
                return true;
            } else {
                QMessageBox::critical(this, tr("Error installing the extension"), err);
            }
        } else {
            // FIXME Display error
        }

        return false;
    }

    QString InstallExtension::readExtensionManifest(const QString &archivePath)
    {
        QProcess process;
        QByteArray output;
        process.setWorkingDirectory(Notepadqq::extensionToolsPath());
        process.start(Notepadqq::nodejsPath(), QStringList() << "readmanifest.js" << archivePath);

        if (process.waitForStarted(20000)) {
            while (process.waitForReadyRead(30000)) {
                output.append(process.readAllStandardOutput());
            }

            return QString(output);
        }

        return QString();
    }

}

void Extensions::InstallExtension::on_btnCancel_clicked()
{
    reject();
}

void Extensions::InstallExtension::on_btnInstall_clicked()
{
    // FIXME Show progress bar

    if (m_runtime == "nodejs") {
        if (installNodejsExtension(m_extensionFilename)) {
            accept();
        }
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Unsupported runtime: %1").arg(m_runtime));
        return;
    }

}
