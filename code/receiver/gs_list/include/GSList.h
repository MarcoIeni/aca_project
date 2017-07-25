#ifndef GSLIST_H
#define GSLIST_H

#include <string>
#include <vector>
#include <margot.hpp>

// this is a wrapper that will tell the MqttReceiver to which states and goals to subscribe

namespace gslist
{

class GSList
{
    std::vector<std::string> states;
    std::map<std::string, margot::goal_t*> goals;

public:
    GSList();
    std::map<std::string, margot::goal_t*> getGoals();
    std::vector<std::string> getStates();

};

}
#endif // GSLIST_H
