# Train.py
# Trains a yolo11 model on a dataset
# Arguments: --data cansuperset-1/data.yaml --model yolo11n.pt --epochs 100 --out ./model_runs

# Imports
import argparse
import os
import datetime

from ultralytics import YOLO

# Parse arguments
parser = argparse.ArgumentParser(description="Train a YOLO model on a dataset")
parser.add_argument("--data",   type=str, help="Path to the data.yaml file",    default="/home/daniel/github/EE579-Project/AI_Development/datasets/cansuperset-1/data.yaml")
parser.add_argument("--model",  type=str, help="Model name",                    default="yolo11n.pt")
parser.add_argument("--epochs", type=int, help="Number of epochs to train",     default=32)
parser.add_argument("--out",    type=str, help="Output directory for model",    default="./model_runs")
args = parser.parse_args()

# Train the model
model = YOLO(args.model)
results = model.train(data=args.data, epochs=args.epochs)
val_result = model.val()  # Validate the model
fitness = val_result.fitness  # Show the validation results


# create model name based on map50, epochs, date, and time
model_name = f"yolo11n_{fitness:.2f}_{args.epochs}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}"
# Save the model with the new name
if not os.path.exists(args.out):
    os.makedirs(args.out)

model.save(f"{args.out}/{model_name}.pt")

val_result.mean_results()