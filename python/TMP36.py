import serial
import matplotlib.pyplot as plt
import datetime
from drawnow import *

arduinoData = serial.Serial('COM9', 9600)
plt.ion()
cnt = 0
temperatureC = []
x = []


def makeFig():
    plt.ylim(-50, 50)
    plt.gcf().autofmt_xdate()
    plt.title('Real-time temperatuur: {}°C'.format(temperatureC[-1]))
    plt.grid(True)
    plt.ylabel('Temp C')
    plt.xlabel('Tijd')
    plt.plot(x, temperatureC, 'r-', label='Graden C')
    plt.legend(loc='upper left')

while True:
    while (arduinoData.inWaiting() == 0):
        pass
    arduinoString = arduinoData.readline().decode('utf-8').strip()
    s = arduinoString.split('=')
    if s[0] == '#temp':
        temp_value = float(s[1])
        temperatureC.append(temp_value)
        x.append(datetime.datetime.now().strftime("%H:%M:%S"))
        drawnow(makeFig)
        plt.pause(.000001)
        cnt = cnt + 1
        if(cnt > 720):
            temperatureC.pop(0)
