
#ifndef BOOKMARKDIALOG_H_
#define BOOKMARKDIALOG_H_

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class BookmarkDialog : public QDialog
{
    Q_OBJECT

public:
    BookmarkDialog(QWidget *parent);
    ~BookmarkDialog();

    public slots:
    void saveBookmark();
    void enableSaveButton(const QString &);

private:
	QLabel *nameLabel;
	QLineEdit *nameLineEdit;
	QPushButton *saveButton;
	QPushButton *cancelButton;
	QHBoxLayout *nameHBoxLayout;
	QHBoxLayout *buttonsHBoxLayout;
	QVBoxLayout *vBoxLayout;

	signals:
	void bookmarkAdded(const QString &);
};

#endif /* BOOKMARKDIALOG_H_ */
