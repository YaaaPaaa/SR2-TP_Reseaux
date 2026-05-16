import csv
import matplotlib.pyplot as plt
import numpy as np

# Load the CSV file
filename = "perf.txt"
time = []
packets_sent = []
packets_droped = []

with open(filename, mode='r') as file:
    reader = csv.reader(file, delimiter=';')
    next(reader)  # Skip the header row
    for row in reader:
        time.append(int(row[0]))
        packets_sent.append(int(row[1]))
        packets_droped.append(int(row[2]))

period = time[1] - time[0]

# Create the plot
fig, ax = plt.subplots()

# Set axis labels
ax.set_xlabel('Time (ms)')
ax.set_ylabel('Packets / ' + str(period) + ' ms')
# Plot packets sent and droped on the y-axis
ax.plot(time, packets_sent, color='tab:blue', label='Throughtput')
ax.axhline(y=np.nanmean(packets_sent), color='tab:blue', linestyle='--', label='Avg Throughput')
ax.plot(time, packets_droped, color='tab:red', label='Drops')
ax.legend()

# Add a title
plt.title('Throughput and packet loss over time')

# Show the plot
plt.show()
