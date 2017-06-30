#include <margot.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include "mqtt/client.h"
#include "MqttReceiver.h"
#include <chrono>
#include <thread>

/*

 The function that will be managed by margot must already be a tunable function

 */
void do_work (int trials)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(trials));
}

int main()
{
  std::chrono::seconds interval( 9 );

  mqttreceiver::MqttReceiver mr("app1", "foo", "127.0.0.1", 1883);
  mr.start();

	margot::init();
	int trials = 1;
	int repetitions = 10;
	for (int j = 0; j <3 ; j++){
		for (int i = 0; i<repetitions; ++i)
		{
			//check if the configuration is different wrt the previous one
			//NOTE: trials is an output parameter!
			if (margot::foo::update(trials))
			{
				//Writing the "trials" variable is enough to change configuration
				margot::foo::manager.configuration_applied();
			}
			//monitors wrap the autotuned function
			margot::foo::start_monitor();
			do_work(trials);
			margot::foo::stop_monitor();
			//print values to file
			margot::foo::log();
			// I sleep so I have the time to modify goals and states from the sender
			std::this_thread::sleep_for( interval );
		}
	}


}
