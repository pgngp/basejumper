#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "globalview.h"
#include "intermediateview.h"
#include "search.h"
#include "zoomWidget.h"
#include "readsNavWidget.h"
#include "snpNavWidget.h"
#include "annotationNavWidget.h"
#include "database.h"
#include "geneOverlapExporter.h"


class QAction;
class QLabel;
class MapArea;
class Parser;
class QSlider;
class QHBoxLayout;
class QVBoxLayout;
class QGroupBox;
class QProgressBar;
class QToolButton;
class QPushButton;
class QLineEdit;
class QScrollArea;
class QProgressDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();
    int getSliderMaxValue() const;
    int getNumSelectedFiles() const;

    Parser *parser;
    int selectedFilesNum;
    static QString APPLICATION_ORGANIZATION;
    static QString APPLICATION_NAME;
    static QString VERSION_NUMBER;

    // names of the variables that hold application-level settings
    static QString SETTINGS_GEOMETRY;
    static QString SETTINGS_RECENT_FILES;
    static QString SETTINGS_SNP_THRESHOLD;
    static QString SETTINGS_OPEN_FILE_DIRECTORY;
    static QString SETTINGS_OPEN_REF_FILE_DIRECTORY;

	public slots:
	void enableOpenRefAction();
	void disableOpenRefAction();
	void enableBookmarkAction();
	void open(const QStringList &);
	void setProgressDialogValue(const int);
	void setProgressDialogMax(const int);
	void setProgressDialogLabel(const QString &);
	void enableSearchAction();
	void disableSearchAction();
	void enableCoverageAction();
	void disableCoverageAction();
	void enableExportGeneOverlapAction();
	void disableExportGeneOverlapAction();
	void modifySize();
	void onEscape();

protected:
    void closeEvent(QCloseEvent *event);


private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    void updateRecentFileActions();
    QString strippedName(const QString &fullFileName);
    void parseFile();
    void getAllBookmarkNames(QList<QString> &);
    void loadStyleSheet(const QString &);

    const QString defaultWindowTitle;

    QLabel *locationLabel;
    QLabel *searchLabel;
    QProgressBar *progressBar;
    QProgressDialog *progressDialog;
    QStringList selectedFiles;
    QStringList recentFiles;
    QString curFile;

    enum { MaxRecentFiles = 5 };
    QAction *recentFileActions[MaxRecentFiles];
    QAction *separatorAction;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *helpMenu;
    QMenu *bookmarkMenu;
    QToolBar *fileToolBar;
    QToolBar *whatsThisToolBar;
    QAction *openAction;
    QAction *openRefAction;
    QAction *exportAction;
    QAction *exportGeneOverlapAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *whatsThisAction;
    QAction *snpThresholdAction;
    QAction *searchAction;
    QAction *bookmarkAction;
    QAction *coverageAction;
    QVector<QAction *> bookmarkVector;

    MapArea *mapArea;
    QHBoxLayout *hBoxLayout;
    QWidget *centralWidget;
    QVBoxLayout *rightVBoxLayout;
    QVBoxLayout *leftVBoxLayout;
    QGroupBox *globalViewGroupBox;
    ZoomWidget *zoomWidget;
    IntermediateView *intermediateView;
    GlobalView *globalView;
    Search *search;
    ReadsNavWidget *readsNavWidget;
    SnpNavWidget *snpNavWidget;
    AnnotationNavWidget *annotNavWidget;
    Database *db;
    QMessageBox *coverageMessageBox;
    GeneOverlapExporter *geneExporter;

    private slots:
    void open();
    void openRef();
    void about();
    void openRecentFile();
    void showBookmark();
    void addBookmarkAction(const QString &);
    void addRecentFile(const QString &);
    void getSnpThresholdInput();
    void enterWhatsThisMode();
    void showCoverage();
    void finishParsing();

    signals:
    void messageChanged(const QString &);
    void newFileOpened();
    void bookmarkClicked(const QString &);
    void resetZoom();
};

#endif
