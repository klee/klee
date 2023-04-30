import argparse
import json
import os


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', type=str, required=True)
    parser.add_argument('--output', type=str, required=True)
    return parser.parse_args()

def merge_dir(input_dir: str, output: str):
    out = {"results": []}
    for f in os.listdir(input_dir):
        if f.endswith('.sarif'):
            with open(os.path.join(input_dir, f), 'r') as file:
                input_report = json.load(file)
            run = input_report["runs"][0]
            if "tool" not in out:
                out["tool"] = run["tool"]
            else:
                assert(out["tool"]["driver"]["name"] == run["tool"]["driver"]["name"])
            out["results"].extend(run["results"])
    with open(output, 'w') as f:
        json.dump({"runs": [out]}, f, indent=4) 


def main():
    args = get_args()
    merge_dir(args.input_dir, args.output)

if __name__ == "__main__":
    main()
