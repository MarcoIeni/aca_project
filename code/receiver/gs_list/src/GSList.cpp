#include "GSList.h"

#include <string>
#include <vector>
#include <margot.hpp>

namespace gslist
{
GSList::GSList(void)
{
    goals.insert(std::pair<std::string,margot::goal_t*>("my_execution_time_goal",
                                                           &margot::foo::goal::my_execution_time_goal));
    goals.insert(std::pair<std::string,margot::goal_t*>("my_error_goal",
                                                           &margot::foo::goal::my_error_goal));

    states = { "my_state_minimize_error", "my_state_minimize_exec_time"};
}

std::map<std::string,margot::goal_t*> GSList::getGoals()
{
    return goals;
}

std::vector<std::string> GSList::getStates()
{
    return states;
}

}
