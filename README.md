# EstUAV

Tooling for processing DJI Mavic 3T drone flight data into datasets suitable for visual geolocation pipelines such as [WILDNAV](https://github.com/TIERS/wildnav).

Raw datasets are published on Hugging Face: <https://huggingface.co/datasets/Gekd/EstUAV>.

## Dataset Notes

The Hugging Face dataset ships two subsets. Use the matching `--timezone` value when processing each:

| Hugging Face subset | Required `--timezone` |
| ------------------- | --------------------- |
| `snow/`             | `2`                   |
| `no_snow/`          | `3`                   |

## Dependencies

System libraries (install via your package manager or the upstream guides):

- [libcurl](https://curl.se/libcurl/)
- [OpenCV](https://docs.opencv.org/4.x/df/d65/tutorial_table_of_content_introduction.html)
- [PROJ](https://proj.org/en/stable/install.html#install)

Vendored dependencies (included under `extern/`): CLI11 and nlohmann/json.

## Data Layout

Place input data under `data/` at the repository root:

1. Copy the contents of the Mavic 3T's `/DCIM` directory into `data/`.
2. Add the flight `.csv` log file(s) to `data/`.

> DJI log files are stored on the remote controller and are encrypted on firmware v13 and above. Extracting `.csv` files from the CLI requires a DJI API key; otherwise an online log viewer can be used.

Processed results are written to `output/`.

## Build

```sh
mkdir build && cd build
cmake ..
make
```

## Usage

```sh
./estuav --timezone <offset> [flags]
```

| Flag               | Description                       |
| ------------------ | --------------------------------- |
| `-z, --timezone N` | Timezone offset (required)        |
| `-w, --wildnav`    | Produce WILDNAV-formatted output  |
| `-r, --rgb`        | Include RGB images in output      |
| `-t, --thermal`    | Include thermal images in output  |

### Example

```sh
./estuav -w -z 3 -r -t
```
