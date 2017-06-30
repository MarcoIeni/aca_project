#ifndef GSLIST_H
#define GSLIST_H

#include <string>
#include <vector>

// this is a wrapper that will tell the MqttReceiver to which states and goals to subscribe

namespace gslist
{

class GSList
{
    std::vector<std::string> states;
    std::vector<std::string> goals;

public:
    GSList();
    std::vector<std::string> getGoals();
    std::vector<std::string> getStates();

};

}
#endif // GSLIST_H
