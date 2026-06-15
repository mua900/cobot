import argparse
import subprocess

from typing import List
from pathlib import Path

def compile_shaders(shaders : List[str], shader_stage : str):
    stage_argument = "" 
    if shader_stage == "vertex":
        stage_argument = "vs"
    elif shader_stage == "fragment":
        stage_argument = "ps"
    else:
        print(f"Invalid shader stage argument {stage_argument}")
        return # invalid

    shader_model = "_6_0"
    target = stage_argument + shader_model

    for s in shaders:
        path = Path(s).stem

        subprocess.run(["dxc", "-T", target, "-E", "main", s, "-Fo", "binary/" + path + ".dxil"])
        subprocess.run(["dxc", "-spirv", "-T", target, "-E", "main", s, "-Fo", "binary/" + path + ".spv"])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-vertex', action="append")
    parser.add_argument('-fragment', action="append")

    args = parser.parse_args()

    if args.vertex:
        compile_shaders(args.vertex, "vertex")
    if args.fragment:
        compile_shaders(args.fragment, "fragment")

    if not (args.vertex or args.fragment):
        print("Please provide shaders to compile")

if __name__ == "__main__":
    main()