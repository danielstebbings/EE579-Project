# Download and setup datasets for the project
# Imports
from roboflow import Roboflow
from dotenv import load_dotenv, find_dotenv
import os
import shutil

# Load environment variables
load_dotenv(find_dotenv(".env"))
ROBOFLOW_API_KEY = os.getenv("ROBOFLOW_API_KEY")

# Setup Roboflow
rf = Roboflow(api_key="oFwawva7oxqugqHJX2IB")

# Custom Can dataset
print("Roboflow Custom Can dataset: cansuperset ############################")
if not os.path.exists('cansuperset-1') | os.path.exists('datasets/cansuperset-1'):
    project = rf.workspace("personal-s95f1").project("cansuperset")
    version = project.version(1)
    dataset = version.download("yolov11")
    # move to datasets folder
    if not os.path.exists('datasets'):
        os.makedirs('datasets')
    shutil.move('cansuperset-1', 'datasets')
    print("Custom Can dataset downloaded and setup successfully")
else:
    print("Custom Can dataset already exists")

