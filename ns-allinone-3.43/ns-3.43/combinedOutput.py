import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Define correct column names
column_names = [
    "SimulationSecond",
    "ReceiveRate",
    "PacketsReceived",
    "NumberOfSinks",
    "RoutingProtocol",
    "TransmissionPower",
    "SecurityType"
]

# Load CSV files with header names
aodv = pd.read_csv("AODV-secure-manet-routing-output.csv", names=column_names)
dsdv = pd.read_csv("DSDV-secure-manet-routing-output.csv", names=column_names)
olsr = pd.read_csv("OLSR-secure-manet-routing-output.csv", names=column_names)

# Combine into one dataframe
df = pd.concat([aodv, dsdv, olsr], ignore_index=True)

# Clean column names (remove spaces)
df.columns = df.columns.str.strip()

# Calculate Metrics
df['Throughput (kbps)'] = df['ReceiveRate'] / 1000
df['PacketDeliveryRatio (%)'] = (df['PacketsReceived'] / (df['NumberOfSinks'] * 100)) * 100
df['PacketLoss (%)'] = 100 - df['PacketDeliveryRatio (%)']
df['PacketDrop (%)'] = df['PacketLoss (%)']
df['Latency (ms)'] = 100  # Placeholder
df['EndToEndDelay (ms)'] = 100  # Placeholder
df['Jitter (ms)'] = 10  # Placeholder

# Group by protocol
metrics = df.groupby('RoutingProtocol').mean(numeric_only=True)[[
    'Throughput (kbps)',
    'PacketDeliveryRatio (%)',
    'PacketLoss (%)',
    'PacketDrop (%)',
    'Latency (ms)',
    'EndToEndDelay (ms)',
    'Jitter (ms)'
]].reset_index()

# Show Table
print("\n--- Comparison Table ---\n")
print(metrics)

# Save table to CSV
metrics.to_csv("comparative_metrics.csv", index=False)
print("\nSaved comparison table as 'comparative_metrics.csv'.")

# Set seaborn theme
sns.set(style="whitegrid")

# Plot individual graphs
metrics_melted = metrics.melt(id_vars='RoutingProtocol', var_name='Metric', value_name='Value')

plt.figure(figsize=(14, 8))
sns.barplot(x='RoutingProtocol', y='Value', hue='Metric', data=metrics_melted, palette='Set2')
plt.title('Comparison of Networking Metrics for Routing Protocols')
plt.ylabel('Metric Value')
plt.xlabel('Routing Protocol')
plt.xticks(rotation=0)
plt.legend(title='Metric', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()
plt.savefig('combined_comparison_graph.png')
plt.close()

print("\n✅ Combined graph saved as 'combined_comparison_graph.png'.")
