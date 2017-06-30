import paho.mqtt.client as mqtt
import socket
import os
import logging

from loop_thread import LoopThread

DISCONNECT_TOPIC = "disconnect"
STATE_SEPARATOR = ";"
STATE = "state"
GOAL = "goal"


class State:
    def __init__(self, name, domain):
        self.name = name
        self.domain = domain.split(STATE_SEPARATOR)  # possible values of the state

    def is_valid(self, value):
        """
        checks if a value is contained in the domain, i.e. in the possible values of the state
        :param value: the value that you want to check
        :return: true if the value is valid, false otherwise
        """
        if value in self.domain:
            return True
        else:
            return False

    def __str__(self):
        return str(self.__dict__)

    def __repr__(self):
        return self.__str__()


class Topic:
    """
    A generalization of an MQTT topic.
    
    Every topic can have subtopics (for example if you have hostname/pid,
    you will an object Topic with the name "hostname" and with a subtopic
    named "pid".
    """

    def __init__(self, name):
        self.name = name
        self.goals = set()
        self.states = set()
        self.subtopics = set()

    def add_goal(self, goal):
        self.goals.add(goal)

    def add_state(self, state):
        self.states.add(state)

    def add_subtopic(self, subtopic):
        self.subtopics.add(subtopic)

    def get_subtopic(self, subtopic_name):
        """
        given the name of the subtopic, it returns the topic object with that name,
        recursively searched among all the subtopics
        :param subtopic_name: the name of the subtopic that you want to get
        :return: the topic with the given name
        """
        for s in self.subtopics:
            if s.name == subtopic_name:
                return s
        return None

    def get_state(self, state_name):
        for s in self.states:
            if s.name == state_name:
                return s
        return None

    def get_valid_topics(self):
        """
        It adds to the topics list all the topics that contains some goals or states.
        :return: 
        """
        topics = []
        self.__get_valid_topics(topics)
        return topics

    def __get_valid_topics(self, topics, prev=""):
        if self.goals or self.states:
            topics.append(prev + self.name + "/")
        else:
            for t in self.subtopics:
                t.__get_valid_topics(topics, prev + self.name + "/")

    def __str__(self, level=0):
        ret = "\t" * level + self.name
        if self.goals:
            ret += " - goals: " + str(self.goals)
        if self.states:
            ret += " - states: " + str(self.states)
        ret += "\n"
        for t in self.subtopics:
            ret += t.__str__(level + 1)
        return ret


class Sender:
    def __init__(self, broker_address="localhost", broker_port=1883, id=""):
        self.broker_address = broker_address
        self.broker_port = broker_port
        if id == "":
            self.id = socket.gethostname() + ":" + str(os.getpid())
        else:
            self.id=id
        self.topics = set()
        self.mqttc = mqtt.Client(self.id)  # create new instance
        self.mqttc.on_connect = on_connect  # attach function to callback
        self.mqttc.on_message = self.on_message  # attach function to callback
        self.loop_thread = LoopThread(self.mqttc)

    def start(self):
        self.mqttc.connect(self.broker_address, self.broker_port)  # connect to broker
        self.loop_thread.start()

    def get_topic(self, topic_name):
        for t in self.topics:
            if t.name == topic_name:
                return t
        return None

    def del_topic(self, topic_name):
        """
        It deletes the topic with the given name and all the relatives subtopics.
        :param topic_name: the name of the topic that you want to delete
        :return: 
        """
        for t in self.topics:
            if t.name == topic_name:
                self.topics.discard(t)
                return

    def subscribe(self, topic):
        self.mqttc.subscribe(topic)

    def is_goal_valid(self, topic, goal):
        last_topic = self.get_last_topic(topic)
        if not last_topic:
            return False
        if goal in last_topic.goals:
            return True
        else:
            return False

    def is_state_valid(self, topic, state, value):
        last_topic = self.get_last_topic(topic)
        if not last_topic:
            return False
        existing_state = last_topic.get_state(state)
        if existing_state and existing_state.is_valid(value):
            return True
        else:
            return False

    def get_last_topic(self, topic_string):
        topics = topic_string.split('/')
        existing_topic = self.get_topic(topics[0])
        if existing_topic:
            for t in topics[1:]:
                existing_topic = existing_topic.get_subtopic(t)
                if not existing_topic:
                    return None
        else:
            return None
        return existing_topic

    def publish_goal(self, topic, goal, value):
        if self.is_goal_valid(topic, goal):
            full_topic = "sender/" + topic + "/" + GOAL + "/" + goal
            self.mqttc.publish(full_topic, value)
        else:
            print("goal not valid")

    def publish_state(self, topic, state, value):
        if self.is_state_valid(topic, state, value):
            full_topic = "sender/" + topic + "/" + STATE + "/" + state
            self.mqttc.publish(full_topic, value)
        else:
            print("state not valid")

    def manage_disconnect(self, hostname):
        topic = self.get_topic(hostname)
        # I remove disconnect retain message sent from client
        self.mqttc.publish("client/disconnect/" + hostname, payload=None, qos=1, retain=True)
        if topic:
            valid_topics = topic.get_valid_topics()
            # I remove the previous retained messages by sending
            # an empty message to the same topics
            for t in valid_topics:
                topics_name = [t + GOAL, t + STATE]
                for n in topics_name:
                    self.mqttc.publish("client/" + n, payload=None, qos=1, retain=True)
            self.del_topic(hostname)

    def on_message(self, mqttc, obj, msg):
        """
        it is called everytime a message is received, i.e. a message is published in the "client/" topic
        :param mqttc: unused
        :param obj: unused
        :param msg: the received message, it contains both the topic and the payload
        :return: 
        """
        payload = str(msg.payload.decode("utf-8"))
        logging.debug("message received: " + msg.topic + " - " + payload)
        if not payload:
            # message empty, maybe it was a message sent to delete by the sender
            # to delete a retained message sent by the client
            return
        topics = msg.topic.split("/")
        if topics[1] == DISCONNECT_TOPIC:
            # the client communicated its disconnect
            self.manage_disconnect(topics[2])
        else:
            existing_topic = self.get_topic(topics[1])
            if not existing_topic:
                # a new client is connected
                new_topic = Topic(topics[1])
                self.topics.add(new_topic)
                existing_topic = new_topic
            for t in topics[2:-1]:
                # I build the topic recursive structure
                new_topic = existing_topic.get_subtopic(t)
                if new_topic:
                    existing_topic = new_topic
                else:
                    new_topic = Topic(t)
                    existing_topic.add_subtopic(new_topic)
                    existing_topic = new_topic
            if topics[-1] == STATE:
                state_domain = payload.split(STATE_SEPARATOR, 1)
                state_name = state_domain[0]
                domain = state_domain[1]
                state = State(state_name, domain)
                new_topic.add_state(state)
            elif topics[-1] == GOAL:
                new_topic.add_goal(payload)

    def stop(self):
        self.loop_thread.stop()


def on_connect(client, userdata, flags, rc):
    m = "Connected flags" + str(flags) + "result code " \
        + str(rc) + "client1_id  " + str(client)
    logging.debug(m)

# logger configuration
logging.basicConfig(filename='sender.log',level=logging.DEBUG, format='%(asctime)s - %(levelname)s: %(message)s')
