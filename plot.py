import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import sys
import os

CSV_PATH = os.path.join("build", "trades.csv")

if len(sys.argv) > 1:
    CSV_PATH = sys.argv[1]

df = pd.read_csv(CSV_PATH)
df = df[df["bid"] > 0].reset_index(drop=True)
df["spread"] = df["ask"] - df["bid"]
x = df.index

fig = plt.figure(figsize=(14, 12))
fig.suptitle("Order Book — Market Maker Analysis", fontsize=14, fontweight="bold")
gs = gridspec.GridSpec(4, 2, figure=fig, hspace=0.5, wspace=0.35)

# Bid
ax1 = fig.add_subplot(gs[0, 0])
ax1.plot(x, df["bid"], color="#00cc44", linewidth=1.2)
ax1.set_title("Bid Quote")
ax1.set_ylabel("Price")
ax1.grid(True, alpha=0.3)

# Ask
ax2 = fig.add_subplot(gs[0, 1])
ax2.plot(x, df["ask"], color="#ff4444", linewidth=1.2)
ax2.set_title("Ask Quote")
ax2.set_ylabel("Price")
ax2.grid(True, alpha=0.3)

# Mid price
ax3 = fig.add_subplot(gs[1, 0])
ax3.plot(x, df["mid"], color="#3366ff", linewidth=1.2)
ax3.set_title("Mid Price")
ax3.set_ylabel("Price")
ax3.grid(True, alpha=0.3)

# Spread
ax4 = fig.add_subplot(gs[1, 1])
ax4.plot(x, df["spread"], color="#ff8800", linewidth=1.2)
ax4.set_title("Bid-Ask Spread")
ax4.set_ylabel("Spread (ticks)")
ax4.grid(True, alpha=0.3)

# Trade price
ax5 = fig.add_subplot(gs[2, 0])
ax5.plot(x, df["trade_price"], color="#9933cc", linewidth=1.2, alpha=0.85)
ax5.set_title("Trade Price")
ax5.set_ylabel("Price")
ax5.set_xlabel("Trade #")
ax5.grid(True, alpha=0.3)

# Inventory
ax6 = fig.add_subplot(gs[2, 1])
ax6.plot(x, df["inventory"], color="#cc6600", linewidth=1.2)
ax6.fill_between(x, df["inventory"], 0, alpha=0.2, color="#cc6600")
ax6.axhline(0, color="black", linewidth=0.8, linestyle="--")
ax6.set_title("Market Maker Inventory")
ax6.set_ylabel("Inventory")
ax6.set_xlabel("Trade #")
ax6.grid(True, alpha=0.3)

plt.savefig("order_book_analysis.png", dpi=150, bbox_inches="tight")
print("Saved order_book_analysis.png")
plt.show()
