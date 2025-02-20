// Copyright 2017 Neverware
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "gondarwizard.h"

#include <QTimer>

#include "about_dialog.h"
#include "admin_check_page.h"
#include "chromeover_login_page.h"
#include "device_select_page.h"
#include "error_page.h"
#include "feedback_dialog.h"
#include "log.h"
#include "metric.h"
#include "site_select_page.h"
#include "update_check.h"

class GondarWizard::Private {
 public:
  explicit Private(std::unique_ptr<gondar::DevicePicker>&& picker)
      : deviceSelectPage(std::move(picker)) {}

  gondar::UpdateCheck updateCheck;
  gondar::AboutDialog aboutDialog;
  gondar::FeedbackDialog feedbackDialog;

  AdminCheckPage adminCheckPage;
  DeviceSelectPage deviceSelectPage;
  ChromeoverLoginPage chromeoverLoginPage;
  SiteSelectPage siteSelectPage;
  ErrorPage errorPage;

  std::vector<GondarSite> sites;

  QDateTime runTime;
};

GondarWizard::GondarWizard(std::unique_ptr<gondar::DevicePicker> picker_in,
                           QWidget* parent)
    : QWizard(parent),
      p_(std::make_unique<Private>(std::move(picker_in))),
      about_shortcut_(QKeySequence::HelpContents, this),
      formatOnly(false) {
  init();
}

void GondarWizard::init() {
  // set shared error state to false
  session_error = false;
  // these pages are automatically cleaned up
  // new instances are made whenever navigation moves on to another page
  // according to qt docs
  setPage(Page_adminCheck, &p_->adminCheckPage);
  // chromeoverLogin and imageSelect are alternatives to each other
  // that both progress to usbInsertPage
  setPage(Page_chromeoverLogin, &p_->chromeoverLoginPage);
  setPage(Page_siteSelect, &p_->siteSelectPage);
  setPage(Page_imageSelect, &imageSelectPage);
  setPage(Page_usbInsert, &usbInsertPage);
  setPage(Page_deviceSelect, &p_->deviceSelectPage);
  setPage(Page_downloadProgress, &downloadProgressPage);
  setPage(Page_writeOperation, &writeOperationPage);
  setPage(Page_error, &p_->errorPage);
  setWizardStyle(QWizard::ClassicStyle);
  setWindowTitle(tr("ThoriumOS USB Maker"));
  setPixmap(QWizard::LogoPixmap, QPixmap(":/images/crlogo.png"));

  setButtonText(QWizard::CustomButton1, "Make Another USB");
  setButtonText(QWizard::CustomButton2, "About");
  setButtonText(QWizard::CustomButton3, "Feedback");
  setNormalLayout();
  // remove '?' button that does not do anything in our current setup
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  connect(&about_shortcut_, &QShortcut::activated, &p_->aboutDialog,
          &gondar::AboutDialog::show);
  connect(this, &GondarWizard::customButtonClicked, this,
          &GondarWizard::handleCustomButton);
  // appropriately move to error screen if there is a problem getting
  // latest beerover url
  connect(&newestImageUrl, &NewestImageUrl::errorOccurred, this,
          &GondarWizard::handleNewestImageUrlError);
  // wizard is responsible for this connection as wizard() is not callable
  // at construction time
  connect(&meepo_, &gondar::Meepo::finished, &p_->chromeoverLoginPage,
          &ChromeoverLoginPage::handleMeepoFinished);
  connect(&meepo_, &gondar::Meepo::failed, &p_->chromeoverLoginPage,
          &ChromeoverLoginPage::handleMeepoFailed);

  p_->feedbackDialog.setWizard(this);

  p_->runTime = QDateTime::currentDateTime();
  p_->updateCheck.start(this);

  // resize the window (width, height)
  resize(720, 480);
}

GondarWizard::~GondarWizard() {}

void GondarWizard::setNormalLayout() {
  QList<QWizard::WizardButton> button_layout;
  button_layout << QWizard::CustomButton2 << QWizard::CustomButton3
                << QWizard::Stretch << QWizard::NextButton
                << QWizard::FinishButton;
  setButtonLayout(button_layout);
}

void GondarWizard::setMakeAnotherLayout() {
  QList<QWizard::WizardButton> button_layout;
  button_layout << QWizard::CustomButton2 << QWizard::CustomButton3
                << QWizard::Stretch << QWizard::CustomButton1
                << QWizard::NextButton << QWizard::FinishButton;
  setButtonLayout(button_layout);
}

const std::vector<GondarSite>& GondarWizard::sites() const {
  return p_->sites;
}

void GondarWizard::setSites(const std::vector<GondarSite>& sites) {
  p_->sites = sites;
}

// handle event when 'make another usb' button pressed
void GondarWizard::handleCustomButton(int buttonIndex) {
  if (buttonIndex == QWizard::CustomButton1) {
    setNormalLayout();
    // works as long as usbInsertPage is not the last page in wizard
    setStartId(usbInsertPage.nextId() - 1);
    restart();
  } else if (buttonIndex == QWizard::CustomButton2) {
    p_->aboutDialog.show();
  } else if (buttonIndex == QWizard::CustomButton3) {
    p_->feedbackDialog.show();
  } else {
    LOG_ERROR << "Unknown custom button pressed";
  }
}

int GondarWizard::nextId() const {
  if (p_->errorPage.errorEmpty()) {
    return QWizard::nextId();
  } else {
    if (currentId() == Page_error) {
      return -1;
    } else {
      return Page_error;
    }
  }
}

void GondarWizard::postError(const QString& error) {
  setSessionError(true);
  QTimer::singleShot(0, this, [=]() { catchError(error); });
}

void GondarWizard::catchError(const QString& error) {
  LOG_ERROR << "displaying error: " << error;
  p_->errorPage.setErrorString(error);
  // TODO(kendall): sanitize error string?
  gondar::SendMetric(this, gondar::Metric::Error, error.toStdString());
  next();
}

qint64 GondarWizard::getRunTime() {
  return p_->runTime.secsTo(QDateTime::currentDateTime());
}

bool GondarWizard::getSessionError() {
  return session_error;
}

// set wizard-wide error bool so pages with validate page functions
// can pass through
void GondarWizard::setSessionError(bool error_in) {
  session_error = error_in;
}

// if beerover, start the requisite image fetching
void GondarWizard::maybe_fetch() {
  if (!gondar::isChromeover()) {
    // for beerover, we'll have to check what the latest release is
    newestImageUrl.fetch();
  }
}

bool GondarWizard::newestIsReady() {
  return newestImageUrl.isReady();
}

void GondarWizard::handleNewestImageUrlError() {
  postError("An error has occurred fetching the latest image");
}

int GondarWizard::getSiteId() {
  return meepo_.getSiteId();
}

void GondarWizard::setSiteId(int site_id_in) {
  meepo_.setSiteId(site_id_in);
}
