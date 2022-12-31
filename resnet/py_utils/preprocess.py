import argparse
import os

import torch
import tqdm
from PIL import Image
from test_batchnorm import makeBatchNormTests
from test_pooling import makePoolingTests
from torch.utils.data import DataLoader
from torchvision import models, transforms
from utils import writeNetwork, writeTensor


class ImageDataset(torch.utils.data.Dataset):
    def __init__(self, path, transform=None):
        if not os.path.exists(path):
            raise FileNotFoundError(f"Path {path} does not exist")

        self.path = path
        self.image_names = os.listdir(path)
        self.transform = transform
        print(self.image_names[:10])

    def __getitem__(self, index):
        image_name = self.image_names[index]
        image = Image.open(os.path.join(self.path, image_name)).convert("RGB")
        if self.transform is not None:
            image = self.transform(image)
        # HACK: the label will be generated by the model
        return image, 0

    def __len__(self):
        return len(self.image_names)


if __name__ == "__main__":
    # fmt: off
    parser = argparse.ArgumentParser()
    parser.add_argument("--batch-size", type=int, default=16)
    parser.add_argument("--num-workers", type=int, default=1)
    parser.add_argument("--dataset-dir", type=str, default="dataset")
    parser.add_argument("--dataset-output-dir", type=str, default="dataset_tensor")
    parser.add_argument("--network-output-dir", type=str, default="resnet18")
    parser.add_argument("--test-data-dir", type=str, default="test_data")
    # fmt: on
    args = parser.parse_args()

    # make tests
    if not os.path.exists(args.test_data_dir):
        os.mkdir(args.test_data_dir)
    makeBatchNormTests(args.test_data_dir)
    makePoolingTests(args.test_data_dir)

    # Check if pytorch supports cuda
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    data_mean = [0.485, 0.456, 0.406]
    data_std = [0.229, 0.224, 0.225]
    input_shape = (3, 224, 224)

    weights = models.ResNet18_Weights.IMAGENET1K_V1
    model = models.resnet18(weights=weights)
    if not os.path.exists(args.network_output_dir):
        os.mkdir(args.network_output_dir)
    print("Exporting resnet18")
    writeNetwork(model, "resnet18", args.network_output_dir)

    transform = transforms.Compose([
        transforms.Resize(256),
        transforms.CenterCrop(224),
        transforms.ToTensor(),
        transforms.Normalize(mean=data_mean, std=data_std)
    ])

    dataset = ImageDataset(args.dataset_dir, transform)

    data_loader = DataLoader(
        dataset, batch_size=args.batch_size,
        shuffle=False, num_workers=args.num_workers, pin_memory=True)

    model = model.to(device)
    model.eval()
    total = 0

    # No need to add cwd of already inside them.
    if not os.path.exists(args.dataset_output_dir):
        os.mkdir(args.dataset_output_dir)

    print("Making dataset")
    with torch.no_grad() and tqdm.tqdm(total=len(dataset)) as pbar:
        for i, (images, dummy_labels) in enumerate(data_loader):
            writeTensor(images, os.path.join(
                args.dataset_output_dir, f"images_{i:04d}.bin"))
            images_device = images.to(device)
            outputs = model(images_device)
            predicted_labels = outputs.cpu().detach()
            writeTensor(predicted_labels, os.path.join(
                args.dataset_output_dir, f"labels_{i:04d}.bin"))
            total += dummy_labels.size(0)
            # Don't compute the accuracy here. We only check if the implemented result
            # moves just like original model.
            pbar.update(dummy_labels.size(0))

            if i < 3:
                # We need to store the first 3 batches to text to test whether loading is correct.
                val_img_filename = os.path.join(
                    args.test_data_dir, f"validation_{i:04d}_image.txt")
                with open(val_img_filename, "w") as f:
                    # Write shape of the tensor.
                    shape = images.shape
                    f.write("{} {} ".format(
                        len(shape), " ".join([str(s) for s in shape])))
                    # Write flattened tensor to text file.
                    flattened_images = images.flatten()
                    size = flattened_images.size(0)
                    for j in range(size):
                        f.write(f"{flattened_images[j]} ")

                val_label_filename = os.path.join(
                    args.test_data_dir, f"validation_{i:04d}_label.txt")
                with open(val_label_filename, "w") as f:
                    # Write shape of the tensor.
                    shape = predicted_labels.shape
                    f.write("{} {} ".format(
                        len(shape), " ".join([str(s) for s in shape])))
                    # Write flattened tensor to text file.
                    flattened_labels = predicted_labels.flatten()
                    size = flattened_labels.size(0)
                    for j in range(size):
                        f.write(f"{flattened_labels[j]} ")