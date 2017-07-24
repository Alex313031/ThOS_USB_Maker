// Copyright 2017 Neverware

#include "device_picker.h"

#include <QDebug>
#include "deviceguy.h"
#include "log.h"
namespace gondar {

constexpr int kIdRole = Qt::UserRole + 1;

DevicePicker::DevicePicker() {
  layout_.addWidget(&list_view_);
  layout_.setContentsMargins(0, 0, 0, 0);
  setLayout(&layout_);

  list_view_.setModel(&model_);
  list_view_.setStyleSheet("::item { height: 3em; }");

  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &DevicePicker::selectionChanged);
}

bool DevicePicker::hasSelection() {
  return selectionModel()->hasSelection();
}

void DevicePicker::refresh() {
  const auto devices = Device::sortedDevices();

  // Remove rows for any devices that are no longer present
  for (int row = 0; row < model_.rowCount(); row++) {
    try {
      const auto rowDevice = deviceFromRow(row);
      if (!std::binary_search(devices.begin(), devices.end(), rowDevice)) {
        qDebug() << "remove" << rowDevice.name().c_str()
                 << rowDevice.id().value();
        model_.removeRow(row);
        row--;
      }
    } catch (const std::exception& err) {
      LOG_ERROR << err.what();
    }
  }

  // Add any new devices to the end of the model (to keep order
  // stable)
  for (const auto& device : devices) {
    if (!containsDevice(device)) {
      auto* item = new QStandardItem(QString::fromStdString(device.name()));
      item->setData(device.id().value(), kIdRole);
      model_.appendRow(item);
      qDebug() << "add" << device.name().c_str() << device.id().value();
    }
  }

  // TODO(nicholasbishop): check that selection is stable in the above
  // loops

  ensureSomethingSelected();
}

Device DevicePicker::deviceFromRow(const int row) const {
  if (const auto item = model_.item(row)) {
    const auto variant = item->data(kIdRole);
    bool ok = false;
    const auto id = variant.toInt(&ok);
    if (ok) {
      return Device(item->text().toStdString(), Device::Id(id));
    }
  }

  throw std::runtime_error("invalid row");
}

bool DevicePicker::containsDevice(const Device& device) const {
  for (int row = 0; row < model_.rowCount(); row++) {
    try {
      const auto rowDevice = deviceFromRow(row);
      if (rowDevice == device) {
        return true;
      }
    } catch (const std::exception& err) {
      LOG_ERROR << err.what();
    }
  }

  return false;
}

QItemSelectionModel* DevicePicker::selectionModel() {
  return list_view_.selectionModel();
}

void DevicePicker::selectionChanged(const QItemSelection&,
                                    const QItemSelection&) {
  ensureSomethingSelected();
}

void DevicePicker::ensureSomethingSelected() {
  if (!hasSelection() && model_.rowCount() >= 1) {
    selectionModel()->select(model_.item(0)->index(),
                             QItemSelectionModel::ClearAndSelect);
  }
}

}  // namespace gondar
