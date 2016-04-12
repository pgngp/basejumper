#include <QtGui>
#include "mainwindow.h"
#include "maparea.h"
#include "math.h"
#include "limits.h"
#include <algorithm>
#include "assert.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include "file.h"
#include "iostream"

#define	FIRST_FILE_INDEX		1
#define	SNP_THRESHOLD			30

QString MainWindow::APPLICATION_ORGANIZATION = "SJCRH";
QString MainWindow::APPLICATION_NAME = "Basejumper";
QString MainWindow::VERSION_NUMBER = "0.01";

//settings for application stored on user's machine
QString MainWindow::SETTINGS_GEOMETRY = "geometry";
QString MainWindow::SETTINGS_RECENT_FILES = "recentFiles";
QString MainWindow::SETTINGS_SNP_THRESHOLD = QString(SNP_THRESHOLD);
QString MainWindow::SETTINGS_OPEN_FILE_DIRECTORY = "openFileDirectory";
QString MainWindow::SETTINGS_OPEN_REF_FILE_DIRECTORY = ".";

/**
 * Constructor
 */
MainWindow::MainWindow()
{
	const QString defaultWindowTitle = MainWindow::APPLICATION_NAME;

	centralWidget = new QWidget;
	loadStyleSheet("mainwindow");
	connect(this, SIGNAL(messageChanged(const QString &)),
			statusBar(), SLOT(showMessage(const QString &)));
	QWidget::setWindowIcon(QIcon(":images/dna.png"));
	setWindowTitle(defaultWindowTitle);

    db = new Database;
    db->createConnection();

	/* Parser */
	parser = new Parser;
	parser->setParent(this);
	connect(parser, SIGNAL(messageChanged(const QString &)),
			statusBar(), SLOT(showMessage(const QString &)));
	connect(parser, SIGNAL(parsingFinished()),
			statusBar(), SLOT(clearMessage()));
	connect(parser, SIGNAL(annotationParsingFinished()),
			statusBar(), SLOT(clearMessage()));
	connect(parser, SIGNAL(orderParsingFinished()),
			statusBar(), SLOT(clearMessage()));
	connect(parser, SIGNAL(parsingFinished()),
			this, SLOT(enableOpenRefAction()));
	connect(parser, SIGNAL(parsingStarted()),
			this, SLOT(disableOpenRefAction()));
	connect(parser, SIGNAL(annotationLoaded()),
			this, SLOT(disableOpenRefAction()));
	connect(parser, SIGNAL(parsingStarted()),
			this, SLOT(disableSearchAction()));
	connect(parser, SIGNAL(parsingFinished()),
			this, SLOT(enableSearchAction()));
    connect(parser, SIGNAL(parsingStarted()),
    		this, SLOT(disableCoverageAction()));
    connect(parser, SIGNAL(parsingFinished()),
        	this, SLOT(enableCoverageAction()));
    //connect(parser, SIGNAL(parsingStarted()),
    //		db, SLOT(deleteAll()));
    connect(parser, SIGNAL(parsingFinished()),
    		this, SLOT(finishParsing()));

    /* Maparea or Contig View widget */
	mapArea = new MapArea(this, centralWidget);
	connect(mapArea, SIGNAL(messageChanged(const QString &)),
			statusBar(), SLOT(showMessage(const QString &)));
	//connect(parser, SIGNAL(annotationLoaded()),
	//		mapArea, SLOT(annotationLoaded()));
	connect(mapArea, SIGNAL(zoomedIn(bool)),
			this, SLOT(enableBookmarkAction()));
	connect(this, SIGNAL(bookmarkClicked(const QString &)),
			mapArea, SLOT(loadBookmark(const QString &)));
	connect(parser, SIGNAL(orderParsingFinished()),
			mapArea, SLOT(getContigOrderIdHash()));
	connect(parser, SIGNAL(parsingFinished()),
			mapArea, SLOT(getContigOrderIdHash()));
	//connect(parser, SIGNAL(parsingStarted()),
	//		mapArea, SLOT(update()));
	//connect(parser, SIGNAL(cleanWidgets()),
	//		mapArea, SLOT(initialize()));
	connect(mapArea, SIGNAL(fileChanged(const QString &)),
			this, SLOT(setWindowTitle(const QString &)));

	/* Intermediate view widget */
	intermediateView = new IntermediateView(centralWidget);
	connect(intermediateView, SIGNAL(intermediateViewClicked(const int, const int)),
			mapArea, SLOT(goToPos(const int, const int)));
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			intermediateView, SLOT(setContig(Contig *)));
	//connect(parser, SIGNAL(parsingStarted()),
	//		intermediateView, SLOT(reset()));
	connect(intermediateView, SIGNAL(messageChanged(const QString &)),
			statusBar(), SLOT(showMessage(const QString &)));
	connect(mapArea, SIGNAL(viewChanged(const Contig *)),
			intermediateView, SLOT(updateView(const Contig *)));

	/* Global view widget */
	globalView = new GlobalView(centralWidget);
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			globalView, SLOT(setContig(Contig *)));
	//connect(parser, SIGNAL(cleanWidgets()), globalView, SLOT(clean()));
	connect(globalView, SIGNAL(globalViewClicked(const int, const int)),
			mapArea, SLOT(goToPos(const int, const int)));
	connect(globalView, SIGNAL(messageChanged(const QString &)),
			statusBar(), SLOT(showMessage(const QString &)));
	connect(mapArea, SIGNAL(viewChanged(const Contig *)),
			globalView, SLOT(updateView(const Contig *)));
	//connect(parser, SIGNAL(parsingFinished()),
	//		globalView, SLOT(createContigPixmap()));
	//connect(parser, SIGNAL(orderParsingFinished()),
	//		globalView, SLOT(createContigPixmap()));

	/* Search widget */
	search = new Search(this);
	connect(parser, SIGNAL(annotationLoaded()), search, SLOT(enableTextBox()));
	connect(parser, SIGNAL(parsingStarted()), search, SLOT(disableTextBox()));
	connect(parser, SIGNAL(parsingFinished()), search, SLOT(enableTextBox()));
	connect(search, SIGNAL(textChanged(const QString &)),
			mapArea, SLOT(resetHighlighting()));
	connect(search, SIGNAL(textChanged(const QString &)),
			globalView, SLOT(resetHighlighting()));
	connect(search, SIGNAL(textChanged(const QString &)),
			intermediateView, SLOT(resetHighlighting()));
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			search, SLOT(setContig(Contig *)));
	connect(search, SIGNAL(resultFound(QMap<int, QMap<int, int> *> *)),
			globalView, SLOT(showSearchResults(QMap<int, QMap<int, int> *> *)));
	connect(search, SIGNAL(resultFound(QMap<int, QMap<int, int> *> *)),
			intermediateView, SLOT(showSearchResults(QMap<int, QMap<int, int> *> *)));
	connect(search, SIGNAL(resultFound(QMap<int, QMap<int, int> *> *)),
			mapArea, SLOT(showSearchResults(QMap<int, QMap<int, int> *> *)));
	connect(search, SIGNAL(resultFound(const int, const int)),
			mapArea, SLOT(goToPos(const int, const int)));
	//when a new file is opened, search results/positions are probably no longer valid
	connect(this, SIGNAL(newFileOpened()), search, SLOT(refreshSearch()));

	(void) new QShortcut(Qt::Key_Escape, this, SLOT(onEscape()));

	/* Vertical box-layout containing Contig view, Intermediate view,
	 * Global view, and Search widget */
	leftVBoxLayout = new QVBoxLayout;
	leftVBoxLayout->addWidget(globalView->getGroupBox(), 0, Qt::AlignBottom);
	leftVBoxLayout->addWidget(intermediateView->getGroupBox(), 0, Qt::AlignBottom);
	leftVBoxLayout->addWidget(mapArea->getGroupBox(), 10, Qt::AlignTop);
	leftVBoxLayout->addWidget(search->getGroupBox(), 0, Qt::AlignBottom);

	/* Zoom widget */
	zoomWidget = new ZoomWidget(this);
	connect(zoomWidget, SIGNAL(sliderMoved(int)),
			mapArea, SLOT(zoomSliderMoved(int)));
	connect(this, SIGNAL(resetZoom()), zoomWidget, SLOT(reset()));
	connect(mapArea, SIGNAL(zoom(const int)), zoomWidget, SLOT(zoom(const int)));
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			zoomWidget, SLOT(setContig(Contig *)));

	/* Read navigation widget */
    readsNavWidget = new ReadsNavWidget(this);
    connect(readsNavWidget, SIGNAL(goToVPos(const int)),
    		mapArea, SLOT(setVScrollbarValue(const int)));
    connect(readsNavWidget, SIGNAL(goToHPos(const int, const int)),
    		mapArea, SLOT(goToPos(const int, const int)));
    connect(mapArea, SIGNAL(zoomedIn(bool)),
    		readsNavWidget, SLOT(setEnabled(bool)));
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			readsNavWidget, SLOT(setContig(Contig *)));

	/* SNP navigation widget */
    snpNavWidget = new SnpNavWidget(this);
    connect(snpNavWidget, SIGNAL(goToVPos(const int)),
    		mapArea, SLOT(setVScrollbarValue(const int)));
    connect(snpNavWidget, SIGNAL(goToHPos(const int, const int)),
    		mapArea, SLOT(goToPos(const int, const int)));
    connect(mapArea, SIGNAL(zoomedIn(bool)),
    		snpNavWidget, SLOT(setEnabled(bool)));
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			snpNavWidget, SLOT(setContig(Contig *)));

	/* Annotation navigation widget */
	annotNavWidget = new AnnotationNavWidget(this);
    connect(annotNavWidget, SIGNAL(goToVPos(const int)),
    		mapArea, SLOT(setVScrollbarValue(const int)));
    connect(annotNavWidget, SIGNAL(goToHPos(const int, const int)),
    		mapArea, SLOT(goToPos(const int, const int)));
    connect(mapArea, SIGNAL(zoomedIn(bool)),
    		annotNavWidget, SLOT(setEnabled(bool)));
	connect(mapArea, SIGNAL(contigChanged(Contig *)),
			annotNavWidget, SLOT(setContig(Contig *)));
	connect(parser, SIGNAL(annotationLoaded()),
			annotNavWidget, SLOT(setItems()));

    /* Vertical box-layout containing Zoom widget,
     * Reads widget, and SNP widget */
	rightVBoxLayout = new QVBoxLayout;
	rightVBoxLayout->addWidget(readsNavWidget->getGroupBox());
	rightVBoxLayout->addWidget(snpNavWidget->getGroupBox());
	rightVBoxLayout->addWidget(annotNavWidget->getGroupBox());
	rightVBoxLayout->addWidget(zoomWidget->getGroupBox());
	rightVBoxLayout->addStretch(1);

	/* Horizontal box-layout */
	hBoxLayout = new QHBoxLayout;
	hBoxLayout->addLayout(leftVBoxLayout);
	hBoxLayout->addLayout(rightVBoxLayout);

	centralWidget->setLayout(hBoxLayout);
	setCentralWidget(centralWidget);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    readSettings();
    setCurrentFile("");

    /* Progress Dialog */
    progressDialog = new QProgressDialog(this, Qt::Dialog);
    progressDialog->setModal(true);
    connect(parser, SIGNAL(totalSize(int)),
    		progressDialog, SLOT(setMaximum(int)));
    connect(parser, SIGNAL(parsingProgress(int)),
    		progressDialog, SLOT(setValue(int)));
    connect(parser, SIGNAL(messageChanged(const QString &)),
    		progressDialog, SLOT(setLabelText(const QString &)));
    connect(parser, SIGNAL(parsingFinished()),
    		progressDialog, SLOT(reset()));
    connect(parser, SIGNAL(annotationParsingFinished()),
    		progressDialog, SLOT(reset()));
    connect(parser, SIGNAL(orderParsingFinished()),
    		progressDialog, SLOT(reset()));
//    connect(progressDialog, SIGNAL(canceled()),
//    		progressDialog, SLOT(close()));
    connect(progressDialog, SIGNAL(canceled()),
    		this, SLOT(close()));

    /* Disable the horizontal and vertical scrollbars */
    mapArea->getHScrollBar()->setDisabled(true);
    mapArea->getVScrollBar()->setDisabled(true);

    coverageMessageBox = new QMessageBox(this);

    /* Create gene overlap exporter object */
    geneExporter = new GeneOverlapExporter;
    connect(exportGeneOverlapAction, SIGNAL(triggered()),
    		geneExporter, SLOT(exportGenes()));
}


/**
 * Destructor
 */
MainWindow::~MainWindow()
{
	delete coverageMessageBox;
	delete locationLabel;
	delete progressBar;
	delete progressDialog;

	for (int i = 0; i < MaxRecentFiles; ++i)
	{
		if (recentFileActions[i] != NULL)
			delete recentFileActions[i];
	}

    delete openAction;
    delete openRefAction;
    delete exitAction;
    delete aboutAction;
    delete whatsThisAction;
    delete exportAction;
    delete exportGeneOverlapAction;
    delete bookmarkAction;
    delete snpThresholdAction;
    delete searchAction;
	foreach (QAction *action, bookmarkVector)
		delete action;

	delete search;
    delete readsNavWidget;
    delete snpNavWidget;
    delete annotNavWidget;
    delete zoomWidget;

	delete parser;
	delete geneExporter;

    delete intermediateView;
    delete globalView;
    delete mapArea;

    delete rightVBoxLayout;
    delete leftVBoxLayout;
    delete hBoxLayout;
    delete centralWidget;

    delete db;
}


/*
 * Event handler function that is invoked when the application is closed
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();

    /* Emit status bar message signal indicating cleaning-up
     * of memory and temporary files */
    emit messageChanged("Cleaning up...");

    //db->closeConnection();
    db->deleteAll();
}


/**
 * Opens a "Open File..." dialog
 */
void MainWindow::open()
{
	QSettings settings(MainWindow::APPLICATION_ORGANIZATION, MainWindow::APPLICATION_NAME);
	QString directory = settings.value(MainWindow::SETTINGS_OPEN_FILE_DIRECTORY, ".").toString();
	/* Get the selected file names */

    selectedFiles = QFileDialog::getOpenFileNames(
    		this,
    		tr("Select one or more files to open"),
    		directory,
    		tr("ACE files (*.ace)"));
    selectedFilesNum = selectedFiles.count();
    if (selectedFilesNum == 0) return;

    /* Update the max value of the progress bar */
    progressBar->setMaximum(selectedFilesNum);

    QFileInfo fileInfo(selectedFiles[0]);
    settings.setValue(MainWindow::SETTINGS_OPEN_FILE_DIRECTORY, fileInfo.absolutePath());

	/* Parse file */
	parseFile();

	/* Set main window title */
   //setWindowTitle(
   // 		tr("%1[*] - %2")
   // 		.arg(File::getName(FIRST_FILE_INDEX))
   // 		.arg(MainWindow::APPLICATION_NAME));

	/* Add these files to the list of recent files */
	foreach (QString file, selectedFiles)
		addRecentFile(file);

	//emit newFileOpened();
}


/*
 * Open a reference directory
 */
void MainWindow::openRef()
{
	QDir dir;
	QString path;
	QStringList filesList, nameFilters;
	int filesListSize;

	QSettings settings(MainWindow::APPLICATION_ORGANIZATION, MainWindow::APPLICATION_NAME);
	QString directory = settings.value(MainWindow::SETTINGS_OPEN_REF_FILE_DIRECTORY, ".").toString();

	path = QFileDialog::getExistingDirectory(this,
					tr("Select directory containing BED files"),
					directory,
					QFileDialog::ShowDirsOnly);
	if (path == "")
		return;
	dir.setPath(path);

	settings.setValue(MainWindow::SETTINGS_OPEN_REF_FILE_DIRECTORY, path);

	nameFilters << "*.bed" << "order.txt";
	filesList = dir.entryList(nameFilters);
	filesListSize = filesList.size();

	if (filesListSize == 0)
	{
		QMessageBox::critical(
				this,
				MainWindow::APPLICATION_NAME,
				tr("Error: Reference directory does not contain "
						"'order.txt' and 'bed' files."));
		return;
	}
	else if (!filesList.contains("order.txt", Qt::CaseInsensitive))
	{
		QMessageBox::critical(
				this,
				MainWindow::APPLICATION_NAME,
				tr("Error: Reference directory does not contain file 'order.txt'."));
		return;
	}

	progressBar->setMaximum(filesListSize);
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	progressBar->show();

	parser->readBedFiles(filesList, filesListSize, path);

	progressBar->hide();
	QApplication::restoreOverrideCursor();

	emit newFileOpened();
}


/*
 * Function that is invoked when a recent file is opened from the File menu
 */
void MainWindow::openRecentFile()
{
	QAction *action;

	/* Setup file boundaries */
	action = qobject_cast<QAction *>(sender());
	selectedFiles.clear();
	selectedFiles.insert(0, action->data().toString());
	selectedFilesNum = 1;

    /* Update the max value of the progress bar */
    progressBar->setMaximum(selectedFilesNum);

	/* Parse file */
	parseFile();

	/* Set main window title */
    //setWindowTitle(
    //		tr("%1[*] - %2")
    //		.arg(File::getName(FIRST_FILE_INDEX))
    //		.arg(MainWindow::APPLICATION_NAME));

	/* Add this file to the list of recent files */
	addRecentFile(action->data().toString());

	//emit newFileOpened();
}


/*
 * Open the given files. This is an overloaded function.
 */
void MainWindow::open(const QStringList &fileList)
{
	/* Get the selected file names */
    selectedFiles = fileList;
    selectedFilesNum = selectedFiles.count();
    if (selectedFilesNum == 0) return;

    /* Update the max value of the progress bar */
    progressBar->setMaximum(selectedFilesNum);

	/* Parse file */
	parseFile();

	/* Set main window title */
    //setWindowTitle(
    //		tr("%1[*] - %2")
    //		.arg(File::getName(FIRST_FILE_INDEX))
    //		.arg(MainWindow::APPLICATION_NAME));

	/* Add these files to the list of recent files */
	foreach (QString file, fileList)
		addRecentFile(file);

	//emit newFileOpened();
}


/*
 * This function is invoked when a new contig file is to be parsed.
 */
void MainWindow::parseFile()
{
    /* Show the progress bar */
    progressBar->show();

    /* Show wait cursor */
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	if (!parser->readAce(selectedFiles, selectedFilesNum))
	{
		statusBar()->clearMessage();
		setWindowTitle(defaultWindowTitle);

		/* Hide the progress bar */
		progressBar->hide();

		/* Disable certain buttons and actions */
		zoomWidget->setEnabled(false);
		bookmarkAction->setDisabled(true);

		/* Restore the normal cursor */
		QApplication::restoreOverrideCursor();

		/* Disable the horizontal and vertical scrollbars */
		mapArea->getHScrollBar()->setDisabled(true);
		mapArea->getVScrollBar()->setDisabled(true);

		return;
	}
//	qDebug() << "Parsing finished...";
//    /* Hide the progress bar */
//    progressBar->hide();
//
//    /* Restore the normal cursor */
//    QApplication::restoreOverrideCursor();
//
//    /* Enable buttons and actions */
//    zoomWidget->setDisabled(false);
//    bookmarkAction->setDisabled(false);
//
//    /* Enable the horizontal and vertical scrollbars */
//    mapArea->getHScrollBar()->setDisabled(false);
//    mapArea->getVScrollBar()->setDisabled(false);
//    qDebug() << "End of parseFile()";
//	//emit resetZoom();
//	qDebug() << "Exiting parseFile()";
}


void MainWindow::finishParsing()
{
	qDebug() << "Parsing finished...";
    progressBar->hide();
    QApplication::restoreOverrideCursor();
    zoomWidget->setDisabled(false);
    bookmarkAction->setDisabled(false);
    mapArea->getHScrollBar()->setDisabled(false);
    mapArea->getVScrollBar()->setDisabled(false);
    qDebug() << "End of parseFile()";
	emit resetZoom();
	qDebug() << "Exiting parseFile()";

	/* Set main window title */
    setWindowTitle(
    		tr("%1[*] - %2")
    		.arg(File::getName(FIRST_FILE_INDEX))
    		.arg(MainWindow::APPLICATION_NAME));

	emit newFileOpened();
}


/*
 * Function to create action items in the File menu
 */
void MainWindow::createActions()
{
	QString tmp;

	/* Open action */
    openAction = new QAction(tr("&Open..."), this);
    openAction->setShortcut(tr("Ctrl+O"));
    openAction->setStatusTip(tr("Open an ACE file"));
    openAction->setToolTip(tr("Open an ACE file"));
    openAction->setIcon(QIcon(":/images/folder_page.png"));
	tmp = "<b>Open</b> action allows the user to select "
			"the sequence files to be loaded into " + MainWindow::APPLICATION_NAME + ".";
	openAction->setWhatsThis(tmp);
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    /* Open action for opening reference dir/files */
    openRefAction = new QAction(tr("Select Ref Dir"), this);
    openRefAction->setStatusTip(tr("Select reference directory"));
    openRefAction->setToolTip(tr("Select reference directory"));
    openRefAction->setIcon(QIcon(":/images/folder.png"));
    openRefAction->setDisabled(true);
	tmp = "<b>Select Ref Dir</b> action allows the user to select "
			"annotation files to be loaded into " + MainWindow::APPLICATION_NAME + ".";
	openRefAction->setWhatsThis(tmp);
    connect(openRefAction, SIGNAL(triggered()), this, SLOT(openRef()));


    /* Export action */
    exportAction = new QAction(tr("&Export"), this);
    exportAction->setStatusTip(tr("Export the selected region as an ACE file"));
    exportAction->setDisabled(true);
	tmp = tr("<b>Export</b> action allows the user to "
			"export the displayed sequence into a text file.");
	exportAction->setWhatsThis(tmp);
	connect(exportAction, SIGNAL(triggered()),
			mapArea, SLOT(exportSelection()));

    /* Export gene-overlap action */
    exportGeneOverlapAction = new QAction(tr("Export Gene Overlaps"), this);
    exportGeneOverlapAction->setStatusTip(tr("Export names of genes that overlap with fragments"));
    exportGeneOverlapAction->setDisabled(true);
    tmp = tr("<b>Export Gene Overlaps</b> action allows the user to "
    		"export to a text file names of genes that overlap with fragments.");
    exportGeneOverlapAction->setWhatsThis(tmp);
    connect(parser, SIGNAL(annotationLoaded()),
    		this, SLOT(enableExportGeneOverlapAction()));

    /* SNP threshold modification action */
    QByteArray statusTip = "Change SNP threshold value";
    snpThresholdAction = new QAction(tr("Change SNP Threshold"), this);
    snpThresholdAction->setStatusTip(tr(statusTip));
	tmp = tr("<b>Change SNP Threshold</b> action allows the user to "
			"change the threshold value for SNPs.");
	snpThresholdAction->setWhatsThis(tmp);
    connect(snpThresholdAction, SIGNAL(triggered()),
    		this, SLOT(getSnpThresholdInput()));

    /* Search action */
    searchAction = new QAction(tr("Search"), this);
    searchAction->setStatusTip(tr("Search sequence, gene, or position"));
    searchAction->setToolTip(tr("Search sequence, gene, or position"));
    searchAction->setIcon(QIcon(":images/find.png"));
    searchAction->setShortcut(tr("CTRL+F"));
    searchAction->setDisabled(true);
    connect(searchAction, SIGNAL(triggered()),
    		search, SLOT(setFocus()));

    /* Bookmark action */
    bookmarkAction = new QAction(tr("Add Bookmark"), this);
    bookmarkAction->setStatusTip(tr("Bookmark the selected region"));
    bookmarkAction->setDisabled(true);
    bookmarkAction->setIcon(QIcon(":/images/tag_blue_add.png"));
	tmp = tr("<b>Bookmaark</b> action allows the user to "
			"bookmark a particular region of the contig.");
	bookmarkAction->setWhatsThis(tmp);
    connect(bookmarkAction, SIGNAL(triggered()), mapArea, SLOT(bookmark()));

    /* Recent files */
    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
        recentFileActions[i]->setStatusTip("Open a recent file");
        connect(recentFileActions[i], SIGNAL(triggered()),
        		this, SLOT(openRecentFile()));
    }

    /* Exit action */
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    exitAction->setIcon(QIcon(":/images/cross.png"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    /* About action */
    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show the application's About box"));
    aboutAction->setIcon(QIcon(":/images/information.png"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    /* "What's This" action */
    whatsThisAction = new QAction(tr("&What's This?"), this);
    whatsThisAction->setStatusTip(tr("Enter into What's This mode"));
    whatsThisAction->setIcon(QIcon(":/images/help.png"));
    whatsThisAction->setShortcut(QKeySequence(tr("Shift+F1")));
    connect(whatsThisAction, SIGNAL(triggered()),
    		this, SLOT(enterWhatsThisMode()));

    /* Coverage action */
    coverageAction = new QAction(tr("Show Contig Coverage"), this);
    coverageAction->setStatusTip(tr("Show Contig Coverage"));
    coverageAction->setDisabled(true);
    connect(coverageAction, SIGNAL(triggered()),
    		this, SLOT(showCoverage()));
}


/*
 * Function that creates the different menus
 */
void MainWindow::createMenus()
{
	QAction *action;
	QList<QString> bookmarkNames;

	/* File menu */
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(openRefAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exportAction);
    fileMenu->addAction(exportGeneOverlapAction);
    fileMenu->addSeparator();
    fileMenu->addAction(coverageAction);
    fileMenu->addSeparator();
    separatorAction = fileMenu->addSeparator();

    for (int i = 0; i < MaxRecentFiles; ++i)
        fileMenu->addAction(recentFileActions[i]);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    /* Edit menu */
    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(snpThresholdAction);
    editMenu->addAction(searchAction);

    /* Bookmark menu */
    bookmarkMenu = menuBar()->addMenu(tr("&Bookmark"));
    bookmarkMenu->addAction(bookmarkAction);

    /* Show existing bookmarks */
    getAllBookmarkNames(bookmarkNames);
    if (bookmarkNames.size() > 0)
    	bookmarkMenu->addSeparator();
    foreach (QString str, bookmarkNames)
    {
    	action = new QAction(str, this);
    	action->setStatusTip(tr("Display this bookmark"));
    	connect(action, SIGNAL(triggered()),
    			this, SLOT(showBookmark()));
    	bookmarkVector.append(action);
    	bookmarkMenu->addAction(action);
    }

    /* Help menu */
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
    helpMenu->addAction(whatsThisAction);

    action = NULL;
}


/*
 * Function that creates the status bar of the application window
 */
void MainWindow::createStatusBar()
{
    locationLabel = new QLabel(" Ready ");
    locationLabel->setAlignment(Qt::AlignHCenter);
    locationLabel->setMinimumSize(locationLabel->sizeHint());

    progressBar = new QProgressBar;
    progressBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    progressBar->setMinimum(0);
    progressBar->setMinimumWidth(100);
    progressBar->hide();
    connect(parser, SIGNAL(totalSize(int)),
    		progressBar, SLOT(setMaximum(int)));
    connect(parser, SIGNAL(parsingProgress(int)),
    		progressBar, SLOT(setValue(int)));

    statusBar()->addWidget(locationLabel);
    statusBar()->addPermanentWidget(progressBar);
}


/*
 * Function to create tool bars
 */
void MainWindow::createToolBars()
{
	fileToolBar = addToolBar(tr("&File"));
	fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	fileToolBar->addAction(openAction);
	fileToolBar->addAction(openRefAction);
	fileToolBar->addAction(searchAction);

	whatsThisToolBar = addToolBar(tr("&Help"));
	whatsThisToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	whatsThisToolBar->addAction(whatsThisAction);
}


/*
 * Function to read the settings
 */
void MainWindow::readSettings()
{
	QSettings settings(MainWindow::APPLICATION_ORGANIZATION, MainWindow::APPLICATION_NAME);

    QRect rect = settings.value(MainWindow::SETTINGS_GEOMETRY,
                                QRect(200, 200, 400, 400)).toRect();
    move(rect.topLeft());
    resize(rect.size());

    recentFiles = settings.value(MainWindow::SETTINGS_RECENT_FILES).toStringList();
    updateRecentFileActions();

    mapArea->setSnpThreshold(
    		settings.value(MainWindow::SETTINGS_SNP_THRESHOLD, SNP_THRESHOLD).toInt());
    snpNavWidget->setThreshold(
    		settings.value(MainWindow::SETTINGS_SNP_THRESHOLD, SNP_THRESHOLD).toInt());
}


/*
 * Function to write the settings
 */
void MainWindow::writeSettings()
{
	QSettings settings(MainWindow::APPLICATION_ORGANIZATION, MainWindow::APPLICATION_NAME);

    settings.setValue(MainWindow::SETTINGS_GEOMETRY, geometry());
    settings.setValue(MainWindow::SETTINGS_RECENT_FILES, recentFiles);
    settings.setValue(MainWindow::SETTINGS_SNP_THRESHOLD, mapArea->getSnpThreshold());
}


/*
 * Function that sets the current filename
 */
bool MainWindow::loadFile(const QString &fileName)
{
    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
    return true;
}


/*
 * Funtion that sets the current filename and updates the main window title
 */
void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    setWindowModified(false);

    QString shownName = "Untitled";
    if (!curFile.isEmpty())
    {
        shownName = strippedName(curFile);
        recentFiles.removeAll(curFile);
        recentFiles.prepend(curFile);
        updateRecentFileActions();
    }
}


/*
 * Add the given file to the list of recent files
 */
void MainWindow::addRecentFile(const QString &fileName)
{
    if (!fileName.isEmpty())
    {
        recentFiles.removeAll(fileName);
        recentFiles.prepend(fileName);
        updateRecentFileActions();
    }
}


/*
 * Function that updates the recent file list in the File menu
 */
void MainWindow::updateRecentFileActions()
{
    QMutableStringListIterator i(recentFiles);
    while (i.hasNext())
    {
        if (!QFile::exists(i.next()))
            i.remove();
    }

    for (int j = 0; j < MaxRecentFiles; ++j)
    {
        if (j < recentFiles.count())
        {
            QString text = tr("&%1 %2")
                           .arg(j + 1)
                           .arg(strippedName(recentFiles[j]));
            recentFileActions[j]->setText(text);
            recentFileActions[j]->setData(recentFiles[j]);
            recentFileActions[j]->setVisible(true);
        }
        else
        {
            recentFileActions[j]->setVisible(false);
        }
    }
    separatorAction->setVisible(!recentFiles.isEmpty());
}


/*
 * Function that accepts a fully qualified file name and returns
 * just the actual file name (excluding the path)
 */
QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}


/*
 * Function that is invoked when Help->About is selected
 */
void MainWindow::about()
{
    QMessageBox::about(this, "About " + MainWindow::APPLICATION_NAME,
            "<h2>" + MainWindow::APPLICATION_NAME + " " + MainWindow::VERSION_NUMBER + "</h2>"
               "<p>Copyright &copy; 2008 St. Jude Children's Research Hospital."
               "<p>" + MainWindow::APPLICATION_NAME + " is a lightweight viewer  "
               "for quickly identifying where experimental data maps to "
               "a reference genome.</p>"
                "<p>" + MainWindow::APPLICATION_NAME + " is free software, and can be used, modified, "
                "and redistributed under the terms of the GNU General "
                "Public License.</p>"
                "<p>"
                "Credits: <br />"
                "&nbsp;&nbsp;&nbsp;"
                "Icons: Mark James "
                "(<a href='http://www.famfamfam.com/lab/icons/silk/'>"
                "http://www.famfamfam.com/lab/icons/silk/</a>) <br />"
                "&nbsp;&nbsp;&nbsp;"
                "Icons: <a href='http://commons.wikimedia.org/wiki/Image:DNA_icon_(25x25).png'> "
                "http://commons.wikimedia.org/wiki/Image:DNA_icon_(25x25).png</a>"
                "</p>");
}


/*
 * Get the number of selected files
 */
int MainWindow::getNumSelectedFiles() const
{
	return selectedFilesNum;
}


/**
 * Enables the 'Open Reference Action'
 */
void MainWindow::enableOpenRefAction()
{
	openRefAction->setEnabled(true);
}


/*
 * Disable the 'Open Reference Action'
 */
void MainWindow::disableOpenRefAction()
{
	openRefAction->setEnabled(false);
}


/*
 * Enable or disable bookmark action
 */
void MainWindow::enableBookmarkAction()
{
	bookmarkAction->setEnabled(true);
}


/*
 * Get a list of all bookmark names from the DB
 */
void MainWindow::getAllBookmarkNames(QList<QString> &bookmarkList)
{
	QString connectionName = "Mainwindow_getAllBookmarkNames";
	{
	QString str;
	QSqlDatabase db =
		Database::createConnection(
			connectionName,
			Database::getContigDBName());
	QSqlQuery query(db);

	str = "select name from bookmark";
	if (!query.exec(str))
	{
		QMessageBox::critical(
			reinterpret_cast<QWidget *>(this),
			MainWindow::APPLICATION_NAME,
			tr("Error fetching bookmarks from DB.\nReason: "
					+ query.lastError().text().toAscii()));
		return;
	}

	while (query.next())
		bookmarkList.append(query.value(0).toString());
	}
	QSqlDatabase::removeDatabase(connectionName);
}


/*
 * Display the bookmarked location
 */
void MainWindow::showBookmark()
{
	QAction *action;
	QString bookmarkName;

	/* Get bookmark name */
	action = qobject_cast<QAction *>(sender());
	bookmarkName = action->text();

	/* Send signal to load bookmark */
	emit bookmarkClicked(bookmarkName);
}


/*
 * Add bookmark to the bookmark menu
 */
void MainWindow::addBookmarkAction(const QString &name)
{
	QAction *action;

	action = new QAction(name, this);
	action->setStatusTip(tr("Display this bookmark"));
	connect(action, SIGNAL(triggered()), this, SLOT(showBookmark()));
	bookmarkVector.append(action);
	bookmarkMenu->addAction(action);
}


/*
 * Get SNP threshold input value from the user
 */
void MainWindow::getSnpThresholdInput()
{
	int snpThreshold, existingSnpThreshold;
	bool ok;
	QByteArray label;

	existingSnpThreshold = mapArea->getSnpThreshold();
	label = "Enter SNP threshold percent value (current threshold = "
		+ QByteArray::number(existingSnpThreshold) + "%): ";
	snpThreshold = QInputDialog::getInteger(
			this,
			tr("Enter SNP threshold"),
			tr(label),
			existingSnpThreshold, 10, 100, 10, &ok);

	if (ok)
	{
		mapArea->setSnpThreshold(snpThreshold);
		snpNavWidget->setThreshold(snpThreshold);
	}
}


/*
 * Set the current value of the progress dialog
 */
void MainWindow::setProgressDialogValue(const int val)
{
	progressDialog->setValue(val);
}


/*
 * Set the maximum value of the progress dialog
 */
void MainWindow::setProgressDialogMax(const int max)
{
	progressDialog->setMaximum(max);
}


/*
 * Set the label of the progress dialog
 */
void MainWindow::setProgressDialogLabel(const QString &label)
{
	progressDialog->setLabelText(label);
}


/*
 * Load style sheet
 */
void MainWindow::loadStyleSheet(const QString &sheetName)
{
    QFile file(":/qss/" + sheetName.toLower() + ".qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
}


/*
 * Enter or leave What's This mode
 */
void MainWindow::enterWhatsThisMode()
{
	if (QWhatsThis::inWhatsThisMode())
		QWhatsThis::leaveWhatsThisMode();
	else
		QWhatsThis::enterWhatsThisMode();
}


/*
 * Enable searchAction
 */
void MainWindow::enableSearchAction()
{
	searchAction->setEnabled(true);
}


/*
 * Disable searchAction
 */
void MainWindow::disableSearchAction()
{
	searchAction->setDisabled(true);
}


/*
 * Enable coverage action
 */
void MainWindow::enableCoverageAction()
{
	coverageAction->setEnabled(true);
}


/*
 * Disable coverage action
 */
void MainWindow::disableCoverageAction()
{
	coverageAction->setDisabled(true);
}


/*
 * Modify window size
 */
void MainWindow::modifySize()
{
	adjustSize();
	qDebug() << "modifySize";
}


/*
 * Show contig coverage
 */
void MainWindow::showCoverage()
{
	QSqlDatabase db =
		Database::createConnection(
				QString(this->metaObject()->className()),
				Database::getContigDBName());
	QSqlQuery query(db);
	QString str, displayStr;

	displayStr = "";
	str = "select contigOrder, coverage "
			" from contig "
			" order by contigOrder";
	if (!query.exec(str))
	{
		QMessageBox::critical(
			this,
			MainWindow::APPLICATION_NAME,
			tr("Error fetching contigs from DB.\nReason: "
					+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	while (query.next())
	{
		displayStr += "contig ";
		displayStr += query.value(0).toString() + " => ";
		displayStr += query.value(1).toString() + "<br />";
	}

	coverageMessageBox->setTextFormat(Qt::RichText);
	coverageMessageBox->setText(displayStr);
	coverageMessageBox->setWindowTitle("Contig Coverage");
	coverageMessageBox->show();
	db.close();
}


/*
 * Enable exportGeneOverlapAction
 */
void MainWindow::enableExportGeneOverlapAction()
{
	exportGeneOverlapAction->setEnabled(true);
}


/*
 * Disable exportGeneOverlapAction
 */
void MainWindow::disableExportGeneOverlapAction()
{
	exportGeneOverlapAction->setDisabled(true);
}

/*
 * Handler for what to do when escape is pressed, which will depend on several factors
 */
void MainWindow::onEscape()
{
	if ( search->areaHasFocus() )
	{
		search->hideArea();
	}
}












