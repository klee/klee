import argparse
import json


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, required=True)
    parser.add_argument('--output', type=str, required=True)
    return parser.parse_args()

def prepare_cppcheck(input: str, output: str):
    with open(input, 'r') as file:
        input_report = json.load(file)
    run = input_report["runs"][0]
    for i, result in enumerate(run["results"]):
        result["id"] = i
        if "codeFlows" in result:
            locations = result["codeFlows"][0]["threadFlows"][0]["locations"]
            new_locs = []
            prev = -1
            for i, loc in enumerate(locations):
                if prev == -1:
                    new_locs.append(loc)
                    prev = i
                else:
                    if loc["location"]["physicalLocation"]["artifactLocation"]["uri"] == locations[prev]["location"]["physicalLocation"]["artifactLocation"]["uri"] and loc["location"]["physicalLocation"]["region"]["startLine"] == locations[prev]["location"]["physicalLocation"]["region"]["startLine"]:
                        if "startColumn" in loc["location"]["physicalLocation"]["region"]:
                            if loc["location"]["physicalLocation"]["region"]["startColumn"] != locations[prev]["location"]["physicalLocation"]["region"]["startColumn"]:
                                new_locs.append(loc)
                                prev = i
                    else:
                        new_locs.append(loc)
                        prev = i

            result["codeFlows"][0]["threadFlows"][0]["locations"] = new_locs
    with open(output, 'w') as f:
        json.dump({"runs": [run]}, f, indent=4) 


def main():
    args = get_args()
    prepare_cppcheck(args.input, args.output)

if __name__ == "__main__":
    main()
