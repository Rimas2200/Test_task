import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


# Функция для вычисления APCER и BPCER относительно порогового значения
def calculate_apcer_bpcer(df, threshold, threshold_index=3363):
    # Разделяем данные на реальные лица и атаки
    bona_fide_data = df.iloc[:threshold_index]
    attack_data = df.iloc[threshold_index:]

    # Подсчет ошибок для реальных лиц (BPCER) относительно порога
    bpc_error_count = bona_fide_data[bona_fide_data['Liveness Score'] < threshold].shape[0]
    bpc_total = bona_fide_data.shape[0]
    bpc_error_rate = bpc_error_count / bpc_total if bpc_total > 0 else 0

    # Подсчет ошибок для атак (APCER) относительно порога
    apc_error_count = attack_data[attack_data['Liveness Score'] >= threshold].shape[0]
    apc_total = attack_data.shape[0]
    apc_error_rate = apc_error_count / apc_total if apc_total > 0 else 0

    return apc_error_rate, bpc_error_rate


# Функция для построения графика APCER и BPCER по пороговым значениям
def plot_graphs(file_name):
    df = pd.read_csv(file_name)

    # Получаем диапазон значений Liveness Score для возможных порогов
    liveness_scores = np.linspace(df['Liveness Score'].min(), df['Liveness Score'].max(), 100)

    # Массивы для хранения результатов
    apcer_values = []
    bpcer_values = []

    # Рассчитываем APCER и BPCER для каждого порога
    for threshold in liveness_scores:
        apcer, bpcer = calculate_apcer_bpcer(df, threshold)
        apcer_values.append(apcer)
        bpcer_values.append(bpcer)

    plt.figure(figsize=(10, 6))
    plt.plot(liveness_scores, apcer_values, label='APCER', color='red')
    plt.plot(liveness_scores, bpcer_values, label='BPCER', color='blue')

    plt.title(f'Liveness Quality Metrics {file_name}')
    plt.xlabel('Confidence')
    plt.ylabel('APCER/BPCER')
    plt.legend()
    plt.grid(True)
    plt.show()


if __name__ == "__main__":
    files = ['test_2d_light_v3.csv', 'Liveness_Detection.csv']
    for file in files:
        plot_graphs(file)
