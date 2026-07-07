/*
  =====================================================================
  TANAALERT - MODEL RANDOM FOREST (rf_model.h)
  =====================================================================
  File ini di-generate OTOMATIS oleh train_and_generate.py

  Konfigurasi training:
    - Jumlah data latih : 1200 sampel
    - n_estimators       : 9 pohon
    - max_depth          : 3

  Mapping kelas (urutan ini ditentukan otomatis oleh LabelEncoder,
  JANGAN diubah manual):
    0 = Aman
    1 = Bahaya
    2 = Waspada
  =====================================================================
*/

#ifndef RF_MODEL_H
#define RF_MODEL_H

// ====================== 9 POHON KEPUTUSAN (HASIL TRAINING) ======================

int treeVote_0(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (gyroMag <= 151.035f) {
    if (soilADC <= 2603.500f) {
      if (soilPct <= 70.000f) {
        return 2;
      } else {
        return 2;
      })
    } else {
      if (gyroMag <= 4.270f) {
        return 0;
      } else {
        return 2;
      })
    })
  } else {
    if (rainPct <= 21.000f) {
      if (rainADC <= 4090.000f) {
        return 1;
      } else {
        return 1;
      })
    } else {
      if (accelMag <= 837.500f) {
        return 1;
      } else {
        return 2;
      })
    })
  })
}
int treeVote_1(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1178.500f) {
    if (accelMag <= 1.025f) {
      if (rainADC <= 2410.500f) {
        return 1;
      } else {
        return 2;
      })
    } else {
      if (soilADC <= 1171.000f) {
        return 1;
      } else {
        return 2;
      })
    })
  } else {
    if (soilPct <= 36.000f) {
      if (rainPct <= 66.500f) {
        return 0;
      } else {
        return 2;
      })
    } else {
      if (gyroMag <= 3.630f) {
        return 2;
      } else {
        return 2;
      })
    })
  })
}
int treeVote_2(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (accelMag <= 1.015f) {
    if (soilADC <= 2600.000f) {
      if (pitch <= -0.875f) {
        return 1;
      } else {
        return 2;
      })
    } else {
      if (rainADC <= 1364.500f) {
        return 2;
      } else {
        return 0;
      })
    })
  } else {
    if (rainPct <= 23.500f) {
      if (soilADC <= 1160.500f) {
        return 1;
      } else {
        return 1;
      })
    } else {
      if (gyroMag <= 151.075f) {
        return 2;
      } else {
        return 1;
      })
    })
  })
}
int treeVote_3(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (gyroMag <= 151.035f) {
    if (accelMag <= 1.015f) {
      if (rainADC <= 1447.500f) {
        return 2;
      } else {
        return 0;
      })
    } else {
      if (gyroMag <= 3.630f) {
        return 0;
      } else {
        return 2;
      })
    })
  } else {
    if (accelMag <= 842.500f) {
      if (accelMag <= 1.082f) {
        return 2;
      } else {
        return 1;
      })
    } else {
      if (rainPct <= 24.000f) {
        return 2;
      } else {
        return 2;
      })
    })
  })
}
int treeVote_4(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (pitch <= -1.235f) {
    if (soilPct <= 61.500f) {
      if (gyroMag <= 150.485f) {
        return 2;
      } else {
        return 1;
      })
    } else {
      return 1;
    })
  } else {
    if (soilADC <= 2602.500f) {
      if (soilADC <= 1179.500f) {
        return 1;
      } else {
        return 2;
      })
    } else {
      if (pitch <= 5.045f) {
        return 0;
      } else {
        return 1;
      })
    })
  })
}
int treeVote_5(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (rainPct <= 63.500f) {
    if (pitch <= -1.305f) {
      if (rainADC <= 3071.500f) {
        return 2;
      } else {
        return 1;
      })
    } else {
      if (soilADC <= 2240.000f) {
        return 2;
      } else {
        return 0;
      })
    })
  } else {
    if (soilPct <= 70.000f) {
      if (rainADC <= 1458.500f) {
        return 2;
      } else {
        return 0;
      })
    } else {
      if (gyroMag <= 4.135f) {
        return 1;
      } else {
        return 1;
      })
    })
  })
}
int treeVote_6(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1178.500f) {
    if (gyroMag <= 4.140f) {
      if (roll <= 0.765f) {
        return 2;
      } else {
        return 1;
      })
    } else {
      return 1;
    })
  } else {
    if (gyroMag <= 4.270f) {
      if (rainPct <= 66.500f) {
        return 0;
      } else {
        return 2;
      })
    } else {
      if (rainADC <= 3400.000f) {
        return 2;
      } else {
        return 1;
      })
    })
  })
}
int treeVote_7(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilPct <= 36.000f) {
    if (rainPct <= 66.500f) {
      if (rainADC <= 3682.000f) {
        return 0;
      } else {
        return 0;
      })
    } else {
      return 2;
    })
  } else {
    if (soilPct <= 71.500f) {
      if (rainPct <= 67.500f) {
        return 2;
      } else {
        return 2;
      })
    } else {
      if (rainADC <= 926.500f) {
        return 1;
      } else {
        return 1;
      })
    })
  })
}
int treeVote_8(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (accelMag <= 1.015f) {
    if (rainPct <= 66.500f) {
      if (rainADC <= 3524.000f) {
        return 0;
      } else {
        return 0;
      })
    } else {
      if (soilPct <= 70.000f) {
        return 2;
      } else {
        return 1;
      })
    })
  } else {
    if (rainADC <= 3117.000f) {
      if (soilPct <= 61.500f) {
        return 2;
      } else {
        return 1;
      })
    } else {
      if (accelMag <= 908.500f) {
        return 1;
      } else {
        return 0;
      })
    })
  })
}

// ====================== VOTING (RANDOM FOREST) ======================
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

inline String riskClassToString(int riskClass) {
  switch (riskClass) {
    case 0: return "AMAN";
    case 1: return "BAHAYA";
    case 2: return "WASPADA";
    default: return "UNKNOWN";
  }
}

#endif
