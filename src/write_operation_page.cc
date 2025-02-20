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

#include "write_operation_page.h"

#include "diskwritethread.h"
#include "gondarwizard.h"
#include "log.h"
#include "metric.h"

WriteOperationPage::WriteOperationPage(QWidget* parent)
    : WizardPage(parent), device(0, std::string(), 0) {
  layout.addWidget(&progress);
  bolded.setObjectName("bolded");
  bolded.setText("<br>What's next?<br>");
  layout.addWidget(&bolded);

  whatsNext.setObjectName("whatsNext");
  whatsNext.setText(
      "<p>You're ready to install ThoriumOS!  Head back to <a "
      "href=\"https://guide.neverware.com/install-cloudready\">the install "
      "guide</a> for help in how to use your USB installer.<br></p><p>Don't "
      "forget to check the 'Details' link for your devices on the <a "
      "href=\"https://guide.neverware.com/supported-devices\">certified models "
      "list</a>.  There may be special install instructions or important notes "
      "for each model.</p>");
  whatsNext.setTextFormat(Qt::RichText);
  whatsNext.setTextInteractionFlags(Qt::TextBrowserInteraction);
  whatsNext.setOpenExternalLinks(true);
  whatsNext.setWordWrap(true);
  layout.addWidget(&whatsNext);
  setLayout(&layout);
}

void WriteOperationPage::setDevice(const DeviceGuy& device_in) {
  device = device_in;
}

void WriteOperationPage::initializePage() {
  // set the titles in initializePage for 'make another' flow
  if (wizard()->isFormatOnly()) {
    setTitle("Formatting your USB");
    setSubTitle("This process should only take a moment.");
  } else {
    setTitle("Creating your ThoriumOS USB installer");
    setSubTitle("This process may take up to 20 minutes.");
  }
  bolded.hide();
  whatsNext.hide();
  writeFinished = false;
  writeToDrive();
}

bool WriteOperationPage::isComplete() const {
  return writeFinished;
}

bool WriteOperationPage::validatePage() {
  return writeFinished;
}

void WriteOperationPage::writeToDrive() {
  LOG_INFO << "Writing to drive...";
  // if we're in format-only mode, we don't need logic about an image file name
  if (wizard()->isFormatOnly()) {
    // make a disk write thread in format mode
    diskWriteThread = new DiskWriteThread(&device, this);
    gondar::SendMetric(wizard(), gondar::Metric::FormatAttempt);
  } else {
    image_path.clear();
    image_path.append(wizard()->downloadProgressPage.getImageFileName());
    diskWriteThread = new DiskWriteThread(&device, image_path, this);
    gondar::SendMetric(wizard(), gondar::Metric::UsbAttempt);
  }
  connect(diskWriteThread, &DiskWriteThread::finished, this,
          &WriteOperationPage::onDoneWriting);
  showProgress();
  LOG_INFO << "launching thread...";
  diskWriteThread->start();
}

void WriteOperationPage::showProgress() {
  progress.setRange(0, 0);
  progress.setValue(0);
}

void WriteOperationPage::showWhatsNext() {
  bolded.setObjectName("bolded");
  bolded.setText("<br>What's next?<br>");
  layout.addWidget(&bolded);

  whatsNext.setObjectName("whatsNext");
  if (wizard()->isFormatOnly()) {
    setTitle("USB has been formatted!");
    setSubTitle("You may now use this USB for file storage.");
    whatsNext.setText(
        "<p>Your USB has been formatted to remove all ThoriumOS installer "
        "data."
        "  You can now proceed to use it for file storage or other "
        "purposes.</p>");
  } else {
    setTitle("ThoriumOS USB created!");
    setSubTitle("You may now either exit or create another USB.");
    whatsNext.setText(
        "<p>You're ready to install ThoriumOS!  Head back to <a "
        "href=\"https://guide.neverware.com/install-cloudready\">the install "
        "guide</a> for help in how to use your USB installer.<br></p><p>Don't "
        "forget to check the 'Details' link for your devices on the <a "
        "href=\"https://guide.neverware.com/supported-devices\">certified "
        "models "
        "list</a>.  There may be special install instructions or important "
        "notes "
        "for each model.</p>");
  }
  whatsNext.setTextFormat(Qt::RichText);
  whatsNext.setTextInteractionFlags(Qt::TextBrowserInteraction);
  whatsNext.setOpenExternalLinks(true);
  whatsNext.setWordWrap(true);
  layout.addWidget(&whatsNext);

  bolded.show();
  whatsNext.show();
}

void WriteOperationPage::onDoneWriting() {
  switch (diskWriteThread->state()) {
    case DiskWriteThread::State::Initial:
    case DiskWriteThread::State::Running:
      // OVER-5124: we reach this state if the user unplugs usb prior to
      // writing, for example during the download segment
      writeFailed("Cannot read USB device; is it still connected?");
      return;

    case DiskWriteThread::State::GetFileSizeFailed:
      writeFailed("Error reading the disk image's file size");
      return;

    case DiskWriteThread::State::InstallFailed:
      writeFailed("Error writing to the USB device");
      return;

    case DiskWriteThread::State::Success:
      // on success, break out to normal onDoneWriting logic
      break;
  }

  showWhatsNext();
  qDebug() << "install call returned";
  writeFinished = true;
  progress.setRange(0, 100);
  progress.setValue(100);
  if (wizard()->isFormatOnly()) {
    gondar::SendMetric(wizard(), gondar::Metric::FormatSuccess);
  } else {
    wizard()->setMakeAnotherLayout();
    gondar::SendMetric(wizard(), gondar::Metric::UsbSuccess);
    // when a USB was successfully created, report time the run took
    gondar::SendMetric(wizard(), gondar::Metric::SuccessDuration,
                       QString::number(wizard()->getRunTime()).toStdString());
  }
  emit completeChanged();
}

// though error page follows in index, this is the end of the wizard for
// healthy flows
int WriteOperationPage::nextId() const {
  return -1;
}

void WriteOperationPage::writeFailed(const QString& errorMessage) {
  wizard()->postError(errorMessage);
  writeFinished = true;
  emit completeChanged();
}
