/*
 * search.cpp
 *
 *  Created on: Apr 17, 2009
 */
#include "search.h"
#include <iostream>
#include <QSqlQuery>
#include <QSqlError>

#define MINIMUM_SEARCH_SUGGESTION_LENGTH 4
#define MAXIMUM_VISIBLE_SEARCH_SUGGESTIONS 4
#define	TEXT_BOX_WIDTH		300

// searchTypes followed by display value
#define SEARCH_ALL "All"
#define SEARCH_ALL_GENES "Genes"
#define SEARCH_ALL_TFBS "TFBS"
#define SEARCH_ALL_SNPS "SNPS"

bool Search::isSeqHighlighted = false;
QList<QString> Search::suggestions;
QMap<int, QMap<int, int> *> Search::resultsMap = QMap<int, QMap<int, int> *>();

/*
 * Constructor
 */
Search::Search(QWidget *parent)
	: QWidget(parent)
{
	typeSelector = new QComboBox;
	typeSelector->setEnabled(false);

	initSearchTypeSelector();

	textBox = new QLineEdit;
	textBox->setDisabled(true);
	textBox->setMinimumWidth(TEXT_BOX_WIDTH);

	searchButton = new QPushButton;
	searchButton->setIcon(QIcon(":/images/find.png"));
	//searchButton->setText(tr("Search"));
	searchButton->setToolTip(tr("Search"));
	searchButton->setStatusTip(tr("Search"));
	searchButton->setDisabled(true);
	searchButton->setProperty("searchButton", true);

	clearButton = new QPushButton;
	clearButton->setIcon(QIcon(":images/delete.png"));
	//clearButton->setText(tr("Clear"));
	clearButton->setToolTip(tr("Clear search text"));
	clearButton->setStatusTip(tr("Clear search text"));
	clearButton->setDisabled(true);
	clearButton->setProperty("searchButton", true);

	nextSearchResultButton = new QPushButton;
	nextSearchResultButton->setIcon(QIcon(":images/resultset_next.png"));
	//nextSearchResultButton->setText(tr("Next"));
	nextSearchResultButton->setToolTip(tr("Next Search Result"));
	nextSearchResultButton->setStatusTip(tr("Next Search Result"));
	nextSearchResultButton->setDisabled(true);
	nextSearchResultButton->setProperty("searchButton", true);

	previousSearchResultButton = new QPushButton;
	previousSearchResultButton->setIcon(QIcon(":images/resultset_previous.png"));
	//previousSearchResultButton->setText(tr("Previous"));
	previousSearchResultButton->setToolTip(tr("Previous Search Result"));
	previousSearchResultButton->setStatusTip(tr("Previous Search Result"));
	previousSearchResultButton->setDisabled(true);
	previousSearchResultButton->setProperty("searchButton", true);

	closeButton = new QPushButton;
	closeButton->setIcon(QIcon(":images/cross.png"));
	closeButton->setToolTip(tr("Hide Search"));
	closeButton->setStatusTip(tr("Hide Search"));
	closeButton->setFlat(true);
	connect(closeButton, SIGNAL(clicked()), this, SIGNAL(searchHidden()));

	suggestionsModel = new QStringListModel(this);
	searchSuggestionsDisplay = new QCompleter(suggestionsModel);
	searchSuggestionsDisplay->setCaseSensitivity(Qt::CaseInsensitive);
	searchSuggestionsDisplay->setCompletionMode(QCompleter::PopupCompletion);
	textBox->setCompleter(searchSuggestionsDisplay);

	messageLabel = new QLabel;

	layout = new QHBoxLayout;
	layout->addWidget(closeButton, 0, Qt::AlignRight);
	layout->addWidget(typeSelector, 0, Qt::AlignRight);
	layout->addWidget(textBox, 0, Qt::AlignLeft);
	layout->addWidget(searchButton, 0, Qt::AlignLeft);
	layout->addWidget(clearButton, 0, Qt::AlignLeft);
	layout->addWidget(previousSearchResultButton, 0, Qt::AlignLeft);
	layout->addWidget(nextSearchResultButton, 0, Qt::AlignLeft);
	layout->addWidget(messageLabel, 0, Qt::AlignLeft);
	layout->addStretch(0);
	layout->addWidget(closeButton, 0, Qt::AlignRight);

	groupBox = new QGroupBox(tr("Search"));
	groupBox->setToolTip("Search using sequence, position, or gene");
	QString tmp = tr("Search using:"
		"<ul>"
		"<li><b> sequence </b>"
		"<li><b> gene </b>"
		"(<i>Note:</i> You need to upload Annotation file and Order file for this.)"
		"<li><b> position </b>"
		"<span style='white-space:pre'>(Ex: chr4:12130000-121350000)</span> "
		"(<i>Note:</i> You need to upload Order file for this.)"
		"</ul>");
	groupBox->setWhatsThis(tmp);
	groupBox->setLayout(layout);
	groupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	connect(closeButton, SIGNAL(clicked()), groupBox, SLOT(hide()));

	contig = NULL;

	/* textBox */
	connect(textBox, SIGNAL(textChanged(const QString &)),
			this, SLOT(reset()));
	connect(textBox, SIGNAL(textChanged(const QString &)),
			this, SLOT(enableButton(const QString &)));
	connect(textBox, SIGNAL(textChanged(const QString &)),
			this, SIGNAL(textChanged(const QString &)));
	connect(this, SIGNAL(noResultsFound()), this, SLOT(handleNoResultsFound()));
	connect(textBox, SIGNAL(textChanged(const QString &)), this, SLOT(resetSearchText()));

	/* Search buttons */
	connect(searchButton, SIGNAL(clicked()), this, SLOT(find()));
	connect(clearButton, SIGNAL(clicked()), this, SLOT(clearText()));
	connect(this, SIGNAL(message(const QString &)),
			this, SLOT(showStatus(const QString &)));
	connect(this, SIGNAL(resultFound(QMap<int, QMap<int, int> *> *)),
			this, SLOT(enableNavigationButtons()));
	connect(clearButton, SIGNAL(clicked()), this, SLOT(disableNavigationButtons()));
	connect(nextSearchResultButton, SIGNAL(clicked()), this, SLOT(goToNextSearchResult()));
	connect(previousSearchResultButton, SIGNAL(clicked()), this, SLOT(goToPreviousSearchResult()));

	/* search suggestion connections */
	connectSuggestions();
	// We do not want the suggestions to be recomputed when a user traverses
	// the completer using arrow keys
	connect(searchSuggestionsDisplay, SIGNAL(highlighted(const QString &)),
			this, SLOT(disconnectSuggestions()));
	// We want the new suggestions to be computed when the text changes
	connect(textBox, SIGNAL(textChanged(const QString &)),
				this, SLOT(connectSuggestions()));
	// We want the selection of a suggestion to carry out the search
	connect(searchSuggestionsDisplay, SIGNAL(activated(const QString &)),
			this, SLOT(find()));
	initShortcuts();
	updateShortcuts();
}

/*
 * Destructor
 */
Search::~Search()
{
	//delete textBox;
	//delete searchButton;
	//delete clearButton;
	//delete nextSearchResultButton;
	//delete previousSearchResultButton;
	//delete closeButton;
	//delete messageLabel;
	//delete searchSuggestionsDisplay;
	delete layout;
	delete groupBox;
}

/*
 * initializes the selector widget for selecting what scope to search
 */
void Search::initSearchTypeSelector()
{
	typeSelector->addItem(tr(SEARCH_ALL));
	typeSelector->addItem(tr(SEARCH_ALL_GENES));
	typeSelector->addItem(tr(SEARCH_ALL_TFBS));
	typeSelector->addItem(tr(SEARCH_ALL_SNPS));
}

/**
 *
 * Initializes the keyboard shortcuts (but does not map them)
 *
 * @see: updateShortcuts
 */
void Search::initShortcuts()
{
	// sets to null pointers so they can be tested for existence
	enterShortcut = 0;
	returnShortcut = 0;
	shiftEnterShortcut = 0;
	shiftReturnShortcut = 0;
}

/**
 * Shortcuts will change depending on different situations in window
 */
void Search::updateShortcuts()
{
	if ( resultsMap.size() > 0 )
	{
		// if there are search results, pressing Enter/Return should move through them
		deleteShortcuts();
		enterShortcut = new QShortcut(Qt::Key_Enter, nextSearchResultButton, SIGNAL(clicked()));
		returnShortcut = new QShortcut(Qt::Key_Return, nextSearchResultButton, SIGNAL(clicked()));
		shiftEnterShortcut = new QShortcut(Qt::SHIFT + Qt::Key_Enter, previousSearchResultButton, SIGNAL(clicked()));
		shiftReturnShortcut = new QShortcut(Qt::SHIFT + Qt::Key_Return, previousSearchResultButton, SIGNAL(clicked()));
	} else {
		// before a search has occurred, pressing the Enter/Return should start search
		deleteShortcuts();
		enterShortcut = new QShortcut(Qt::Key_Enter, searchButton, SIGNAL(clicked()));
		returnShortcut = new QShortcut(Qt::Key_Return, searchButton, SIGNAL(clicked()));
	}
}

/**
 * deletes shortcut class variables and sets pointers to null
 */
void Search::deleteShortcuts()
{
	if ( enterShortcut )
	{
		delete enterShortcut;
	}
	if ( returnShortcut )
	{
		delete returnShortcut;
	}
	if ( shiftEnterShortcut )
	{
		delete shiftEnterShortcut;
	}
	if ( shiftReturnShortcut )
	{
		delete shiftReturnShortcut;
	}
	initShortcuts();
}

/*
 * If the given string contains text, then enable the search
 * button.
 */
void Search::enableButton(const QString &str)
{
	if (str.trimmed().isEmpty())
	{
		searchButton->setDisabled(true);
		clearButton->setDisabled(true);
		nextSearchResultButton->setDisabled(true);
		previousSearchResultButton->setDisabled(true);
	}
	else
	{
		searchButton->setDisabled(false);
		clearButton->setDisabled(false);
	}
	showStatus("");
}

/*
 * enables the next/previous buttons
 */
void Search::enableNavigationButtons()
{
	nextSearchResultButton->setDisabled(false);
	previousSearchResultButton->setDisabled(false);
}

/*
 * disables the next/previous buttons
 */
void Search::disableNavigationButtons()
{
	nextSearchResultButton->setDisabled(true);
	previousSearchResultButton->setDisabled(true);
}

/*
 * Show search related message
 */
void Search::showStatus(const QString &str)
{
	messageLabel->setText(str);
}

/*
 * Updates various widgets to show that a result was not found during the search
 */
void Search::handleNoResultsFound()
{
	// change textBox to have error style
	textBox->setStyleSheet("background-color: #ed7777; color: white;");

	// update message
	emit message("No results found");
}

/*
 * Resets various widgets to the default state (does not clear textBox)
 */
void Search::resetSearchText()
{
	// change textBox to have default style
	textBox->setStyleSheet("background-color: white; color: black;");
	emit message("");
}

/*
 * Call appropriate search function based on the search string pattern
 */
void Search::find()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QRegExp posRegExp("^[a-z0-9]+\\:([0-9]\\,?)+\\-([0-9]\\,?)+$",
			Qt::CaseInsensitive);
	QRegExp seqRegExp("^(a|c|g|t|n)+$", Qt::CaseInsensitive);
	QString str = textBox->text();
	clearResults();
	if ( typeSelector->currentText() == SEARCH_ALL )
	{
		if (posRegExp.exactMatch(str))
		{
			searchPos(str);
		} else if (seqRegExp.exactMatch(str)) {
			searchSeq(str);
		} else {
			searchGene(str);
		}
	} else if ( typeSelector->currentText() == SEARCH_ALL_GENES ) {
		searchGene(str);
	} else if ( typeSelector->currentText() == SEARCH_ALL_TFBS ) {

	} else if ( typeSelector->currentText() == SEARCH_ALL_SNPS ) {

	}

	if ( resultsMap.size() > 0 )
	{
		setSearchResultVars();
		saveSearchQuery(str);
		goToNextSearchResult();
	} else {
		emit noResultsFound();
	}
	updateShortcuts();
	QApplication::restoreOverrideCursor();
}


/*
 * Search for the given string in the contig sequence
 */
void Search::searchSeq(const QString &str)
{
	QStringMatcher matcher(str, Qt::CaseInsensitive);
	QString queryStr, seq;
	QSqlQuery query;
	int index, contigId, strLen;
	QList<int> contigIdList;
	QMap<int, int> *map;

	/* Initialize */
	seq = "";
	contigId = 0;
	index = -1;
	strLen = str.length();
	map = NULL;
	queryStr = "select id "
			" from contig ";
	if (!query.exec(queryStr))
	{
		qDebug() << "Error fetching contig from DB.\nReason: " << query.lastError().text() << endl;
		return;
	}
	while (query.next())
		contigIdList.append(query.value(0).toInt());

	/* Find matches for each contig */
	foreach (contigId, contigIdList)
	{
		index = -1;
		Contig::getSeq(contigId, seq);

		/* Find all possible matches in a contig */
		do
		{
			index = matcher.indexIn(seq, index+1);
			if (index != -1)
			{
				if (map == NULL)
					map = new QMap<int, int>();
				map->insert(index, index+strLen-1);
			}
		} while (index != -1);
		if ( map != NULL && map->size() > 0 )
		{
			resultsMap.insert(contigId, map);
		}
		map = NULL;
	}

	/* If results were found, send a signal */
	if (resultsMap.size() > 0)
	{
		isSeqHighlighted = true;
		//emit resultFound(contigId, index, str.length());
		emit resultFound(&resultsMap);
		emit message("");
	}
	else
		emit message("Sorry, sequence not found!");
}


/*
 * Search for the given position
 */
void Search::searchPos(const QString &str)
{
	QStringList strList1, strList2;
	QString chromName, queryStr;
	int start, end, contigId, start2, end2;
	QSqlQuery query;
	QMap<int, int> *map;

	map = NULL;
	strList1 = str.split(":");
	chromName = strList1.at(0);

	strList2 = strList1.at(1).split("-");
	start = strList2[0].remove(QChar(','), Qt::CaseInsensitive).toInt();
	end = strList2[1].remove(QChar(','), Qt::CaseInsensitive).toInt();

	queryStr = "select contigId, chromStart "
			" from chrom_contig, chromosome "
			" where chrom_contig.chromId = chromosome.id "
			" and chromosome.name = '" + chromName + "' "
			" and chromStart <= " + QString::number(start) +
			" and chromEnd >= " + QString::number(start) +
			" and chromStart <= " + QString::number(end) +
			" and chromEnd >= " + QString::number(end) +
			" order by contigId asc, chromStart asc, chromEnd asc ";
	if (!query.exec(queryStr))
	{
		QMessageBox::critical(
				this,
				tr("Basejumper"),
				tr("Error fetching chromosome and contig from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		return;
	}
	while (query.next())
	{
		contigId = query.value(0).toInt();
		start2 = start - query.value(1).toInt() + 1;
		end2 = start2 + (end - start);
		if (resultsMap.contains(contigId))
			resultsMap.value(contigId)->insert(start2, end2);
		else
		{
			map = new QMap<int, int>();
			map->insert(start2, end2);
			resultsMap.insert(contigId, map);
			map = NULL;
		}
	}

	if (resultsMap.size() > 0)
	{
		isSeqHighlighted = true;
		emit resultFound(&resultsMap);
		emit message("");
	}
	else
		emit message("Sorry, chromosome position not found!");
}


/*
 * Search for the given gene
 */
void Search::searchGene(const QString &name)
{
	QSqlQuery query;
	QString str;
	QMap<int, int> *map;
	int contigId, start, end;

	map = NULL;
	str = "select contigId, startPos, endPos "
			" from annotation "
			" where name = '" + name + "'"
			" order by contigId asc, startPos asc ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this,
				tr("Basejumper"),
				tr("Error fetching annotation from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		return;
	}

	clearResults();
	while (query.next())
	{
		contigId = query.value(0).toInt();
		start = query.value(1).toInt() - 1; /* Convert to 0-based position */
		end = query.value(2).toInt() - 1; /* Convert to 0-based position */

		if (resultsMap.contains(contigId))
			resultsMap.value(contigId)->insert(start, end);
		else
		{
			map = new QMap<int, int>();
			map->insert(start, end);
			resultsMap.insert(contigId, map);
			map = NULL;
		}
	}

	if (resultsMap.size() > 0)
	{
		isSeqHighlighted = true;
		emit resultFound(&resultsMap);
		emit message("");
	}
	else
		emit message("Sorry, gene not found!");
}


/*
 * Enable text box and selector
 */
void Search::enableTextBox()
{
	textBox->setEnabled(true);
	typeSelector->setEnabled(true);
}


/*
 * Disable text box and selector
 */
void Search::disableTextBox()
{
	textBox->setEnabled(false);
	typeSelector->setEnabled(false);
}


/*
 * Set current contig
 */
void Search::setContig(Contig *c)
{
	contig = c;
}


/*
 * Clear the text in the search box
 */
void Search::clearText()
{
	textBox->clear();
	textBox->setFocus();
	updateShortcuts();
}


/*
 * Enable or disable child widgets
 */
void Search::setEnabled(bool enable)
{
	textBox->setEnabled(enable);
}


/*
 * Get a pointer to the group box
 */
QGroupBox* Search::getGroupBox()
{
	return groupBox;
}


/*
 * Set whether sequence is highlighted
 */
void Search::setIsSeqHighlighted(bool b)
{
	isSeqHighlighted = b;

	/* If sequence is not highlighted, then clean up the results */
	// Removed so clearResults doesn't need to be static
	// It doesn't appear that clearResults was ever going to be called from here anyway
	//if (!b)
		//clearResults();
}


/*
 * Reset
 */
void Search::reset()
{
	setIsSeqHighlighted(false);
	clearResults();
	//clearSuggestions();
}


/*
 * Clear/delete the search results
 */
void Search::clearResults()
{
	int key;
	QList<int> keys = resultsMap.keys();
	foreach (key, keys)
		delete resultsMap.value(key);
	resultsMap.clear();
	//clearSuggestions();
	updateShortcuts();
	resetSearchText();
}


/*
 * Return whether sequence is highlighted
 */
bool Search::getIsSeqHighlighted()
{
	return isSeqHighlighted;
}


/*
 * Set focus on the text box
 */
void Search::setFocus()
{
	if (groupBox->isHidden())
		groupBox->setVisible(true);
	textBox->setFocus();
}

/*
 * gets the next (closest to the right) search result or wraps around left
 */
void Search::goToNextSearchResult()
{
	if ( resultsMap.size() < 1 )
	{
		return;
	}

	int startFromPos = Contig::startPos;
	int contigId;
	int nextContigId;
	int contigOrderNum;

	int currentOrderNum = Contig::orderMap.key(Contig::currentId);

	for (QMap<int, int>::iterator i = Contig::orderMap.find(currentOrderNum);
		i != Contig::orderMap.end(); i++)
	{
		contigOrderNum = i.key();
		contigId = i.value();
		// needed to prevent crashing when contig has no results?
		if ( !resultsMap.contains(contigId) )
		{
			continue;
		}
		// look for next search result in this contig
		for ( QMap<int, int>::iterator result = resultsMap.value(contigId)->begin();
				result != resultsMap.value(contigId)->end(); ++result )
		{
			if ( result.key() > startFromPos )
			{
				//emit resultFound(contigOrderNum, result.key());
				emit resultFound(contigId, result.key());
				return;
			}
		}

		//switching contigs after no more results in this one,
		//so start from beginning of next one
		if ( resultsMap.size() == contigOrderNum )
		{
			// wrap
			nextContigId = Contig::orderMap[1];
		} else {
			nextContigId = Contig::orderMap[contigOrderNum + 1];
		}
		startFromPos = 0;
		resultFound(nextContigId, startFromPos);
	}

	// if they get here, there are no more search results to the right,
	// so wrap by sending them back to the first search result
	emit resultFound(firstResultContigId, 0);
	emit resultFound(firstResultContigId, firstResultStartPos);
	return;
}

/*
 * gets the previous (closest to the left) search result or wraps around right
 */
void Search::goToPreviousSearchResult()
{
	if ( resultsMap.size() < 1 )
	{
		return;
	}

	int startFromPos = Contig::startPos;
	int contigId;
	int prevContigId;
	int contigOrderNum;

	int currentOrderNum = Contig::orderMap.key(Contig::currentId);

	QMap<int, int>::iterator i = Contig::orderMap.find(currentOrderNum);

	QMap<int, int>::iterator j = Contig::orderMap.begin();
	j--;

	for ( ; i != j; i--)
	{
		contigOrderNum = i.key();
		contigId = i.value();

		// needed to prevent crashing when contig has no results?
		if ( !resultsMap.contains(contigId) )
		{
			continue;
		}

		QMap<int, int>::iterator result = resultsMap.value(contigId)->end();
		result--;

		QMap<int, int>::iterator contigBegin = resultsMap.value(contigId)->begin();
		contigBegin--;

		for ( ; result != contigBegin; --result )
		{
			if ( result.key() < startFromPos )
			{
				emit resultFound(contigId, result.key());
				return;
			}
		}

		// list of contig Id's in next search order - why not just from orderMap???
		QList<int> orderIdList = Contig::orderMap.values();
		if ( contigOrderNum == 1 )
		{
			//wrap
			prevContigId = orderIdList[orderIdList.size() - 1];
		} else {
			//contig before current one
			prevContigId = orderIdList[currentOrderNum - 1];
		}
		startFromPos = Contig::getSize(prevContigId);
		// else not needed because we will be wrapping after this loop
	}

	// If they get here, there are no more search results to the
	// left, so wrap by sending them back to the last search result
	emit resultFound(lastResultContigId, Contig::getSize(lastResultContigId));
	emit resultFound(lastResultContigId, lastResultStartPos);
}

/*
 * Sets some values related to the current search which don't change until a new search attempt
 */
void Search::setSearchResultVars()
{
	QList<int> contigIdsWithResults = resultsMap.keys();

	// Get the lowest and highest order numbers that have a search result to allow wrapping
	firstResultOrderNum = lastResultOrderNum = 0;
	int order = 0;
	foreach ( int id, Contig::orderMap.values() )
	{
		order++; // incremental key starting with 1 in orderMap
		if ( contigIdsWithResults.contains(id) )
		{
			if ( !firstResultOrderNum )
			{
				firstResultOrderNum = order; // only get first one
			}
			lastResultOrderNum = order; // get last one
		}
	}

	// first and last (according to current order) contig ID's with results
	firstResultContigId = Contig::orderMap[firstResultOrderNum];
	lastResultContigId = Contig::orderMap[lastResultOrderNum];

	//QList<int> positions = resultsMap.value(firstResultOrderNum)->keys();
	QList<int> positions = resultsMap.value(firstResultContigId)->keys();
	//QList<int> positions = resultsMap[0];
	firstResultStartPos = positions[0];

	//positions = resultsMap.value(lastResultOrderNum)->keys();
	positions = resultsMap.value(lastResultContigId)->keys();
	lastResultStartPos = positions[positions.size() - 1];
}

/*
 * Refreshes the search results if something changes that might affect them or their locations
 */
void Search::refreshSearch()
{
	if ( resultsMap.size() < 1 )
	{
		//nothing to do
		return;
	}

	reset();
	find();
}

/*
 * Store a string in the search query table
 */
void Search::saveSearchQuery(const QString str)
{
	QSqlQuery selectQuery;
	QSqlQuery modifyQuery;

	if ( selectQuery.exec("Select query, count, id From searchQueries Where query = '"+str+"' Limit 1") )
	{
		if ( selectQuery.next() )
		{
			// should only be one result
			// query already existed so update count and lastDatetime
			int count = selectQuery.value(1).toInt() + 1;
			modifyQuery.exec("Update searchQueries set count = "+QString::number(count)+", "
					"lastDatetime = datetime('now') "
					"Where id = "+QString::number(selectQuery.value(2).toInt()));
		} else {
			// insert the new query
			modifyQuery.exec("Insert into searchQueries (query, count, lastDatetime) "
							"Values ('" + str + "', 1, datetime('now'))");
		}
	}
}

/*
 * Suggest search strings based on str as a prefix
 */
QStringList Search::findSuggestions(const QString str)
{
	QStringList queries;
	if ( str.length() < MINIMUM_SEARCH_SUGGESTION_LENGTH )
	{
		return queries;
	}
	QSqlQuery query;

	// ordering search queries by week (most recent) and count (most often)
	if ( query.exec("Select query From searchQueries "
			"Where query Like '"+str+"%' Order By strftime('%Y %W', lastDatetime) "
			"Desc, count Desc") )
	{
		int i = 0;
		while ( query.next() )
		{
			if ( ++i > MAXIMUM_VISIBLE_SEARCH_SUGGESTIONS )
			{
				break;
			}
			queries.append(QString(query.value(0).toString()));
		}

		return queries;
	}
	return queries;
}

/*
 * Updates suggestionsModel to have results based on search string
 */
void Search::showSuggestions(const QString &str)
{
	QStringList queries;
	queries = findSuggestions(str);

	if ( queries.size() < 1 )
	{
		return;
	}
	suggestionsModel->setStringList(queries);
}

/*
 * Connects relevant suggestion signals to slots
 */
void Search::connectSuggestions()
{
	connect(textBox, SIGNAL(textChanged(const QString &)),
			this, SLOT(showSuggestions(const QString &)));
}

/*
 * Disconnects relevant suggestion signals to slots
 */
void Search::disconnectSuggestions()
{
	disconnect(textBox, SIGNAL(textChanged(const QString &)),
			this, SLOT(showSuggestions(const QString &)));
}

/*
 * A hasFocus function for the entire widget and all its focus-able widgets
 */
bool Search::areaHasFocus()
{
	if ( 	textBox->hasFocus() ||
			typeSelector->hasFocus() ||
			closeButton->hasFocus() ||
			searchButton->hasFocus() ||
			clearButton->hasFocus() ||
			nextSearchResultButton->hasFocus() ||
			previousSearchResultButton->hasFocus()
		)
	{
		return true;
	}

	return false;
}

/*
 *
 */
void Search::hideArea()
{
	groupBox->hide();
}

