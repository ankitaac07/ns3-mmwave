# import matplotlib.pyplot as plt
# import seaborn as sns
# import numpy as np

# sinr_files = [
#     'MmWaveSinrTime_Building_indoor_fixedUE.txt', 
#     'MmWaveSinrTime_Building_outdoor_fixedUE.txt'
# ]

# sinr_val_list_files = []

# for file in sinr_files:
#     sinr_val_list = []
#     with open(file, 'r') as fp:
#         timestamp_dict = {}
#         for line in fp:
#             time_val, _, _, sinr_val = line.split(' ')
#             time_val = float(time_val)
#             sinr_val = float(sinr_val)
#             if time_val not in timestamp_dict or sinr_val > timestamp_dict[time_val]:
#                 timestamp_dict[time_val] = sinr_val
#         sinr_val_list.extend(timestamp_dict.values())
#     sinr_val_list_files.append(sinr_val_list)

# plt.figure(figsize=(8, 6))
# sns.violinplot(
#     data=sinr_val_list_files, 
#     palette="muted", 
#     bw=0.2
# )

# plt.xticks(ticks=range(len(sinr_files)), labels=['ue_indoor', 'ue_outdoor'])
# plt.ylabel('SINR (dB)')
# plt.grid(axis='y', linestyle='--', alpha=0.7)

# plt.savefig('sinr_violin_plot.png')
# plt.show()


# import csv
# import glob
# import matplotlib.pyplot as plt
# import numpy as np

# sinr_files = ['MmWaveSinrTime_Building_indoor_fixedUE.txt', 'MmWaveSinrTime_Building_outdoor_fixedUE.txt']
# energy_files = ['energy_consumption_Building_indoor_fixedue.csv','energy_consumption_Building_outdoor_fixedue.csv']
# sinr_val_list_files = []
# for file in sinr_files:
#     time_val_list, ue_id_list, cell_id_list, sinr_val_list = [], [], [], []
#     with open(file, 'r') as fp:
#         lines = fp.readlines()
#         for line in lines:
#             time_val, ue_id, cell_id, sinr_val = line.split(' ')
#             time_val_list.append(time_val)
#             ue_id_list.append(ue_id)
#             cell_id_list.append(cell_id)
#             # Convert sinr_val to float
#             sinr_val_list.append(float(sinr_val))  # Convert to float here
#     sinr_val_list_files.append(sinr_val_list)

# # Now plot the bar chart
# plt.bar(
#     ['ue_indoor', 'ue_outdoor'], 
#     [np.mean(sinr_val_list_files[0]), np.mean(sinr_val_list_files[1])],  # Only two files
#     yerr=[np.std(sinr_val_list_files[0]), np.std(sinr_val_list_files[1])]
# )
# plt.ylabel('SINR (dB)')
# plt.grid()
# plt.show()


import csv
import glob
import matplotlib.pyplot as plt
import numpy as np

# File names
sinr_files = ['MmWaveSinrTime_Building_indoor_fixedUE.txt', 'MmWaveSinrTime_Building_outdoor_fixedUE.txt']
energy_files = ['energy_consumption_Building_indoor_fixedue.csv', 'energy_consumption_Building_outdoor_fixedue.csv']

# Reading SINR data
sinr_val_list_files = []
for file in sinr_files:
    time_val_list, ue_id_list, cell_id_list, sinr_val_list = [], [], [], []
    with open(file, 'r') as fp:
        lines = fp.readlines()
        for line in lines:
            time_val, ue_id, cell_id, sinr_val = line.split(' ')
            time_val_list.append(time_val)
            ue_id_list.append(ue_id)
            cell_id_list.append(cell_id)
            # Convert sinr_val to float
            sinr_val_list.append(float(sinr_val))  # Convert to float here
    sinr_val_list_files.append(sinr_val_list)

# Reading Energy Consumption data
energy_val_list_files = []
for file in energy_files:
    energy_val_list = []
    with open(file, 'r') as fp:
        reader = csv.reader(fp)
        next(reader)  # Skip the header row
        for row in reader:
            try:
                # Assuming energy consumption is in the first column
                energy_val_list.append(float(row[0]))  # Convert to float here
            except ValueError:
                # If the value cannot be converted to float, skip the row or handle accordingly
                continue
    energy_val_list_files.append(energy_val_list)

# Plotting SINR data
plt.figure(figsize=(12, 6))

# First subplot for SINR
plt.subplot(1, 2, 1)
plt.bar(
    ['ue_indoor', 'ue_outdoor'], 
    [np.mean(sinr_val_list_files[0]), np.mean(sinr_val_list_files[1])],  # Only two files
    yerr=[np.std(sinr_val_list_files[0]), np.std(sinr_val_list_files[1])]
)
plt.ylabel('SINR (dB)')
plt.grid()
plt.title('SINR Comparison')

# Second subplot for Energy Consumption
plt.subplot(1, 2, 2)
plt.bar(
    ['ue_indoor', 'ue_outdoor'], 
    [np.mean(energy_val_list_files[0]), np.mean(energy_val_list_files[1])],  # Only two files
    yerr=[np.std(energy_val_list_files[0]), np.std(energy_val_list_files[1])]
)
plt.ylabel('Energy Consumption (Joules)')
plt.grid()
plt.title('Energy Consumption Comparison')

# Show both plots
plt.tight_layout()
plt.show()


