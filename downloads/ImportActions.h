#ifndef IMPORTACTIONS_H
#define IMPORTACTIONS_H

#include <QWidget>

#include "data/TvShow.h"
#include "downloads/ImportDialog.h"

namespace Ui {
class ImportActions;
}

class ImportActions : public QWidget
{
    Q_OBJECT

public:
    explicit ImportActions(QWidget *parent = 0);
    ~ImportActions();
    void setButtonEnabled(bool enabled);
    void setBaseName(QString baseName);
    QString baseName();
    void setType(QString type);
    QString type();
    void setTvShow(TvShow *show);
    TvShow *tvShow();
    void setImportDir(QString dir);
    QString importDir();
    void setFiles(QStringList files);
    QStringList files();
    void setExtraFiles(QStringList extraFiles);
    QStringList extraFiles();

private slots:
    void onImport();

private:
    Ui::ImportActions *ui;
    QString m_baseName;
    QString m_type;
    QString m_importDir;
    TvShow *m_tvShow;
    ImportDialog *m_importDialog;
    QStringList m_files;
    QStringList m_extraFiles;
};

#endif // IMPORTACTIONS_H