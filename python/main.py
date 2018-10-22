
from python.includes.field2 import *


class WindowController:

    def __init__(self, title="window controller", width=800, height=600):
        # VARS INIT
        self.title = title
        self.width = width
        self.height = height

        # TKINTER INIT
        self.root = Tk()
        self.root.geometry(str(self.width) + "x" + str(self.height))
        self.root.title(title)

        self.field1 = Field(self.root, "naam", ("from", "to"), input_type="int", callback=self.send)
        self.field2 = Field(self.root, "dinges", ("van", "tot"), input_type="int", row=1, callback=self.send)

        button = Button(self.root, text="update", command=self.send_fields)
        button.grid(row=3, column=2)

        self.root.mainloop()

    def send(self, label, value):
        print(label+"="+value)

    def send_fields(self):
        print(self.field1.get(), self.field2.get())


controller = WindowController()



# class window controller:
# class serial
# class field
