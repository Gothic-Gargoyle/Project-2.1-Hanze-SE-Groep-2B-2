from field import *
from tkinter import *
from tkinter.ttk import Notebook


title_config = {
    "font": ("Comic Sans MS", 30)
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
    "font": ("Comic Sans MS", 12)
}
text_config = {
    "font": ("Comic Sans MS", 12)
}
input_config = {
    "bg": "white",
    "bd": 0,
    "fg": "black",
    "font": ("Comic Sans MS", 12)
}
button_area_config = {
    "side": LEFT,
    "padx": 5,
    "pady": 5
}

class WindowController:

    def __init__(self, title="window controller", width=800, height=600):

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
        frames = dict([("COM{}".format(i), Frame(tab_area)) for i in range(nr_of_ports)])
        print(dict(frames))

        for name, frame in frames.items():
            self.creat_tab(tab_area, nb, frame, name)
            self.tabs[name] = self.create_input_group(name, Frame(frame)
            )




        # inputs





        # ##############################
        # ## BUTTONS
        # ##############################
        self.button_area = Frame(self.root)

        # area left
        self.button_area_left = Frame(self.button_area)

        self.open_shutter_button = Button(self.button_area_left, command=lambda: self.send("shutter_status", "open"), text="open rolluiken", **button_config)
        self.close_shutter_button = Button(self.button_area_left, command=lambda: self.send("shutter_status", "closed"), text="sluit rolluiken", **button_config)
        self.automatic_shutter_button = Button(self.button_area_left, command=lambda: self.send("shutter_status", "auto"), text="automatisch", **button_config)

        self.open_shutter_button.pack(**button_area_config)
        self.close_shutter_button.pack(**button_area_config)
        self.automatic_shutter_button.pack(**button_area_config)

        self.button_area_left.pack(side=LEFT)

        # area right
        self.button_area_right = Frame(self.button_area)

        self.open_temp_graph = Button(self.button_area_right, text="Temperatuur grafiek", **button_config)
        self.open_light_graph = Button(self.button_area_right, text="Licht grafiek", **button_config)

        self.open_temp_graph.pack(**button_area_config)
        self.open_light_graph.pack(**button_area_config)

        self.button_area_right.pack(side=RIGHT)

        # finalizing
        self.button_area.pack(fill="x")

        # ##############################
        # ## START LOOP
        # ##############################

        self.root.mainloop()



    def send(self, port, label, value):
        print("sending to port: {}".format(port), str(label)+"="+str(value))

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

        self.minmax_input = Range(input_area, "minmax", "min - max opening (in %)", pattern=percentage,  tcnf=text_config, cnf=input_config, pcnf=input_area_config)
        self.temperature_input = Range(input_area, "temperature_input", "sluit rolluiken van tot (in C)", pattern=temperature, row=1, tcnf=text_config, cnf=input_config, pcnf=input_area_config)
        self.light_input = Range(input_area, "light_input", "sluit rolluiken vanaf licht intensiteit (in %)", pattern=percentage, row=2, tcnf=text_config, cnf=input_config, pcnf=input_area_config)

        button = Button(input_area, text="update", command=lambda: self.send_fields(port), **button_config)
        button.grid(row=3, column=2)

        input_area.pack(**input_area_config)

        return self.minmax_input, self.temperature_input, self.light_input

    def send_fields(self, port):
        print(self.tabs)

        input_list = self.tabs[port]

        for entry in input_list:
            print(entry.get())
            data = entry.validate()
            if data is not False:
                self.send(port, entry.id, data)


controller = WindowController()



# class window controller:
# class serial
# class field
