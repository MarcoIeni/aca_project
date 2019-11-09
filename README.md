# Advanced Computer Architectures Project
Project for Advanced Computer Architecture course of M.Sc. Computer Science and Engineering at Politecnico di Milano. It consists in using MQTT to modify extra-functional requirements in adaptive application.

Make sure to read the presentation first. To do it you can download the `presentation` directory and open the `presentation/presentation.html` file in your browser or simply click [here](https://marcoieni.github.io/aca_project/)

## Installation

### Sender
The sender only depends on the external library [paho-mqtt](https://pypi.python.org/pypi/paho-mqtt/), so to install it simply use:
```shell
$ pip install paho-mqtt
```

Then to run the sender:
```shell
$ python shell.py
```

### Receiver

#### GSList
The GS list lib does not depend on external libraries, just make sure to follow the C++11 ISO language standard when you compile it [-std=c++11].

#### MQTTReceiver
The MqttReceiver depends on the following libraries:
* [paho.mqtt.cpp](https://github.com/eclipse/paho.mqtt.cpp)
* GSList
* [margot_heel_if](https://gitlab.com/margot_project/core/tree/master/margot_heel/margot_heel_if)
* [margot_asrtm](https://gitlab.com/margot_project/core/tree/master/framework/asrtm)
* [margot_monitor](https://gitlab.com/margot_project/core/tree/master/framework/monitor)

You can include them in this order. Also, make sure to follow the C++11 ISO language standard when you compile it [-std=c++11].

#### mARGOt example
The `code/receiver/margot` directory contains some files that you have to substitute to the ones contained in the [tutorial](https://gitlab.com/margot_project/tutorial) of the [mARGOt project](https://gitlab.com/margot_project).

So you have to substitute the `code/receiver/margot/config` directory with the [config directory](https://gitlab.com/margot_project/tutorial/tree/master/config) of the tutorial and the `code/receiver/margot/src/main.cpp` file with the [main file](https://gitlab.com/margot_project/tutorial/blob/master/src/main.cpp) of the tutorial.

After you do this, you have to edit the [CMake file](https://gitlab.com/margot_project/tutorial/blob/master/CMakeLists.txt) in order to include the following libraries:
* MQTTReceiver
* GSList
* [paho.mqtt.cpp](https://github.com/eclipse/paho.mqtt.cpp)

You can include them in this order.

After that, follow the [tutorial instructions](https://gitlab.com/margot_project/tutorial/blob/master/README.md) to correctly execute the example.
