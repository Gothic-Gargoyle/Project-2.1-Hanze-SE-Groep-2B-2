from threading import Timer, Thread, Event
import datetime


class Graph:
    def __init__(self, port, timer=1, ):
        self.temp_cnt = 0
        self.timer = timer
        self.temp_list = []
        self.temp_time = []
        self.timer_active = True
        self.port = port

    def start(self):
        print("starting graph timer at {}".format(self.port.serial.name))
        # self.timer_obj = Timer(self.timer, self.temperature_loop).start()
        self.timer_active = True
        self.temperature_loop()

    def temperature_loop(self):
        if self.timer_active:
            # print(self.temp_list, port)
            arduinoString = self.port.read("temp")
            print(self, self.port.serial.name, arduinoString)
            # print(port.serial.name, self.temp_list)
            s = arduinoString.split('=')
            if s[0] == '#temp':
                temp_value = float(s[1])
                self.temp_list.append(temp_value)
                self.temp_time.append(datetime.datetime.now().strftime("%H:%M:%S"))
                self.temp_cnt = self.temp_cnt + 1
                if self.temp_cnt > 720:
                    self.temp_list.pop(0)
            print(self.timer_active)
            Timer(self.timer, self.temperature_loop).start()


    def stop(self):
        print("canceling graph timer at {}".format(self.port.serial.name))
        self.timer_active = False

    # takes a list of ports and creates tabs for it.