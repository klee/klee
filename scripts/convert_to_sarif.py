import argparse
from collections import defaultdict
import json
import os


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', type=str, required=True)
    parser.add_argument('--output_dir', type=str, required=True)
    return parser.parse_args()


def convert_error(error):
    converted_error = {}
    locations = []
    codeFlows = [{"threadFlows": [{"locations": locations}]}]
    converted_error["codeFlows"] = codeFlows
    for trace in error["trace"]:
        traceLocation = trace["location"]
        physicalLocation = {
            "artifactLocation": {
                "uri": traceLocation["file"]
            },
            "region": {
                "startLine": traceLocation["reportLine"],
                "startColumn": traceLocation.get("column")
            }
        }
        location = {
            "physicalLocation": physicalLocation
        }
        locations.append({"location": location})

    trace = error["trace"][-1]
    if trace["event"]["kind"] == "Error":
        converted_error["ruleId"] = trace["event"]["error"]["sort"]
    else:
        converted_error["ruleId"] = "Reached"
    converted_error["locations"] = [locations[-1]["location"]]
    converted_error["id"] = error["id"]


    return converted_error


def convert_file(input: str, output_dir: str):
    with open(input, 'r') as f:
        input_report = json.load(f)

    output_report = {}
    output_report["runs"] = [defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: 0)))))]
    run = output_report["runs"][0]
    run["tool"]["driver"]["name"] = "SecB"
    run["results"] = []
    results = run["results"]

    for error in input_report["errors"]:
        results.append(convert_error(error))

    with open(os.path.join(output_dir, os.path.basename(input)), 'w') as f:
        json.dump(output_report, f, indent=4)

def main():
    args = get_args()
    if os.path.isdir(args.input):
        for f in os.listdir(args.input):
            if f.endswith('.json'):
                convert_file(os.path.join(args.input, f), args.output_dir)
    else:
        convert_file(args.input, args.output_dir)

if __name__ == "__main__":
    main()
