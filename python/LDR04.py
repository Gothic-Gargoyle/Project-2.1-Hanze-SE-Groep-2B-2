import serial
import matplotlib.pyplot as plt
import datetime
from drawnow import *

arduinoData = serial.Serial('COM9', 9600)
plt.ion()
cnt = 0
light = []
x = []


def makeFig():
    plt.ylim(0, 100)
    plt.gcf().autofmt_xdate()
    plt.title('Real-time licht percentage: {}%'.format(light[-1]))
    plt.grid(True)
    plt.ylabel('Licht percentage')
    plt.xlabel('Tijd')
    plt.plot(x, light, 'r-', label='Licht percentage')
    plt.legend(loc='upper left')

while True:
    while (arduinoData.inWaiting() == 0):
        pass
    arduinoString = arduinoData.readline().decode('utf-8')
    light_value = int(arduinoString)
    temp_value = light_value / 1024 * 100
    value = float("{0:.2f}".format(temp_value))
    light.append(value)
    x.append(datetime.datetime.now().strftime("%H:%M:%S"))
    drawnow(makeFig)
    plt.pause(.000001)
    cnt = cnt+1
    if(cnt > 720):
        light.pop(0)
