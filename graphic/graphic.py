import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


# Функция для расчета APCER и BPCER
def calculate_apcer_bpcer(df, thresholds):
    apcer = []
    bpcer = []

    total_attacks = len(df[df['Result'] == 'attack'])
    total_real = len(df[df['Result'] == 'real'])

    for threshold in thresholds:
        false_negative_attacks = len(df[(df['Result'] == 'attack') & (df['Confidence'] >= threshold)])
        false_positive_real = len(df[(df['Result'] == 'real') & (df['Confidence'] < threshold)])

        # APCER и BPCER
        apcer.append(false_negative_attacks / total_attacks if total_attacks != 0 else 0)
        bpcer.append(false_positive_real / total_real if total_real != 0 else 0)

    return apcer, bpcer

df_v3 = pd.read_csv("2d_light_version_3.csv")
df_v3['Confidence'] = pd.to_numeric(df_v3['Confidence'], errors='coerce')
df_v3 = df_v3[df_v3['Confidence'] != -1]

df_v1 = pd.read_csv("liveness_results.csv")
df_v1['Confidence'] = pd.to_numeric(df_v1['Confidence'], errors='coerce')
df_v1 = df_v1[df_v1['Confidence'] != -1]

thresholds = np.linspace(0, 1, 100)

apcer_v3, bpcer_v3 = calculate_apcer_bpcer(df_v3, thresholds)
apcer_v1, bpcer_v1 = calculate_apcer_bpcer(df_v1, thresholds)

plt.figure()
plt.plot(thresholds, apcer_v3, label="APCER", color='red', linestyle='--')
plt.plot(thresholds, bpcer_v3, label="BPCER", color='blue', linestyle='-')
plt.xlabel("Confidence")
plt.ylabel("APCER / BPCER")
plt.title("2d_light, version 3")
plt.legend()
plt.grid(True)
plt.show()

plt.figure()
plt.plot(thresholds, apcer_v1, label="APCER", color='red', linestyle='--')
plt.plot(thresholds, bpcer_v1, label="BPCER", color='blue', linestyle='-')
plt.xlabel("Confidence")
plt.ylabel("APCER / BPCER")
plt.title("Liveness Detection")
plt.legend()
plt.grid(True)
plt.show()

plt.figure()

times_v3 = df_v3['Time(ms)']
times_v1 = df_v1['Time(ms)']

indexes_v3 = np.arange(len(times_v3))
indexes_v1 = np.arange(len(times_v1))

plt.plot(indexes_v3, times_v3, label="2d_light, version 3", color='green', linestyle='--')
plt.plot(indexes_v1, times_v1, label="Liveness Detection", color='purple', linestyle='-')

plt.xlabel("Image Number")
plt.ylabel("Time (ms)")
plt.title("Время обработки изображений (2d_light, version 3/Liveness Detection)")
plt.legend()
plt.grid(True)
plt.show()
