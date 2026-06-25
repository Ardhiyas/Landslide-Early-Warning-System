/*
  =====================================================================
  TANAALERT - MODEL RANDOM FOREST (rf_model.h)
  =====================================================================
  File ini di-generate OTOMATIS dari hasil training Random Forest
  (scikit-learn) menggunakan dataset 90 sampel (30 per kelas: Aman,
  Bahaya, Waspada).

  Konfigurasi training:
    - n_estimators = 9 pohon
    - max_depth    = 3
    - Akurasi cross-validation (5-fold) = 98.9%

  Mapping kelas (HARUS sama dengan urutan ini, jangan diubah manual):
    0 = Aman
    1 = Bahaya
    2 = Waspada

  CATATAN PENTING:
  - Dataset masih kecil (90 sampel). Threshold di bawah valid untuk
    pola pada dataset awal ini. Saat data lapangan NODE-07 sudah
    terkumpul lebih banyak (disarankan 300+ sampel dengan variasi
    kondisi nyata), sebaiknya model dilatih ulang dan file ini
    di-generate ulang.
  - Jangan edit angka threshold manual kecuali Anda re-train modelnya,
    karena pohon-pohon ini saling melengkapi (voting), mengubah satu
    angka bisa menggeser keseimbangan voting keseluruhan.
  =====================================================================
*/

#ifndef RF_MODEL_H
#define RF_MODEL_H

// ====================== 9 POHON KEPUTUSAN (HASIL TRAINING) ======================

int treeVote_0(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (gyroMag <= 24.395f) {
    return 0;
  } else {
    if (accelMag <= 1.720f) {
      return 2;
    } else {
      return 1;
    }
  }
}

int treeVote_1(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1839.000f) {
    return 1;
  } else {
    if (soilPct <= 34.000f) {
      return 0;
    } else {
      return 2;
    }
  }
}

int treeVote_2(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (accelMag <= 1.725f) {
    if (gyroMag <= 8.820f) {
      return 0;
    } else {
      return 2;
    }
  } else {
    return 1;
  }
}

int treeVote_3(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1839.000f) {
    return 1;
  } else {
    if (pitch <= 3.700f) {
      if (rainADC <= 2804.500f) {
        return 2;
      } else {
        return 0;
      }
    } else {
      return 2;
    }
  }
}

int treeVote_4(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (pitch <= 13.375f) {
    if (rainPct <= 67.500f) {
      if (soilADC <= 2608.000f) {
        return 2;
      } else {
        return 0;
      }
    } else {
      return 1;
    }
  } else {
    if (soilADC <= 1818.500f) {
      return 1;
    } else {
      return 2;
    }
  }
}

int treeVote_5(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (rainPct <= 67.000f) {
    if (rainADC <= 2931.500f) {
      return 2;
    } else {
      return 0;
    }
  } else {
    return 1;
  }
}

int treeVote_6(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (rainADC <= 2804.500f) {
    if (soilPct <= 67.500f) {
      return 2;
    } else {
      return 1;
    }
  } else {
    return 0;
  }
}

int treeVote_7(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (rainADC <= 2804.500f) {
    if (soilPct <= 67.500f) {
      return 2;
    } else {
      return 1;
    }
  } else {
    return 0;
  }
}

int treeVote_8(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (accelMag <= 1.720f) {
    if (rainPct <= 29.500f) {
      if (gyroMag <= 42.310f) {
        return 0;
      } else {
        return 2;
      }
    } else {
      return 2;
    }
  } else {
    return 1;
  }
}

// ====================== VOTING (RANDOM FOREST) ======================
// Menjalankan 9 pohon, lalu mengambil kelas dengan jumlah vote terbanyak
// (majority voting), sesuai cara kerja Random Forest standar.
//
// Return value: 0 = Aman, 1 = Bahaya, 2 = Waspada
int classifyRiskRF(int rainADC, int rainPct, int soilADC, int soilPct,
                    float accelMag, float gyroMag, float pitch, float roll) {
  int votes[3] = {0, 0, 0};

  votes[treeVote_0(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_1(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_2(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_3(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_4(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_5(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_6(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_7(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;
  votes[treeVote_8(rainADC, rainPct, soilADC, soilPct, accelMag, gyroMag, pitch, roll)]++;

  int bestClass = 0;
  for (int i = 1; i < 3; i++) {
    if (votes[i] > votes[bestClass]) bestClass = i;
  }
  return bestClass;
}

// Helper: ubah index kelas menjadi teks
inline String riskClassToString(int riskClass) {
  switch (riskClass) {
    case 0: return "AMAN";
    case 1: return "BAHAYA";
    case 2: return "WASPADA";
    default: return "UNKNOWN";
  }
}

#endif
