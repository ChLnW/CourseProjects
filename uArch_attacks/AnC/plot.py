import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.backends.backend_pdf import PdfPages


def plot_pml(pml):
    pml_data = data[data[0] == pml].drop(columns=[0])
    if not pml_data.empty:
        plt.figure(figsize=(16, 16))
        sns.heatmap(pml_data.to_numpy(), cmap="Reds", linewidths=0.5, linecolor="white", cbar=False, square=True)
        # plt.ylabel("Page Number")
        # plt.xlabel("Cache Line Number")
        plt.title(pml.upper())

if __name__ == '__main__':
    data = pd.read_csv("pml.csv", header=None)

    plot_pml("pml1")
    plt.savefig("figures/pml1.pdf", format="pdf")
    plt.clf()
    
    with PdfPages("figures/pml234.pdf") as pdf:
        for pml in ['pml2', 'pml3', 'pml4']:
           plot_pml(pml)
           pdf.savefig()
           plt.clf()
