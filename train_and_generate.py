"""
=======================================================================
TANAALERT - Script Training & Code Generator
=======================================================================
Fungsi script ini:
1. Membaca dataset CSV (bisa lebih dari satu file, akan digabung)
2. Melatih model Random Forest
3. Mengevaluasi performa (cross-validation + train/test split)
4. Generate ULANG file rf_model.h secara OTOMATIS, siap upload ke ESP32

Cara pakai:
    python train_and_generate.py dataset1.csv dataset2.csv ...

Kalau cuma satu file:
    python train_and_generate.py dataset_sensor_baru.csv

Format kolom CSV WAJIB sama seperti dataset awal:
    Hujan ADC, Hujan %, Soil ADC, Soil %, accel_magnitude,
    gyro_magnitude, pitch, roll, Label
(Label isinya: Aman / Waspada / Bahaya)

Requirement: pip install pandas scikit-learn
=======================================================================
"""

import sys
import pandas as pd
import numpy as np
from sklearn.model_selection import (
    train_test_split, cross_val_score, StratifiedKFold
)
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import classification_report, confusion_matrix

# ====================== KONFIGURASI ======================
N_ESTIMATORS = 9      # jumlah pohon -- naikkan kalau dataset makin besar/variatif
MAX_DEPTH = 3          # kedalaman tiap pohon -- jaga kecil agar tetap ringan di ESP32
RANDOM_STATE = 42
OUTPUT_HEADER = "rf_model.h"

FEATURES = [
    "Hujan ADC", "Hujan %", "Soil ADC", "Soil %",
    "accel_magnitude", "gyro_magnitude", "pitch", "roll",
]

# Mapping nama kolom dataset -> nama variabel di kode C (HARUS sama dengan
# parameter classifyRiskRF() di receiver_edge_gateway.ino)
CVAR = {
    "Hujan ADC": "rainADC",
    "Hujan %": "rainPct",
    "Soil ADC": "soilADC",
    "Soil %": "soilPct",
    "accel_magnitude": "accelMag",
    "gyro_magnitude": "gyroMag",
    "pitch": "pitch",
    "roll": "roll",
}

# Urutan kelas HARUS konsisten dengan riskClassToString() di rf_model.h
# (LabelEncoder akan urutkan alfabetis: Aman=0, Bahaya=1, Waspada=2)
EXPECTED_CLASS_ORDER = ["Aman", "Bahaya", "Waspada"]


def load_dataset(paths):
    dfs = []
    for p in paths:
        df = pd.read_csv(p, sep=None, engine="python", decimal=",")
        df.columns = [c.strip() for c in df.columns]
        dfs.append(df)
    full = pd.concat(dfs, ignore_index=True)
    print(f"[INFO] Total data setelah digabung: {len(full)} baris")
    print(full["Label"].value_counts())
    return full


def evaluate_model(X, y, class_names):
    skf = StratifiedKFold(n_splits=5, shuffle=True, random_state=RANDOM_STATE)
    rf = RandomForestClassifier(
        n_estimators=N_ESTIMATORS, max_depth=MAX_DEPTH, random_state=RANDOM_STATE
    )
    scores = cross_val_score(rf, X, y, cv=skf)
    print(f"\n[HASIL] Cross-validation accuracy (5-fold): {scores}")
    print(f"[HASIL] Rata-rata akurasi: {scores.mean()*100:.2f}%")

    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, stratify=y, random_state=RANDOM_STATE
    )
    rf_eval = RandomForestClassifier(
        n_estimators=N_ESTIMATORS, max_depth=MAX_DEPTH, random_state=RANDOM_STATE
    )
    rf_eval.fit(X_train, y_train)
    pred = rf_eval.predict(X_test)

    print("\n[HASIL] Classification report (test set):")
    print(classification_report(y_test, pred, target_names=class_names))
    print("[HASIL] Confusion matrix (baris=aktual, kolom=prediksi):")
    print(class_names)
    print(confusion_matrix(y_test, pred))

    if scores.mean() < 0.85:
        print("\n[PERINGATAN] Akurasi cross-validation di bawah 85%.")
        print("  -> Pertimbangkan tambah data, cek label yang mungkin salah,")
        print("     atau tambah N_ESTIMATORS/MAX_DEPTH sedikit.")


def gen_tree_code(tree, func_name):
    t = tree.tree_
    lines = [
        f"int {func_name}(int rainADC, int rainPct, int soilADC, int soilPct, "
        f"float accelMag, float gyroMag, float pitch, float roll) {{"
    ]

    def recurse(node, depth):
        indent = "  " * (depth + 1)
        if t.feature[node] != -2:  # bukan leaf
            fname = CVAR[FEATURES[t.feature[node]]]
            thresh = t.threshold[node]
            lines.append(f"{indent}if ({fname} <= {thresh:.3f}f) {{")
            recurse(t.children_left[node], depth + 1)
            lines.append(f"{indent}}} else {{")
            recurse(t.children_right[node], depth + 1)
            lines.append(f"{indent}}}")
        else:
            cls = int(np.argmax(t.value[node][0]))
            lines.append(f"{indent}return {cls};")

    recurse(0, 0)
    lines.append("}")
    return "\n".join(lines)


def generate_header(rf, class_names, n_samples):
    tree_codes = [
        gen_tree_code(est, f"treeVote_{i}") for i, est in enumerate(rf.estimators_)
    ]
    n_trees = len(rf.estimators_)

    vote_calls = "\n".join(
        f"  votes[treeVote_{i}(rainADC, rainPct, soilADC, soilPct, accelMag, "
        f"gyroMag, pitch, roll)]++;"
        for i in range(n_trees)
    )

    class_switch = "\n".join(
        f"    case {i}: return \"{name.upper()}\";" for i, name in enumerate(class_names)
    )

    header = f'''/*
  =====================================================================
  TANAALERT - MODEL RANDOM FOREST (rf_model.h)
  =====================================================================
  File ini di-generate OTOMATIS oleh train_and_generate.py

  Konfigurasi training:
    - Jumlah data latih : {n_samples} sampel
    - n_estimators       : {n_trees} pohon
    - max_depth          : {MAX_DEPTH}

  Mapping kelas (urutan ini ditentukan otomatis oleh LabelEncoder,
  JANGAN diubah manual):
{chr(10).join(f"    {i} = {name}" for i, name in enumerate(class_names))}
  =====================================================================
*/

#ifndef RF_MODEL_H
#define RF_MODEL_H

// ====================== {n_trees} POHON KEPUTUSAN (HASIL TRAINING) ======================

{chr(10).join(tree_codes)}

// ====================== VOTING (RANDOM FOREST) ======================
int classifyRiskRF(int rainADC, int rainPct, int soilADC, int soilPct,
                    float accelMag, float gyroMag, float pitch, float roll) {{
  int votes[{len(class_names)}] = {{{", ".join(["0"] * len(class_names))}}};

{vote_calls}

  int bestClass = 0;
  for (int i = 1; i < {len(class_names)}; i++) {{
    if (votes[i] > votes[bestClass]) bestClass = i;
  }}
  return bestClass;
}}

inline String riskClassToString(int riskClass) {{
  switch (riskClass) {{
{class_switch}
    default: return "UNKNOWN";
  }}
}}

#endif
'''
    with open(OUTPUT_HEADER, "w") as f:
        f.write(header)
    print(f"\n[OK] {OUTPUT_HEADER} berhasil di-generate ulang ({n_trees} pohon).")
    print("     Salin file ini ke folder receiver_edge_gateway, lalu upload ulang.")


def main():
    if len(sys.argv) < 2:
        print("Pakai: python train_and_generate.py dataset1.csv [dataset2.csv ...]")
        sys.exit(1)

    df = load_dataset(sys.argv[1:])
    X = df[FEATURES].values

    le = LabelEncoder()
    y = le.fit_transform(df["Label"].values)
    class_names = list(le.classes_)

    if class_names != EXPECTED_CLASS_ORDER:
        print(f"[PERINGATAN] Urutan label terdeteksi: {class_names}")
        print(f"             (Biasanya: {EXPECTED_CLASS_ORDER})")
        print("             Pastikan riskClassToString() di kode C tetap sesuai urutan ini.")

    evaluate_model(X, y, class_names)

    # Latih model FINAL pakai SELURUH data (bukan cuma 80% training split)
    rf_final = RandomForestClassifier(
        n_estimators=N_ESTIMATORS, max_depth=MAX_DEPTH, random_state=RANDOM_STATE
    )
    rf_final.fit(X, y)

    generate_header(rf_final, class_names, len(df))


if __name__ == "__main__":
    main()
