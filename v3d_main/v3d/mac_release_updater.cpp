#include "mac_release_updater.h"
#include <QtWidgets>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QNetworkProxyFactory>
#include <QNetworkProxyQuery>
#include <QTemporaryDir>
#include <QVersionNumber>

namespace {
// Modal progress keeps the event loop responsive and prevents overlapping updates.
bool execute(QWidget *parent, const QString &program, const QStringList &args,
             QByteArray &output, QString &error, const QString &label)
{
    QProcess process;
    QProgressDialog progress(label, QObject::tr("Cancel"), 0, 0, parent);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    QEventLoop loop;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&process, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    QObject::connect(&progress, &QProgressDialog::canceled, &process, &QProcess::kill);
    QObject::connect(&deadline, &QTimer::timeout, &process, &QProcess::kill);
    process.start(program, args);
    deadline.start(300000);
    loop.exec();
    output = process.readAllStandardOutput();
    error = QString::fromUtf8(process.readAllStandardError());
    if (process.error() == QProcess::FailedToStart)
        error = process.errorString();
    const bool ok = !progress.wasCanceled() && deadline.isActive()
        && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (!ok && error.isEmpty()) error = QObject::tr("Operation canceled or timed out.");
    return ok;
}

QString plistValue(const QString &app, const QString &key)
{
    QProcess p;
    p.start("/usr/libexec/PlistBuddy", {"-c", "Print :" + key, app + "/Contents/Info.plist"});
    if (!p.waitForFinished(5000) || p.exitCode() != 0) return QString();
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

bool download(QWidget *parent, const QString &url, const QString &file, QString &error)
{
    QByteArray output;
    QStringList args = {"--fail", "--location", "--silent", "--show-error",
        "--proto", "=https", "--proto-redir", "=https", "--connect-timeout", "20",
        "--max-time", "280", "--user-agent", "Vaa3D-Mac-Updater", "--output", file};
    const auto proxies = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(QUrl(url)));
    for (const QNetworkProxy &proxy : proxies) {
        if (proxy.type() == QNetworkProxy::NoProxy) break;
        if (proxy.type() == QNetworkProxy::HttpProxy || proxy.type() == QNetworkProxy::Socks5Proxy) {
            QUrl address;
            address.setScheme(proxy.type() == QNetworkProxy::HttpProxy ? "http" : "socks5h");
            address.setHost(proxy.hostName()); address.setPort(proxy.port());
            args << "--proxy" << address.toString();
            break;
        }
    }
    args << url;
    return execute(parent, "/usr/bin/curl", args, output, error, QObject::tr("Downloading update…"));
}

QByteArray readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();
    return f.readAll();
}
struct ReleaseChoice {
    QVersionNumber version;
    QJsonObject asset;
    QString tag;
    bool prerelease;
};

ReleaseChoice chooseRelease(const QJsonDocument &document, const QString &current)
{
    QVersionNumber best = QVersionNumber::fromString(current);
    QJsonObject chosen;
    QString tag;
    bool prerelease = false;
    const QRegularExpression releaseTag("^local-([0-9]+)$");
    for (const QJsonValue &value : document.array()) {
        const QJsonObject release = value.toObject();
        if (release.value("draft").toBool()) continue;
        const auto match = releaseTag.match(release.value("tag_name").toString());
        if (!match.hasMatch()) continue;
        const QVersionNumber version(0, match.captured(1).toInt(), 0);
        if (version <= best) continue;
        for (const QJsonValue &asset : release.value("assets").toArray()) {
            const QJsonObject candidate = asset.toObject();
            const QString expected = "Vaa3D-macOS26-arm64-" + match.captured(0) + ".zip";
            if (candidate.value("name").toString() != expected) continue;
            chosen = candidate; best = version; tag = match.captured(0);
            prerelease = release.value("prerelease").toBool();
        }
    }
    return {best, chosen, tag, prerelease};
}

bool matchesDigest(const QString &path, const QString &digest)
{
    if (!QRegularExpression("^sha256:[0-9a-f]{64}$").match(digest).hasMatch()) return false;
    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return file.open(QIODevice::ReadOnly) && hash.addData(&file)
        && QString::fromLatin1(hash.result().toHex()) == digest.mid(7);
}

}

void checkMacReleaseUpdate(QWidget *parent)
{
    static bool active = false;
    if (active) return;
    QScopedValueRollback<bool> guard(active, true);
    const QString app = QDir(QCoreApplication::applicationDirPath() + "/../..").canonicalPath();
    const QString current = plistValue(app, "CFBundleShortVersionString");
    const QString identity = plistValue(app, "CFBundleIdentifier");
    auto fail = [parent](const QString &message) {
        QMessageBox::warning(parent, QObject::tr("Vaa3D update"), message);
    };
    if (!app.endsWith(".app") || identity != "org.exiasit.vaa3d" || current.isEmpty()) {
        fail(QObject::tr("This development build cannot install updates. Install a packaged Mac release first."));
        return;
    }
    if (QFileInfo(QFileInfo(app).absolutePath() + "/.vaa3d-install.lock").exists()) {
        fail(QObject::tr("An update is already prepared. Save your work and quit Vaa3D to finish installation."));
        return;
    }
    // Stage on the same filesystem so renames at installation are atomic.
    QTemporaryDir stage(QFileInfo(app).absolutePath() + "/.vaa3d-update-XXXXXX");
    if (!stage.isValid()) {
        fail(QObject::tr("Cannot write beside this app. Move Vaa3D to your user's Applications folder and try again."));
        return;
    }
    QString error;
    const QString api = "https://raw.githubusercontent.com/ExiaSit/Vaa3D-Codex/mac/platforms/macos/updates.json";
    if (!download(parent, api, stage.filePath("releases.json"), error)) { fail(error); return; }
    QJsonParseError parse;
    const QJsonDocument document = QJsonDocument::fromJson(readFile(stage.filePath("releases.json")), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isArray()) {
        fail(QObject::tr("Invalid response from GitHub.")); return;
    }
    const ReleaseChoice release = chooseRelease(document, current);
    const QVersionNumber best = release.version;
    const QJsonObject chosen = release.asset;
    const QString tag = release.tag;
    const bool prerelease = release.prerelease;
    if (chosen.isEmpty()) {
        QMessageBox::information(parent, QObject::tr("Vaa3D update"),
            QObject::tr("Installed version %1 is up to date for this Mac release channel.").arg(current));
        return;
    }
    const QString url = chosen.value("browser_download_url").toString();
    const QString prefix = "https://github.com/ExiaSit/Vaa3D-Codex/releases/download/" + tag + "/";
    const QString digest = chosen.value("digest").toString();
    if (!url.startsWith(prefix) || !QRegularExpression("^sha256:[0-9a-f]{64}$").match(digest).hasMatch()) {
        fail(QObject::tr("The release has no valid SHA-256 digest or download URL.")); return;
    }
    const QString message = QObject::tr("Installed: %1\nAvailable: %2%3\n\nDownload and prepare this update? Installation starts after you save your work and quit Vaa3D. The previous app will be backed up.")
        .arg(current, best.toString(), prerelease ? QObject::tr(" (pre-release)") : QString());
    if (QMessageBox::question(parent, QObject::tr("Vaa3D update"), message) != QMessageBox::Yes) return;
    const QString archive = stage.filePath("update.zip");
    if (!download(parent, url, archive, error)) { fail(error); return; }
    if (!matchesDigest(archive, digest)) {
        fail(QObject::tr("Download checksum failed. The installed app has not been changed.")); return;
    }
    QByteArray output;
    if (!execute(parent, "/usr/bin/ditto", {"-x", "-k", archive, stage.path()}, output, error,
                 QObject::tr("Unpacking update…"))) { fail(error); return; }
    const QString incoming = stage.filePath("Vaa3D.app");
    if (QFileInfo(incoming).isSymLink() || plistValue(incoming, "CFBundleIdentifier") != identity
        || QVersionNumber::fromString(plistValue(incoming, "CFBundleShortVersionString")) != best) {
        fail(QObject::tr("The downloaded application identity or version does not match.")); return;
    }
    QProcess os;
    os.start("/usr/bin/sw_vers", {"-productVersion"}); os.waitForFinished(5000);
    const auto minimum = QVersionNumber::fromString(plistValue(incoming, "LSMinimumSystemVersion"));
    if (minimum.isNull() || QVersionNumber::fromString(QString::fromUtf8(os.readAllStandardOutput())) < minimum) {
        fail(QObject::tr("This update requires macOS %1 or later.").arg(minimum.toString())); return;
    }
    if (!execute(parent, "/usr/bin/codesign", {"--verify", "--deep", "--strict", incoming}, output, error,
                 QObject::tr("Verifying application…"))) { fail(error); return; }
    const QString executable = incoming + "/Contents/MacOS/" + plistValue(incoming, "CFBundleExecutable");
    if (!execute(parent, "/usr/bin/lipo", {"-verify_arch", "arm64", executable}, output, error,
                 QObject::tr("Verifying architecture…"))) { fail(error); return; }
    const QString helper = stage.filePath("install-update.sh");
    if (!QFile::copy(app + "/Contents/Resources/install-update.sh", helper)) {
        fail(QObject::tr("The update installer is missing.")); return;
    }
    const QString backupDir = QDir::homePath() + "/Library/Application Support/Vaa3D/Updates";
    if (!QDir().mkpath(backupDir)) { fail(QObject::tr("Cannot create the update backup folder.")); return; }
    const QString backup = backupDir + "/Vaa3D-" + current + "-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmsszzz") + ".zip";
    if (!QProcess::startDetached("/bin/bash", {helper, QString::number(QCoreApplication::applicationPid()), app,
                                               stage.path(), backup})) {
        fail(QObject::tr("Could not start the update installer.")); return;
    }
    stage.setAutoRemove(false);
    QMessageBox::information(parent, QObject::tr("Update ready"),
        QObject::tr("Update downloaded and verified. Save your work and quit Vaa3D within 10 minutes. It will install and reopen automatically.\n\nBackup and installation log:\n%1").arg(backupDir));
}
