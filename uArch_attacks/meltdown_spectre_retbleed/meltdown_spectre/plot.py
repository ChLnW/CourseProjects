import pandas as pd
import matplotlib.pyplot as plt
import os

os.makedirs("./figures", exist_ok=True)

data = pd.read_csv("meltdown_data.csv")

plt.figure(figsize=(10, 6))

segv_data = data[data["type"] == "segv"]
plt.scatter(segv_data["time"], segv_data["acc"], label="segv", marker="o")

tsx_data = data[data["type"] == "tsx"]
plt.scatter(tsx_data["time"], tsx_data["acc"], label="tsx", marker="x")

tsx_data = data[data["type"] == "spectre"]
plt.scatter(tsx_data["time"], tsx_data["acc"], label="spectre", marker=".")


plt.xlabel("Time")
plt.ylabel("Accuracy")
plt.title("Meltdown")
plt.legend()
plt.grid(True)

plt.savefig("./figures/meltdown.pdf")
plt.close()

print("Plot saved to ./figures/meltdown.pdf")
