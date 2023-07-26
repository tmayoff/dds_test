#pragma once

#include "MessageC.h"
#include <dds/DCPS/LocalObject.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <functional>

class ReaderListener
    : virtual public OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  ReaderListener() = default;
  explicit ReaderListener(
      std::function<void(const Testing::Message &)> callback)
      : dataCallback(std::move(callback)) {}

  void SetDataCallback(
      const std::function<void(const Testing::Message &)> &dataCallback_) {
    dataCallback = dataCallback_;
  }

  void on_data_available(DDS::DataReader_ptr reader) override;

  void on_requested_deadline_missed(
      DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus &status) override;

  void on_requested_incompatible_qos(
      DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus &status) override;

  void on_sample_rejected(DDS::DataReader_ptr reader,
                          const DDS::SampleRejectedStatus &status) override;

  void
  on_liveliness_changed(DDS::DataReader_ptr reader,
                        const DDS::LivelinessChangedStatus &status) override;

  void on_subscription_matched(
      DDS::DataReader_ptr reader,
      const DDS::SubscriptionMatchedStatus &status) override;

  void on_sample_lost(DDS::DataReader_ptr reader,
                      const DDS::SampleLostStatus &status) override;

private:
  std::function<void(const Testing::Message &)> dataCallback;
};
