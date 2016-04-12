/*
 * search.h
 *
 *  Created on: Apr 17, 2009
 *      Author: John Obenauer, Pankaj Gupta
 */

#ifndef SEARCH_H_
#define SEARCH_H_

#include <QtGui>
#include <QWidget>
#include <QComboBox>
#include <QCompleter>
#include <QStringList>
#include <QStringListModel>
#include "contig.h"

class Search : public QWidget
{
	Q_OBJECT

public:
	Search(QWidget *parent = 0);
	~Search();
	QGroupBox *getGroupBox();
	static void setIsSeqHighlighted(bool);
	static bool getIsSeqHighlighted();
	static QList<QString> suggestions;
	QStringListModel *suggestionsModel;
	void clearResults();
	void setSearchResultVars();
	void saveSearchQuery(const QString);
	void hideArea();
	bool areaHasFocus();

	public slots:
	void enableTextBox();
	void disableTextBox();
	void setContig(Contig *);
	void setEnabled(bool);
	void setFocus();
	void reset();
	QStringList findSuggestions(const QString);
	void connectSuggestions();
	void disconnectSuggestions();

private:
	QLineEdit *textBox;
	QPushButton *searchButton;
	QPushButton *clearButton;
	QPushButton *nextSearchResultButton;
	QPushButton *previousSearchResultButton;
	QShortcut *enterShortcut;
	QShortcut *returnShortcut;
	QShortcut *shiftEnterShortcut;
	QShortcut *shiftReturnShortcut;
	QPushButton *closeButton;
	QLabel *messageLabel;
	QHBoxLayout *layout;
	QGroupBox *groupBox;
	QCompleter *searchSuggestionsDisplay;
	Contig *contig;
	QComboBox *typeSelector;
	QList<QString> *searchTypes;

	static QMap<int, QMap<int, int> *> resultsMap;
	static QMap<int, int> nextSearchResult;
	static QMap<int, int> previousSearchResult;
	static bool isSeqHighlighted;
	int firstResultContigId;
	int lastResultContigId;
	int firstResultOrderNum;
	int lastResultOrderNum;
	int firstResultStartPos;
	int lastResultStartPos;

	void initSearchTypeSelector();

	private slots:
	void enableButton(const QString &);
	void enableNavigationButtons();
	void disableNavigationButtons();
	void showStatus(const QString &);
	void initShortcuts();
	void updateShortcuts();
	void deleteShortcuts();
	void find();
	void searchPos(const QString &);
	void searchSeq(const QString &);
	void searchGene(const QString &);
	void clearText();
	void goToNextSearchResult();
	void goToPreviousSearchResult();
	void refreshSearch();
	void showSuggestions(const QString &);
	void handleNoResultsFound();
	void resetSearchText();

	signals:
	void resultFound(const int, const int);
	void resultFound(QMap<int, QMap<int, int> *> *);
	void message(const QString &);
	void textChanged(const QString &);
	void searchHidden();
	void foundSuggestions();
	void noResultsFound();
};
#endif /* SEARCH_H_ */
