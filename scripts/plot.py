import sys
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

read_from_file = False
filename = ""
title = "Filter"
offset=0

def move_figure(f, x, y):
    """Move figure's upper left corner to pixel (x, y)"""
    backend = matplotlib.get_backend()
    if backend == 'TkAgg':
        f.canvas.manager.window.wm_geometry("+%d+%d" % (x, y))
    elif backend == 'WXAgg':
        f.canvas.manager.window.SetPosition((x, y))
    else:
        # This works for QT and GTK
        # You can also use window.setGeometry
        f.canvas.manager.window.move(x, y)

if (len(sys.argv) > 1):
    read_from_file = True
    filename = sys.argv[1]
    print("Reading from file: " + filename)        
    file = open(filename, 'r') 

if (len(sys.argv) > 2):
    title = sys.argv[2]

if (len(sys.argv) > 3):
    offset=sys.argv[3]

data = [0]

print "Processing data.."
while True:
    if (read_from_file):
        line = file.readline()
    else:
        line = sys.stdin.readline()
    if line == '':
        break
    line.strip()
    print line
    try:
        number = float(line)
    except ValueError as e:
        print e
    #add number to array, plot the data
    data.append(number)

if (read_from_file):
    file.close()

f,ax = plt.subplots()
ax.set_title(title)
ax.set_ylabel('response')
ax.set_xlabel('samples')
ax.xaxis.set_major_locator(ticker.MultipleLocator(20))
ax.grid(True)
# create a figure with a single axis
move_figure(f, 100+int(offset), 100)
ax.plot(data)
plt.show()
