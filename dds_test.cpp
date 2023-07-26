
#include "ReaderListener.hpp"
#include <MessageTypeSupportImpl.h>
#include <cstdlib>
#include <dds/DCPS/Discovery.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/debug.h>
#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsDataReaderSeqC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsTopicC.h>
#include <future>
#include <memory>
#include <stdexcept>

struct PubData {
  DDS::Publisher_var publisher;
  DDS::DataWriter_var writer;
  Testing::MessageDataWriter_var message_writer;
};

struct SubData {
  DDS::Subscriber_var publisher;
  DDS::DataReader_var reader;
  Testing::MessageDataReader_var message_reader;
  std::unique_ptr<ReaderListener> listener;
};

PubData create_publisher(const DDS::DomainParticipant_var &dp,
                         const DDS::Topic_var &topic);
SubData create_subsciber(const DDS::DomainParticipant_var &dp,
                         const DDS::Topic_var &topic);

void wait_for_subscriber(const DDS::DataWriter_var &writer);

int main() {
  TheServiceParticipant->set_default_discovery(
      OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory->get_instance();
  OpenDDS::DCPS::log_level = OpenDDS::DCPS::LogLevel::Debug;

  // Participant
  DDS::DomainParticipant_var participant = dpf->create_participant(
      42, PARTICIPANT_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!participant)
    throw std::runtime_error("Failed to create participant");

  // Topic
  Testing::MessageTypeSupport_var ts = new Testing::MessageTypeSupportImpl;
  if (ts->register_type(participant, "") != DDS::RETCODE_OK)
    throw std::runtime_error("Failed to register type");

  auto type_name = ts->get_type_name();

  DDS::Topic_var topic =
      participant->create_topic("Messages", type_name, TOPIC_QOS_DEFAULT,
                                nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!topic)
    throw std::runtime_error("Failed to create topic");

  auto sub_ = create_subsciber(participant, topic);

  std::promise<bool> gotData;
  sub_.listener->SetDataCallback([&gotData](const Testing::Message &) {
    std::cout << "Callback 1" << std::endl;
    gotData.set_value(true);
  });

  std::promise<bool> data2;
  auto sub2 = create_subsciber(participant, topic);
  sub2.listener->SetDataCallback([&data2](const Testing::Message &) {
    std::cout << "Callback 2" << std::endl;
    data2.set_value(true);
  });

  {
    auto pub_ = create_publisher(participant, topic);
    wait_for_subscriber(pub_.writer);

    // Send Message
    Testing::Message msg;
    msg.id(0);
    msg.msg("Hello World");
    pub_.message_writer->write(msg, DDS::HANDLE_NIL);
  }

  gotData.get_future().get();
  data2.get_future().get();

  // Create second subscriber

  participant->delete_contained_entities();
  dpf->delete_participant(participant);

  TheServiceParticipant->shutdown();

  return EXIT_SUCCESS;
}

inline PubData create_publisher(const DDS::DomainParticipant_var &dp,
                                const DDS::Topic_var &topic) {
  DDS::Publisher_var publisher = dp->create_publisher(
      PUBLISHER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher)
    throw std::runtime_error("Failed to create publisher");

  DDS::DataWriterQos qos = DATAWRITER_QOS_DEFAULT;
  qos.history.kind = DDS::HistoryQosPolicyKind::KEEP_LAST_HISTORY_QOS;
  qos.history.depth = 1;
  qos.durability.kind = DDS::DurabilityQosPolicyKind::TRANSIENT_DURABILITY_QOS;
  qos.liveliness.kind = DDS::LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS;

  DDS::DataWriter_var writer = publisher->create_datawriter(
      topic, qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!writer)
    throw std::runtime_error("Failed to create writer");

  Testing::MessageDataWriter_var message_writer =
      Testing::MessageDataWriter::_narrow(writer);
  if (!message_writer)
    throw std::runtime_error("Failed to narrow writer");

  return {publisher, writer, message_writer};
}

inline SubData create_subsciber(const DDS::DomainParticipant_var &dp,
                                const DDS::Topic_var &topic) {
  DDS::Subscriber_var subscriber = dp->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!subscriber)
    throw std::runtime_error("Failed to create subscriber");

  auto listener = std::make_unique<ReaderListener>();

  DDS::DataReaderQos qos = DATAREADER_QOS_DEFAULT;
  qos.durability.kind = DDS::DurabilityQosPolicyKind::TRANSIENT_DURABILITY_QOS;
  qos.liveliness.kind = DDS::LivelinessQosPolicyKind::AUTOMATIC_LIVELINESS_QOS;

  DDS::DataReader_var reader = subscriber->create_datareader(
      topic, qos, listener.get(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!reader)
    throw std::runtime_error("Failed to create reader");

  Testing::MessageDataReader_var message_reader =
      Testing::MessageDataReader::_narrow(reader);

  return {subscriber, reader, message_reader, std::move(listener)};
}

inline void wait_for_subscriber(const DDS::DataWriter_var &writer) {
  DDS::StatusCondition_var condition = writer->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

  while (true) {
    DDS::PublicationMatchedStatus matches;
    if (writer->get_publication_matched_status(matches) != DDS::RETCODE_OK)
      throw std::runtime_error("get_publication_matched_status failed");

    if (matches.current_count >= 1)
      break;

    DDS::ConditionSeq conditions;
    DDS::Duration_t timeout = {5, 0};
    if (ws->wait(conditions, timeout))
      throw std::runtime_error("Failed to wait");
  }

  ws->detach_condition(condition);
}
