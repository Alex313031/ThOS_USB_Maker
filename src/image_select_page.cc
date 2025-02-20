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

#include "image_select_page.h"

#include <QMessageBox>
#include <QPushButton>

#include "gondarimage.h"
#include "gondarwizard.h"
#include "log.h"
#include "util.h"

class DownloadButton : public QRadioButton {
 public:
  explicit DownloadButton(QUrl& url_in) : url(url_in) {}
  QUrl getUrl() const { return url; }

 private:
  QUrl url;
};

ImageSelectPage::ImageSelectPage(QWidget* parent) : WizardPage(parent) {
  setTitle("Which version of ThoriumOS do you need?");
  setSubTitle(" ");
  setLayout(&layout);
}

bool ImageSelectPage::validatePage() {
  // currently this is only a concern in the chromeover case, but we would
  // be equally worried were this true in either case
  if (!bitnessButtons.checkedButton()) {
    return false;
  }
  return true;
}

int ImageSelectPage::nextId() const {
  return GondarWizard::Page_usbInsert;
}

void ImageSelectPage::addImage(GondarImage image) {
  // do not list deployable images
  if (image.isDeployable()) {
    return;
  }
  DownloadButton* newButton = new DownloadButton(image.url);
  newButton->setText(image.getCompositeName());
  bitnessButtons.addButton(newButton);
  layout.addWidget(newButton);
}

void ImageSelectPage::addImages(QList<GondarImage> images) {
  for (const auto& curImage : images) {
    addImage(curImage);
  }
}

// this is what is used later in the wizard to find what url should be used
QUrl ImageSelectPage::getUrl() {
  QAbstractButton* selected = bitnessButtons.checkedButton();
  if (gondar::isChromeover()) {
    // for chromeover, use the download button's url attribute
    DownloadButton* selected_download_button =
        dynamic_cast<DownloadButton*>(selected);
    return selected_download_button->getUrl();
  } else {
    return wizard()->newestImageUrl.get64Url();
  }
}
