import serial
import csv
from datetime import datetime
import time

now = datetime.now().strftime("%d_%m_%Y_%H_%M_%S")
file_name = "SmallBaseStationData" + now + ".csv"
with open(file_name, "a", newline="") as csv_f:
           writer = csv.writer(csv_f)
           writer.writerow(["message_index","gps_hour","gps_minute","gps_second","gps_latitude","gps_longitude","gps_altitude","gps_speed","temp_scd30","pressure","humidity_scd30","rssi","snr"])

serial_object = serial.Serial("COM4", 115200)
while True:
    try:
        if serial_object == None:
            serial_object = serial.Serial("COM4", 115200)
            print("Reconnecting")
        data = str(serial_object.readline())[2:-5]
        data = data.split(",")
        with open(file_name, "a", newline="") as csv_f:
            writer = csv.writer(csv_f)
            writer.writerow(data)
    except:
        if(not(serial_object == None)):
            serial_object.close()
            serial_object = None
            print("Disconnecting")
        print("No Connection")
        time.sleep(0.25)
