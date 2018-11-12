from python.field import *
from python.graph import *
from tkinter import *
from tkinter.ttk import Notebook
from threading import Timer, Thread, Event
import serial
import serial.tools.list_ports
import time
import datetime

title_config = {
    "font": ("Helvetica", 30)
}
input_area_config = {
    "padx": 5,
    "pady": 100
}
input_area_pack_config = {
    "side": TOP,
    "expand": True,
    "fill": BOTH,
}
button_config = {
    "bg": "#393939",
    "bd": 0,
    "padx": 20,
    "pady": 2,
    "fg": "white",
    "activebackground": "#212121",
    "activeforeground": "white",
    "font": ("Helvetica", 12)
}
text_config = {
    "font": ("Helvetica", 12)
}
input_config = {
    "bd": 0,
    "fg": "black",
    "font": ("Helvetica", 12),
}

input_container_config = {
    "padx": 10,
    "pady": 10,
    "bg": "white",
}
button_area_config = {
    "side": LEFT,
    "padx": 5,
    "pady": 5
}


class WindowController:

    def __init__(self, width=800, height=600):

        # TKINTER INIT
        self.width = width
        self.height = height

        self.root = Tk()
        self.tk_init()
        self.root.protocol("WM_DELETE_WINDOW", self.close_program)

        temperature = '^\-[0-9][0-9]?$|^[0-9][0-9]?$|^\-$'  # -99 to +99 or - for infinite
        percentage = '^[0-9][0-9]?$|^100$'  # 0 - 100

        self.field_list = [
            {
                "label": "min - max opening (in %)",
                "fields": ("ondergrensuitrol", "bovengrensuitrol"),
                "pattern": percentage,
                "mult": 1
            },
            {
                "label": "sluit rolluiken van tot (in C)",
                "fields": ("ondergrenstemperatuur", "bovengrenstemperatuur"),
                "pattern": temperature,
                "mult": 10
            },
            {
                "label": "sluit rolluiken vanaf licht intensiteit (in %)",
                "fields": ("ondergrenslichtintensiteit", "bovengrenslichtintensiteit"),
                "pattern": percentage,
                "mult": 1
            },
        ]

        self.buttons_list = [
            {
                "text": "inrollen",
                "commands": {
                    "autonoom": 0,
                    "schermuitrol":0
                }
            },
            {
                "text": "uitrollen",
                "commands": {
                    "autonoom": 0,
                    "schermuitrol": 100
                }
            },
            {
                "text": "autonoom",
                "commands": {
                    "autonoom": 1
                }
            }
        ]

        # PORTCONTROLLER

        self.fields = {}
        self.graphs = {}        
        self.graph_buttons = {}

        self.ports = {}
        self.port_controller = PortController(self.ports, callback=self.update, timeout=1)

        self.root.mainloop()

        # CREATE FIELDS

    def tk_init(self):
        self.root.geometry(str(self.width) + "x" + str(self.height))
        self.root.title("bedieningspaneel")

    def update(self):

        print("redrawing tabs")
        try:
            self.tab_area.destroy()
        except:
            print("cannot destoy tab area")
            pass
        print(self.ports, len(self.ports))



        if len(self.ports) > 0:

            self.tab_area = Notebook(self.root)
            self.tab_area.pack(fill=BOTH, expand=1)

            self.tabs = self.create_tabs_for_ports(self.root, self.tab_area, self.ports)

            self.populate_tabs(self.tabs, self.field_list)

            self.add_buttons_to_tabs(self.tabs, self.buttons_list)

            for name, graph in self.graphs.items():
                for type, graph in self.graphs[name].items():
                    if graph.timer_active:
                        graph.stop()

            for name, port in self.ports.items():
                print("creating graph at {}".format(name))

                light_graph = Graph(port, "licht", self.ports, "pecentage licht", "realtime licht {} in %", timer=5)
                temp_graph = Graph(port, "temp", self.ports, "graden C", "realtime temperuur {} in C", timer=5)
                self.graphs[name] = {"temp": temp_graph, "licht": light_graph}
                if name not in self.ports:
                    for type, graph in self.graphs[name].items():
                        graph.stop()
                else:
                    for type, graph in self.graphs[name].items():
                        graph.start()

        else:
            print("no comports found")
            self.tab_area = Frame()
            self.tab_area.pack(fill=BOTH, expand=1)

            label = Label(self.tab_area, text="geen rolluiken gevonden")
            label.pack()
            

    # takes a list of ports and creates tabs for it.
    def create_tabs_for_ports(self, root, tab_area, ports):
        tab_frames = {}
        if ports:
            for name, port in ports.items():
                if port.is_valid and port.is_active:
                    tab_frame = Frame(root)
                    tab_frames[name] = tab_frame

                    tab_area.add(tab_frame, text=name)


            tab_area.pack(expand=1, fill="both")

        return tab_frames

    # takes a frame and a list and creates tabs for it.
    def populate_tabs(self, tabs, field_list):
        for port, tab in tabs.items():
            index = 0
            fields = []
            input_area = Frame(tab, **input_area_config)
            for field_settings in field_list:
                field = Range(input_area, field_settings["fields"], field_settings["label"], pattern=field_settings["pattern"], mult=field_settings["mult"], row=index, tcnf=text_config, cnf=input_config, pcnf=input_area_config)
                values = self.get_fields(port, field_settings["fields"])
                field.set(values)
                fields.append(field)

                index += 1

            input_area.pack(**input_area_pack_config)

            self.fields[port] = fields
            print(self.fields)

            button_area = Frame(tab)
            fields = [field["fields"] for field in field_list]
            print(fields)
            update_button = FieldButton(input_area, self.ports[port], "update", self.fields[port], row=len(field_list) + 1, cnf=button_config)
            button_area.pack()

    def add_buttons_to_tabs(self, tabs, buttons_list):
        for port, tab in tabs.items():
            index = 0
            button_area = Frame(tab)
            for button_settings in buttons_list:
                port_obj = self.ports[port]
                button = ActionButton(button_area, port_obj, button_settings["text"], commands=button_settings["commands"], cnf=button_config)
            print('YA YEET {}'.format(port))
            GraphButton(button_area, self.graphs, port, "temp", "open temp graph", cnf=button_config)
            GraphButton(button_area, self.graphs, port, "licht", "open light graph", cnf=button_config)
            button_area.pack()

    # def open_graph(self, port):
    #     print(self.ports)
    #     # print(self.graphs[port].temp_list)
    #     for port, graph in self.graphs.items():
    #         print(self.ports[port])
    #         self.graphs[port].show()
    
    # gets values from input fields
    def get_input_fields(self, fields):
        pass

    # takes a port and a list of fields. returns values the value stored on the arduino
    def get_fields(self, port, fields):
        values = {}
        for field in fields:
            values[field] = self.send(port, field)

        return values

    # this function gets fired when the program closes
    def close_program(self):
        print("closing ports")
        for name, port in self.ports.items():
            port.close()
        print("closing program")
        sys.exit()

    def send(self, port, command, value=None):
        return self.ports[port].send(command, value)


class Port:
    def __init__(self, port, settings, is_valid=False, is_active=False):
        self.port = port
        self.serial = serial.Serial(port, **settings)
        self.settings = settings
        self.is_valid = is_valid
        self.is_active = is_active
        self.queue = {}

    def send(self, command, value=None):
        if value is not None:
            print("sending command to {}".format(self.port), "!{0}={1}\r".format(command, value))
            self.serial.write(bytes("!{0}={1}\r".format(command, value), encoding="utf-8"))
        else:
            print("sending command to {}".format(self.port), "!{0}\r".format(command, value))
            self.serial.write(bytes("!{0}\r".format(command), encoding="utf-8"))

        while True:
            print("reading command")

            response = self.read()
            if self.is_response_2(response):

                return response.split("=")[-1]

    def is_response_2(self, response):
        print(response)
        if len(response) > 0:
            if "@" == response[0]:
                return True
        return False

    def is_response(self, response, command):
        print(response)
        if len(response) > 0:
            response_ = response.split("=")
            if response_[0][2:] == command:
                return True
        return False

    def handshake_sequence(self):
        for i in range(10):
            valid = self.handshake('!connectie-check\r', "Moi eem")
            if valid:
                return True
        return False

    # does a handshake with a device on a specific port
    def handshake(self, command, valid):
        _command = bytes(command, encoding="utf-8")
        print("sending !connectie-check to", self.serial.name)
        self.serial.write(_command)
        return True if self.read() == valid else False

    # returns readdata from a device on a given comport
    def read(self, command=None):
        print("queue:", self.queue)
        if command in self.queue.keys():
            for i in range(len(self.queue[command])):
                queue_command = self.queue[command][i]
                if command == queue_command.split("=")[0]:
                    value = self.queue[command].pop(i)
                    print("found in queue ", value)
                    return value
        eol = b'\r'
        leneol = len(eol)
        line = bytearray()
        while True:
            c = self.serial.read(1)
            if c:
                line += c
                if line[-leneol:] == eol:
                    break
            else:
                break
        try:
            line.decode(encoding="utf-8")
        except:
            print("cannot decode response")
            return "@ongeldig\r"
        response = line.decode(encoding="utf-8").strip()
        if command is not None and response is not "":
            response_command = response.split("=")[0]
            if command == response_command:
                return response
            else:
                if command not in self.queue.keys():
                    self.queue[response_command] = []
                self.queue[response_command].append(response)
                return self.read(command=command)
        return response


class PortController:

    def __init__(self, ports, callback=None, timeout=1):
        self.ports = ports
        self.connection_loop(timeout, callback)
        callback()

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

    def send(self, port, command, value=None):
        ser = self.ports[port]["serial"]

        if value is not None:
            return ser.write(bytes("!{0}={1}\r".format(command, value), encoding="utf-8"))
        else:
            return ser.write(bytes("!{0}\r".format(command), encoding="utf-8"))

    # returns ports that are known but disabled
    def get_disabled_ports(self, ports):
        disables_ports = []
        for port in ports:
            if port in self.ports:                                # if port is used but not active
                if self.ports[port].is_valid and self.ports[port].is_active is False:
                    disables_ports.append(port)

        return disables_ports

    # returns ports that are known, disconnected and not jet registered as disconnected
    def get_disconnected_ports(self, ports):
        disconnected_ports = []
        for port in self.ports.keys():
            if port not in ports:
                if self.ports[port].is_valid:
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
                if self.ports[port].is_valid:
                    self.ports[port].is_active = True
                    self.ports[port].is_valid = True
                    print("reopening port {}".format(port))
                    self.ports[port].serial.open()

    # disables known ports
    def disable_ports(self, ports):
        if ports:
            print("disable ports:", ports)
            for port in ports:
                self.ports[port].serial.close()
                del self.ports[port]
                print("ports???", self.ports)
                # self.ports[port]['is_active'] = False
                # self.ports[port]['serial'].close()

    # add ports to list of known ports
    def add_ports(self, ports):
        if ports:
            for port in ports:
                print("adding port {} to list".format(port))
                print("opening port {}".format(port))
                port_instance = Port(port, {"baudrate": 9600, "timeout": .1})
                self.ports[port] = port_instance
                valid = self.ports[port].handshake_sequence()
                print("handshake is", valid)
                if valid:
                    self.ports[port].is_valid = True
                    self.ports[port].is_active = True
                    # self.ports[port].is_active = True
                    print(self.ports[port])


controller = WindowController()
