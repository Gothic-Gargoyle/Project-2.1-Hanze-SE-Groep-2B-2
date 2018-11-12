from field import *
from tkinter import *
from tkinter.ttk import Notebook
from pprint import *
import serial
import serial.tools.list_ports
import time
import sys
from threading import Timer, Thread, Event

title_config = {
    "font": ("Helvetica", 30)
}
input_area_config = {
    "padx": 5,
    "pady": 5,
    "side": TOP,
    "expand": True
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
    "bg": "white",
    "bd": 0,
    "fg": "black",
    "font": ("Helvetica", 12)
}
button_area_config = {
    "side": LEFT,
    "padx": 5,
    "pady": 5
}

class WindowController:

    def __init__(self, title="window controller", width=800, height=600):

        # ##############################
        # ## SERIAL SETUP
        # ##############################
        # self.ser = serial.Serial('COM3', 9600, timeout=1)
        all_ports = serial.tools.list_ports.comports()
        self.ports = {}
        self.port_states = {}

        for port in all_ports:
            name = port.device
            print(port)
            # self.ser = serial.Serial(name, 9600, timeout=1)
            self.ser = serial.Serial(name, 9600, timeout=1)

            self.show_loading_bar(2)

            response = self.handshake(name, self.ser)
            print(response)
            if response == "Temperatuurmeetsensor v0.1":
                self.ports[name] = self.ser
                self.port_states[name] = True
            else:
                self.ser.close()

        # print(self.ports)




        # ##############################
        # ## VARS INIT
        # ##############################
        self.title = title
        self.width = width
        self.height = height

        # ##############################
        # STYLING
        # ##############################


        # ##############################
        # ## TKINTER INIT
        # ##############################
        self.root = Tk()
        self.root.geometry(str(self.width) + "x" + str(self.height))
        self.root.title(title)
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

        # ##############################
        # ## TITLE AREA
        # ##############################

        self.title_area = Frame(self.root)

        self.title = Label(self.title_area, text="Rolluiken Bedieningspaneel", **title_config)
        self.title.pack()

        self.title_area.pack()

        # ##############################
        # ## CREATE TABS
        # ##############################

        # setup
        self.tabs = {}
        tab_area = Frame(self.root, name='demo')
        nb = Notebook(tab_area, name='notebook')
        nb.enable_traversal()
        nb.pack(fill=BOTH, expand=Y, padx=2, pady=3)

        # fill tabs
        nr_of_ports = 4
        frames = dict([(name, Frame(tab_area)) for name, value in self.ports.items()])

        print(frames)

        if not frames:
            self.show_message("Er zijn geen rolluikbedieningsmodules gevonden.")
        else:
            self.populate_with_tabs(frames, tab_area, nb)




        # inputs





        # ##############################
        # ## BUTTONS
        # ##############################
        self.button_area = Frame(self.root)

        # area left
        self.button_area_left = Frame(self.button_area)

        # self.open_shutter_button = Button(self.button_area_left, command=lambda: self.send("shutter_status", "open"), text="open rolluiken", **button_config)
        # self.close_shutter_button = Button(self.button_area_left, command=lambda: self.send("shutter_status", "closed"), text="sluit rolluiken", **button_config)
        # self.automatic_shutter_button = Button(self.button_area_left, command=lambda: self.send("shutter_status", "auto"), text="automatisch", **button_config)
        #
        # self.open_shutter_button.pack(**button_area_config)
        # self.close_shutter_button.pack(**button_area_config)
        # self.automatic_shutter_button.pack(**button_area_config)

        self.button_area_left.pack(side=LEFT)

        # area right
        self.button_area_right = Frame(self.button_area)

        self.open_graph = Button(self.button_area_right, text="Grafieken", **button_config)

        self.open_graph.pack(**button_area_config)

        self.button_area_right.pack(side=RIGHT)

        # finalizing
        self.button_area.pack(fill="x")

        # ##############################
        # ## START LOOP
        # ##############################

        self.root.mainloop()

    def send_multiple(self, *send_array):
        for send in send_array:
            response = self.send(*send)

    def show_message(self, message):
        label = Label(self.root, text=message)
        label.pack()
        print(message)

    def populate_with_tabs(self, frames, tab_area, nb):
        for name, frame in frames.items():
            self.creat_tab(tab_area, nb, frame, name)
            self.tabs[name] = {"inputs": (), "buttons": ()}
            self.tabs[name]["inputs"] = self.create_input_group(name, Frame(frame))

            button_area = Frame(frame)
            self.tabs[name]["buttons"] = self.create_buttons(name, frame)

            # print(self.tabs[name]["buttons"])

            button_area.pack()

    def handshake(self, port, ser):
        command = bytes('!connectie-check\r', encoding="utf-8")
        ser.write(command)
        print("sending !connectie-check to", ser.name)

        return self.read(ser).strip()

    def show_loading_bar(self, timeout):
        length = 30
        for i in range(length):
            loading_bar = "loading:" + "/"*i
            sys.stdout.write('\r'+loading_bar)
            time.sleep(timeout / length)

    def check_connection(self, port):
        ports = [tuple(p) for p in list(serial.tools.list_ports.comports())]
        print(ports)
        for available_ports in ports:
            name = available_ports[0]
            print("name:", name, "port:", port)
            if name == port:
                return True
            else:
                print("Lost connection!")
        return False

    def reestablish_connection(self, port):
        self.ports[port].open()

        Timer(2, lambda: self.reestablish_connection_handshake(port))

    def reestablish_connection_handshake(self, port):
        response = self.handshake(port, self.ports[port])
        print("restablishing connection:", response)
        if response == "Temperatuurmeetsensor v0.1":
            print("connection establised")
            self.port_states[port] = True

    def send(self, port, key, value=None):
        if self.check_connection(port):
            if value is None:
                command = bytes("!" + str(key) + '\r', encoding="utf-8")
            else:
                # print("sending to port: {}".format(send[0]), str(send[1])+"="+str(send[2]))
                command = bytes("!" + str(key) + "=" + str(value) + '\r', encoding="utf-8")

            # print(self.ports, port)
            self.ports[port].flushInput()
            self.ports[port].write(command)

            response = self.read(self.ports[port]).strip()
            print("sending command to port {}:".format(port), command, "response", response)
            return response
        else:
            self.show_message("Sorry, er is iets mis gegaan.")


    # source: https://stackoverflow.com/questions/16470903/pyserial-2-6-specify-end-of-line-in-readline
    def read(self, ser, command=None):
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

        response = line.decode(encoding="utf-8")
        if command is not None:
            if command == response:
                return response
            else:
                # push in queue
                self.read(ser, command=command)
        return response

    def creat_tab(self, tab_area, nb, frame,  name):
        # source: https://gist.github.com/mikofski/5851633

        new_frame = nb.add(frame, text=name, underline=0, padding=2)

        tab_area.pack(side=TOP, fill=X, expand=Y)

        return new_frame

    def create_input_group(self, port, input_area):
        # ##############################
        # ## INPUTS
        # ##############################
        temperature = '^\-[0-9][0-9]?$|^[0-9][0-9]?$|^\-$' # -99 to +99 or - for infinite
        percentage = '^[0-9][0-9]?$|^100$' # 0 - 100

        self.minmax_input = Range(input_area, ("ondergrensuitrol", "bovengrensuitrol"), "min - max opening (in %)", pattern=percentage,  tcnf=text_config, cnf=input_config, pcnf=input_area_config)
        self.temperature_input = Range(input_area, ("ondergrenstemperatuur", "bovengrenstemperatuur"), "sluit rolluiken van tot (in C)", pattern=temperature, row=1, tcnf=text_config, cnf=input_config, pcnf=input_area_config)
        self.light_input = Range(input_area, ("ondergrenslichtintensiteit", "bovengrenslichtintensiteit"), "sluit rolluiken vanaf licht intensiteit (in %)", pattern=percentage, row=2, tcnf=text_config, cnf=input_config, pcnf=input_area_config)

        # get settings and push them to the inputs
        self.minmax_input.set(0, self.send(port, "ondergrensuitrol"))
        self.minmax_input.set(1, self.send(port, "bovengrensuitrol"))

        self.temperature_input.set(0, self.send(port, "ondergrenstemperatuur"))
        self.temperature_input.set(1, self.send(port, "bovengrenstemperatuur"))

        self.light_input.set(0, self.send(port, "ondergrenslichtintensiteit"))
        self.light_input.set(1, self.send(port, "bovengrenslichtintensiteit"))


        button = Button(input_area, text="update", command=lambda: self.send_fields(port), **button_config)
        button.grid(row=3, column=2)

        input_area.pack(**input_area_config)

        return self.minmax_input, self.temperature_input, self.light_input

    def create_buttons(self, port, frame):
        open_button = Button(frame, command=lambda: self.send_multiple((port, "schermuitrol", "0"), (port, "autonoom", "0")), text="open rolluiken",
                   **button_config)
        close_button = Button(frame, command=lambda: self.send_multiple((port, "schermuitrol", "100"), (port, "autonoom", "0")), text="sluit rolluiken",
                   **button_config)
        auto_button = Button(frame, command=lambda: self.send(port, "autonoom", "1"), text="automatisch",
                   **button_config)

        open_button.pack(**button_area_config)
        close_button.pack(**button_area_config)
        auto_button.pack(**button_area_config)

        return open_button, close_button, auto_button

    def send_fields(self, port):
        # pprint(self.tabs)

        input_list = self.tabs[port]["inputs"]

        # print(input_list)
        for entry in input_list:
            # print(entry)
            data = entry.validate()
            # print(data)
            if data is not False:
                for key, value in data.items():
                    self.send(port, key, value)

    def on_closing(self):

        for port, ser in self.ports.items():
            ser.close()

        print("closing program")
        sys.exit()


controller = WindowController()



# class window controller:
# class serial
# class field
