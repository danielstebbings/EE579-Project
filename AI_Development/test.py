# Test.py
# Uses provided model and unlabelled data to make predictions
# Arguments --model model_runs/yolo11n_0.85_32_20210501_123456.pt --data can_images/table --out ./predictions

# Imports
import argparse
import os
from ultralytics import YOLO

# Parse arguments
parser = argparse.ArgumentParser(description="Make predictions on a dataset using a YOLO model")
parser.add_argument("--model", type=str, help="Path to the model file", default="/home/daniel/github/EE579-Project/AI_Development/model_runs/yolo11n_0.82_32_20250217_052653.pt")
parser.add_argument("--data", type=str, help="Path to the data directory", default="/home/daniel/github/EE579-Project/AI_Development/can_images/table")
parser.add_argument("--out", type=str, help="Output directory for predictions", default="/home/daniel/github/EE579-Project/AI_Development/predictions")
args = parser.parse_args()

# make predictions on each image in provided folder
model = YOLO(args.model)

# get a list of image file paths in the data directory
images = [os.path.join(args.data, f) for f in os.listdir(args.data) if os.path.isfile(os.path.join(args.data, f))]
# make predictions
results = model.predict(images)

i = 1
for result in results:
    result.show()  # display to screen
    filename = os.path.join(args.out, f"result_{i}.jpg")
    result.save(filename=filename)  # save to disk




print("Predictions saved to", args.out)
