import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Define correct column names (because your CSVs have NO header row)
column_names = [
    "SimulationSecond",
    "ReceiveRate",
    "PacketsReceived",
    "NumberOfSinks",
    "RoutingProtocol",
    "TransmissionPower",
    "SecurityType"
]

# Load CSVs with proper column names
aodv = pd.read_csv("AODV-secure-manet-routing-output.csv", names=column_names)
dsdv = pd.read_csv("DSDV-secure-manet-routing-output.csv", names=column_names)
olsr = pd.read_csv("OLSR-secure-manet-routing-output.csv", names=column_names)

# Combine into one dataframe
df = pd.concat([aodv, dsdv, olsr], ignore_index=True)

# Clean column names (remove spaces, if any)
df.columns = df.columns.str.strip()

# Calculate Metrics
df['Throughput (kbps)'] = df['ReceiveRate'] / 1000  # assuming ReceiveRate is in bits/sec
df['PacketDeliveryRatio (%)'] = (df['PacketsReceived'] / (df['NumberOfSinks'] * 100)) * 100  # assumed 100 packets sent per sink
df['PacketLoss (%)'] = 100 - df['PacketDeliveryRatio (%)']
df['PacketDrop (%)'] = df['PacketLoss (%)']  # same as packet loss in basic case
df['Latency (ms)'] = 100  # placeholder, since your CSV doesn't have timing info
df['EndToEndDelay (ms)'] = 100  # placeholder
df['Jitter (ms)'] = 10  # placeholder

# Group by RoutingProtocol and calculate average metrics
metrics = df.groupby('RoutingProtocol').mean(numeric_only=True)[[
    'Throughput (kbps)',
    'PacketDeliveryRatio (%)',
    'PacketLoss (%)',
    'PacketDrop (%)',
    'Latency (ms)',
    'EndToEndDelay (ms)',
    'Jitter (ms)'
]].reset_index()

# Print Table
print("\n--- Comparison Table ---\n")
print(metrics)

# Save the table to CSV
metrics.to_csv("comparative_metrics.csv", index=False)
print("\nSaved comparison table as 'comparative_metrics.csv'.")

# Plot Graphs
sns.set(style="whitegrid")

# Plot 1: Throughput
plt.figure(figsize=(8,6))
sns.barplot(x='RoutingProtocol', y='Throughput (kbps)', data=metrics, palette='muted')
plt.title('Throughput Comparison')
plt.ylabel('Throughput (kbps)')
plt.savefig('throughput_comparison.png')
plt.close()

# Plot 2: Packet Delivery Ratio
plt.figure(figsize=(8,6))
sns.barplot(x='RoutingProtocol', y='PacketDeliveryRatio (%)', data=metrics, palette='muted')
plt.title('Packet Delivery Ratio Comparison')
plt.ylabel('Packet Delivery Ratio (%)')
plt.savefig('pdr_comparison.png')
plt.close()

# Plot 3: Packet Loss
plt.figure(figsize=(8,6))
sns.barplot(x='RoutingProtocol', y='PacketLoss (%)', data=metrics, palette='muted')
plt.title('Packet Loss Comparison')
plt.ylabel('Packet Loss (%)')
plt.savefig('packet_loss_comparison.png')
plt.close()

print("\n✅ Graphs saved: 'throughput_comparison.png', 'pdr_comparison.png', 'packet_loss_comparison.png'.")
