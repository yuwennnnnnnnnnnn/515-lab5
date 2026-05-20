import os, numpy as np, pandas as pd, joblib
from sklearn.neural_network import MLPClassifier
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split

GESTURE_CLASSES = ["O", "V", "Z"]
SEQUENCE_LENGTH = 100
DATA_DIR = "../data"

features, labels = [], []
for idx, cls in enumerate(GESTURE_CLASSES):
    cls_dir = os.path.join(DATA_DIR, cls)
    for f in os.listdir(cls_dir):
        if not f.endswith('.csv'): continue
        df = pd.read_csv(os.path.join(cls_dir, f))
        if not {'x','y','z'}.issubset(df.columns): continue
        seq = df[['x','y','z']].values.tolist()
        if len(seq) >= SEQUENCE_LENGTH:
            features.append(seq[:SEQUENCE_LENGTH])
            labels.append(idx)
        elif len(seq) > 10:
            features.append(seq + [[0,0,0]]*(SEQUENCE_LENGTH-len(seq)))
            labels.append(idx)

X = np.array(features).reshape(len(features), -1)
y = np.array(labels)
print(f"Loaded {len(X)} samples")

scaler = StandardScaler()
X = scaler.fit_transform(X)

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)

model = MLPClassifier(hidden_layer_sizes=(128, 64), max_iter=200, random_state=42)
model.fit(X_train, y_train)
print(f"Test accuracy: {model.score(X_test, y_test):.4f}")

joblib.dump(model, "wand_model.pkl")
joblib.dump(scaler, "scaler.pkl")
print("Saved: wand_model.pkl, scaler.pkl")
