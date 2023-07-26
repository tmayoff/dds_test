#include "ReaderListener.hpp"
#include "MessageTypeSupportC.h"
#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <iostream>
#include <stdexcept>

void ReaderListener::on_data_available(DDS::DataReader_ptr reader) {
  std::cout << "ReaderListener on_data_available" << std::endl;

  Testing::MessageDataReader_var reader_i =
      Testing::MessageDataReader::_narrow(reader);
  if (!reader_i)
    throw std::runtime_error("failed to narrow reader");

  if (dataCallback) {
    Testing::Message msg;
    DDS::SampleInfo info;

    const DDS::ReturnCode_t error = reader_i->take_next_sample(msg, info);
    if (error == DDS::RETCODE_OK) {
      if (info.valid_data)
        dataCallback(msg);
    }
  }
}

void ReaderListener::on_requested_deadline_missed(
    DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus &) {}

void ReaderListener::on_requested_incompatible_qos(
    DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus &) {}

void ReaderListener::on_sample_rejected(DDS::DataReader_ptr,
                                        const DDS::SampleRejectedStatus &) {}

void ReaderListener::on_liveliness_changed(
    DDS::DataReader_ptr, const DDS::LivelinessChangedStatus &) {}

void ReaderListener::on_subscription_matched(
    DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus &) {}

void ReaderListener::on_sample_lost(DDS::DataReader_ptr,
                                    const DDS::SampleLostStatus &) {}
