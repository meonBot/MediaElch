#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QToolBar>

#ifdef Q_OS_MAC
#include <QMenuBar>
#endif

#include "concerts/ConcertSearch.h"
#include "data/MediaCenterInterface.h"
#include "data/ScraperInterface.h"
#include "data/Storage.h"
#include "globals/Globals.h"
#include "globals/Helper.h"
#include "globals/ImageDialog.h"
#include "globals/ImagePreviewDialog.h"
#include "globals/Manager.h"
#include "globals/NameFormatter.h"
#include "globals/TrailerDialog.h"
#include "main/Update.h"
#include "movies/MovieMultiScrapeDialog.h"
#include "movies/MovieSearch.h"
#include "music/MusicMultiScrapeDialog.h"
#include "music/MusicSearch.h"
#include "notifications/NotificationBox.h"
#include "notifications/Notificator.h"
#include "sets/MovieListDialog.h"
#include "settings/Settings.h"
#include "tvShows/TvShowMultiScrapeDialog.h"
#include "tvShows/TvShowSearch.h"
#include "tvShows/TvShowUpdater.h"
#include "tvShows/TvTunesDialog.h"
#include "xbmc/XbmcSync.h"

MainWindow *MainWindow::m_instance = nullptr;

/**
 * @brief MainWindow::MainWindow
 * @param parent
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
#ifdef Q_OS_MAC
    QMenuBar *macMenuBar = new QMenuBar();
    QMenu *menu = macMenuBar->addMenu("File");
    QAction *mAbout = menu->addAction("About");
    mAbout->setMenuRole(QAction::AboutRole);
    connect(mAbout, SIGNAL(triggered()), new AboutDialog(this), SLOT(exec()));
#endif

    ui->setupUi(this);
    setMinimumHeight(500);

    MainWindow::m_instance = this;
    qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    qDebug() << "MediaElch version" << QApplication::applicationVersion() << "starting up";

    QMap<MainActions, bool> allActions;
    allActions.insert(MainActions::Search, false);
    allActions.insert(MainActions::Save, false);
    allActions.insert(MainActions::SaveAll, false);
    allActions.insert(MainActions::FilterWidget, false);
    allActions.insert(MainActions::Rename, false);
    allActions.insert(MainActions::Export, false);

    // initialize all widgets with all actions set to false
    m_actions.insert(MainWidgets::Movies, allActions);
    m_actions.insert(MainWidgets::MovieSets, allActions);
    m_actions.insert(MainWidgets::TvShows, allActions);
    m_actions.insert(MainWidgets::Concerts, allActions);
    m_actions.insert(MainWidgets::Music, allActions);
    m_actions.insert(MainWidgets::Genres, allActions);
    m_actions.insert(MainWidgets::Certifications, allActions);
    m_actions.insert(MainWidgets::Downloads, allActions);

    // enable filtering for some widgets
    m_actions[MainWidgets::Movies][MainActions::FilterWidget] = true;
    m_actions[MainWidgets::TvShows][MainActions::FilterWidget] = true;
    m_actions[MainWidgets::Concerts][MainActions::FilterWidget] = true;
    m_actions[MainWidgets::Music][MainActions::FilterWidget] = true;

    m_aboutDialog = new AboutDialog(this);
    m_supportDialog = new SupportDialog(this);
    m_settingsWindow = new SettingsWindow(this);
    m_fileScannerDialog = new FileScannerDialog(this);
    m_xbmcSync = new XbmcSync(this);
    m_renamer = new Renamer(this);
    m_settings = Settings::instance(this);
    m_exportDialog = new ExportDialog(this);
    setupToolbar();

    Helper::instance(this);
    NotificationBox::instance(this)->reposition(this->size());
    Manager::instance();

    if (!m_settings->mainSplitterState().isNull()) {
        ui->movieSplitter->restoreState(m_settings->mainSplitterState());
        ui->tvShowSplitter->restoreState(m_settings->mainSplitterState());
        ui->setsWidget->splitter()->restoreState(m_settings->mainSplitterState());
        ui->concertSplitter->restoreState(m_settings->mainSplitterState());
        ui->genreWidget->splitter()->restoreState(m_settings->mainSplitterState());
        ui->certificationWidget->splitter()->restoreState(m_settings->mainSplitterState());
        ui->musicSplitter->restoreState(m_settings->mainSplitterState());
    } else {
        ui->movieSplitter->setSizes(QList<int>() << 200 << 600);
        ui->tvShowSplitter->setSizes(QList<int>() << 200 << 600);
        ui->setsWidget->splitter()->setSizes(QList<int>() << 200 << 600);
        ui->concertSplitter->setSizes(QList<int>() << 200 << 600);
        ui->genreWidget->splitter()->setSizes(QList<int>() << 200 << 600);
        ui->certificationWidget->splitter()->setSizes(QList<int>() << 200 << 600);
        ui->musicSplitter->setSizes(QList<int>() << 200 << 600);
    }

    if (m_settings->mainWindowSize().isValid() && !m_settings->mainWindowPosition().isNull()) {
        resize(m_settings->mainWindowSize());
        move(m_settings->mainWindowPosition());
#ifdef Q_OS_WIN32
        if (m_settings->mainWindowMaximized())
            showMaximized();
#endif
    }
    // Size for Screenshots
    // resize(1200, 676);

    m_buttonActiveColor = QColor(70, 155, 198);
    m_buttonColor = QColor(128, 129, 132);
    foreach (QToolButton *btn, ui->menuWidget->findChildren<QToolButton *>()) {
        connect(btn, SIGNAL(clicked()), this, SLOT(onMenu()));
        btn->setIcon(Manager::instance()->iconFont()->icon(btn->property("iconName").toString(), m_buttonColor));
    }

    // clang-format off
    connect(ui->filesWidget, &FilesWidget::movieSelected,   ui->movieWidget, &MovieWidget::setMovie);
    connect(ui->filesWidget, &FilesWidget::movieSelected,   ui->movieWidget, &MovieWidget::setEnabledTrue);
    connect(ui->filesWidget, &FilesWidget::noMovieSelected, ui->movieWidget, &MovieWidget::clear);
    connect(ui->filesWidget, &FilesWidget::noMovieSelected, ui->movieWidget, &MovieWidget::setDisabledTrue);
    connect(ui->filesWidget, &FilesWidget::sigStartSearch,  this,            &MainWindow::onActionSearch);

    connect(ui->concertFilesWidget, &ConcertFilesWidget::concertSelected,   ui->concertWidget, &ConcertWidget::setConcert);
    connect(ui->concertFilesWidget, &ConcertFilesWidget::concertSelected,   ui->concertWidget, &ConcertWidget::setEnabledTrue);
    connect(ui->concertFilesWidget, &ConcertFilesWidget::noConcertSelected, ui->concertWidget, &ConcertWidget::clear);
    connect(ui->concertFilesWidget, &ConcertFilesWidget::noConcertSelected, ui->concertWidget, &ConcertWidget::setDisabledTrue);

    connect(ui->musicFilesWidget, &MusicFilesWidget::sigArtistSelected,  ui->musicWidget, &MusicWidget::onArtistSelected);
    connect(ui->musicFilesWidget, &MusicFilesWidget::sigAlbumSelected,   ui->musicWidget, &MusicWidget::onAlbumSelected);
    connect(ui->musicFilesWidget, SIGNAL(sigArtistSelected(Artist *)),   ui->musicWidget, SLOT(onSetEnabledTrue(Artist *)));
    connect(ui->musicFilesWidget, SIGNAL(sigAlbumSelected(Album *)),     ui->musicWidget, SLOT(onSetEnabledTrue(Album *)));
    connect(ui->musicFilesWidget, &MusicFilesWidget::sigNothingSelected, ui->musicWidget, &MusicWidget::onClear);
    connect(ui->musicFilesWidget, &MusicFilesWidget::sigNothingSelected, ui->musicWidget, &MusicWidget::onSetDisabledTrue);

    connect(ui->tvShowFilesWidget, &TvShowFilesWidget::sigTvShowSelected,       ui->tvShowWidget, &TvShowWidget::onTvShowSelected);
    connect(ui->tvShowFilesWidget, &TvShowFilesWidget::sigSeasonSelected,       ui->tvShowWidget, &TvShowWidget::onSeasonSelected);
    connect(ui->tvShowFilesWidget, &TvShowFilesWidget::sigEpisodeSelected,      ui->tvShowWidget, &TvShowWidget::onEpisodeSelected);
    connect(ui->tvShowFilesWidget, SIGNAL(sigTvShowSelected(TvShow *)),         ui->tvShowWidget, SLOT(onSetEnabledTrue(TvShow *)));
    connect(ui->tvShowFilesWidget, SIGNAL(sigSeasonSelected(TvShow *, int)),    ui->tvShowWidget, SLOT(onSetEnabledTrue(TvShow *, int)));
    connect(ui->tvShowFilesWidget, SIGNAL(sigEpisodeSelected(TvShowEpisode *)), ui->tvShowWidget, SLOT(onSetEnabledTrue(TvShowEpisode *)));
    connect(ui->tvShowFilesWidget, &TvShowFilesWidget::sigNothingSelected,      ui->tvShowWidget, &TvShowWidget::onClear);
    connect(ui->tvShowFilesWidget, &TvShowFilesWidget::sigNothingSelected,      ui->tvShowWidget, &TvShowWidget::onSetDisabledTrue);
    connect(ui->tvShowFilesWidget, &TvShowFilesWidget::sigStartSearch,          this,             &MainWindow::onActionSearch);

    connect(ui->movieWidget, &MovieWidget::actorDownloadProgress, this, &MainWindow::progressProgress);
    connect(ui->movieWidget, &MovieWidget::actorDownloadStarted,  this, &MainWindow::progressStarted);
    connect(ui->movieWidget, &MovieWidget::actorDownloadFinished, this, &MainWindow::progressFinished);

    connect(ui->tvShowWidget, &TvShowWidget::sigDownloadsStarted,  this, &MainWindow::progressStarted);
    connect(ui->tvShowWidget, &TvShowWidget::sigDownloadsProgress, this, &MainWindow::progressProgress);
    connect(ui->tvShowWidget, &TvShowWidget::sigDownloadsFinished, this, &MainWindow::progressFinished);

    connect(ui->musicWidget, &MusicWidget::sigDownloadsStarted,  this, &MainWindow::progressStarted);
    connect(ui->musicWidget, &MusicWidget::sigDownloadsProgress, this, &MainWindow::progressProgress);
    connect(ui->musicWidget, &MusicWidget::sigDownloadsFinished, this, &MainWindow::progressFinished);

    connect(ui->navbar, &Navbar::sigFilterChanged, this, &MainWindow::onFilterChanged);

    connect(ui->movieSplitter,                   &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);
    connect(ui->tvShowSplitter,                  &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);
    connect(ui->setsWidget->splitter(),          &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);
    connect(ui->genreWidget->splitter(),         &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);
    connect(ui->certificationWidget->splitter(), &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);
    connect(ui->concertSplitter,                 &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);
    connect(ui->musicSplitter,                   &QSplitter::splitterMoved, this, &MainWindow::moveSplitter);

    connect(Manager::instance()->tvShowFileSearcher(), &TvShowFileSearcher::tvShowsLoaded, ui->tvShowFilesWidget, &TvShowFilesWidget::renewModel);
    connect(Manager::instance()->tvShowFileSearcher(), &TvShowFileSearcher::tvShowsLoaded, this, &MainWindow::updateTvShows);
    connect(m_fileScannerDialog,                       &QDialog::accepted,                 this, &MainWindow::setNewMarks);
    connect(ui->downloadsWidget,                       &DownloadsWidget::sigScanFinished,  this, &MainWindow::setNewMarks);

    connect(m_xbmcSync, &XbmcSync::sigTriggerReload, this, &MainWindow::onTriggerReloadAll);
    connect(m_xbmcSync, &XbmcSync::sigFinished,      this, &MainWindow::onXbmcSyncFinished);

    connect(m_renamer, &Renamer::sigFilesRenamed, this, &MainWindow::onFilesRenamed);

    connect(m_settingsWindow, &SettingsWindow::sigSaved, this, &MainWindow::onRenewModels, Qt::QueuedConnection);

    connect(ui->setsWidget,          &SetsWidget::sigJumpToMovie,          this, &MainWindow::onJumpToMovie);
    connect(ui->certificationWidget, &CertificationWidget::sigJumpToMovie, this, &MainWindow::onJumpToMovie);
    connect(ui->genreWidget,         &GenreWidget::sigJumpToMovie,         this, &MainWindow::onJumpToMovie);
    // clang-format on

    MovieSearch::instance(this);
    MusicSearch::instance(this);
    TvShowSearch::instance(this);
    ImageDialog::instance(this);
    MovieListDialog::instance(this);
    ImagePreviewDialog::instance(this);
    ConcertSearch::instance(this);
    TrailerDialog::instance(this);
    TvTunesDialog::instance(this);
    NameFormatter::instance(this);
    MovieMultiScrapeDialog::instance(this);
    MusicMultiScrapeDialog::instance(this);
    TvShowMultiScrapeDialog::instance(this);
    Notificator::instance(nullptr, ui->centralWidget);

#ifdef Q_OS_WIN32
    setStyleSheet(styleSheet() + " #centralWidget { border-bottom: 1px solid rgba(0, 0, 0, 100); } ");

    QFont font = ui->labelMovies->font();
    font.setPointSize(font.pointSize() - 3);
    font.setBold(true);
    ui->labelMovies->setFont(font);
    ui->labelConcerts->setFont(font);
    ui->labelShows->setFont(font);
    ui->labelMusic->setFont(font);
    ui->labelDownloads->setFont(font);
#endif

#ifdef Q_OS_WIN
    foreach (QToolButton *btn, ui->menuWidget->findChildren<QToolButton *>())
        btn->setIconSize(QSize(32, 32));
    ui->navbar->setFixedHeight(56);
#endif

    if (Settings::instance()->startupSection() == "tvshows") {
        onMenu(ui->buttonTvshows);
    } else if (Settings::instance()->startupSection() == "concerts") {
        onMenu(ui->buttonConcerts);
    } else if (Settings::instance()->startupSection() == "music") {
        onMenu(ui->buttonMusic);
    } else if (Settings::instance()->startupSection() == "import") {
        onMenu(ui->buttonDownloads);
    } else {
        onMenu(ui->buttonMovies);
    }

    // hack. without only the fileScannerDialog pops up and blocks until it has finished
    show();

    // Start scanning for files
    QTimer::singleShot(0, m_fileScannerDialog, SLOT(exec()));

    if (Settings::instance()->checkForUpdates()) {
        Update::instance()->checkForUpdate();
    }
}

/**
 * @brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
    m_settings->setMainWindowSize(size());
    m_settings->setMainWindowPosition(pos());
    m_settings->setMainSplitterState(ui->movieSplitter->saveState());
    m_settings->setMainWindowMaximized(isMaximized());
    delete ui;
}

MainWindow *MainWindow::instance()
{
    return MainWindow::m_instance;
}

/**
 * @brief Repositions the MessageBox
 * @param event
 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    bool isBigWindow = event->size().width() >= 1500;
    ui->movieWidget->setBigWindow(isBigWindow);
    ui->tvShowWidget->setBigWindow(isBigWindow);
    ui->concertWidget->setBigWindow(isBigWindow);
    ui->musicWidget->setBigWindow(isBigWindow);

    NotificationBox::instance()->reposition(event->size());
    QWidget::resizeEvent(event);
}

/**
 * @brief Sets up the toolbar
 */
void MainWindow::setupToolbar()
{
    qDebug() << "Entered";

    connect(ui->navbar, &Navbar::sigSearch, this, &MainWindow::onActionSearch);
    connect(ui->navbar, &Navbar::sigSave, this, &MainWindow::onActionSave);
    connect(ui->navbar, &Navbar::sigSaveAll, this, &MainWindow::onActionSaveAll);
    connect(ui->navbar, &Navbar::sigReload, this, &MainWindow::onActionReload);
    connect(ui->navbar, SIGNAL(sigAbout()), m_aboutDialog, SLOT(exec()));
    connect(ui->navbar, SIGNAL(sigSettings()), m_settingsWindow, SLOT(show()));
    connect(ui->navbar, &Navbar::sigLike, m_supportDialog, &QDialog::exec);
    connect(ui->navbar, &Navbar::sigSync, this, &MainWindow::onActionXbmc);
    connect(ui->navbar, &Navbar::sigRename, this, &MainWindow::onActionRename);
    connect(ui->navbar, SIGNAL(sigExport()), m_exportDialog, SLOT(exec()));

    ui->navbar->setActionSearchEnabled(false);
    ui->navbar->setActionSaveEnabled(false);
    ui->navbar->setActionSaveAllEnabled(false);
    ui->navbar->setActionRenameEnabled(false);
}

/**
 * @brief Called when a subwidget starts a progress, displays a progress MessageBox
 * @param msg Message
 * @param id (Unique) Id of the progress
 */
void MainWindow::progressStarted(QString msg, int id)
{
    qDebug() << "Entered, msg=" << msg << "id=" << id;
    NotificationBox::instance()->showProgressBar(msg, id);
}

/**
 * @brief Updates the progress MessageBox
 * @param current Current value
 * @param max Maximum value
 * @param id (Unique) Id of the progress
 */
void MainWindow::progressProgress(int current, int max, int id)
{
    NotificationBox::instance()->progressBarProgress(current, max, id);
}

/**
 * @brief Called when a progress has finished
 * @param id (Unique) Id of the progress
 */
void MainWindow::progressFinished(int id)
{
    qDebug() << "Entered, id=" << id;
    NotificationBox::instance()->hideProgressBar(id);
}

/**
 * @brief Called when the action "Search" was clicked
 * Delegates the event down to the current subwidget
 */
void MainWindow::onActionSearch()
{
    qDebug() << "Entered, currentIndex=" << ui->stackedWidget->currentIndex();
    if (ui->stackedWidget->currentIndex() == 0) {
        if (ui->filesWidget->selectedMovies().count() > 1) {
            ui->filesWidget->multiScrape();
        } else {
            QTimer::singleShot(0, ui->movieWidget, &MovieWidget::startScraperSearch);
        }
    } else if (ui->stackedWidget->currentIndex() == 1) {
        if (ui->tvShowFilesWidget->selectedEpisodes(false).count() + ui->tvShowFilesWidget->selectedShows().count()
            > 1) {
            ui->tvShowFilesWidget->multiScrape();
        } else {
            QTimer::singleShot(0, ui->tvShowWidget, &TvShowWidget::onStartScraperSearch);
        }
    } else if (ui->stackedWidget->currentIndex() == 3) {
        QTimer::singleShot(0, ui->concertWidget, &ConcertWidget::onStartScraperSearch);
    } else if (ui->stackedWidget->currentIndex() == 7) {
        if ((ui->musicFilesWidget->selectedArtists().count() + ui->musicFilesWidget->selectedAlbums().count()) > 1) {
            ui->musicFilesWidget->multiScrape();
        } else {
            QTimer::singleShot(0, ui->musicWidget, &MusicWidget::onStartScraperSearch);
        }
    }
}

/**
 * @brief Called when the action "Save" was clicked
 * Delegates the event down to the current subwidget
 */
void MainWindow::onActionSave()
{
    qDebug() << "Entered, currentIndex=" << ui->stackedWidget->currentIndex();
    if (ui->stackedWidget->currentIndex() == 0) {
        ui->movieWidget->saveInformation();
    } else if (ui->stackedWidget->currentIndex() == 1) {
        ui->tvShowWidget->onSaveInformation();
    } else if (ui->stackedWidget->currentIndex() == 2) {
        ui->setsWidget->saveSet();
    } else if (ui->stackedWidget->currentIndex() == 3) {
        ui->concertWidget->onSaveInformation();
    } else if (ui->stackedWidget->currentIndex() == 4) {
        ui->genreWidget->onSaveInformation();
    } else if (ui->stackedWidget->currentIndex() == 5) {
        ui->certificationWidget->onSaveInformation();
    } else if (ui->stackedWidget->currentIndex() == 7) {
        ui->musicWidget->onSaveInformation();
    }
    setNewMarks();
}

/**
 * @brief Called when the action "Save all" was clicked
 * Delegates the event down to the current subwidget
 */
void MainWindow::onActionSaveAll()
{
    qDebug() << "Entered, currentIndex=" << ui->stackedWidget->currentIndex();
    if (ui->stackedWidget->currentIndex() == 0) {
        ui->movieWidget->saveAll();
    } else if (ui->stackedWidget->currentIndex() == 1) {
        ui->tvShowWidget->onSaveAll();
    } else if (ui->stackedWidget->currentIndex() == 3) {
        ui->concertWidget->onSaveAll();
    } else if (ui->stackedWidget->currentIndex() == 7) {
        ui->musicWidget->onSaveAll();
    }
    setNewMarks();
}

/**
 * @brief Executes the file scanner dialog
 */
void MainWindow::onActionReload()
{
    if (ui->stackedWidget->currentIndex() == 6) {
        ui->downloadsWidget->scanDownloadFolders();
        return;
    }

    m_fileScannerDialog->setForceReload(true);

    if (ui->stackedWidget->currentIndex() == 0) {
        m_fileScannerDialog->setReloadType(FileScannerDialog::TypeMovies);
    } else if (ui->stackedWidget->currentIndex() == 1) {
        m_fileScannerDialog->setReloadType(FileScannerDialog::TypeTvShows);
    } else if (ui->stackedWidget->currentIndex() == 3) {
        m_fileScannerDialog->setReloadType(FileScannerDialog::TypeConcerts);
    } else if (ui->stackedWidget->currentIndex() == 7) {
        m_fileScannerDialog->setReloadType(FileScannerDialog::TypeMusic);
    }

    m_fileScannerDialog->exec();
}

void MainWindow::onActionRename()
{
    if (ui->stackedWidget->currentIndex() == 0) {
        m_renamer->setRenameType(Renamer::RenameType::Movies);
        m_renamer->setMovies(ui->filesWidget->selectedMovies());
    } else if (ui->stackedWidget->currentIndex() == 1) {
        m_renamer->setRenameType(Renamer::RenameType::TvShows);
        m_renamer->setShows(ui->tvShowFilesWidget->selectedShows());
        m_renamer->setEpisodes(ui->tvShowFilesWidget->selectedEpisodes());
    } else if (ui->stackedWidget->currentIndex() == 3) {
        m_renamer->setRenameType(Renamer::RenameType::Concerts);
        m_renamer->setConcerts(ui->concertFilesWidget->selectedConcerts());
    } else {
        return;
    }
    m_renamer->exec();
}

/**
 * @brief Called when the filter text was changed or a filter was added/removed
 * Delegates the event down to the current subwidget
 * @param filters List of filters
 * @param text Filter text
 */
void MainWindow::onFilterChanged(QList<Filter *> filters, QString text)
{
    if (ui->stackedWidget->currentIndex() == 0) {
        ui->filesWidget->setFilter(filters, text);
    } else if (ui->stackedWidget->currentIndex() == 1) {
        ui->tvShowFilesWidget->setFilter(filters, text);
    } else if (ui->stackedWidget->currentIndex() == 3) {
        ui->concertFilesWidget->setFilter(filters, text);
    } else if (ui->stackedWidget->currentIndex() == 7) {
        ui->musicFilesWidget->setFilter(filters, text);
    }
}

/**
 * @brief Sets the status of the save and save all action
 * @param enabled Status
 * @param widget Widget to set the status for
 */
void MainWindow::onSetSaveEnabled(bool enabled, MainWidgets widget)
{
    qDebug() << "Entered, enabled=" << enabled;

    m_actions[widget][MainActions::Save] = enabled;

    if (widget != MainWidgets::MovieSets && widget != MainWidgets::Certifications) {
        m_actions[widget][MainActions::SaveAll] = enabled;
        if (widget != MainWidgets::Music) {
            m_actions[widget][MainActions::Rename] = enabled;
        }
    }

    if ((widget == MainWidgets::Movies && ui->stackedWidget->currentIndex() == 0)
        || (widget == MainWidgets::TvShows && ui->stackedWidget->currentIndex() == 1)
        || (widget == MainWidgets::Music && ui->stackedWidget->currentIndex() == 7)
        || (widget == MainWidgets::Concerts && ui->stackedWidget->currentIndex() == 3)) {
        ui->navbar->setActionSaveEnabled(enabled);
        ui->navbar->setActionSaveAllEnabled(enabled);
        if (widget != MainWidgets::Concerts && widget != MainWidgets::Music) {
            ui->navbar->setActionRenameEnabled(enabled);
        }
    }
    if ((widget == MainWidgets::MovieSets && ui->stackedWidget->currentIndex() == 2)
        || (widget == MainWidgets::Certifications && ui->stackedWidget->currentIndex() == 5)
        || (widget == MainWidgets::Genres && ui->stackedWidget->currentIndex() == 4)) {
        ui->navbar->setActionSaveEnabled(enabled);
    }
}

/**
 * @brief Sets the status of the search action
 * @param enabled Status
 * @param widget Widget to set the status for
 */
void MainWindow::onSetSearchEnabled(bool enabled, MainWidgets widget)
{
    qDebug() << "Entered, enabled=" << enabled;
    m_actions[widget][MainActions::Search] = enabled;

    if ((widget == MainWidgets::Movies && ui->stackedWidget->currentIndex() == 0)
        || (widget == MainWidgets::TvShows && ui->stackedWidget->currentIndex() == 1)
        || (widget == MainWidgets::Concerts && ui->stackedWidget->currentIndex() == 3)
        || (widget == MainWidgets::Music && ui->stackedWidget->currentIndex() == 7)) {
        ui->navbar->setActionSearchEnabled(enabled);
    }
}

/**
 * @brief Moves all splitters
 * @param pos
 * @param index
 */
void MainWindow::moveSplitter(int pos, int index)
{
    Q_UNUSED(index)
    QList<int> sizes;
    QList<QSplitter *> splitters;
    splitters << ui->movieSplitter << ui->tvShowSplitter << ui->setsWidget->splitter() << ui->genreWidget->splitter()
              << ui->certificationWidget->splitter() << ui->concertSplitter << ui->musicSplitter;
    foreach (QSplitter *splitter, splitters) {
        if (splitter->sizes().at(0) == pos) {
            sizes = splitter->sizes();
            break;
        }
    }

    foreach (QSplitter *splitter, splitters)
        splitter->setSizes(sizes);

    Manager::instance()->movieModel()->update();
    Manager::instance()->concertModel()->update();
}

/**
 * @brief Sets or removes the new mark in the main menu on the left
 */
void MainWindow::setNewMarks()
{
    int newMovies = Manager::instance()->movieModel()->countNewMovies();
    int newShows = Manager::instance()->tvShowModel()->hasNewShowOrEpisode();
    int newConcerts = Manager::instance()->concertModel()->countNewConcerts();
    int newMusic = Manager::instance()->musicModel()->hasNewArtistsOrAlbums();
    int newDownloads = ui->downloadsWidget->hasNewItems();

    ui->buttonMovies->setIcon(Manager::instance()->iconFont()->icon(ui->buttonMovies->property("iconName").toString(),
        (ui->buttonMovies->property("isActive").toBool()) ? m_buttonActiveColor : m_buttonColor,
        (newMovies > 0) ? "star" : "",
        newMovies));

    ui->buttonTvshows->setIcon(Manager::instance()->iconFont()->icon(ui->buttonTvshows->property("iconName").toString(),
        (ui->buttonTvshows->property("isActive").toBool()) ? m_buttonActiveColor : m_buttonColor,
        (newShows > 0) ? "star" : "",
        newShows));

    ui->buttonConcerts->setIcon(
        Manager::instance()->iconFont()->icon(ui->buttonConcerts->property("iconName").toString(),
            (ui->buttonConcerts->property("isActive").toBool()) ? m_buttonActiveColor : m_buttonColor,
            (newConcerts > 0) ? "star" : "",
            newConcerts));

    ui->buttonMusic->setIcon(Manager::instance()->iconFont()->icon(ui->buttonMusic->property("iconName").toString(),
        (ui->buttonMusic->property("isActive").toBool()) ? m_buttonActiveColor : m_buttonColor,
        (newMusic > 0) ? "star" : "",
        newMusic));

    ui->buttonDownloads->setIcon(
        Manager::instance()->iconFont()->icon(ui->buttonDownloads->property("iconName").toString(),
            (ui->buttonDownloads->property("isActive").toBool()) ? m_buttonActiveColor : m_buttonColor,
            (newDownloads > 0) ? "star" : "",
            newDownloads));

    ui->filesWidget->setAlphaListData();
    ui->concertFilesWidget->setAlphaListData();
}

void MainWindow::onActionXbmc()
{
    m_xbmcSync->exec();
}

void MainWindow::onTriggerReloadAll()
{
    m_fileScannerDialog->setForceReload(true);
    m_fileScannerDialog->setReloadType(FileScannerDialog::TypeAll);
    m_fileScannerDialog->exec();
}

void MainWindow::onXbmcSyncFinished()
{
    ui->filesWidget->movieSelectedEmitter();
    ui->tvShowFilesWidget->emitLastSelection();
    ui->concertFilesWidget->concertSelectedEmitter();
}

void MainWindow::onFilesRenamed(Renamer::RenameType type)
{
    if (m_renamer->renameErrorOccured()) {
        m_fileScannerDialog->setForceReload(true);
        if (type == Renamer::RenameType::Movies) {
            m_fileScannerDialog->setReloadType(FileScannerDialog::TypeMovies);
        } else if (type == Renamer::RenameType::Concerts) {
            m_fileScannerDialog->setReloadType(FileScannerDialog::TypeConcerts);
        } else if (type == Renamer::RenameType::TvShows) {
            m_fileScannerDialog->setReloadType(FileScannerDialog::TypeTvShows);
        } else if (type == Renamer::RenameType::All) {
            m_fileScannerDialog->setReloadType(FileScannerDialog::TypeAll);
        }
        m_fileScannerDialog->exec();
    } else {
        if (type == Renamer::RenameType::Movies) {
            ui->movieWidget->updateMovieInfo();
        } else if (type == Renamer::RenameType::Concerts) {
            ui->concertWidget->updateConcertInfo();
        } else if (type == Renamer::RenameType::TvShows) {
            ui->tvShowWidget->updateInfo();
        }
    }
}

void MainWindow::onRenewModels()
{
    ui->filesWidget->renewModel();
    ui->tvShowFilesWidget->renewModel();
    ui->concertFilesWidget->renewModel();
    ui->downloadsWidget->scanDownloadFolders();
}

void MainWindow::onJumpToMovie(Movie *movie)
{
    onMenu(ui->buttonMovies);
    ui->filesWidget->selectMovie(movie);
}

void MainWindow::updateTvShows()
{
    foreach (TvShow *show, Manager::instance()->tvShowModel()->tvShows()) {
        if (show->showMissingEpisodes()) {
            TvShowUpdater::instance()->updateShow(show);
        }
    }
}

void MainWindow::onMenu(QToolButton *button)
{
    if (button == nullptr) {
        button = static_cast<QToolButton *>(QObject::sender());
    }

    if (!button) {
        return;
    }


    foreach (QToolButton *btn, ui->menuWidget->findChildren<QToolButton *>()) {
        btn->setIcon(Manager::instance()->iconFont()->icon(btn->property("iconName").toString(), m_buttonColor));
        btn->setProperty("isActive", false);
    }
    button->setIcon(
        Manager::instance()->iconFont()->icon(button->property("iconName").toString(), m_buttonActiveColor));
    button->setProperty("isActive", true);
    setNewMarks();

    int page = button->property("page").toInt();
    ui->stackedWidget->setCurrentIndex(page);

    ui->navbar->setActionReloadEnabled(page == 0 || page == 1 || page == 3 || page == 6 || page == 7);
    MainWidgets widget;
    switch (page) {
    case 0:
        // Movies
        ui->navbar->setReloadToolTip(
            tr("Reload all Movies (%1)").arg(QKeySequence(QKeySequence::Refresh).toString(QKeySequence::NativeText)));
        widget = MainWidgets::Movies;
        break;
    case 1:
        // Tv Shows
        ui->navbar->setReloadToolTip(
            tr("Reload all TV Shows (%1)").arg(QKeySequence(QKeySequence::Refresh).toString(QKeySequence::NativeText)));
        widget = MainWidgets::TvShows;
        break;
    case 2:
        // Movie Sets
        widget = MainWidgets::MovieSets;
        ui->setsWidget->loadSets();
        break;
    case 3:
        // Concerts
        ui->navbar->setReloadToolTip(
            tr("Reload all Concerts (%1)").arg(QKeySequence(QKeySequence::Refresh).toString(QKeySequence::NativeText)));
        widget = MainWidgets::Concerts;
        break;
    case 4:
        // Genres
        widget = MainWidgets::Genres;
        ui->genreWidget->loadGenres();
        break;
    case 5:
        // Certification
        widget = MainWidgets::Certifications;
        ui->certificationWidget->loadCertifications();
        break;
    case 6:
        // Import
        widget = MainWidgets::Downloads;
        ui->navbar->setReloadToolTip(tr("Reload all Downloads (%1)")
                                         .arg(QKeySequence(QKeySequence::Refresh).toString(QKeySequence::NativeText)));
        break;
    case 7:
        // Music
        ui->navbar->setReloadToolTip(
            tr("Reload Music (%1)").arg(QKeySequence(QKeySequence::Refresh).toString(QKeySequence::NativeText)));
        widget = MainWidgets::Music;
        break;
    }
    ui->navbar->setActionSearchEnabled(m_actions[widget][MainActions::Search]);
    ui->navbar->setActionSaveEnabled(m_actions[widget][MainActions::Save]);
    ui->navbar->setActionSaveAllEnabled(m_actions[widget][MainActions::SaveAll]);
    ui->navbar->setActionRenameEnabled(m_actions[widget][MainActions::Rename]);
    ui->navbar->setFilterWidgetEnabled(m_actions[widget][MainActions::FilterWidget]);
    ui->navbar->setActiveWidget(widget);
}
