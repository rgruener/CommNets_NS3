from pylab import *
import numpy as np
import sys

cwnds = []
cur_cwnd = []
legends = []
X = []
cur_X = []
colors = ['b', 'g', 'r', 'y']

for line in sys.stdin:
    try:
        nums = line[0:-1].split(':')
        cur_X.append(float(nums[0]))
        cur_cwnd.append(float(nums[1]))
    except ValueError:
        legends.append(line[0:-1])
        if len(cur_cwnd) > 0:
            X.append(cur_X)
            cwnds.append(cur_cwnd)
        cur_cwnd = []
        cur_X = []
if len(cur_cwnd) > 0:
    X.append(cur_X)
    cwnds.append(cur_cwnd)
        

figure(figsize=(8,6), dpi=80)


for i in range(0,len(cwnds)):
    subplot(2,2,i+1)
    plot(X[i], cwnds[i], color=colors[i], label=legends[i])
    legend()
    ylabel('Cwnd Size')
    xlabel('Time (seconds)')
    title('Command Window Size vs. Time')

savefig("graph_2.png",dpi=72)
show()
