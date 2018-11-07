import serial
import matplotlib.pyplot as plt
import datetime
from drawnow import *

arduinoData = serial.Serial('COM9', 9600)
plt.ion()
temp_cnt = 0
temp_list = []
temp_time = []


def makeFig():
    plt.ylim(-50, 50)
    plt.gcf().autofmt_xdate()
    plt.title('Real-time temperatuur: {}°C'.format(temp_list[-1]))
    plt.grid(True)
    plt.ylabel('Temp C')
    plt.xlabel('Tijd')
    plt.plot(temp_time, temp_list, 'r-', label='Graden C')
    plt.legend(loc='upper left')

while True:
    while (arduinoData.inWaiting() == 0):
        pass
    arduinoString = arduinoData.readline().decode('utf-8').strip()
    s = arduinoString.split('=')
    if s[0] == '#temp':
        temp_value = float(s[1])
        temp_list.append(temp_value)
        temp_time.append(datetime.datetime.now().strftime("%H:%M:%S"))
        drawnow(makeFig)
        plt.pause(.000001)
        temp_cnt = temp_cnt + 1
        if(temp_cnt > 720):
            temp_list.pop(0)
