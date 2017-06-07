
#include <QtWidgets>
#include <QNetworkReply>
#include <QProgressBar>

#include "gondarwizard.h"
#include "downloader.h"

#include "gondar.h"
#include "deviceguy.h"
#include "neverware_unzipper.h"

DeviceGuyList * drivelist = NULL;
DeviceGuy * selected_drive = NULL;

bool writeFinished = false;

GondarButton::GondarButton(const QString & text,
                           unsigned int device_num,
                           QWidget *parent)
                           : QRadioButton(text, parent) {
    index = device_num;
    
}
GondarWizard::GondarWizard(QWidget *parent)
    : QWizard(parent)
{
    // these pages are automatically cleaned up
    // new instances are made whenever navigation moves on to another page
    // according to qt docs
    addPage(new AdminCheckPage);
    addPage(new ImageSelectPage);
    addPage(new DownloadProgressPage);
    addPage(new UsbInsertPage);
    addPage(new DeviceSelectPage);
    addPage(new KewlPage);
    setWizardStyle(QWizard::ModernStyle);
    setWindowTitle(tr("Cloudready USB Creation Utility"));
}

AdminCheckPage::AdminCheckPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Insert USB Drive"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/frogmariachis.png"));

    label.setText("Please wait...");
    is_admin = false; // assume false until we discover otherwise.
                      // this holds the user at this screen

    layout.addWidget(& label);
    setLayout(& layout);

    // the next button is grayed out if user does not have appropriate rights
    QObject::connect(this, SIGNAL(isAdminRequested()),
                     this, SLOT(getIsAdmin()));
    QObject::connect(this, SIGNAL(isAdminReady()),
                     this, SLOT(showIsAdmin()));
    QObject::connect(this, SIGNAL(isNotAdminReady()),
                     this, SLOT(showIsNotAdmin()));
}

void AdminCheckPage::initializePage() {
    tim = new QTimer(this);
    connect(tim, SIGNAL(timeout()), SLOT(getIsAdmin()));
    emit isAdminRequested();
}

bool AdminCheckPage::isComplete() const {
    return is_admin;
}

void AdminCheckPage::getIsAdmin() {
    qDebug() << "kendall: getIsAdmin fires";
    is_admin = IsCurrentProcessElevated();
    if (!is_admin) {
        emit isNotAdminReady();
        tim->stop();
    } else {
        tim->stop();
        emit isAdminReady();
    }
}

void AdminCheckPage::showIsAdmin() {
    label.setText("User has admin rights.");
    emit completeChanged();
}

void AdminCheckPage::showIsNotAdmin() {
    label.setText("User does not have admin rights.");
}

ImageSelectPage::ImageSelectPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Select Remote Image"));
}

void ImageSelectPage::initializePage() {
    label.setText("Target image url:");
    //TODO(kendall): make required
    registerField("imageurl", & urlLineEdit);
    layout.addWidget(& label);
    layout.addWidget(& urlLineEdit);
    setLayout(& layout);
}

DownloadProgressPage::DownloadProgressPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Downloading..."));
    download_finished = false;
    layout.addWidget(& progress);
    setLayout(& layout);
    range_set = false;
    url = NULL;
}

void DownloadProgressPage::initializePage() {
    label.setText("Downloading...");
    layout.addWidget(& label);
    setLayout(& layout);
    qDebug() << "url = " << field("imageurl");
    QObject::connect(&manager, SIGNAL(finished()), this, SLOT(markComplete()));
    manager.append(field("imageurl").toString());
    QObject::connect(&manager, SIGNAL(started()), this, SLOT(onDownloadStarted()));
}

void DownloadProgressPage::onDownloadStarted() {
    QNetworkReply * cur_download = manager.getCurrentDownload();
    QObject::connect(cur_download,
                     &QNetworkReply::downloadProgress,
                     this,
                     &DownloadProgressPage::downloadProgress);
}

void DownloadProgressPage::downloadProgress(qint64 sofar, qint64 total) {
    if (!range_set) {
        range_set = true;
        progress.setRange(0, total);
    }
    progress.setValue(sofar);
}

void DownloadProgressPage::markComplete() {
    download_finished = true;
    label.setText("Download is complete.");
    // now that the download is finished, let's unzip the build.
    notifyUnzip();
    //TODO(kendall): do the unzip in another thread so progress bar animates
    url = field("imageurl").toString().toStdString().c_str();
    startUnzip();
    // unzip has now completed
    emit completeChanged();
}

void DownloadProgressPage::startUnzip() {
    neverware_unzip(url);
}

void DownloadProgressPage::notifyUnzip() {
    label.setText("Extracting image...");
    // setting range and value to zero results in an 'infinite' progress bar
    progress.setRange(0, 0);
    progress.setValue(0);
}

bool DownloadProgressPage::isComplete() const {
    return download_finished;
}

UsbInsertPage::UsbInsertPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Insert USB Drive"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/frogmariachis.png"));

    label.setText("Please insert the destination USB drive to create a "
                        "USB Cloudready(tm) bootable USB drive.");;
    label.setWordWrap(true);

    layout.addWidget(& label);
    setLayout(& layout);

    // the next button should be grayed out until the user inserts a USB
    QObject::connect(this, SIGNAL(driveListRequested()),
                     this, SLOT(getDriveList()));
    QObject::connect(this, SIGNAL(driveListReady()),
                     this, SLOT(showDriveList()));
}

void UsbInsertPage::initializePage() {
    tim = new QTimer(this);
    connect(tim, SIGNAL(timeout()), SLOT(getDriveList()));
    // send a signal to check for drives
    emit driveListRequested();
}

bool UsbInsertPage::isComplete() const {
    // this should return false unless we have a non-empty result from
    // GetDevices()
    if (drivelist == NULL) {
        return false;
    }
    else {
        return true;
    }
}

void UsbInsertPage::getDriveList() {
    drivelist = GetDeviceList();
    if (DeviceGuyList_length(drivelist) == 0) {
        DeviceGuyList_free(drivelist); 
        drivelist = NULL;
        tim->start(1000);
    } else {
        tim->stop();
        emit driveListReady();
    }
}

void UsbInsertPage::showDriveList() {
    emit completeChanged();
}

DeviceSelectPage::DeviceSelectPage(QWidget *parent)
    : QWizardPage(parent)
{
    // this page should just say 'hi how are you' while it stealthily loads
    // the usb device list.  or it could ask you to insert your device
    setTitle(tr("Select Drive"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/frogmariachis.png"));
}

void DeviceSelectPage::initializePage()
{
    drivesLabel.setText("Select Drive:");
    if (drivelist == NULL) {
        return;
    }
    DeviceGuy * itr = drivelist->head;
    // Line up widgets horizontally
    // use QVBoxLayout for vertically, H for horizontal
    layout.addWidget(& drivesLabel);

    radioGroup = new QButtonGroup();
    // i could extend the button object to also have a secret index
    // then i could look up index later easily
    while (itr != NULL) {
        //FIXME(kendall): clean these up
        GondarButton * curRadio = new GondarButton(itr->name,
                                                    itr->device_num,
                                                    this);
        radioGroup->addButton(curRadio);
        //FIXME(kendall): occassionally get a warning about null pointer
        layout.addWidget(curRadio);
        itr = itr->next;
    }
    setLayout(& layout);
}

bool DeviceSelectPage::validatePage() {
    //TODO(kendall): check for NULL on bad cast
    GondarButton * selected = dynamic_cast<GondarButton *>(radioGroup->checkedButton());
    if (selected == NULL) {
        return false;
    } else {
        unsigned int selected_index = selected->index;
        selected_drive = DeviceGuyList_getByIndex(drivelist, selected_index);
        return true;
    }
}

KewlPage::KewlPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Writing to disk will like, totally wipe your drive, dude."));
    QObject::connect(this, SIGNAL(writeDriveRequested()),
                     this, SLOT(writeToDrive()));
}

void KewlPage::initializePage()
{
}

bool KewlPage::validatePage() {
    if (selected_drive == NULL) {
        qDebug() << "ERROR: no drive selected";
    } else {
        emit writeDriveRequested();
    }
    return writeFinished;
}

void KewlPage::writeToDrive() {
    static bool isWriting = false;
    if (!isWriting) {
        qDebug() << "Writing to drive...";
        isWriting = true;
        char image_path[] = "chromiumos_image.bin";
        Install(selected_drive, image_path);
        qDebug() << "install call returned";
        writeFinished = true;
    }
}
