import pandas as pd
import matplotlib.pyplot as plt
import os

# Define the columns manually
columns = ["SimulationSecond", "ReceiveRate", "PacketsReceived", "NumberOfSinks", 
           "RoutingProtocol", "TransmissionPower", "SecurityType"]

# List of CSV files
files = [
    "DSDV-secure-fanet-routing-output.csv",
    "DSDV-unsecure-fanet-routing-output.csv",
    "AODV-secure-fanet-routing-output.csv",
    "AODV-unsecure-fanet-routing-output.csv",
    "OLSR-secure-fanet-routing-output.csv",
    "OLSR-unsecure-fanet-routing-output.csv"
]

# Store results
data = []

# Load data from files
for file in files:
    try:
        df = pd.read_csv(file, header=None, names=columns)
        avg_throughput = df["ReceiveRate"].mean()
        total_packets = df["PacketsReceived"].sum()
        protocol = df["RoutingProtocol"].iloc[0]
        secure = df["SecurityType"].iloc[0]

        data.append({
            "Protocol": protocol,
            "Security": secure,
            "Throughput (kbps)": avg_throughput,
            "Total Packets Received": total_packets
        })
    except Exception as e:
        print(f"⚠️ Error processing {file}: {e}")

# Convert to DataFrame
metrics_df = pd.DataFrame(data)

# Create separate graphs for each protocol
protocols = metrics_df["Protocol"].unique()

for protocol in protocols:
    proto_df = metrics_df[metrics_df["Protocol"] == protocol]

    # Convert 'Security' column to string for plotting
    security_labels = proto_df["Security"].astype(str)

    fig, axs = plt.subplots(1, 2, figsize=(10, 5))
    fig.suptitle(f"{protocol} - Secure vs Unsecure Comparison", fontsize=14)

    axs[0].bar(security_labels, proto_df["Throughput (kbps)"], color=['green', 'orange'])
    axs[0].set_title("Average Throughput (kbps)")
    axs[0].set_ylabel("Throughput (kbps)")

    axs[1].bar(security_labels, proto_df["Total Packets Received"], color=['blue', 'red'])
    axs[1].set_title("Total Packets Received")
    axs[1].set_ylabel("Packets Received")

    plt.tight_layout(rect=[0, 0, 1, 0.95])
    output_filename = f"{protocol}_comparison.png"
    plt.savefig(output_filename)
    print(f"✅ Saved graph for {protocol} as {output_filename}")
