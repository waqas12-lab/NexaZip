
#include "MainWindow.h"
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("NexaZip");
    setWindowIcon(QIcon(":/icons/app.svg"));
    setWindowFlags(Qt::Window);
    setAcceptDrops(true);
    resize(1180, 720);
    setMinimumSize(920, 600);
    buildUi();
}

void MainWindow::buildUi() {
    auto fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New Archive", this, &MainWindow::newArchive);
    fileMenu->addAction("Open Archive", this, &MainWindow::openArchive);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", this, &QWidget::close);
    menuBar()->addMenu("&Commands");
    menuBar()->addMenu("&Tools");
    menuBar()->addMenu("&Favorites");
    auto optionsMenu = menuBar()->addMenu("&Options");
    optionsMenu->addAction("Toggle Dark/Light Mode", this, &MainWindow::toggleTheme);
    menuBar()->addMenu("&Help");

    auto central = new QWidget;
    central->setObjectName("root");
    setCentralWidget(central);
    auto rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(buildToolbar());

    auto pathBar = new QWidget; pathBar->setObjectName("pathBar");
    auto pathLayout = new QHBoxLayout(pathBar);
    pathLayout->setContentsMargins(12,8,12,8);
    auto icon = new QLabel; icon->setPixmap(QIcon(":/icons/app.svg").pixmap(24,24));
    m_pathLabel = new QLabel("NexaZip - Open or create a ZIP archive"); m_pathLabel->setObjectName("pathLabel");
    m_search = new QLineEdit; m_search->setPlaceholderText("Search"); m_search->setMaximumWidth(260);
    connect(m_search, &QLineEdit::textChanged, this, [this]{ showFolder(currentFolder()); });
    pathLayout->addWidget(icon); pathLayout->addWidget(m_pathLabel,1); pathLayout->addWidget(m_search);
    rootLayout->addWidget(pathBar);

    auto splitter = new QSplitter(Qt::Horizontal);
    m_tree = new QTreeWidget; m_tree->setHeaderLabel("Folders"); m_tree->setMinimumWidth(230); m_tree->setMaximumWidth(360);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::treeSelectionChanged);
    m_table = new QTableWidget(0,5);
    m_table->setHorizontalHeaderLabels({"Name","Size","Packed","Type","Modified"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &MainWindow::tableDoubleClicked);
    splitter->addWidget(m_tree); splitter->addWidget(m_table); splitter->setStretchFactor(0,1); splitter->setStretchFactor(1,4);
    rootLayout->addWidget(splitter,1);

    auto bottom = new QWidget; bottom->setObjectName("statusPanel");
    auto bottomLayout = new QHBoxLayout(bottom); bottomLayout->setContentsMargins(12,6,12,6);
    m_statusLeft = new QLabel("Ready"); m_statusRight = new QLabel("0 items");
    m_progress = new QProgressBar; m_progress->setMaximumWidth(180); m_progress->setRange(0,100); m_progress->setValue(0);
    bottomLayout->addWidget(m_statusLeft); bottomLayout->addStretch(); bottomLayout->addWidget(m_progress); bottomLayout->addWidget(m_statusRight);
    rootLayout->addWidget(bottom);
    rebuildTree();
    showFolder("");
}

QWidget* MainWindow::buildToolbar() {
    auto bar = new QWidget; bar->setObjectName("toolbar");
    auto layout = new QHBoxLayout(bar); layout->setContentsMargins(12,10,12,10); layout->setSpacing(10);
    auto newBtn = toolButton("➕","New");
    auto openBtn = toolButton("📂","Open");
    auto addFilesBtn = toolButton("📄","Add Files");
    auto addFolderBtn = toolButton("📁","Add Folder");
    auto createBtn = toolButton("📦","Create");
    auto extractBtn = toolButton("⬆","Extract");
    auto testBtn = toolButton("✅","Test");
    auto deleteBtn = toolButton("🗑","Delete");
    auto findBtn = toolButton("🔍","Find");
    auto infoBtn = toolButton("ℹ","Info");
    auto themeBtn = toolButton("🌙","Theme");
    connect(newBtn,&QPushButton::clicked,this,&MainWindow::newArchive);
    connect(openBtn,&QPushButton::clicked,this,&MainWindow::openArchive);
    connect(addFilesBtn,&QPushButton::clicked,this,&MainWindow::addFiles);
    connect(addFolderBtn,&QPushButton::clicked,this,&MainWindow::addFolder);
    connect(createBtn,&QPushButton::clicked,this,&MainWindow::createArchive);
    connect(extractBtn,&QPushButton::clicked,this,&MainWindow::extractArchive);
    connect(testBtn,&QPushButton::clicked,this,&MainWindow::testArchive);
    connect(deleteBtn,&QPushButton::clicked,this,&MainWindow::deleteSelected);
    connect(findBtn,&QPushButton::clicked,this,&MainWindow::findInArchive);
    connect(infoBtn,&QPushButton::clicked,this,&MainWindow::showInfo);
    connect(themeBtn,&QPushButton::clicked,this,&MainWindow::toggleTheme);
    for (auto* b : {newBtn,openBtn,addFilesBtn,addFolderBtn,createBtn,extractBtn,testBtn,deleteBtn,findBtn,infoBtn,themeBtn}) layout->addWidget(b);
    layout->addStretch();
    return bar;
}

QPushButton* MainWindow::toolButton(const QString& icon, const QString& text) {
    auto b = new QPushButton(icon + "\n" + text);
    b->setObjectName("toolButton");
    b->setMinimumSize(82,68);
    return b;
}

void MainWindow::applyTheme(bool dark) {
    QFile f(dark ? ":/themes/dark.qss" : ":/themes/light.qss");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    m_darkTheme = dark;
    if (m_statusLeft) m_statusLeft->setText(dark ? "Dark mode enabled" : "Light mode enabled");
}
void MainWindow::toggleTheme() { applyTheme(!m_darkTheme); }

void MainWindow::newArchive() {
    m_archivePath.clear(); m_entries.clear(); m_pendingInputs.clear(); m_search->clear();
    setWindowTitle("NexaZip - New Archive");
    m_pathLabel->setText("New Archive - Add files or folders");
    rebuildTree(); refreshPendingView();
}

void MainWindow::openArchive() {
    QString file = QFileDialog::getOpenFileName(this, "Open ZIP Archive", defaultDownloadsPath(), "ZIP Archive (*.zip)");
    if (!file.isEmpty()) loadArchive(file);
}

void MainWindow::loadArchive(const QString& archivePath) {
    m_progress->setRange(0,0); m_statusLeft->setText("Opening archive...");
    [[maybe_unused]] auto future = QtConcurrent::run([this, archivePath]{
        QString error; auto entries = ZipEngine::listZip(archivePath, &error);
        QMetaObject::invokeMethod(this, [this, archivePath, entries, error]{
            m_progress->setRange(0,100); m_progress->setValue(100);
            if (!error.isEmpty() && entries.isEmpty()) { QMessageBox::critical(this,"Open ZIP Failed",error); m_statusLeft->setText("Failed"); return; }
            m_archivePath = archivePath; m_entries = entries; m_pendingInputs.clear(); m_search->clear();
            QFileInfo info(archivePath);
            setWindowTitle("NexaZip - " + info.fileName());
            m_pathLabel->setText("📦 " + info.fileName());
            rebuildTree(); showFolder(""); m_statusLeft->setText("Archive opened");
        });
    });
}

void MainWindow::addFiles() {
    const auto files = QFileDialog::getOpenFileNames(this, "Add Files");
    for (const auto& f : files) addPendingPath(f);
    showFolder(currentFolder());
}
void MainWindow::addFolder() {
    const auto folder = QFileDialog::getExistingDirectory(this, "Add Folder");
    if (!folder.isEmpty()) addPendingPath(folder);
    showFolder(currentFolder());
}
void MainWindow::addPendingPath(const QString& path) {
    QFileInfo info(path);
    if (!info.exists()) return;
    if (!m_pendingInputs.contains(path)) m_pendingInputs << path;
    if (!m_archivePath.isEmpty()) {
        setWindowTitle("NexaZip - " + QFileInfo(m_archivePath).fileName() + " *");
        m_pathLabel->setText("📦 " + QFileInfo(m_archivePath).fileName() + " • " + QString::number(m_pendingInputs.size()) + " new item(s) staged");
        m_statusLeft->setText("New item staged. Press Create to update ZIP.");
    } else {
        setWindowTitle("NexaZip - New Archive");
        m_pathLabel->setText("New Archive - selected items");
    }
}

void MainWindow::addPendingDisplayRow(const QString& path, int depth, qint64* total) {
    QFileInfo info(path);
    if (!info.exists()) return;
    bool isFolder = info.isDir();
    qint64 size = isFolder ? 0 : info.size();
    if (total) *total += isFolder ? folderSize(path) : size;
    int row = m_table->rowCount();
    m_table->insertRow(row);
    QString indent(depth * 4, ' ');
    QString prefix = isFolder ? "📁 " : "📄 ";
    auto nameItem = new QTableWidgetItem((depth == 0 ? "" : indent + "↳ ") + prefix + info.fileName());
    nameItem->setData(Qt::UserRole, path);
    nameItem->setData(Qt::UserRole + 1, isFolder);
    m_table->setItem(row,0,nameItem);
    m_table->setItem(row,1,new QTableWidgetItem(isFolder ? "" : humanSize(size)));
    m_table->setItem(row,2,new QTableWidgetItem(depth == 0 ? "New" : ""));
    m_table->setItem(row,3,new QTableWidgetItem(isFolder ? "Folder" : "File"));
    m_table->setItem(row,4,new QTableWidgetItem(info.lastModified().toString("yyyy-MM-dd HH:mm")));
}

void MainWindow::refreshPendingView() {
    rebuildTree();
    m_table->setSortingEnabled(false); m_table->setRowCount(0);
    qint64 total = 0;
    for (const QString& path : m_pendingInputs) {
        addPendingDisplayRow(path, 0, &total);
        QFileInfo info(path);
        if (info.isDir()) {
            QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDirIterator::Subdirectories);
            QDir rootDir(path);
            int shown = 0;
            while (it.hasNext() && shown < 300) {
                QString current = it.next();
                QString rel = rootDir.relativeFilePath(current);
                int depth = rel.count('/') + 1;
                addPendingDisplayRow(current, depth, nullptr);
                shown++;
            }
            if (shown >= 300) {
                int row = m_table->rowCount();
                m_table->insertRow(row);
                m_table->setItem(row,0,new QTableWidgetItem("    … more files"));
                m_table->setItem(row,1,new QTableWidgetItem(""));
                m_table->setItem(row,2,new QTableWidgetItem(""));
                m_table->setItem(row,3,new QTableWidgetItem("Info"));
                m_table->setItem(row,4,new QTableWidgetItem(""));
            }
        }
    }
    m_table->setSortingEnabled(true);
    m_statusLeft->setText("Selected items ready");
    m_statusRight->setText(QString("%1 item(s) • %2").arg(m_pendingInputs.size()).arg(humanSize(total)));
}

void MainWindow::createArchive() {
    if (!m_archivePath.isEmpty() && !m_pendingInputs.isEmpty()) { updateOpenArchiveWithPending(); return; }
    if (m_pendingInputs.isEmpty()) { QMessageBox::information(this,"No Files","Add files or folders first."); return; }
    QString output = QFileDialog::getSaveFileName(this,"Save ZIP Archive",QDir(defaultDownloadsPath()).filePath("Archive.zip"),"ZIP Archive (*.zip)");
    if (output.isEmpty()) return;
    if (!output.endsWith(".zip", Qt::CaseInsensitive)) output += ".zip";
    m_progress->setRange(0,0); m_statusLeft->setText("Creating archive...");
    QStringList inputs = m_pendingInputs;
    [[maybe_unused]] auto future = QtConcurrent::run([this, inputs, output]{
        QString error; bool ok = ZipEngine::createZip(inputs, output, &error);
        QMetaObject::invokeMethod(this, [this, ok, output, error]{
            m_progress->setRange(0,100); m_progress->setValue(ok ? 100 : 0);
            if (ok) { QMessageBox::information(this,"Done","Archive created:\n" + output); loadArchive(output); }
            else QMessageBox::critical(this,"Create ZIP Failed",error);
        });
    });
}

void MainWindow::updateOpenArchiveWithPending() {
    if (m_archivePath.isEmpty() || m_pendingInputs.isEmpty()) return;
    if (QMessageBox::question(this,"Update ZIP Archive","Add selected files/folders into the currently opened ZIP archive?", QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;
    QString tempRoot = QDir::temp().filePath("NexaZip_Update_" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    QString extractRoot = QDir(tempRoot).filePath("content");
    QDir().mkpath(extractRoot);
    QString archivePath = m_archivePath;
    QStringList additions = m_pendingInputs;
    m_progress->setRange(0,0); m_statusLeft->setText("Updating archive...");
    [[maybe_unused]] auto future = QtConcurrent::run([this, archivePath, additions, tempRoot, extractRoot]{
        QString error; bool ok = ZipEngine::extractZip(archivePath, extractRoot, &error);
        if (ok) {
            for (const QString& source : additions) {
                QFileInfo info(source);
                QString dest = QDir(extractRoot).filePath(info.fileName());
                if (QFileInfo(dest).exists()) {
                    if (QFileInfo(dest).isDir()) QDir(dest).removeRecursively(); else QFile::remove(dest);
                }
                if (!copyPathRecursive(source, dest, &error)) { ok = false; break; }
            }
        }
        QString tempZip = QDir(tempRoot).filePath("updated.zip");
        if (ok) {
            QStringList children;
            for (const QFileInfo& child : QDir(extractRoot).entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden))
                children << child.absoluteFilePath();
            ok = ZipEngine::createZip(children, tempZip, &error);
        }
        if (ok) { QFile::remove(archivePath); ok = QFile::copy(tempZip, archivePath); if (!ok) error = "Could not replace original archive."; }
        QDir(tempRoot).removeRecursively();
        QMetaObject::invokeMethod(this, [this, ok, archivePath, error]{
            m_progress->setRange(0,100); m_progress->setValue(ok ? 100 : 0);
            if (ok) { m_pendingInputs.clear(); m_statusLeft->setText("Archive updated"); loadArchive(archivePath); QMessageBox::information(this,"Done","New files/folders added to ZIP archive."); }
            else { m_statusLeft->setText("Update failed"); QMessageBox::critical(this,"Update ZIP Failed",error); }
        });
    });
}

void MainWindow::extractArchive() {
    if (m_archivePath.isEmpty()) { QMessageBox::information(this,"No Archive","Open a ZIP archive first."); return; }
    QString destination = QFileDialog::getExistingDirectory(this,"Extract To",defaultDownloadsPath());
    if (destination.isEmpty()) return;
    m_progress->setRange(0,0); m_statusLeft->setText("Extracting...");
    QString archivePath = m_archivePath;
    [[maybe_unused]] auto future = QtConcurrent::run([this, archivePath, destination]{
        QString error; bool ok = ZipEngine::extractZip(archivePath, destination, &error);
        QMetaObject::invokeMethod(this, [this, ok, error, destination]{
            m_progress->setRange(0,100); m_progress->setValue(ok ? 100 : 0);
            if (ok) { m_statusLeft->setText("Extraction completed"); QDesktopServices::openUrl(QUrl::fromLocalFile(destination)); QMessageBox::information(this,"Done","Archive extracted to:\n" + destination); }
            else { m_statusLeft->setText("Extraction failed"); QMessageBox::critical(this,"Extraction Failed",error); }
        });
    });
}

void MainWindow::testArchive() {
    if (m_archivePath.isEmpty()) { QMessageBox::information(this,"No Archive","Open a ZIP archive first."); return; }
    QString archivePath = m_archivePath;
    m_progress->setRange(0,0); m_statusLeft->setText("Testing archive...");
    [[maybe_unused]] auto future = QtConcurrent::run([this, archivePath]{
        QString error; bool ok = ZipEngine::testZip(archivePath, &error);
        QMetaObject::invokeMethod(this, [this, ok, error]{
            m_progress->setRange(0,100); m_progress->setValue(ok ? 100 : 0);
            if (ok) { m_statusLeft->setText("ZIP test passed"); QMessageBox::information(this,"Test ZIP","No errors found."); }
            else { m_statusLeft->setText("ZIP test failed"); QMessageBox::critical(this,"Test Failed",error); }
        });
    });
}

void MainWindow::deleteSelected() {
    QList<int> rows;
    for (const QModelIndex& idx : m_table->selectionModel()->selectedRows()) {
        rows << idx.row();
    }

    if (rows.isEmpty()) {
        m_statusLeft->setText("No selected item");
        return;
    }

    QStringList stagedToRemove;
    for (int row : rows) {
        auto item = m_table->item(row, 0);
        if (!item) continue;

        const QString path = item->data(Qt::UserRole).toString();
        if (m_pendingInputs.contains(path)) {
            stagedToRemove << path;
        }
    }

    if (!stagedToRemove.isEmpty()) {
        for (const QString& path : stagedToRemove) {
            m_pendingInputs.removeAll(path);
        }

        if (m_archivePath.isEmpty()) {
            refreshPendingView();
        } else {
            showFolder(currentFolder());
        }

        m_statusLeft->setText("Selected staged item removed");
        return;
    }

    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) {
        if (row >= 0 && row < m_table->rowCount()) {
            m_table->removeRow(row);
        }
    }

    m_statusLeft->setText("Removed from current view");
}

void MainWindow::findInArchive() { m_search->setFocus(); }

void MainWindow::showInfo() {
    QMessageBox::information(this, "NexaZip", "NexaZip 3.2\nStandalone ZIP archive manager\nZIP Deflate support\nDark/Light mode\nNative OS title bar");
}

void MainWindow::treeSelectionChanged() { if (!m_updatingTree) showFolder(currentFolder()); }

void MainWindow::tableDoubleClicked(int row, int) {
    auto item = m_table->item(row,0);
    if (!item) return;
    bool isDir = item->data(Qt::UserRole + 1).toBool();
    QString target = item->data(Qt::UserRole).toString();
    if (!isDir || target.isEmpty() || m_archivePath.isEmpty()) return;
    QList<QTreeWidgetItem*> all = m_tree->findItems("*", Qt::MatchWildcard | Qt::MatchRecursive);
    for (auto* it : all) {
        if (it->data(0, Qt::UserRole).toString() == target) { m_tree->setCurrentItem(it); return; }
    }
}

void MainWindow::rebuildTree() {
    m_updatingTree = true;
    m_tree->clear();
    auto rootItem = new QTreeWidgetItem(QStringList() << (m_archivePath.isEmpty() ? "New Archive" : QFileInfo(m_archivePath).fileName()));
    rootItem->setData(0, Qt::UserRole, "");
    rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    m_tree->addTopLevelItem(rootItem);
    QMap<QString,QTreeWidgetItem*> nodes; nodes[""] = rootItem;
    auto ensurePath = [&](QString path) {
        if (path.endsWith('/')) path.chop(1);
        if (path.isEmpty() || path == ".") return;
        QString current; QTreeWidgetItem* parent = rootItem;
        for (const QString& part : path.split('/', Qt::SkipEmptyParts)) {
            current = current.isEmpty() ? part : current + "/" + part;
            if (!nodes.contains(current)) {
                auto child = new QTreeWidgetItem(QStringList() << part);
                child->setData(0, Qt::UserRole, current);
                child->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
                parent->addChild(child); nodes[current] = child;
            }
            parent = nodes[current];
        }
    };
    for (const auto& e : m_entries) ensurePath(e.isDir ? e.path : e.folder);
    rootItem->setExpanded(true);
    m_tree->setCurrentItem(rootItem);
    m_updatingTree = false;
}

QString MainWindow::currentFolder() const {
    auto item = m_tree->currentItem();
    return item ? item->data(0, Qt::UserRole).toString() : "";
}

void MainWindow::showFolder(const QString& folder) {
    if (m_archivePath.isEmpty()) { refreshPendingView(); return; }
    m_table->setSortingEnabled(false); m_table->setRowCount(0);
    QString query = m_search->text().trimmed().toLower();
    int count = 0; qint64 total = 0; QSet<QString> shownFolders;
    auto addRow = [&](QString display, QString path, bool isDir, qint64 size, qint64 packed, QString type, QDateTime mod, bool staged=false) {
        if (!query.isEmpty() && !display.toLower().contains(query)) return;
        int row = m_table->rowCount(); m_table->insertRow(row);
        auto nameItem = new QTableWidgetItem((staged ? "➕ " : "") + QString(isDir ? "📁 " : "📄 ") + display);
        nameItem->setData(Qt::UserRole, path); nameItem->setData(Qt::UserRole+1, isDir);
        m_table->setItem(row,0,nameItem);
        m_table->setItem(row,1,new QTableWidgetItem(isDir ? "" : humanSize(size)));
        m_table->setItem(row,2,new QTableWidgetItem(staged ? "New" : (isDir ? "" : humanSize(packed))));
        m_table->setItem(row,3,new QTableWidgetItem(type));
        m_table->setItem(row,4,new QTableWidgetItem(mod.isValid() ? mod.toString("yyyy-MM-dd HH:mm") : ""));
        count++; total += size;
    };
    for (const auto& e : m_entries) {
        QString p = e.path; if (p.endsWith('/')) p.chop(1); if (p.isEmpty()) continue;
        QString rel = p;
        if (!folder.isEmpty()) { QString prefix = folder + "/"; if (!p.startsWith(prefix)) continue; rel = p.mid(prefix.size()); }
        auto parts = rel.split('/', Qt::SkipEmptyParts);
        if (parts.size() > 1) {
            QString childName = parts.first(); QString childPath = folder.isEmpty() ? childName : folder + "/" + childName;
            if (!shownFolders.contains(childPath)) { shownFolders.insert(childPath); addRow(childName, childPath, true, 0, 0, "Folder", QDateTime()); }
        }
    }
    for (const auto& e : m_entries) {
        QString p = e.path; if (p.endsWith('/')) p.chop(1);
        QString parent = e.isDir ? (QFileInfo(p).path() == "." ? "" : QFileInfo(p).path()) : e.folder;
        if (parent != folder) continue;
        QString itemPath = folder.isEmpty() ? e.name : folder + "/" + e.name;
        if (e.isDir && shownFolders.contains(itemPath)) continue;
        addRow(e.name, itemPath, e.isDir, e.size, e.packedSize, e.type, e.modified);
    }
    if (folder.isEmpty()) {
        for (const QString& p : m_pendingInputs) {
            QFileInfo info(p); addRow(info.fileName(), p, info.isDir(), info.isDir()?folderSize(p):info.size(), 0, info.isDir()?"Folder":"File", info.lastModified(), true);
        }
    }
    m_table->setSortingEnabled(true);
    m_statusRight->setText(QString("%1 items • %2").arg(count).arg(humanSize(total)));
    m_pathLabel->setText("📦 " + QFileInfo(m_archivePath).fileName() + (folder.isEmpty() ? "" : " / " + folder));
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) { if (event->mimeData()->hasUrls()) event->acceptProposedAction(); }
void MainWindow::dropEvent(QDropEvent* event) {
    for (const QUrl& url : event->mimeData()->urls()) if (url.isLocalFile()) {
        QString p = url.toLocalFile();
        if (p.endsWith(".zip", Qt::CaseInsensitive) && m_pendingInputs.isEmpty()) loadArchive(p);
        else addPendingPath(p);
    }
    showFolder(currentFolder());
}

bool MainWindow::copyPathRecursive(const QString& source, const QString& destination, QString* error) const {
    QFileInfo src(source);
    if (!src.exists()) { if (error) *error = "Source does not exist: " + source; return false; }
    if (src.isFile()) {
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QFile::remove(destination);
        if (!QFile::copy(source, destination)) { if (error) *error = "Could not copy file: " + source; return false; }
        return true;
    }
    if (src.isDir()) {
        QDir().mkpath(destination);
        QDirIterator it(source, QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::Hidden, QDirIterator::Subdirectories);
        QDir root(source);
        while (it.hasNext()) {
            QString cur = it.next(); QFileInfo ci(cur);
            QString target = QDir(destination).filePath(root.relativeFilePath(cur));
            if (ci.isDir()) QDir().mkpath(target);
            else { QDir().mkpath(QFileInfo(target).absolutePath()); QFile::remove(target); if (!QFile::copy(cur, target)) { if (error) *error = "Could not copy file: " + cur; return false; } }
        }
    }
    return true;
}

QString MainWindow::defaultDownloadsPath() const {
    QString p = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return p.isEmpty() ? QDir::homePath() : p;
}
QString MainWindow::humanSize(qint64 bytes) const {
    static const char* units[] = {"B","KB","MB","GB","TB"};
    double size = bytes; int unit = 0;
    while (size >= 1024.0 && unit < 4) { size /= 1024.0; unit++; }
    return QString::number(size, 'f', unit == 0 ? 0 : 1) + " " + units[unit];
}
qint64 MainWindow::folderSize(const QString& path) const {
    qint64 total = 0;
    QDirIterator it(path, QDir::Files|QDir::NoSymLinks|QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) { it.next(); total += it.fileInfo().size(); }
    return total;
}
