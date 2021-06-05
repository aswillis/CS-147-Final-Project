from flask import Flask, render_template, jsonify
from flask import request
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
app = Flask(__name__)

# If the measured alcohol content is 10x higher than reference, then
# the starter must be fed.
ALC_THRESHOLD = 10;

humidityReadings = []
tempReadings = []
alcReadings = []
readingTimes = []
starterRisen = ""

@app.route("/")
def hello():
	global starterRisen
	now=datetime.now().strftime("%d/%m/%Y %H:%M:%S")
	newTemp = request.args.get("tempC")
	newHum = request.args.get("hum")
	newAlc = request.args.get("alc")
	starterRisen = request.args.get("risen") == "1";
	readingTimes.append(now)
	humidityReadings.append(float(newHum))
	tempReadings.append(float(newTemp))
	alcReadings.append(float(newAlc))
	print(newTemp)
	print(newHum)
	return "feed" if alcReadings[-1] > ALC_THRESHOLD else "";


@app.route("/hist/")
def getData():
	return jsonify({'temps': tempReadings, 
			'alcs': alcReadings,
			'readTimes': readingTimes,
			'hums': humidityReadings,
			'feed': alcReadings[-1] > ALC_THRESHOLD,
			'risen': starterRisen})


@app.route("/data/")
def graphing():
	f, ax = plt.subplots()

	# One reading every ten seconds
	times = [i*10 for i in range(1, len(tempReadings)+1)];
	
	plt.plot(times, humidityReadings, color = 'green')
	plt.xlabel('Time (seconds)')
	plt.ylabel('Humidity (%)')
	plt.title('Percent humidity over time')
	plt.savefig('static/images/myHumData.png', format="png")
	plt.show()
	
	f, ax = plt.subplots()
	plt.plot(times, tempReadings, color = 'red')
	plt.xlabel('Time (seconds)')
	plt.ylabel('Temperature (C)')
	plt.title('Temperature over time')
	plt.savefig('static/images/myTempData.png', format='png')

	f, ax = plt.subplots()
	plt.plot(times, alcReadings, color = 'red')
	plt.xlabel('Time (seconds)')
	plt.ylabel('Alcohol (Times Concentration over Baseline)')
	plt.title('Alcohol vapor over time')
	plt.savefig('static/images/myAlcData.png', format='png')

	return render_template('data.html', humSource='/static/images/myHumData.png', tempSource='/static/images/myTempData.png', alcSource='/static/images/myAlcData.png')
	

