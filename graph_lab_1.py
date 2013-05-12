from pylab import *
import sys

throughput = []

for line in sys.stdin:
    throughput.append(float(line[0:-1]))

figure(figsize=(8,6), dpi=80)

subplot(1,1,1)

X = range(1,5001)
plot(X, throughput)
ylabel('Throughput (Kbps)')
xlabel('Latency (ms)')
title('Throughput of Link vs. Latency')

savefig("graph.png",dpi=72)
show()
