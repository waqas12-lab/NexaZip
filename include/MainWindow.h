#pragma once
#include <QtWidgets>
#include <QtConcurrent>
#include "ZipEngine.h"

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
private slots:
    void newArchive();
    void openArchive();
    void addFiles();
    void addFolder();
    void createArchive();
    void updateOpenArchiveWithPending();
    void extractArchive();
    void testArchive();
    void deleteSelected();
    void findInArchive();
    void showInfo();
    void toggleTheme();
    void treeSelectionChanged();
    void tableDoubleClicked(int row, int column);
private:
    void buildUi();
    QWidget* buildToolbar();
    QPushButton* toolButton(const QString& icon, const QString& text);
    void applyTheme(bool dark);
    void loadArchive(const QString& archivePath);
    void rebuildTree();
    void showFolder(const QString& folder);
    QString currentFolder() const;
    void addPendingPath(const QString& path);
    void refreshPendingView();
    bool copyPathRecursive(const QString& source, const QString& destination, QString* error) const;
    QString defaultDownloadsPath() const;
    QString humanSize(qint64 bytes) const;
    qint64 folderSize(const QString& path) const;
    void addPendingDisplayRow(const QString& path, int depth, qint64* total);

    QString m_archivePath;
    QStringList m_pendingInputs;
    QVector<ZipEntryInfo> m_entries;
    QTreeWidget* m_tree = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_pathLabel = nullptr;
    QLabel* m_statusLeft = nullptr;
    QLabel* m_statusRight = nullptr;
    QLineEdit* m_search = nullptr;
    QProgressBar* m_progress = nullptr;
    bool m_darkTheme = false;
    bool m_updatingTree = false;
};
