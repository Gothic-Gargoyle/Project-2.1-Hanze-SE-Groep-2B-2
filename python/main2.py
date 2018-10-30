from tkinter import *
from threading import Timer, Thread, Event
import serial
import serial.tools.list_ports

class WindowController:

    def __init__(self, width=800, height=600):

        # TKINTER INIT
        self.width = width
        self.height = height

        self.root = Tk()
        self.tk_init()

        self.ports = {}
        self.connection_loop(1)

        self.root.mainloop()

    def tk_init(self):
        self.root.geometry(str(self.width) + "x" + str(self.height))
        self.root.title("bedieningspaneel")

    def connection_loop(self, timeout):
        ports = self.get_available_ports()

        ports_to_enable = self.get_disabled_ports(ports)
        ports_to_disable = self.get_disconnected_ports(ports)
        ports_to_add = self.get_unused_ports(ports)


        print(self.ports, ports_to_add, ports_to_enable, ports_to_disable)

        self.enable_ports(ports_to_enable)
        self.disable_ports(ports_to_disable)
        self.add_ports(ports_to_add)

        # restart check in timout
        Timer(timeout, lambda: self.connection_loop(timeout)).start()

    # returns ports that are known but disabled
    def get_disabled_ports(self, ports):
        disables_ports = []
        for port in ports:
            if port in self.ports:                                # if port is used but not active
                if self.ports[port]["is_active"] is False:
                    disables_ports.append(port)

        return disables_ports

    # returns ports that are known and disconnected
    def get_disconnected_ports(self, ports):
        disconnected_ports = []
        for port in self.ports.keys():
            if port not in ports:
                disconnected_ports.append(port)

        return disconnected_ports

    # returns ports that are not known
    def get_unused_ports(self, ports):
        unused_ports = []
        for port in ports:
            if port not in self.ports:
                unused_ports.append(port)

        return unused_ports

    # returns a list of available comports
    def get_available_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        return ports


    # enables known ports
    def enable_ports(self, ports):
        if ports:
            for port in ports:
                self.ports[port]['is_active'] = True

    # disables known ports
    def disable_ports(self, ports):
        if ports:
            for port in ports:
                self.ports[port]['is_active'] = False

    # add ports to list of known ports
    def add_ports(self, ports):
        if ports:
            for port in ports:
                print("adding port {} to list".format(port))
                self.ports[port] = {"is_active": True}
                print(self.ports[port])




controller = WindowController()
