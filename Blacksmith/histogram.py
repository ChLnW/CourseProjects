import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.signal import find_peaks
from matplotlib.backends.backend_pdf import PdfPages


def save_threshold(hostname, threshold, thresholds_file):
    if os.path.exists(thresholds_file):
        thresholds_df = pd.read_csv(thresholds_file)
    else:
        thresholds_df = pd.DataFrame(columns=["hostname", "threshold"])

    if hostname not in thresholds_df["hostname"].values:
        new_entry = pd.DataFrame({"hostname": [hostname], "threshold": [threshold]})
        thresholds_df = pd.concat([thresholds_df, new_entry], ignore_index=True)
        thresholds_df.to_csv(thresholds_file, index=False)
        print(f"Threshold for {hostname} saved: {threshold:.2f}")
    else:
        print(f"Threshold for {hostname} already exists in {thresholds_file}")


def plot_histogram(data, hostname, output_dir):
    bucket_size = 1
    min_hits = 30

    # combine values into buckets of bucket_size
    bins = np.arange(min(data), max(data) + bucket_size, bucket_size)

    counts, bin_edges = np.histogram(data, bins=bins)

    peaks, _ = find_peaks(counts, distance=100)
    # ignore buckets that appear less than min_hits times
    valid_bins = counts >= min_hits

    valid_counts = counts[valid_bins]
    valid_bin_edges = bin_edges[:-1][valid_bins]

    valid_peaks = []
    for peak in peaks:
        if valid_bins[peak]:
            new_index = np.where(valid_bin_edges == bin_edges[peak])[0]
            if new_index.size > 0:
                valid_peaks.append(new_index[0])
    peaks = valid_peaks

    plt.figure(figsize=(10, 6))
    if len(peaks) >= 2:
        peak_values = valid_counts[peaks]
        sorted_peaks = np.argsort(peak_values)[-2:]
        peak_1 = peaks[sorted_peaks[0]]
        peak_2 = peaks[sorted_peaks[1]]

        # plotting the 2 highest peaks for debug
        plt.scatter(
            valid_bin_edges[peak_1], valid_counts[peak_1], color="red", zorder=5
        )
        plt.scatter(
            valid_bin_edges[peak_2], valid_counts[peak_2], color="red", zorder=5
        )

        threshold = (valid_bin_edges[peak_1] + valid_bin_edges[peak_2]) / 2
    else:
        threshold = np.median(data)

    plt.bar(valid_bin_edges, valid_counts, width=10)

    # plotting all found peaks for debug
    plt.scatter(valid_bin_edges[peaks], valid_counts[peaks], color="blue", zorder=4)

    plt.axvline(
        x=threshold,
        color="green",
        linestyle="dotted",
        linewidth=2,
        label=f"Bank Conflict Threshold ({threshold:.2f})",
    )

    plt.xlabel("Access Time (ms)", fontsize=14)
    plt.ylabel("Accesses", fontsize=14)
    plt.title(
        f"Access Time Histogram for {hostname}; bucket_size: {bucket_size} min_access: {min_hits}",
        fontsize=16,
    )

    plt.legend()
    plt.tight_layout()

    output_file = os.path.join(output_dir, f"{hostname}__drama.pdf")

    with PdfPages(output_file) as pdf:
        pdf.savefig()
        plt.close()

        plt.figure(figsize=(10, 6))
        data = data[data <= 600]
        unique_times, counts = np.unique(data, return_counts=True)
        plt.bar(unique_times, counts, width=1, color="blue", alpha=0.7)
        plt.xlabel("Access Time (ms)", fontsize=14)
        plt.ylabel("Accesses", fontsize=14)
        plt.title(f"Access Times for {hostname} (All Data < 600ms)", fontsize=16)

        pdf.savefig()
        plt.close()

    return int(threshold)


if __name__ == "__main__":
    data_dir = "./data/"
    output_dir = "./figures/"
    thresholds_file = os.path.join(data_dir, "thresholds.csv")

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(data_dir):
        if filename.endswith("__drama.csv"):
            file_path = os.path.join(data_dir, filename)

            df = pd.read_csv(file_path)

            access_times = df["timing"].values

            hostname = filename.split("__")[0]

            threshold = plot_histogram(access_times, hostname, output_dir)
            save_threshold(hostname, threshold, thresholds_file)
