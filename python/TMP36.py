import serial                   #import serial library
import numpy as np              #import numpy
import matplotlib.pyplot as plt #import matplotlib library
from drawnow import *

temperatureC= []
arduinoData = serial.Serial('COM9', 9600) #Creating our serial object named arduinoData
plt.ion() #Tell matplotlib you want interactive mode to plot live data
cnt=0

def makeFig():             #Create a function that makes our desired plot
    plt.ylim(-50,50)                                        #Set y min and max values
    plt.title('Real-time temperatuur')             #Plot the title
    plt.grid(True)                                         #Turn the grid on
    plt.ylabel('Temp C')                                   #Set ylabels
    plt.xlabel('Tijd')
    plt.plot(temperatureC, 'ro-', label='Graden C')       #plot the temperature
    plt.legend(loc='upper left')                           #plot the legend
 
  

while True: # While loop that loops forever
    while (arduinoData.inWaiting()==0):    #Wait here until there is data
        pass #do nothing
    arduinoString = arduinoData.readline(5).decode('utf-8') #read the line of text from the serial port
    #dataArray = arduinoString.split(',')   #Split it into an array called dataArray
    temp_value = float(arduinoString)
    temperatureC.append(temp_value)      #Build our temperatureF array by appending temp readings
    drawnow(makeFig)                       #Call drawnow to update our live graph
    plt.pause(.000001)                     #Pause Briefly. Important to keep drawnow from crashing
    cnt=cnt+1
    if(cnt>50):                            #If you have 50 or more points, delete the first one from the array
        temperatureC.pop(0)                #This allows us to just see the last 50 data points
 