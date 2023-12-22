#!/usr/bin/python3

import sys
import json
import os

from typing import Tuple


def compare_locations(source, pattern) -> Tuple[bool, str]:
    try:
        if source["message"] != pattern["message"]:
            return (
                False,
                f"different messages: {source['message']['text']} vs {pattern['message']['text']}",
            )
        if (
            source["physicalLocation"]["region"]
            != pattern["physicalLocation"]["region"]
        ):
            return (False, "different locations")

        source_path = source["physicalLocation"]["artifactLocation"]["uri"]
        pattern_path = pattern["physicalLocation"]["artifactLocation"]["uri"]

        if str(source_path).endswith(str(pattern_path)):
            return (True, "")

        return (False, f"different locations: {source_path} vs {pattern_path}")
    except KeyError:
        return (False, "Not a SARIF format given")


def validate_source(source, pattern) -> Tuple[bool, str]:
    try:
        for result_id in range(len(source["runs"][0]["results"])):

            source_loc = source["runs"][0]["results"][result_id]["codeFlows"][0][
                "threadFlows"
            ][0]["locations"][0]["location"]
            pattern_loc = pattern["runs"][0]["results"][result_id]["codeFlows"][0][
                "threadFlows"
            ][0]["locations"][0]["location"]

            (ok, msg) = compare_locations(source_loc, pattern_loc)
            if not ok:
                return (
                    False,
                    f"Source differs from source in pattern in result #{result_id + 1} : {msg}",
                )

        return (True, "")
    except KeyError:
        return (False, "Not a SARIF format given")


def validate_sink(source, pattern) -> Tuple[bool, str]:
    try:
        for result_id in range(len(source["runs"][0]["results"])):

            source_loc = source["runs"][0]["results"][result_id]["codeFlows"][0][
                "threadFlows"
            ][0]["locations"][-1]["location"]
            pattern_loc = pattern["runs"][0]["results"][result_id]["codeFlows"][0][
                "threadFlows"
            ][0]["locations"][-1]["location"]

            (ok, msg) = compare_locations(source_loc, pattern_loc)
            if not ok:
                return (
                    False,
                    f"Sink differs from sink in pattern in result #{result_id + 1} : {msg}",
                )

        return (True, "")
    except KeyError:
        return (False, "Not a SARIF format given")


def validate(source_file, pattern_file) -> Tuple[bool, str]:
    try:
        source = json.load(source_file)
        pattern = json.load(pattern_file)
    except json.JSONDecodeError as e:
        print("Invalid JSON syntax:", e)

    try:
        if len(source["runs"]) != 1:
            return (False, "Expected exactly 1 run in source file")
        if len(pattern["runs"]) != 1:
            return (False, "Expected exactly 1 run in source file")

        if len(source["runs"][0]["results"]) != len(pattern["runs"][0]["results"]):
            return (False, "Number of results does not match expected")
    except KeyError:
        return (False, "Not a SARIF format given")

    (ok, msg) = validate_source(source, pattern)
    if not ok:
        return (ok, msg)

    return validate_sink(source, pattern)


def main():
    if len(sys.argv) != 3:
        print(f"USAGE: {sys.argv[0]} SOURCE PATTERN", file=sys.stderr)
        exit(1)

    source_path = sys.argv[1]
    pattern_path = sys.argv[2]

    with open(source_path, "r") as source_file, open(pattern_path, "r") as pattern_file:
        (ok, msg) = validate(source_file, pattern_file)
        if ok:
            print(f"Validation passed!", file=sys.stderr)
            exit(0)

        print(f"Validation failed: {msg}!", file=sys.stderr)
        exit(1)


if __name__ == "__main__":
    main()
