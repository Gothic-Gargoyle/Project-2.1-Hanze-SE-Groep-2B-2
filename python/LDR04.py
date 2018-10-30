import serial                   #import serial library
import numpy as np              #import numpy
import matplotlib.pyplot as plt #import matplotlib library
import datetime
from drawnow import *

light = []
x = []
arduinoData = serial.Serial('COM9', 9600) #Creating our serial object named arduinoData
plt.ion() #Tell matplotlib you want interactive mode to plot live data
cnt=0

def makeFig():             #Create a function that makes our desired plot
    plt.ylim(0,100)                                        #Set y min and max values
    plt.gcf().autofmt_xdate()
    plt.title('Real-time licht percentage: {}'.format(light[-1]))             #Plot the title
    plt.grid(True)                                         #Turn the grid on
    plt.ylabel('Licht percentage')                                   #Set ylabels
    plt.xlabel('Tijd')
    plt.plot(x, light, 'r-', label='Licht percentage')       #plot the temperature
    plt.legend(loc='upper left')                           #plot the legend

while True: # While loop that loops forever
    while (arduinoData.inWaiting()==0):    #Wait here until there is data
        pass #do nothing
    arduinoString = arduinoData.readline().decode('utf-8') #read the line of text from the serial port
    light_value = int(arduinoString)
    temp_value = light_value / 1024 * 100
    value = float("{0:.2f}".format(temp_value))
    light.append(value)      #Build our temperatureF array by appending temp readings
    x.append(datetime.datetime.now().strftime("%H:%M:%S"))
    print(light)
    # print(x)
    drawnow(makeFig)                       #Call drawnow to update our live graph
    # plt.pause(.000001)                     #Pause Briefly. Important to keep drawnow from crashing
    cnt=cnt+1
    if(cnt>720):                            #If you have 50 or more points, delete the first one from the array
        light.pop(0)                #This allows us to just see the last 50 data points
 