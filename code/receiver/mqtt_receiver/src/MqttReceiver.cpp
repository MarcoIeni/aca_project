// based on: https://github.com/eclipse/paho.mqtt.cpp/blob/master/src/samples/async_subscribe.cpp

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <sstream>
#include <vector>
#include <mqtt/async_client.h>
#include <MqttReceiver.h>
#include <margot.hpp>
#include "GSList.h"

namespace mqttreceiver
{

const char* DISCONNECT_MESSAGE = "@@DISCONNECT@@";
const std::string DISCONNECT_TOPIC {"disconnect"};
const std::string STATE_SEPARATOR {";"};

const char* LWT_PAYLOAD = "last_will."; // this is the payload that is sent when an hard disconnection occurs

const int QOS = 1;

template <typename Out>
void split1(const std::string& s, char delim, Out result)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    split1(s, delim, std::back_inserter(elems));
    return elems;
}

class action_listener : public virtual mqtt::iaction_listener
{
    std::string name_;

    void on_failure(const mqtt::itoken& tok) override
    {
        std::cout << name_ << " failure";
        if (tok.get_message_id() != 0)
            std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
        std::cout << std::endl;
    }

    void on_success(const mqtt::itoken& tok) override
    {
        std::cout << name_ << " success";
        if (tok.get_message_id() != 0)
            std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
        if (!tok.get_topics().empty())
            std::cout << "\ttoken topic: '" << tok.get_topics()[0] << "', ..." << std::endl;
        std::cout << std::endl;
    }

public:
    action_listener(const std::string& name)  : name_(name)
    {
    }
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener
{
    int nretry_;
    mqtt::async_client& cli_;
    mqtt::connect_options& connOpts_;
    MqttReceiver& mr_;
    action_listener subListener_;

    void reconnect()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        try
        {
            cli_.connect(connOpts_, nullptr, *this);
        }
        catch (const mqtt::exception& exc)
        {
            std::cerr << "Error: " << exc.what() << std::endl;
            exit(1);
        }
    }

    // Re-connection failure
    void on_failure(const mqtt::itoken& tok) override
    {
        std::cout << "Connection failed" << std::endl;
        if (++nretry_ > 5)
            exit(1);
        reconnect();
    }

    // Re-connection success
    void on_success(const mqtt::itoken& tok) override
    {
        std::cout << "\nConnection success" << std::endl;
    }

    void connection_lost(const std::string& cause) override
    {
        std::cout << "\nConnection lost" << std::endl;
        if (!cause.empty())
            std::cout << "\tcause: " << cause << std::endl;

        std::cout << "Reconnecting..." << std::endl;
        nretry_ = 0;
        reconnect();
    }

    // used when a goal is received
    void onGoal(std::string goal, float value)
    {
        std::cout << "onGoal: " << goal << " - " << std::to_string(value) << std::endl;
        // TODO don't hard-code this
        if (goal == "my_execution_time_goal")
        {
            margot::foo::goal::my_execution_time_goal.set(value);
        }
        else if (goal == "my_error_goal")
        {
            margot::foo::goal::my_error_goal.set(value);
        }
    }

    // used when a goal is received
    void onState(std::string state, std::string value)
    {
        std::cout << "onState: " << state << " - " << value << std::endl;
        margot::foo::manager.change_active_state(value);
    }

    void message_arrived(const std::string& topic, mqtt::const_message_ptr msg) override
    {
        std::cout << "Message arrived" << std::endl;
        std::vector<std::string> levels = split(topic, '/');
        std::string type = levels.end()[-2];
        std::string resource = levels.back();
        if (type == "state")
        {
            // I check if the state is valid
            std::string stateValue = msg->to_str();
            if(mr_.isValid(resource, stateValue))
            {
                onState(resource, stateValue);
            }
            else
            {
                std::cerr << "state not valid" << std::endl;
            }
        }
        else   // type == "goal"
        {
            onGoal(resource, std::stof(msg->to_str()));
        }
    }

    void delivery_complete(mqtt::idelivery_token_ptr token) override {}

public:

    void subscribe(std::string topic)
    {
        std::cout << "\nSubscribing to topic '" << topic << std::endl;

        cli_.subscribe(topic, QOS, nullptr, subListener_)->wait_for_completion();
    }

    callback(mqtt::async_client& cli, mqtt::connect_options& connOpts, MqttReceiver& mr):
        nretry_(0), cli_(cli), connOpts_(connOpts), mr_(mr), subListener_("Subscription")
    {
    }
};

/////////////////////////////////////////////////////////////////////////////

MqttReceiver::MqttReceiver(std::string app_name, std::string block, std::string broker_address,
                           int broker_port, std::string hostname, int pid)

{
    this->broker_address = broker_address;
    this->broker_port = broker_port;
    if (hostname.size() == 0)
    {
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX);
        this->hostname = hostname;
    }
    else
    {
        this->hostname = hostname;
    }
    if (pid == -1)
    {
        this->pid = ::getpid();
    }
    else
    {
        this->pid = pid;
    }
    this->id = hostname + ":" + std::to_string(pid);
    this->app_name=app_name;
    this->block=block;
    connOpts = new mqtt::connect_options();
    connOpts->set_keep_alive_interval(20);
    connOpts->set_clean_session(true);
    mqtt::will_options will("client/" + DISCONNECT_TOPIC + "/" + this->hostname, LWT_PAYLOAD, 1, true);
    connOpts->set_will(will);

    std::string full_address = this->broker_address + ":" + std::to_string(this->broker_port);

    client = new mqtt::async_client(full_address, this->id);

    cb = new callback(*client, *connOpts, *this);
    client->set_callback(*cb);
}

MqttReceiver::~MqttReceiver()
{
    try
    {
        std::string disconnect_topic = "client/" + DISCONNECT_TOPIC + "/" + this->hostname;
        client->publish(disconnect_topic, DISCONNECT_MESSAGE, strlen(DISCONNECT_MESSAGE), QOS, true)->wait_for_completion();
        std::cout << "\nDisconnecting from the MQTT server..." << std::flush;
        client->disconnect()->wait_for_completion();
        std::cout << "OK" << std::endl;
    }
    catch (const mqtt::exception& exc)
    {
        std::cerr << "Error: " << exc.what() << std::endl;
    }
}

std::string MqttReceiver::get_mean_topic()
{
    return this->hostname + "/" + std::to_string(this->pid) + "/" + this->app_name + "/" + this->block + "/";
}

void MqttReceiver::addGoal(std::string goal)
{
    std::string subscribeTopic = "sender/" + this->get_mean_topic() + "goal/" + goal;
    std::string publishTopic = "client/" + this->get_mean_topic() + "goal";

    // subscribe
    cb->subscribe(subscribeTopic);

    // publish
    const char* payload = goal.c_str();
    client->publish(publishTopic, payload, strlen(payload), QOS, true)->wait_for_completion();
    std::cout << "published: " << publishTopic << " - " << payload << std::endl;
}

void MqttReceiver::addState(State state)
{
    std::string subscribeTopic = "sender/" + this->get_mean_topic() + "state/" + state.getName();
    std::string publishTopic = "client/" + this->get_mean_topic() + "state";

    // subscribe
    cb->subscribe(subscribeTopic);
    std::string possibleStates="";
    std::cout << state.getDomain().size() << std::endl;

    std::vector<std::string> domain = state.getDomain();
    for (std::vector<std::string>::iterator it = domain.begin() ; it != domain.end(); ++it)
    {
        possibleStates += *it + STATE_SEPARATOR;
    }
    possibleStates.pop_back(); // removes the last char from the string

    // publish
    std::cout << "name: " << state.getName() << " domain: " << possibleStates << std::endl;
    std::string payloadString = state.getName() + STATE_SEPARATOR + possibleStates;
    const char* payload = payloadString.c_str();
    client->publish(publishTopic, payload, strlen(payload), QOS, true)->wait_for_completion();
    std::cout << "published: " << publishTopic << " - " << payload << std::endl;

    states.push_back(state);
}

void MqttReceiver::start()
{
    try
    {
        std::cout << "Connecting to the MQTT server..." << std::flush;
        client->connect(*connOpts, nullptr, *cb)->wait_for_completion();
    }
    catch (const mqtt::exception& exc)
    {
        std::cerr << "\nERROR: Unable to connect to MQTT server: " << this->broker_address << std::endl;
    }

    // I add goals and states
    gslist::GSList gslist;
    std::vector<std::string> states = gslist.getStates();
    mqttreceiver::State *state = new mqttreceiver::State("my_state", states);
    addState(*state);

    std::vector<std::string> goals = gslist.getGoals();
    for(auto elem : goals)
    {
        addGoal(elem);
    }

}

bool MqttReceiver::isValid(std::string stateName, std::string stateValue)
{
    State *matchedState = nullptr;
    for (std::vector<State>::iterator it = states.begin() ; it != states.end(); ++it)
    {
        if (it->getName() == stateName)
        {
            matchedState=&(*it);
            break;
        }
    }
    if(matchedState != nullptr && matchedState->isValid(stateValue))
    {
        return true;
    }
    return false;

}

mqttreceiver::State::State(std::string stateName, std::vector<std::string> stateDomain):
    name(stateName), domain(stateDomain)
{
}

bool mqttreceiver::State::isValid(std::string value)
{
    for (std::vector<std::string>::iterator it = domain.begin() ; it != domain.end(); ++it)
    {
        if(*it == value)
        {
            return true;
        }
    }
    return false;
}

std::string mqttreceiver::State::getName()
{
    return name;
}

std::vector<std::string> State::getDomain()
{
    return domain;
}

}
