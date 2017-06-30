#ifndef MQTTRECEIVER_H
#define MQTTRECEIVER_H

namespace mqttreceiver
{

class callback;

class State
{
    std::string name;
    std::vector<std::string> domain;

public:
    State(std::string name, std::vector<std::string> domain);
    std::string getName();
    std::vector<std::string> getDomain();
    bool isValid(std::string value);
};

class MqttReceiver
{

public:
    MqttReceiver(std::string app_name, std::string block, std::string broker_address="localhost",
                 int broker_port=1883, std::string hostname="", int pid=-1);
    virtual ~MqttReceiver();
    void addGoal(std::string goal);
    void addState(State state);
    void start();
    bool isValid(std::string stateName, std::string stateValue);

protected:

private:
    std::vector<State> states;
    std::string id;
    std::string broker_address;
    int broker_port;
    std::string hostname;
    int pid;
    std::string app_name;
    std::string block;
    mqtt::async_client* client;
    mqtt::connect_options* connOpts;
    callback* cb;
    std::string get_mean_topic();

};

}
#endif // MQTTRECEIVER_H
