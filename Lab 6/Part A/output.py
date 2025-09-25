import matplotlib.pyplot as plt
import numpy as np

# Read data from files
def read_data(filename):
    time = []
    value = []
    with open(filename, 'r') as f:
        for line in f:
            t, v = map(float, line.strip().split())
            time.append(t)
            value.append(v)
    return time, value

# Create plots
plt.figure(figsize=(15, 10))

# 1. Throughput vs Time
plt.subplot(221)
time, throughput = read_data('throughput.dat')
plt.plot(time, throughput)
plt.title('Throughput vs Time')
plt.xlabel('Time (s)')
plt.ylabel('Throughput (Mbps)')
plt.grid(True)

# 2. Packet Loss vs Time
plt.subplot(222)
time, packet_loss = read_data('packet_loss.dat')
plt.plot(time, packet_loss)
plt.title('Packet Loss vs Time')
plt.xlabel('Time (s)')
plt.ylabel('Packet Loss (%)')
plt.grid(True)

# 3. Average Delay vs Time
plt.subplot(223)
time, delay = read_data('delay.dat')
plt.plot(time, delay)
plt.title('Average Delay vs Time')
plt.xlabel('Time (s)')
plt.ylabel('Delay (ms)')
plt.grid(True)

# 4. Latency vs Time
plt.subplot(224)
time, latency = read_data('latency.dat')  # Assuming latency data is saved in 'latency.dat'
plt.plot(time, latency)
plt.title('Latency vs Time')
plt.xlabel('Time (s)')
plt.ylabel('Latency (ms)')
plt.grid(True)

# Adjust layout
plt.tight_layout()

# Save and display the plot
plt.savefig('Network_metrics.png', dpi=300, bbox_inches='tight')
#plt.show()  # Display the plot
