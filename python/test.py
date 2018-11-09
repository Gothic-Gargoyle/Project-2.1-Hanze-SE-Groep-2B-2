from serial import Serial
import serial.tools.list_ports
from threading import Timer
import time
from tkinter import *

port = Serial("COM3", baudrate=9600, timeout=.1)


class Ding:
    def __init__(self):
        self.root =Tk()
        self.root.title("henk")

        self.button = Button(self.root, text="clicky testie", command=lambda: self.test())
        self.button.pack()
        self.root.mainloop()

    def test(self):
        port.write(bytes("!connectie-check\r", encoding="utf-8"))
        for i in range(100):
            print(port.read(1))
        Timer(2, lambda: self.test())



ding = Ding()

# Timer(1, lambda: test()).start()
