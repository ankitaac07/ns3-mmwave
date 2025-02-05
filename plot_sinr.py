import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

sinr_files = [
    'MmWaveSinrTime_Building_0_fixedUE', 
    'MmWaveSinrTime_Building_1_fixedUE', 
    'MmWaveSinrTime_Building_2_fixedUE'
]

sinr_val_list_files = []

for file in sinr_files:
    sinr_val_list = []
    with open(file, 'r') as fp:
        timestamp_dict = {}
        for line in fp:
            time_val, _, _, sinr_val = line.split(' ')
            time_val = float(time_val)
            sinr_val = float(sinr_val)
            if time_val not in timestamp_dict or sinr_val > timestamp_dict[time_val]:
                timestamp_dict[time_val] = sinr_val
        sinr_val_list.extend(timestamp_dict.values())
    sinr_val_list_files.append(sinr_val_list)

plt.figure(figsize=(8, 6))
sns.violinplot(
    data=sinr_val_list_files, 
    palette="muted", 
    bw=0.2
)

plt.xticks(ticks=range(len(sinr_files)), labels=['Building_0', 'Building_1', 'Building_2'])
plt.ylabel('SINR (dB)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

plt.savefig('sinr_violin_plot.png')
plt.show()