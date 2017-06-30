#include "GSList.h"

#include <string>
#include <vector>

namespace gslist
{
GSList::GSList(void)
{
    // TODO this must be replaced with a margot call that tells the goals and states
    goals = {"my_execution_time_goal", "my_error_goal"};
    states = { "my_state_minimize_error", "my_state_minimize_exec_time"};
}

std::vector<std::string> GSList::getGoals()
{
    return goals;
}

std::vector<std::string> GSList::getStates()
{
    return states;
}

}
