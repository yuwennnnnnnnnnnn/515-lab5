from flask import Flask, request, jsonify
import numpy as np
import joblib

app = Flask(__name__)

model = joblib.load("wand_model.pkl")
scaler = joblib.load("scaler.pkl")
gesture_labels = ["O", "V", "Z"]

@app.route("/", methods=["GET"])
def home():
    return "Wand Gesture API is running!"

@app.route("/predict", methods=["POST"])
def predict():
    try:
        data = request.json.get("data")
        if not data:
            raise ValueError("Missing 'data' field")

        input_array = np.array(data).reshape(1, -1)
        input_array = scaler.transform(input_array)
        proba = model.predict_proba(input_array)[0]

        top_index = int(np.argmax(proba))
        label = gesture_labels[top_index]
        confidence = float(proba[top_index]) * 100

        return jsonify({
            "gesture": label,
            "confidence": confidence
        })

    except Exception as e:
        return jsonify({"error": str(e)}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)
