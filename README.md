# TECHIN 515 Lab 5 — Edge-Cloud Offloading

## Project Structure

```
.
├── ESP32_to_cloud/
│   └── ESP32_to_cloud.ino      # ESP32 sketch with edge-cloud offloading
├── trainer_scripts/
│   ├── train.ipynb             # Model training script (run on Azure ML)
│   └── register_model.ipynb   # Model registration script
├── app/
│   ├── app.py                  # Flask web app for cloud inference
│   ├── requirements.txt        # Python dependencies
│   └── wand_model.h5           # Trained model (add after training)
├── data/
│   ├── O/                      # O-shape gesture samples
│   ├── V/                      # V-shape gesture samples
│   └── Z/                      # Z-shape gesture samples
└── report.md                   # Discussion questions and screenshots
```

## How to Run

### Flask Web App (Cloud Server)

```bash
cd app
pip install -r requirements.txt
# Place wand_model.h5 in this directory first
python app.py
```

The server runs at `http://0.0.0.0:8000`. Use the `/predict` endpoint with a POST request containing `{"data": [...]}`.

### ESP32 Sketch

1. Open `ESP32_to_cloud/ESP32_to_cloud.ino` in Arduino IDE
2. Update `ssid`, `password`, and `serverUrl` with your WiFi and server IP
3. Flash to your XIAO ESP32S3 board
4. Press button (D1) or send `o` via Serial to trigger gesture capture

## Hardware

- Seeed XIAO ESP32S3
- LSM6DS3 IMU
- NeoPixel LED strip (8 pixels, pin D0)
- Button (pin D1)

## Gesture Classes

| Gesture | LED Color |
|---------|-----------|
| Z       | Red       |
| O       | White     |
| V       | Green     |
