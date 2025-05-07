## https://github.com/espressif/esp-dl/issues/234

import os
import argparse
from ppq import QuantizationSettingFactory
from ppq.api import espdl_quantize_onnx, get_target_platform
from torch.utils.data import DataLoader, Dataset
from torchvision import transforms
from PIL import Image
from onnxsim import simplify
import onnx
import zipfile
import urllib.request
from tqdm import tqdm

sizex = 480
sizey = 320

class CaliDataset(Dataset):
    def __init__(self, path, size):
        self.size = size
        self.transform = transforms.Compose([
            transforms.ToTensor(),
            transforms.Resize((sizex, sizey)),
            transforms.Normalize(mean=[0, 0, 0], std=[1, 1, 1]),
        ])
        self.imgs_path = [os.path.join(path, img_name) for img_name in os.listdir(path)]

    def __len__(self):
        return len(self.imgs_path)

    def __getitem__(self, idx):
        img = Image.open(self.imgs_path[idx])
        img = self.transform(img)
        return img

def report_hook(blocknum, blocksize, total):
    downloaded = blocknum * blocksize
    percent = downloaded / total * 100
    print(f"\rDownloading calibration dataset: {percent:.2f}%", end="")

def resize_images(source_dir, dest_dir, target_size):
    os.makedirs(dest_dir, exist_ok=True)
    for filename in tqdm(os.listdir(source_dir), desc="Redimensionando imágenes"):
        src_path = os.path.join(source_dir, filename)
        dst_path = os.path.join(dest_dir, filename)
        try:
            img = Image.open(src_path).convert("RGB")
            img = img.resize((sizex, sizey))
            img.save(dst_path)
        except Exception as e:
            print(f"Error with {filename}: {e}")

def quant_yolo11n(onnx_path, espdl_path, size):
    BATCH_SIZE = 1
    INPUT_SHAPE = [3, sizex, sizey]
    DEVICE = "cpu"
    TARGET = "esp32s3"
    NUM_OF_BITS = 8
    ONNX_PATH = onnx_path
    ESPDL_MODLE_PATH = espdl_path

    CALIB_DIR = "calib"
    RESIZED_CALIB_DIR = f"{CALIB_DIR}_{size}"
    resize_images(CALIB_DIR, RESIZED_CALIB_DIR, size)

    model = onnx.load(ONNX_PATH)
    model, check = simplify(model)
    assert check, "Simplified ONNX model could not be validated"
    onnx.save(onnx.shape_inference.infer_shapes(model), ONNX_PATH)

    calibration_dataset = CaliDataset(RESIZED_CALIB_DIR, size)
    dataloader = DataLoader(dataset=calibration_dataset, batch_size=BATCH_SIZE, shuffle=False)

    def collate_fn(batch):
        print(f"Batch shape before quantization: {batch.shape}")
        return batch.to(DEVICE)

    quant_setting = QuantizationSettingFactory.espdl_setting()
    quant_setting.dispatching_table.append(
        operation='/model.*/conv/Conv',
        platform=get_target_platform(TARGET, NUM_OF_BITS)
    )

    quant_ppq_graph = espdl_quantize_onnx(
        onnx_import_file=ONNX_PATH,
        espdl_export_file=ESPDL_MODLE_PATH,
        calib_dataloader=dataloader,
        calib_steps=32,
        input_shape=[1] + INPUT_SHAPE,
        target=TARGET,
        num_of_bits=NUM_OF_BITS,
        collate_fn=collate_fn,
        setting=quant_setting,
        device=DEVICE,
        error_report=True,
        skip_export=False,
        export_test_values=False,
        verbose=0,
        inputs=None,
    )
    return quant_ppq_graph

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Quantize ONNX model to ESP-DL format")
    parser.add_argument("--onnx", type=str, required=True, help="Input ONNX model path")
    parser.add_argument("--espdl", type=str, required=True, help="Output ESP-DL model path")
    parser.add_argument("--size", type=str, required=True, help="Input resolution, e.g. 640")
    args = parser.parse_args()
    quant_yolo11n(args.onnx, args.espdl, int(args.size))
