import serial                   #import serial library
import numpy as np              #import numpy
import matplotlib.pyplot as plt #import matplotlib library
import datetime
from drawnow import *

temperatureC = []
x = []
arduinoData = serial.Serial('COM9', 9600) #Creating our serial object named arduinoData
plt.ion() #Tell matplotlib you want interactive mode to plot live data
cnt=0

def makeFig():             #Create a function that makes our desired plot
    plt.ylim(-50,50)                                        #Set y min and max values
    plt.gcf().autofmt_xdate()
    plt.title('Real-time temperatuur: {}'.format(temperatureC[-1]))             #Plot the title
    plt.grid(True)                                         #Turn the grid on
    plt.ylabel('Temp C')                                   #Set ylabels
    plt.xlabel('Tijd')
    plt.plot(x, temperatureC, 'r-', label='Graden C')       #plot the temperature
    plt.legend(loc='upper left')                           #plot the legend

while True: # While loop that loops forever
    while (arduinoData.inWaiting()==0):    #Wait here until there is data
        pass #do nothing
    arduinoString = arduinoData.readline(5).decode('utf-8') #read the line of text from the serial port
    temp_value = float(arduinoString)
    temperatureC.append(temp_value)      #Build our temperatureF array by appending temp readings
    x.append(datetime.datetime.now().strftime("%H:%M"))
    print(temperatureC)
    print(x)
    drawnow(makeFig)                       #Call drawnow to update our live graph
    plt.pause(.000001)                     #Pause Briefly. Important to keep drawnow from crashing
    cnt=cnt+1
    if(cnt>720):                            #If you have 50 or more points, delete the first one from the array
        temperatureC.pop(0)                #This allows us to just see the last 50 data points
 