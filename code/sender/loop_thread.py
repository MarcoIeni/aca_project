import threading


class LoopThread(threading.Thread):
    """
    It keeps alive the server
    """
    def __init__(self, mqttc):
        threading.Thread.__init__(self)
        self.mqttc = mqttc
        self.stopped = False

    def run(self):
        while not self.stopped:
            self.mqttc.loop()

    def stop(self):
        self.mqttc.disconnect()
        self.stopped = True
