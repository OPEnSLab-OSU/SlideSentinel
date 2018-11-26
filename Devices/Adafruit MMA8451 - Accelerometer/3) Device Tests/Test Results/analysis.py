import csv
import numpy
import matplotlib
import matplotlib.pyplot as plt
from bokeh.plotting import figure, output_file, show

def main():
	i = 0
	with open('AC_TMP11_19.csv', newline='') as csvfile:
		reader = csv.DictReader(csvfile)
		l = []
		m = []
		n = []
		for row in reader:
			l.append(row['time'])
			m.append(row['temp'])
			n.append(row['x'])
			i += 1
		plt.plot(l,m)
		plt.grid(True)
		plt.title("Signal-Diagram")
		plt.xlabel("Sample")
		plt.ylabel("In-Phase")
		plt.savefig('foo2.png')		
	# output_file("plot.html")
	# print("here1")
	# p = figure(title="Temp test", x_axis_label='time', y_axis_label='temperature', output_backend="webgl")
	# p.multi_line([l, l], [m, n], legend="Stuff", line_width=1)


	
	show(p)
	
	
main()