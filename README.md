# MxWx
Met Office Observation and Forecast display using hzellers rpi-rgb-led-matrix

At the moment it displays the current observed weather (left), current temperature (green), max high-low temp forecast for today, and then the forecast weather for current day/night. At the bottom is a bar-graph of rain probability over 3-hour periods. Unfortunately the met don't give hourly forecasts, so each bar represents 3 hours. It isn't meant to be accurate, just a glace-at guide covering the next 48 hours. Midnight is denoted by a long white line, and a slightly shorter yellow bar represents noon. The wavy red bar is my attempt at squeezing in air-pressure over the last 24 hour period (again as a guide, not accurate). 

The location is hardcoded in at the moment, but you could easily change it - the MET provide a list of location codes on their datapoint website. (In the unlikely event that you're actually wanting to USE this code - get in touch and i'll happily help you configure it).

![Updated Version](matrix-two.jpg?raw=true "Bit of an update")

![Alt text](matrix-one.jpg?raw=true "Work in progress!")
