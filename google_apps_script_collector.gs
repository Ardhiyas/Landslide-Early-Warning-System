/*
  =====================================================================
  TANAALERT - Google Apps Script (Web App Endpoint)
  =====================================================================
  Cara pakai:
  1. Buka Google Sheets baru -> Extensions -> Apps Script
  2. Hapus isi default, ganti dengan kode ini
  3. Klik Deploy -> New deployment -> pilih tipe "Web app"
     - Execute as: Me
     - Who has access: Anyone
  4. Klik Deploy, copy URL yang muncul (bentuknya
     https://script.google.com/macros/s/XXXXXXXXXXXX/exec )
  5. URL itu yang dipakai sebagai SCRIPT_URL di ketiga sketch ESP32

  Setiap sensor (Soil, Hujan, MPU6050) otomatis disimpan ke tab/sheet
  terpisah dengan nama sesuai parameter "sheet" yang dikirim ESP32.
  Tab akan dibuat otomatis kalau belum ada.
  =====================================================================
*/

function doGet(e) {
  var p = e.parameter;
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheetName = p.sheet;

  if (!sheetName) {
    return ContentService.createTextOutput("ERROR: parameter 'sheet' tidak ada");
  }

  var sheet = ss.getSheetByName(sheetName);
  if (!sheet) {
    sheet = ss.insertSheet(sheetName);
  }

  var timestamp = new Date();

  if (sheetName === "Soil") {
    if (sheet.getLastRow() === 0) {
      sheet.appendRow(["Timestamp", "Soil_ADC", "Soil_Pct", "Label"]);
    }
    sheet.appendRow([timestamp, p.adc, p.pct, p.label]);

  } else if (sheetName === "Hujan") {
    if (sheet.getLastRow() === 0) {
      sheet.appendRow(["Timestamp", "Rain_ADC", "Rain_Pct", "Label"]);
    }
    sheet.appendRow([timestamp, p.adc, p.pct, p.label]);

  } else if (sheetName === "MPU6050") {
    if (sheet.getLastRow() === 0) {
      sheet.appendRow([
        "Timestamp", "Accel_X", "Accel_Y", "Accel_Z",
        "Gyro_X", "Gyro_Y", "Gyro_Z",
        "Accel_Magnitude", "Gyro_Magnitude", "Pitch", "Roll", "Label"
      ]);
    }
    sheet.appendRow([
      timestamp, p.ax, p.ay, p.az,
      p.gx, p.gy, p.gz,
      p.accelMag, p.gyroMag, p.pitch, p.roll, p.label
    ]);

  } else {
    return ContentService.createTextOutput("ERROR: sheet '" + sheetName + "' tidak dikenali");
  }

  return ContentService.createTextOutput("OK");
}
