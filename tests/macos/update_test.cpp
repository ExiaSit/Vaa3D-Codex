#include "../../v3d_main/v3d/mac_release_updater.cpp"
#include <cassert>
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    const QJsonDocument releases = QJsonDocument::fromJson(R"([
      {"tag_name":"local-059","assets":[{"name":"Vaa3D-macOS26-arm64-local-059.zip"}]},
      {"tag_name":"local-060","draft":true,"assets":[{"name":"Vaa3D-macOS26-arm64-local-060.zip"}]},
      {"tag_name":"local-061","assets":[{"name":"Windows.zip"}]},
      {"tag_name":"local-057","assets":[{"name":"Vaa3D-macOS26-arm64-local-057.zip"}]}
    ])");
    assert(chooseRelease(releases, "0.58.0").tag == "local-059");
    assert(chooseRelease(releases, "0.59.0").asset.isEmpty());
    assert(chooseRelease(releases, "0.62.0").asset.isEmpty());
    QTemporaryDir temp;
    const QString path = temp.filePath("test.zip");
    QFile file(path); assert(file.open(QIODevice::WriteOnly)); file.write("abc"); file.close();
    const QString sha = "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    assert(matchesDigest(path, sha));
    assert(!matchesDigest(path, ""));
    assert(file.open(QIODevice::Append)); file.write("corrupted"); file.close();
    assert(!matchesDigest(path, sha));
    QTimer::singleShot(100, [] {
        for (QWidget *widget : QApplication::topLevelWidgets())
            if (auto progress = qobject_cast<QProgressDialog *>(widget))
                QMetaObject::invokeMethod(progress, "canceled", Qt::DirectConnection);
    });
    QByteArray output; QString error;
    assert(!execute(nullptr, "/bin/sleep", {"5"}, output, error, "Cancellation test"));
    puts("MAC_UPDATE_TEST_PASS: release selection, no downgrade, checksum, corruption, cancel");
}
