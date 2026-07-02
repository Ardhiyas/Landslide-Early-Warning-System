/*
  =====================================================================
  TANAALERT - MODEL RANDOM FOREST (rf_model.h)
  =====================================================================
  File ini di-generate OTOMATIS oleh train_and_generate.py

  Konfigurasi training:
    - Jumlah data latih : 300 sampel
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
  if (gyroMag <= 149.205f) {
    if (soilADC <= 2603.500f) {
      if (rainPct <= 67.500f) {
        return 2;
      } else {
        return 2;
      }
    } else {
      if (roll <= -4.090f) {
        return 0;
      } else {
        return 0;
      }
    }
  } else {
    if (rainPct <= 20.500f) {
      if (rainADC <= 4090.000f) {
        return 1;
      } else {
        return 1;
      }
    } else {
      if (accelMag <= 824.000f) {
        return 1;
      } else {
        return 2;
      }
    }
  }
}
int treeVote_1(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1166.500f) {
    if (accelMag <= 1.056f) {
      if (rainADC <= 2339.000f) {
        return 1;
      } else {
        return 2;
      }
    } else {
      if (soilADC <= 1160.500f) {
        return 1;
      } else {
        return 2;
      }
    }
  } else {
    if (soilPct <= 36.000f) {
      if (rainPct <= 68.000f) {
        return 0;
      } else {
        return 2;
      }
    } else {
      if (gyroMag <= 3.600f) {
        return 2;
      } else {
        return 2;
      }
    }
  }
}
int treeVote_2(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (accelMag <= 1.092f) {
    if (soilADC <= 2601.000f) {
      if (pitch <= -0.875f) {
        return 1;
      } else {
        return 2;
      }
    } else {
      if (rainADC <= 1270.500f) {
        return 2;
      } else {
        return 0;
      }
    }
  } else {
    if (rainPct <= 23.500f) {
      if (soilADC <= 1166.500f) {
        return 1;
      } else {
        return 1;
      }
    } else {
      if (rainADC <= 1230.000f) {
        return 1;
      } else {
        return 2;
      }
    }
  }
}
int treeVote_3(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1179.500f) {
    if (pitch <= -0.875f) {
      if (rainADC <= 942.500f) {
        return 1;
      } else {
        return 1;
      }
    } else {
      if (gyroMag <= 79.480f) {
        return 2;
      } else {
        return 1;
      }
    }
  } else {
    if (accelMag <= 1.015f) {
      if (pitch <= -1.175f) {
        return 2;
      } else {
        return 0;
      }
    } else {
      if (rainPct <= 16.500f) {
        return 1;
      } else {
        return 2;
      }
    }
  }
}
int treeVote_4(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (pitch <= -1.120f) {
    if (soilPct <= 61.500f) {
      if (gyroMag <= 140.310f) {
        return 2;
      } else {
        return 1;
      }
    } else {
      if (soilADC <= 1180.000f) {
        return 1;
      } else {
        return 2;
      }
    }
  } else {
    if (soilADC <= 3235.000f) {
      if (soilADC <= 1139.500f) {
        return 1;
      } else {
        return 2;
      }
    } else {
      if (gyroMag <= 77.360f) {
        return 0;
      } else {
        return 2;
      }
    }
  }
}
int treeVote_5(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (rainPct <= 64.500f) {
    if (pitch <= -1.280f) {
      if (rainADC <= 3065.000f) {
        return 2;
      } else {
        return 1;
      }
    } else {
      if (soilADC <= 2593.500f) {
        return 2;
      } else {
        return 0;
      }
    }
  } else {
    if (soilPct <= 70.000f) {
      return 2;
    } else {
      if (rainADC <= 938.500f) {
        return 1;
      } else {
        return 1;
      }
    }
  }
}
int treeVote_6(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilADC <= 1179.500f) {
    if (gyroMag <= 4.140f) {
      if (rainADC <= 2335.000f) {
        return 1;
      } else {
        return 2;
      }
    } else {
      if (soilADC <= 1121.000f) {
        return 1;
      } else {
        return 1;
      }
    }
  } else {
    if (rainPct <= 63.500f) {
      if (rainADC <= 3682.000f) {
        return 0;
      } else {
        return 1;
      }
    } else {
      if (gyroMag <= 3.425f) {
        return 2;
      } else {
        return 2;
      }
    }
  }
}
int treeVote_7(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (soilPct <= 70.000f) {
    if (rainPct <= 64.500f) {
      if (rainADC <= 3537.000f) {
        return 0;
      } else {
        return 0;
      }
    } else {
      return 2;
    }
  } else {
    if (accelMag <= 1.056f) {
      if (rainPct <= 41.000f) {
        return 2;
      } else {
        return 1;
      }
    } else {
      if (roll <= -3.240f) {
        return 1;
      } else {
        return 1;
      }
    }
  }
}
int treeVote_8(int rainADC, int rainPct, int soilADC, int soilPct, float accelMag, float gyroMag, float pitch, float roll) {
  if (accelMag <= 1.015f) {
    if (soilADC <= 2599.500f) {
      if (rainADC <= 1170.000f) {
        return 1;
      } else {
        return 2;
      }
    } else {
      if (gyroMag <= 3.505f) {
        return 0;
      } else {
        return 0;
      }
    }
  } else {
    if (rainADC <= 3129.000f) {
      if (soilPct <= 71.500f) {
        return 2;
      } else {
        return 1;
      }
    } else {
      if (accelMag <= 908.500f) {
        return 1;
      } else {
        return 0;
      }
    }
  }
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
