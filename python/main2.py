from tkinter import *
from threading import Timer, Thread, Event
import serial
import serial.tools.list_ports
import time


class WindowController:

    def __init__(self, width=800, height=600):

        # TKINTER INIT
        self.width = width
        self.height = height

        self.root = Tk()
        self.tk_init()

        # PORTCONTROLLER
        self.ports = {}
        self.port_controller = PortController(self.ports, callback=self.update, timeout=200)

        self.root.mainloop()

    def tk_init(self):
        self.root.geometry(str(self.width) + "x" + str(self.height))
        self.root.title("bedieningspaneel")

    def update(self):
        for key, port in self.ports.items():
            print(key, port)


class PortController:

    def __init__(self, ports, callback=None, timeout=1000):
        self.ports = ports
        self.connection_loop(timeout / 1000, callback)

    def connection_loop(self, timeout, callback):
        ports = self.get_available_ports()

        ports_to_enable = self.get_disabled_ports(ports)
        self.enable_ports(ports_to_enable)

        ports_to_disable = self.get_disconnected_ports(ports)
        self.disable_ports(ports_to_disable)

        ports_to_add = self.get_unused_ports(ports)
        self.add_ports(ports_to_add)

        # if the port configuration has changed
        if ports_to_enable or ports_to_disable or ports_to_add:
            callback()

        # restart check in timeout
        Timer(timeout, lambda: self.connection_loop(timeout, callback)).start()

    # returns ports that are known but disabled
    def get_disabled_ports(self, ports):
        disables_ports = []
        for port in ports:
            if port in self.ports:                                # if port is used but not active
                if self.ports[port]["is_valid"] and self.ports[port]["is_active"] is False:
                    disables_ports.append(port)

        return disables_ports

    # returns ports that are known, disconnected and not jet registered as disconnected
    def get_disconnected_ports(self, ports):
        disconnected_ports = []
        for port in self.ports.keys():
            if port not in ports:
                if self.ports[port]["is_active"]:
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
                if self.ports[port]["is_valid"]:
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
                ser = serial.Serial(port, 9600, timeout=.100)
                self.ports[port] = {"is_valid": False, "is_active": False, "serial": ser}
                valid = self.handshake_sequence(ser)
                if valid:
                    self.ports[port]["is_valid"] = True
                    self.ports[port]["is_active"] = True
                    print(self.ports[port])

    def handshake_sequence(self, ser):
        for i in range(10):
            valid = self.handshake(ser, '!connectie-check\r', "Temperatuurmeetsensor v0.1")
            if valid:
                return True
        return False

    # does a handshake with a device on a specific port
    def handshake(self, ser, command, valid):
        _command = bytes(command, encoding="utf-8")
        print("sending !connectie-check to", ser.name)
        ser.write(_command)
        return True if self.read(ser).strip() == valid else False

    # returns readdata from a device on a given comport
    def read(self, ser):
        eol = b'\r'
        leneol = len(eol)
        line = bytearray()
        while True:
            c = ser.read(1)
            if c:
                line += c
                if line[-leneol:] == eol:
                    break
            else:
                break
        try:
            line.decode(encoding="utf-8")
        except:
            return "@ongeldig\n"
        return line.decode(encoding="utf-8")


controller = WindowController()
