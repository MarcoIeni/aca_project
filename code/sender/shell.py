import cmd
from sender import Sender


class SenderShell(cmd.Cmd):
    intro = 'Welcome to the sender shell.   Type help or ? to list commands.\n'
    prompt = '>> '

    def preloop(self):
        self.sender = Sender()
        self.sender.start()
        # I subscribe to the wildcard 'client/#' because this is the topic
        # where the clients will publish everything
        self.sender.subscribe("client/#")

    def do_set_goal(self, arg):
        """Set a goal to a client:  set_goal client1 pid1 app1 block1 goal1 123"""
        arg = arg.split(" ")
        topic = parse_set(arg)
        if topic:
            goal = arg[4]
            value = float(arg[5])
            self.sender.publish_goal(topic, goal, value)

    def do_set_state(self, arg):
        """Set a state to a client:  set_state client1 pid1 app1 block1 state1 state_value"""
        arg = arg.split(" ")
        topic = parse_set(arg)
        if topic:
            state = arg[4]
            value = arg[5]
            self.sender.publish_state(topic, state, value)

    def do_info(self, arg):
        """Get the goals and states of a given client:  info client1"""
        print(self.sender.get_topic(arg))

    def do_list(self, unused):
        """Get the data of all the clients:  list"""
        for t in self.sender.topics:
            print(t)

    def do_exit(self, arg):
        """Stop the sender, close this window and exit:  exit"""
        self.sender.stop()
        print('sender stopped')
        return True


def parse_set(arg):
    """
    get the topic from the input argument, that must be in the format:
    "client pid app block".
    :param arg: the argument that you want to parse
    :return: the topic that corresponds to the argument
    """
    if len(arg) != 6:
        print("*** invalid number of arguments")
        return None
    client = arg[0]
    pid = arg[1]
    app = arg[2]
    block = arg[3]
    topic = client + "/" + pid + "/" + app + "/" + block
    return topic

if __name__ == '__main__':
    SenderShell().cmdloop()
