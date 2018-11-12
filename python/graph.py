from threading import Timer, Thread, Event
import datetime
import matplotlib.pyplot as plt
from drawnow import *


class Graph:
    def __init__(self, port, listen_to, ports, unit, title, timer=1):
        self.temp_cnt = 0
        self.timer = timer
        self.temp_list = []
        self.temp_time = []
        self.timer_active = True
        self.port = port
        self.active = False
        self.listen_to = listen_to
        self.plt = plt
        self.ports = ports
        self.unit = unit
        self.title = title
        # plt.ion()

    def start(self):
        print("starting graph timer at {}".format(self.port.serial.name))
        # self.timer_obj = Timer(self.timer, self.temperature_loop).start()
        self.timer_active = True
        self.temperature_loop()

    def temperature_loop(self):
        print(self.timer_active)
        if self.timer_active:
            # print(self.temp_list, port)
            arduinoString = self.port.read("#" + self.listen_to)
            # print(self, self.port.serial.name, arduinoString)
            # print(self.port.serial.name, arduinoString)
            # print(port.serial.name, self.temp_list)
            s = arduinoString.split('=')

            print('#' + self.listen_to, "==", s[0])
            if s[0] == '#' + self.listen_to:
                temp_number = float(s[1])
                temp_value = float(temp_number/10)
                self.temp_list.append(temp_value)
                # print(self.temp_list)
                # print(len(self.temp_list))
                self.temp_time.append(datetime.datetime.now().strftime("%H:%M:%S"))
                self.temp_cnt = self.temp_cnt + 1
                if self.temp_cnt > 180:
                    print("popping temp list", self.temp_cnt)
                    self.temp_list.pop(0)
                    self.temp_time.pop(0)
            # print(self.timer_active)
            Timer(self.timer, self.temperature_loop).start()
            # if self.active:
            #     print("graph active")
            #     # self.makeFig()
            #     drawnow(self.makeFig)
            #     plt.pause(.000001)

    def stop(self):
        print("canceling graph timer at {}".format(self.port.serial.name))
        self.timer_active = False

    def makeFig(self):

        # for port in self.ports.keys():
            # print('mijn leven {}'.format(port[3:]))
        print("OPENING GRAPH FOR {}".format(self.port.serial.name))
        # self.plt.figure(port[3:])
        # self.plt.ylim(-50, 50)
        ax = self.plt.gca()
        ax.autoscale_view()
        self.plt.gcf().autofmt_xdate()
        self.plt.title(self.title.format(self.temp_list[-1]))
        self.plt.grid(True)
        self.plt.ylabel(self.unit)
        print(self.unit, self.title)
        self.plt.xlabel('Tijd')
        self.plt.plot(self.temp_time, self.temp_list, 'r-', label=self.unit)
        self.plt.legend(loc='upper left')
        self.plt.show()

    def show(self):
        print(self.temp_list)
        if len(self.temp_list) > 0:
            self.active = True
            print("STARTING GRAPH FOR {}".format(self.port.serial.name))
            drawnow(self.makeFig)
            # self.makeFig()
            self.plt.pause(.000001)
